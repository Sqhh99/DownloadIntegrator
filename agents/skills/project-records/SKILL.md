---
name: project-records
description: Write the mandatory FLiNG Downloader documentation records — a work log after any request that changes files, a code-review archive after reviewing code, and a PR record before opening a pull request. Use whenever finishing such a request, review, or PR.
---

# Project records

This repository keeps three kinds of permanent records under `docs/`. They are
mandatory, not optional extras: a decision that exists only in a conversation is lost
the moment the session ends.

## The three records

| Record | Trigger | Directory | Language |
|--------|---------|-----------|----------|
| Work log | Any request that changes files | `docs/work-logs/` | Chinese |
| Code review | Any code review | `docs/code-reviews/` | English |
| PR record | Before opening a pull request | `docs/pull-requests/` | Chinese |

All three use the filename `YYYY-MM-DD-<topic>.md`, where `<topic>` is a short
kebab-case slug — for a PR record, the branch topic. Use the real current date, and
write dates absolutely (`2026-08-28`, never "today" or "last week").

Cross-link the records for one piece of work: the work log links to the review and the
PR record, the review's status summary links to the work log, and the PR record links to
both. Use repo-relative links (`../work-logs/...`) so they resolve on GitHub.

## Rules that apply to all three

- **Never tick a verification box for a command you did not run.** Leave it unchecked and
  say why in the surrounding prose. This is the rule most likely to be quietly broken;
  breaking it makes every other record untrustworthy.
- **State the verification status explicitly**, including "not built, not tested". The
  build is Windows-only (`build.cmd`, Visual Studio 2022, Qt); work done from WSL cannot
  run it, and that is a fact for the record, not an excuse to omit.
- `CONTRIBUTING.md` requires the **AI-assistance disclosure** to be filled in honestly:
  which parts were AI-generated, and whether they were verified.
- Record what was **not** done or **not** covered as carefully as what was. Scope
  boundaries are the part a future reader cannot reconstruct.
- Quote the user's request verbatim rather than paraphrasing it.

## 1. Work log — `docs/work-logs/YYYY-MM-DD-<topic>.md`

Written after any request that changes files, capturing the whole request → plan →
change cycle. One file per request; append a new section to the day's file when a
request is a direct follow-up to one already logged there.

```markdown
# 工作记录：<一句话主题>

- **日期：** YYYY-MM-DD
- **分支：** `<branch>`（基于 `main` 的 `<commit>`）
- **关联文档：** 链接到审查归档 / PR 记录（如果有）

## 一、用户的请求
> 逐字引用请求原文
简述这条请求实际要求做的事。

## 二、制定的计划
按顺序列出实际执行的计划步骤。

## 三、具体改了哪些文件
| 文件 | 改动 |  ← 代码改动再加一列「对应问题」
说明每个文件为什么改，而不只是改了什么。

## 四、验证情况
跑了什么、没跑什么、为什么。没跑就直说。

## 五、遗留事项
未完成、未覆盖、需要后续处理的部分。
```

## 2. Code review — `docs/code-reviews/YYYY-MM-DD-<short-title>.md`

Written after reviewing code, whether or not anything is fixed as a result.

```markdown
# <Title> Code Review

- **Date:** YYYY-MM-DD
- **Reviewed at commit:** `<sha>` (branch)
- **Scope:** what was read, and how (source reading, call-path tracing, running it)
- **Not covered:** the parts deliberately left out

## Status summary
One paragraph on whether the findings are implemented, plus the findings table.

| # | Finding | Severity | Implemented |
|---|---------|----------|-------------|
| 1 | ... | Medium | ❌ No — open  /  ✅ Yes — YYYY-MM-DD |

## <n>. <Finding title>
**Location:** `file:line` · **Severity:** ... · **Implemented:** ...
What is wrong, the concrete path that reaches it, and how reachable it actually is —
state reachability honestly instead of inflating severity.
**Fix.** Added when the finding is implemented; describes the change, not the intent.

## Verified correct — do not "fix" these
Things that looked like defects but are handled properly, so nobody undoes them later.
```

The `Implemented` column is the point of the document: keep it current. When findings are
later fixed, update this file in place — do not start a second review document for the
same review. Line numbers stay as they were at review time; the header records the commit.

## 3. PR record — `docs/pull-requests/YYYY-MM-DD-<branch-topic>.md`

Written **before** opening the pull request, then used as the PR body. Follow the
sections in `.github/pull_request_template.md` exactly:

```markdown
# PR：<标题>

- **日期：** / **分支：** `<branch>` → `main` / **基线提交：** `<sha>` / **关联记录：**

## 关联
对应 Issue，或要解决的问题。

## 改了什么
用户或开发者能观察到的变化。不要只贴文件列表。

## 怎么验证
- [ ] `build.cmd tests`
- [ ] 本地跑过相关界面 / 下载 / 搜索路径
没执行就留空，并写明为什么、以及合并前需要谁在什么环境补跑。

## 检查项
- [ ] 已阅读 CONTRIBUTING.md
- [ ] UI / QML 改动附了截图（不适用可删）
- [ ] 若改动了翻译库、i18n、模型或打包资源，已在上文写明
- [ ] AI 使用披露：否 / 是（说明用在哪一部分）
```

The PR body and this file should say the same thing. The PR body ends with the Claude Code
generation line; the file does not need it.

## Extending this skill

A new record type needs three things: a trigger, a directory under `docs/`, and a
template here. Add a row to the table above and a numbered section, then add the trigger
to the `Mandatory records` section of `CLAUDE.md` so it is reachable without loading this
file. Keep the naming convention (`YYYY-MM-DD-<topic>.md`) — it is what makes the
directories sortable and greppable by date.
