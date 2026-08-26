# Network & Download Path Code Review

- **Date:** 2026-08-26
- **Reviewed at commit:** `6ae1341` (main)
- **Scope:** C++ correctness in `src/` — network, download queue, file handling, and
  threading paths. Reviewed by reading the source and tracing call paths.
- **Not covered:** the QML layer, regex robustness in `ModifierParser.cpp` against site
  markup changes, and the test suite's own coverage.

## Status summary

**No finding in this document has been implemented.** The review produced no code changes;
every item below is open. The only file modified in the session that produced this review
was `CLAUDE.md` (documentation), not application code.

| # | Finding | Severity | Implemented |
|---|---------|----------|-------------|
| 1 | Pause/cancel can abort the wrong download | Medium | ❌ No — open |
| 2 | Remote-derived version text reaches the save path unsanitized | Medium | ❌ No — open |
| 3 | `m_currentDownloadReply` is never initialized | Low–Medium (latent) | ❌ No — open |
| 4 | `file->write()` result discarded, so short writes report success | Low | ❌ No — open |

Suggested order if these get picked up: #1 and #2 first.

---

## 1. Pause/cancel can abort the wrong download

**Location:** `src/NetworkManager.cpp:463` · **Severity:** Medium · **Implemented:** ❌ No

`NetworkManager` tracks exactly one in-flight download in `m_currentDownloadReply`, but
three independent features write to it:

- modifier downloads — `src/DownloadManager.cpp:39`
- app installer — `src/AppUpdateManager.cpp:69`
- translation database — `src/DatabaseUpdateManager.cpp:73`

Each `downloadFileWithStatus` call overwrites the member, and `cancelDownload()` aborts
whichever reply was assigned last. `Backend::downloadAppUpdate()` (`src/Backend.cpp:692`)
guards only on app-update state and does not check for an active modifier download, so the
overlap is reachable:

1. Start a modifier download → `m_currentDownloadReply` = modifier reply.
2. Click "download update" → `m_currentDownloadReply` = installer reply.
3. Click pause on the modifier → `Backend::pauseDownload` (`src/Backend.cpp:464`) →
   `DownloadManager::cancelDownload()` → **aborts the installer**.

The modifier download keeps running while the app update dies silently, and the task list
still marks the modifier "paused".

**Root cause:** a single shared reply handle for logically independent transfers.
**Direction:** give each download its own handle, or key the abort by task id.

## 2. Remote-derived version text reaches the save path unsanitized

**Location:** `src/Backend.cpp:434` · **Severity:** Medium · **Implemented:** ❌ No

`downloadModifier` sanitizes the modifier name — strips `\ / : * ? " < > |`, trims, removes
trailing dots — then concatenates an unsanitized sibling value:

```cpp
const QString savePath = downloadDir + "/" + sanitizedName + "_" + versionName + ".zip";
```

`versionName` is scraped anchor text (`src/ModifierParser.cpp:445`, `:479`, `:513` — e.g.
`attachmentMatch.captured(2).trimmed()`). `ModifierInfoManager::formatVersionString`
(`src/ModifierInfoManager.cpp:90`) returns the string unchanged when it matches neither the
numeric nor the `v`-prefix pattern, so arbitrary text reaches the path. `meta.tempPath`
inherits it as well (`src/Backend.cpp:935`).

This does not require an attacker: a plausible label such as `Steam/Epic` makes
`NetworkManager` `mkpath` an unintended subdirectory (`src/NetworkManager.cpp:211`), and the
file lands where the rename logic does not expect it. With a compromised or hostile page it
is a straightforward write outside the download directory.

The asymmetry with the carefully sanitized name directly beside it reads as an oversight
rather than a deliberate choice.

## 3. `m_currentDownloadReply` is never initialized

**Location:** `src/include/NetworkManager.h:114` · **Severity:** Low–Medium (latent) · **Implemented:** ❌ No

The member has no default initializer and is absent from the constructor's initialiser list
(`src/NetworkManager.cpp:10-13`), which sets only `m_networkManager` and `m_timeoutInterval`.
`cancelDownload()` dereferences it.

**Reachability, stated precisely.** In the shipped application this is currently masked:
`DownloadManager::cancelDownload` gates on `m_isDownloading` (`src/DownloadManager.cpp:139`),
the assignment at `src/NetworkManager.cpp:246` happens synchronously right after `get()`,
and the `file->open()` failure path self-heals because the finished callback resets
`m_isDownloading` before control returns.

The reachable trigger today is the **test hook**: `downloadFileWithStatus` returns at
`src/NetworkManager.cpp:204` before the assignment, so a test whose handler defers its
callback and then cancels reads an indeterminate pointer.

It is undefined behaviour regardless, and one refactor away from being reachable in
production. `= nullptr` is the entire fix.

## 4. `file->write()` result discarded, so short writes report success

**Location:** `src/NetworkManager.cpp:295` · **Severity:** Low · **Implemented:** ❌ No

```cpp
*bytesWritten += data.size();
file->write(data);
```

`bytesWritten` counts bytes *received*, not bytes *persisted*. Nothing checks the write
result or calls `flush()` / `error()` before `file->close()`. Success is then decided by
`fileSize == 0 || *bytesWritten == 0` (`src/NetworkManager.cpp:326`).

On a full disk or a write error both values are non-zero, so a truncated archive is reported
as a completed download and is renamed out of `.crdownload` into the user's library.

---

## Verified correct — do not "fix" these

Several things that looked like candidate problems are handled properly, recorded here so
they are not undone later:

- `CoverExtractor` keeps `QPixmap` on the GUI thread and passes `QImage` across the worker
  boundary (`src/CoverExtractor.cpp:93-105`).
- It already serialises the shared YOLO detector behind a mutex because `detect()` mutates a
  `mutable` buffer (`src/CoverExtractor.cpp:174`; confirmed at
  `third_party/YOLOs-CPP/yolos/tasks/detection.hpp:146`).
- The cover and search staleness guards (`m_coverRequestId`, `m_activeSearchRequestId`) are
  correct, and `coverGameId` sanitises properly.
- `m_downloadTaskMeta` is cleaned up on task removal (`src/Backend.cpp:548`).
- The callback `QPointer` context guards in `NetworkManager` are sound.
- `parseModifierDetailHTML` never returns null, so the unchecked dereference at
  `src/ModifierManager.cpp:62` is safe as the code stands.

## Related

A separate documentation-accuracy review earlier the same day checked `CLAUDE.md` against
the source and found four incorrect claims (the `NetworkManager`/`CoverExtractor` HTTP
boundary, an `i18n check` CI gate that does not exist, `metadata.schema_version` described as
required, and the cover request-id attributed to the wrong class). Those four **were**
applied to `CLAUDE.md`; they were documentation fixes and changed no application code.
