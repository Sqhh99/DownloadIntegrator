# PR：补全 CLAUDE.md 并归档一次代码审查

- **日期：** 2026-08-26
- **分支：** `docs/claude-md-and-code-review` → `main`
- **基线提交：** `6ae1341`
- **性质：** 纯文档改动，**未改动任何应用代码**

## 关联

无对应 Issue。本 PR 解决两件事：

1. 仓库根目录的 `CLAUDE.md` 此前只有一行内容（指向 `AGENTS.md`），对 Claude Code
   没有实际帮助，且在工作区中处于被删除状态。
2. 一次针对 `src/` 网络与下载链路的代码审查产出了 4 条问题，需要有地方留档，
   否则结论只存在于会话里。

## 改了什么

### 1. `CLAUDE.md`（重写）

从一行指针扩写为完整的项目指引，内容都是需要交叉阅读多个文件才能得出的信息：

- **构建与测试**：`build.cmd` 各子命令；在 WSL 下必须用 `cmd.exe /c` 调用；
  ctest 只注册了一个聚合测试，跑单个用例要直接调可执行文件加 `--gtest_filter`，
  且必须在构建目录根部运行（`CoverExtractor` 依赖 `applicationDirPath()` 旁边的
  `onnxruntime.dll` 与 `models/`）。
- **架构**：`Backend` 是唯一的 QML 桥接对象；`src/` 下各管理器为回调驱动的单例、
  不感知 QML；样式全在 `ThemeProvider.qml`，C++ `ThemeManager` 只存索引；
  新增 QML 文件必须登记进 `qt_add_qml_module`；翻译库内置版与 AppData 覆盖层的
  择优逻辑与必需字段；启动器与 `app\` 目录的打包结构。
- **测试**：测试与主程序共用 `TESTABLE_PROJECT_SOURCES`，因此直接链接生产单例；
  新测试文件需加入 `TEST_SOURCES`；`tests/fixtures/` 提供的隔离接缝。
- **注意事项**：版本号由 git tag 推导、vcpkg triplet 与 `builtin-baseline`、
  i18n 流程，以及 `agents/prompts/knowledge_base.md` 是与本项目无关的 WebRTC 残留。

其中 4 条初稿中的错误说法已在本 PR 内自查修正：`NetworkManager` 并非唯一发起 HTTP 的
位置（`CoverExtractor` 自持 `QNetworkAccessManager`，因此绕过测试钩子）、CI 并没有
`i18n check` 门禁、`metadata.schema_version` 并非必需字段、封面防陈旧的 request id
位于 `Backend` 而非 `CoverExtractor`。

### 2. `docs/code-reviews/2026-08-26-network-download-code-review.md`（新增）

归档一次代码审查记录，含 4 条问题：

| # | 问题 | 严重度 | 是否已修复 |
|---|------|--------|------------|
| 1 | 暂停/取消会中止错误的下载任务 | Medium | ❌ 否 |
| 2 | 远端抓取的版本文本未经清洗即拼入保存路径 | Medium | ❌ 否 |
| 3 | `m_currentDownloadReply` 未初始化 | Low–Medium（潜在） | ❌ 否 |
| 4 | `file->write()` 返回值被丢弃，短写会被判为成功 | Low | ❌ 否 |

文档同时记录了「已确认正确、不要误改」的若干处（`CoverExtractor` 的线程边界与检测器
互斥锁、各处防陈旧 guard、`QPointer` 上下文保护等），以及本次未覆盖的范围。

## 怎么验证

- [ ] `build.cmd tests` — **本 PR 未运行**。改动仅为 Markdown 文档，不进入任何构建
      目标，`CMakeLists.txt`、`src/`、`qml/`、`tests/`、`resources/` 均未触及。
- [ ] 本地跑过相关界面 / 下载 / 搜索路径 — 不适用，无行为变更。

文档中引用的每一处 `file:line` 均已逐条比对源码核对过（初稿有 6 处行号因误读
`grep -A` 偏移而写错，已更正）。

## 检查项

- [x] 已阅读 [CONTRIBUTING.md](https://github.com/Sqhh99/FLiNG-Downloader/blob/main/CONTRIBUTING.md)
- [x] UI / QML 改动附了截图 —— 不适用，无 UI 改动
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明 —— 均未改动
- [x] AI 使用披露：**是**。`CLAUDE.md`、代码审查记录与本 PR 文档均由 Claude Code
      起草；代码审查结论由其阅读 `src/` 源码并追踪调用链得出，其中的行号与事实性
      结论已逐条回查源码验证。本 PR 不含任何由模型生成的应用代码。

## 后续

本 PR 只做留档，不修复上述 4 条问题。若要跟进，建议顺序为 #1、#2。
