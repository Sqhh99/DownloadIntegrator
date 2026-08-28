# PR：修复代码审查发现的 4 个下载链路问题

- **日期：** 2026-08-28
- **分支：** `fix/download-review-findings` → `main`
- **基线提交：** `c894252`
- **关联记录：** [工作记录](../work-logs/2026-08-28-download-review-findings.md)、
  [代码审查归档](../code-reviews/2026-08-26-network-download-code-review.md)

## 关联

无对应 Issue。2026-08-26 那次对 `src/` 网络与下载链路的审查留下 4 条问题，
当时只归档没有实现（PR #38）。本 PR 把这 4 条全部改掉，并把归档文档里的
「Implemented」列同步为已实现。

## 改了什么

### 1. 取消下载不再误伤别的传输（审查 #1，Medium）

`NetworkManager` 过去用一个 `m_currentDownloadReply` 记录「当前下载」，但修改器下载、
应用安装包、翻译数据库三条互不相关的传输都会往里写，于是：先下修改器，再点更新，
此时暂停修改器会 **abort 掉应用更新**——修改器继续下，更新静默失败，任务列表还显示「已暂停」。

改为按保存路径登记：`QHash<QString, QNetworkReply*> m_activeDownloads`，
`cancelDownload()` 改成 `cancelDownload(const QString& savePath)`，只能取消调用方指名的那一路。
`DownloadManager` 记住自己的 `m_currentSavePath` 并传入，取消修改器下载不会碰到另外两条。

### 2. 版本号进入保存路径前先净化（审查 #2，Medium）

`Backend::downloadModifier` 只净化了游戏名，紧挨着的版本号直接拼进路径，而版本号是从站点
抓来的锚文本，`formatVersionString` 匹配不上格式时会原样返回。一个 `Steam/Epic` 这样的
标签就会让下载文件落到意料之外的子目录里；页面被污染时可以写到下载目录之外。

现在抽出 `sanitizePathComponent()`，游戏名和版本号都过一遍：非法字符与控制字符替换成 `_`，
去首尾空白与点，全被去掉时退回占位名。任务里显示用的版本文本保持原样，只有路径被净化。

### 3. 消除未初始化指针（审查 #3，Low–Medium）

`m_currentDownloadReply` 既没有默认初始化也不在构造函数初始化列表里，`cancelDownload()`
会解引用它。第 1 项的改造直接把这个裸指针换成了默认构造为空的 `QHash`，问题从
「初始化一下就好」变成「结构上不可能发生」。

### 4. 写盘失败不再报告成功（审查 #4，Low）

`readyRead` 里 `file->write(data)` 的返回值被丢弃，`bytesWritten` 统计的是**收到**的字节
而不是**落盘**的字节，成功判定又只看 `fileSize == 0 || bytesWritten == 0`。磁盘写满时两者
都非零，于是一个截断的压缩包会被当成下载完成，从 `.crdownload` 改名进用户的库里。

现在比对 `write()` 的返回值、只累计真正写入的字节；`finished` 里在 `close()` 之前
`flush()` 并读取 `file->error()`（`close()` 会清掉错误状态），写失败时回调失败并带上
文件错误信息。未设置 `keepPartialOnAbort` 时删掉残留文件；设置了则保留，等磁盘腾出空间后续传。

## 怎么验证

- [ ] `build.cmd tests`
- [ ] 本地跑过相关界面 / 下载 / 搜索路径

**以上两项都没有勾选，因为确实没有执行。** 本次改动是在 WSL 里完成的，
`build.cmd` 需要 Windows 上的 Visual Studio 2022 与 Qt，无法在此环境运行。
代码是通过通读源码和调用链人工确认的，**合并前需要在 Windows 上补跑**：

1. `cmd.exe /c build.cmd tests`；
2. 手动验证三条路径：下载修改器时暂停/续传、下载中途触发应用更新检查、翻译数据库更新。

新增的回归测试是 `tests/unit/download_manager_test.cpp` 里的
`CancelDownloadWithoutActiveTransferIsSafe`，它复现的正是第 3 项过去会读到未初始化指针的场景。
该文件已在 `TEST_SOURCES` 中，无需改 CMake。

## 检查项

- [x] 已阅读 [CONTRIBUTING.md](https://github.com/Sqhh99/FLiNG-Downloader/blob/main/CONTRIBUTING.md)
- [ ] UI / QML 改动附了截图（本 PR 未改动 QML）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明 —— 均未改动，也没有新增需要翻译的用户可见文案
- [x] AI 使用披露：**是**。全部代码改动、文档与本 PR 说明均由 Claude Code 生成；
      修复方案来自对 `src/` 源码与调用链的通读，未编译验证，见上方「怎么验证」。

## 附带的规则改动

`CLAUDE.md` 新增 `## Mandatory records` 一节，把三条记录规则固定下来：

1. 每次「请求 → 计划 → 改文件」写 `docs/work-logs/YYYY-MM-DD-<topic>.md`；
2. 每次代码审查后归档 `docs/code-reviews/YYYY-MM-DD-<short-title>.md`；
3. 开 PR 前先写 `docs/pull-requests/YYYY-MM-DD-<branch-topic>.md` 并用作 PR 正文。

同时写明：没有真正执行过的验证命令不许打勾。

### 规则抽成可复用的 skill

上述规则随后被抽到 `agents/skills/project-records/SKILL.md`：三类记录的触发条件与目录
对照表、各自的模板骨架、跨文档互链约定，以及「如何新增第四类记录」的扩展指引。
`CLAUDE.md` 里保留触发表（不加载 skill 也要能看到「什么时候欠一份文档」），细节指向 skill。

配套改了 `.gitignore`：原先第 111 行整个忽略 `agents/`，放在那里的 skill 永远提交不上去，
别人克隆后只会看到 `CLAUDE.md` 指向一个不存在的文件。现改为 `agents/*` 加一条
`!agents/skills/` 反向规则，只放出共享的 skill 目录，`agents/prompts/` 维持忽略不变。

**已知限制：** Claude Code 只在 `.claude/skills/<name>/SKILL.md` 下自动发现 skill，
放在 `agents/skills/` 里它是一份由 `CLAUDE.md` 引用的文档，而不是可直接调用的技能。
文件本身带了 YAML frontmatter，若要改成可调用，整个目录移到 `.claude/skills/` 即可，
内容无需改动。
