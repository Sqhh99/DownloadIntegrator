# PR：修复修改器下载全部返回 403

- **日期：** 2026-09-06
- **分支：** `fix/download-referer-403` → `main`
- **基线提交：** `bad5d7e`
- **关联记录：** [工作记录](../work-logs/2026-09-06-download-403-referer.md)

## 关联

关联 issue [#40](https://github.com/Sqhh99/FLiNG-Downloader/issues/40)
（v1.1.9，所有修改器均下载失败）。

这里**故意不写 `Fixes #40`**：按维护者要求，issue 由人工确认线上表现后手动关闭，
不通过合并自动关掉。

## 改了什么

上游 flingtrainer.com 给下载加了防盗链，导致**所有**修改器下载都失败。
现在下载链路是 3 跳，最后一跳被拦：

| # | URL | 结果 |
|---|-----|------|
| 1 | `flingtrainer.com/downloads/<token>,,` | 302 |
| 2 | `flingtrainer.com/download-trainer.php?path=%2Fwp-content%2Fuploads%2F...zip` | 302 |
| 3 | `flingtrainer.com/wp-content/uploads/trainer-files/2026/09/<name>.zip/<name>.exe` | **403** |

用 curl 做的对照实验，确认拦截条件是**同源 Referer**，与 User-Agent 无关：

| 条件 | 结果 |
|------|------|
| `Referer: https://flingtrainer.com/` | **200**，`application/x-msdownload`，魔数 `MZ` |
| 不带 `Referer` | **403** |
| `Referer: https://www.google.com/` | **403**（必须同源） |
| 空 User-Agent + 正确 Referer | **200** |

**用户能观察到的变化：** 下载重新可用，不再几秒后弹失败。落盘文件会是
`<名称>_<版本>.exe` —— 因为第 3 跳返回的是从 zip 里解出来的 exe，而不是 zip 本身；
`Backend` 原本就会调用 `correctFileExtension()`，`detectFileFormat()` 也早已把魔数 `MZ`
映射成 `exe`，所以这一段无需改代码。

**改动本身：** 在 `NetworkManager::downloadFileWithStatus()` 里给下载请求补上 `Referer`。
四个 `downloadFile` / `downloadFileWithStatus` 重载最终都汇到这一个函数，
所以只有一处构造请求的地方要改，**没有改动任何函数签名**。

Referer 取目标 URL 自己的源（`scheme://host[:port]/`），而不是把详情页地址一路传下来：

- 站点只校验同源，取源即可满足；
- 同一套写法对 `AppUpdateManager` / `DatabaseUpdateManager` 的 GitHub、Gitee
  下载同样成立，不用改它们的调用点；
- 只保留源、丢掉 path，也避免把下载 token 泄露给跳转目标。

推导逻辑单独拆成 public 静态方法 `NetworkManager::defaultRefererForUrl()`，以便写单元测试。

**经实测确认不受影响、因此没有改的部分：**

- 搜索页 `/?s=` 与详情页仍返回 200，href 仍是
  `href="https://flingtrainer.com/downloads/<token>,," class="attachment-link"`，
  `ModifierParser` 照旧能解析；`sendGetRequest()` 未改动。
- `/wp-content/uploads/**.jpg` 封面图没有防盗链，绕开 `NetworkManager`
  自己发请求的 `CoverExtractor` 不用动。
- `DownloadManager::cleanUrl()` 去掉结尾 `,,` 无害：带与不带，第 1 跳跳转结果完全一样。
- 第 3 跳支持 Range（206），`.crdownload` 断点续传这条路仍然成立。

## 怎么验证

- [x] `build.cmd tests`
- [x] 本地跑过相关界面 / 下载 / 搜索路径

以上两项由仓库作者在 Windows 本机执行并反馈通过。**AI 未自行构建或运行**：
`build.cmd` 需要 Windows，而改动是在 WSL 里完成的，该环境下 interop 起不来 `cmd.exe`。

新增的两个单元测试只覆盖 `defaultRefererForUrl()` 的推导（同源、保留非默认端口、
无 scheme/host 时返回空）。现有的 `TestDownloadRequestHandler` 测试桩在构造
`QNetworkRequest` **之前**就返回了，因此任何单元测试都无法证明这个请求头真的发上了网络 ——
这一点只能靠上面的实跑。断点续传与扩展名纠正这两个子项，作者未单独反馈结果。

## 检查项

- [x] 已阅读 [CONTRIBUTING.md](https://github.com/Sqhh99/FLiNG-Downloader/blob/main/CONTRIBUTING.md)
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明（本次均未改动）
- [x] AI 使用披露：是。上游行为的排查（curl 对照实验）、代码改动、单元测试与本记录均由
  Claude Code 生成；构建与下载实测由作者在 Windows 上完成。
