# 工作记录：修复修改器下载全部返回 403

- **日期：** 2026-09-06
- **分支：** `fix/download-referer-403`（基于 `main` 的 `bad5d7e`）
- **关联文档：** 对应 issue [#40](https://github.com/Sqhh99/FLiNG-Downloader/issues/40)；[PR 记录](../pull-requests/2026-09-06-download-403-referer.md)

## 一、用户的请求

> An error is currently occurring with the software trainer downloads; in fact, error
> messages appear whenever I try to download any trainer. I suspect that the URL structure
> of the upstream software site has changed. Please help me troubleshoot and resolve this
> issue.

用户同时附上了运行日志，关键几行是：

```
NetworkManager: Error occurred during download: QNetworkReply::ContentAccessDenied
  "... - server replied: Forbidden"
NetworkManager: HTTP status code: 403
```

随后用户补充：

> https://github.com/Sqhh99/FLiNG-Downloader/issues/40, After modifying the code, I will
> first build and test the download functionality, then submit a PR and link it to this issue.

即：先定位并修复，代码改完后由用户自己构建、实测下载，通过后再提 PR 并关联 issue #40。

## 二、制定的计划

1. 用 curl 直接打上游站点，把 302 跳转链完整走一遍，确认到底是哪一跳返回 403。
2. 逐个变量做对照实验（Referer / User-Agent / 带不带结尾的 `,,`），分离出真正的原因。
3. 确认搜索、详情页解析、封面图这几条链路是否也受影响，划清改动范围。
4. 只改真正出问题的那一处，并补可测的单元测试。
5. 写工作记录；PR 记录等用户实测通过后再写。

## 三、排查结论

上游的下载链路从 1 跳变成了 3 跳：

| # | URL | 结果 |
|---|-----|------|
| 1 | `flingtrainer.com/downloads/<token>,,` | 302 |
| 2 | `flingtrainer.com/download-trainer.php?path=%2Fwp-content%2Fuploads%2F...zip` | 302 |
| 3 | `flingtrainer.com/wp-content/uploads/trainer-files/2026/09/<name>.zip/<name>.exe` | **403** |

第 3 跳加了防盗链。对照实验：

| 条件 | 结果 |
|------|------|
| `Referer: https://flingtrainer.com/` | **200**，`application/x-msdownload`，魔数 `MZ` |
| 不带 `Referer` | **403** |
| `Referer: https://www.google.com/` | **403**（必须同源） |
| 空 User-Agent + 正确 Referer | **200**（UA 与此无关） |

所以真正的原因是**缺少同源 `Referer` 请求头**，不是解析失败。以下经实测确认不受影响，因此不改：

- 搜索页 `/?s=` 与详情页仍返回 200，href 仍是
  `href="https://flingtrainer.com/downloads/<token>,," class="attachment-link"`，
  `ModifierParser` 照旧能解析（用户日志里两个版本都正确识别了）。
- `/wp-content/uploads/**.jpg` 封面图**没有**防盗链（不带 Referer 也是 200），
  所以绕开 `NetworkManager` 自己发请求的 `CoverExtractor` 不用动。
- `DownloadManager::cleanUrl()` 去掉结尾 `,,` 无害：带与不带，第 1 跳的跳转结果完全一样。
- 第 3 跳支持 Range（返回 206），所以 `.crdownload` 断点续传这条路仍然成立。

**一个附带变化：** 第 3 跳给出的是从 zip 里解出来的 `.exe`，不再是 zip 本身。
但 `Backend` 在改名后本来就会调用 `DownloadManager::correctFileExtension()`，
而 `detectFileFormat()` 早已把魔数 `MZ` 映射成 `exe`，所以文件会被自动改成 `.exe`，
**这一段不需要改代码**，但需要在实测时确认一下。

## 四、具体改了哪些文件

| 文件 | 改动 | 对应问题 |
|------|------|----------|
| `src/include/NetworkManager.h` | 新增 public 静态方法 `defaultRefererForUrl()` 声明 | 把 Referer 推导单独拆出来，才有办法写单元测试 |
| `src/NetworkManager.cpp` | 实现 `defaultRefererForUrl()`；在 `downloadFileWithStatus()` 里给请求设置 `Referer` | 403 的直接原因 |
| `tests/unit/download_manager_test.cpp` | 新增两个用例，覆盖 Referer 推导 | 回归保护 |
| `docs/work-logs/2026-09-06-download-403-referer.md` | 本文件 | CLAUDE.md 要求 |

几点说明：

- 四个 `downloadFile` / `downloadFileWithStatus` 重载最终都汇到同一个
  `downloadFileWithStatus(url, savePath, context, ...)`，所以只有一处构造请求的地方要改，
  **没有改动任何函数签名**。
- Referer 取的是目标 URL 自己的源（`scheme://host[:port]/`），不是把详情页地址一路传下来。
  理由：站点的规则只校验同源；取源的写法对 `AppUpdateManager` /
  `DatabaseUpdateManager` 的 GitHub、Gitee 下载同样成立，不必改它们的调用点。
  另外只保留源、丢掉 path，也避免把下载 token 泄露给跳转目标。
- 没有动 `sendGetRequest()`：搜索和详情页没有被拦，改它只会扩大 diff。
- 依赖的一个前提：Qt 会把原始请求头复制到重定向请求上。这 3 跳都是同源，所以设置一次即可
  覆盖整条链。**这一点只能靠实跑验证**，见下。

## 五、验证情况

- **AI 侧没有构建、没有跑测试。** 本次改动在 WSL 里完成，`build.cmd` 需要 Windows；
  这台机器上的 WSL interop 也起不来 `cmd.exe`（试了 `cmd.exe /c build.cmd tests`，
  cmd.exe 被当成 shell 脚本执行，报 `MZ...: not found`）。
- **作者已在 Windows 本机构建并实测通过**（2026-09-06 反馈：测试通过，可以提 PR）。
  断点续传与扩展名纠正这两个子项没有单独反馈结果。
- 已完成的验证只有**对站点行为的实测**：上面那两张表全部是 curl 实跑的结果，
  包括下载成功时的 1,765,888 字节与 `MZ` 魔数。
- 新增的单元测试只覆盖 `defaultRefererForUrl()` 的推导逻辑。
  现有的 `TestDownloadRequestHandler` 测试桩在构造 `QNetworkRequest` **之前**就返回了，
  因此任何单元测试都无法证明这个请求头真的发上了网络 —— 那只能靠实跑。

需要用户执行的验证：

1. `build.cmd tests`：编译并跑测试。
2. `build.cmd run debug`，这才是能证明修复有效的一步：
   - 搜索并下载一个修改器，确认能下完，而不是又打印 `HTTP status code: 403`；
   - 确认落盘文件是 `<名称>_<版本>.exe`（由魔数纠正扩展名而来），且出现在已下载列表里；
   - 下载中途暂停再继续，确认 `.crdownload` + Range 这条路在跳转链下仍然可用。

## 六、遗留事项

- 若实测仍然 403，说明 Qt 在重定向时丢掉了这个请求头。备用方案：把
  `RedirectPolicyAttribute` 改成手动模式，每一跳自己重新设置 `Referer`。
- issue #40 **不通过合并自动关闭**：PR 里只写「关联」而不写 `Fixes #40`，
  按维护者要求由人工确认线上表现后手动关掉。
- 工作区里还有两处与本次无关、且不是本次会话产生的内容，**没有**纳入本次提交：
  - `CMakeLists.txt` 未提交的本地改动（第 178 行 `WIN32` 被注释掉），
    这是为了让控制台日志可见的调试手法，不应提交；
  - 未跟踪文件 `docs/code-reviews/2026-09-04-ui-redundancy-performance.md` 与
    `docs/work-logs/2026-09-04-ui-redundancy-performance.md`。
