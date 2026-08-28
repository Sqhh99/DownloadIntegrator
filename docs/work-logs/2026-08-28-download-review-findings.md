# 工作记录：落实网络/下载链路代码审查的 4 条问题

- **日期：** 2026-08-28
- **分支：** `fix/download-review-findings`（基于 `main` 的 `c894252`）
- **关联文档：** [代码审查归档](../code-reviews/2026-08-26-network-download-code-review.md)、
  [PR 记录](../pull-requests/2026-08-28-download-review-findings.md)

## 一、用户的请求

本次会话共两条请求，按时间顺序：

1. > "Add two rules: one to add code-reviews after the review, and the other to add
   > pull-requests before submitting the pull request."

   把「审查后归档到 `docs/code-reviews/`」和「开 PR 前先写 `docs/pull-requests/` 记录」
   两条约定写进 `CLAUDE.md`，成为固定规则。

2. > "This code review is not yet complete. Please help me finish it and submit a pull
   > request. Every time I submit a request and you formulate a plan and make
   > modifications to the files, this entire process needs to be archived in a `docs`
   > directory. This record should document details such as the nature of my request,
   > the plan devised, and the specific files that were modified. This is also a rule."

   包含三件事：**(a)** 把 2026-08-26 那次审查里挂着的 4 条问题真正改掉；**(b)** 提交 PR；
   **(c)** 再加一条规则——每次「请求 → 计划 → 改文件」的完整过程都要归档，也就是本文件。

## 二、制定的计划

1. 在 `CLAUDE.md` 中把原来的两条规则扩成三条，新增 `docs/work-logs/` 工作记录一条。
2. 从 `origin/main` 切出新分支（上一个 PR #38 已合并，不能继续在旧分支上叠加）。
3. 按审查文档逐条实现 4 条问题的修复，顺序为 #1 → #3 → #2 → #4；
   其中 #3 由 #1 的数据结构改造顺带彻底消除。
4. 更新审查归档的「Implemented」列与每条问题的修复说明。
5. 写 PR 记录，提交并开 PR。

## 三、具体改了哪些文件

### 规则与文档

| 文件 | 改动 |
|------|------|
| `CLAUDE.md` | 新增 `## Mandatory records` 一节，写明三条强制记录规则（工作记录 / 审查归档 / PR 记录），并强调没跑过的验证命令不许打勾 |
| `docs/code-reviews/2026-08-26-network-download-code-review.md` | 4 条问题的「Implemented」由 ❌ 改为 ✅，每条补写 **Fix** 段说明改法，新增 Implementation record 小节与「未编译验证」的声明 |
| `docs/work-logs/2026-08-28-download-review-findings.md` | 本文件（新增） |
| `docs/pull-requests/2026-08-28-download-review-findings.md` | PR 记录（新增） |

### 代码

| 文件 | 对应问题 | 改动 |
|------|----------|------|
| `src/include/NetworkManager.h` | #1 #3 | 删除裸指针成员 `m_currentDownloadReply`，改为 `QHash<QString, QNetworkReply*> m_activeDownloads`；`cancelDownload()` 改签名为 `cancelDownload(const QString& savePath)`；新增私有 `unregisterDownload()` |
| `src/NetworkManager.cpp` | #1 #3 #4 | 下载按保存路径登记与注销；`readyRead` 检查 `file->write()` 返回值、只累计真正写入的字节；`finished` 在 `close()` 前 `flush()` 并读取 `file->error()`，写失败时回调失败而非成功 |
| `src/include/DownloadManager.h` | #1 | 新增成员 `m_currentSavePath`，记录自己那一路下载的目标路径 |
| `src/DownloadManager.cpp` | #1 | 开始下载时记录路径、结束时清除；`cancelDownload()` 只取消自己的那一路（先取副本再 abort，避免同步回调把正在使用的字符串清空） |
| `src/Backend.cpp` | #2 | 抽出 `sanitizePathComponent(text, fallback)`，游戏名与版本号**都**经过净化后再拼接保存路径；任务里显示用的版本文本保持原样 |
| `tests/unit/download_manager_test.cpp` | #3 | 新增 `CancelDownloadWithoutActiveTransferIsSafe`：在没有任何传输时取消一个未知路径，正是过去会读到未初始化指针的场景 |

## 四、验证情况

**没有编译、没有跑测试。** `build.cmd tests` 需要 Windows 上的 Visual Studio 2022 与 Qt，
本次是在 WSL 里改的代码，无法执行。改动是通过通读源码和调用链人工确认的，合并前需要在
Windows 上跑一次 `build.cmd tests` 并实际验证下载/暂停/更新三条路径。

因此 PR 的「怎么验证」两个勾选框都留空。

## 五、遗留事项

- 上面那次 Windows 构建与手动验证。
- 审查文档里 `## Verified correct — do not "fix" these` 一节列出的项目本次没有改动。
- 审查范围之外的部分（QML 层、`ModifierParser.cpp` 的正则健壮性、测试自身覆盖率）仍未审查。
