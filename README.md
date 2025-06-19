# DownloadIntegrator


基于 Qt ，用于下载和管理游戏修改器。

## 🚀 功能特性

- 🔍 **智能搜索**: 搜索和浏览在线游戏修改器资源
- 📥 **自动下载**: 自动下载并检测文件类型，自动修正扩展名
- 📂 **文件管理**: 使用有序列表管理已下载的修改器
- 🔧 **智能识别**: 自动识别可执行文件与压缩包文件
- 🌍 **多语言支持**: 支持多种语言（英文、中文、日文）
- 🎨 **主题切换**: 多种主题（浅色、Windows 11、经典、多彩）
- 🔄 **自动更新**: 自动检查已下载修改器的更新
- 🛡️ **安全提示**: 对压缩包文件提供安全警告和详细说明

## 📋 系统要求

- Qt 6.6.3 或更高版本
- C++17 或更高版本编译器
- Windows 10 或更高版本（目前仅支持 Windows）
- CMake 3.16 或更高版本

## 📦 安装方法

### 方式一：下载发布版本
1. 从 [Releases 页面](../../releases) 下载最新版本
2. 将 zip 文件解压到您选择的位置
3. 运行 `DownloadIntegrator.exe` 文件

### 方式二：从源码构建
1. 克隆此仓库：
   ```bash
   git clone https://github.com/yourusername/DownloadIntegrator.git
   cd DownloadIntegrator
   ```

2. 创建构建目录并配置：
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. 构建项目：
   ```bash
   cmake --build . --config Release
   ```

4. 运行应用程序：
   ```bash
   ./Release/DownloadIntegrator.exe
   ```

## 🛠️ 开发说明

### 项目结构
```
DownloadIntegrator/
├── src/                    # 源代码
│   ├── DownloadIntegrator.*  # 主应用程序
│   ├── DownloadManager.*    # 下载管理
│   ├── ModifierManager.*    # 修改器管理
│   └── ...
├── resources/              # 资源文件（图标、样式、翻译）
├── translations/           # 语言文件
├── build/                  # 构建输出
└── release-package/        # 发布打包
```

### 核心组件
- **DownloadManager**: 处理文件下载和自动类型检测
- **ModifierManager**: 管理已下载的修改器和元数据
- **NetworkManager**: 网络操作和 HTTP 请求
- **FileSystem**: 文件操作和路径管理
- **SearchManager**: 搜索功能和结果解析

## 🤝 贡献指南

欢迎贡献代码！请随时提交 Pull Request。对于重大更改，请先创建 issue 讨论您想要进行的更改。

1. Fork 仓库
2. 创建您的功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

## 📄 开源协议

本项目基于 **MIT 协议** 开源 - 查看 [LICENSE](LICENSE) 文件了解详情。

### MIT 协议要点
- ✅ 商业使用
- ✅ 修改
- ✅ 分发
- ✅ 私人使用
- ❌ 责任
- ❌ 保证

## ⚠️ 免责声明

此应用程序仅供教育和个人使用。用户有责任确保遵守所有适用的法律和他们访问的网站的服务条款。开发者不对此软件的任何误用负责。

## 📞 技术支持

如果您遇到任何问题或有疑问：
- 在 GitHub 上创建 [Issue](../../issues)
- 查看 [Wiki](../../wiki) 获取文档
- 查看现有的 [Discussions](../../discussions)

## 🎯 发展路线图

- [ ] 支持更多平台（macOS、Linux）
- [ ] 增强搜索过滤和排序功能
- [ ] 批量下载功能
- [ ] 自定义源的插件系统
- [ ] 高级文件组织功能

---

**Made with ❤️ using Qt and C++**