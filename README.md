<div align="center">

  <img src="https://github.com/user-attachments/assets/e8aceb6b-2534-4aaf-a757-020b654aa285" alt="DownloadIntegrator Logo" width="200">
  
  <h1>DownloadIntegrator</h1>

  <strong>基于 Qt 开发的游戏修改器下载管理工具，支持智能中文搜索</strong>

</div>

<p align="center">
    <a href="https://github.com/Sqhh99/DownloadIntegrator/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="license"></a>
    <a href="https://github.com/Sqhh99/DownloadIntegrator/stargazers"><img src="https://img.shields.io/github/stars/Sqhh99/DownloadIntegrator.svg?style=flat" alt="stars"></a>
    <a href="https://github.com/Sqhh99/DownloadIntegrator/releases"><img src="https://img.shields.io/github/downloads/Sqhh99/DownloadIntegrator/total.svg?style=flat" alt="downloads"></a>
    <a href="https://github.com/Sqhh99/DownloadIntegrator/releases/latest"><img src="https://img.shields.io/github/v/release/Sqhh99/DownloadIntegrator.svg" alt="release"></a>
    <a href="https://github.com/Sqhh99/DownloadIntegrator/commits/main"><img src="https://img.shields.io/github/last-commit/Sqhh99/DownloadIntegrator.svg" alt="last commit"></a>
</p>

<p align="center">
  <a href="./README.md">简体中文</a> |
  <a href="./README.en.md">English</a> |
  <a href="./README.ja.md">日本語</a>
</p>

---

## 🚀 主要功能 (Features)

- 🎨 **现代化用户界面**: 支持浅色、Windows 11、经典、多彩等多种主题。
- 🔍 **智能中文搜索**: 支持中文游戏名搜索，并能自动映射为英文进行查找。
- 📥 **一键下载管理**: 自动下载、分类并管理所有修改器文件。
- 🌍 **多语言界面**: 内置简体中文、英文、日文支持。
- 🔄 **自动更新检测**: 实时检查并提示修改器的可用更新。

## 🖥️ 软件界面 (Screenshot)

![image](https://github.com/user-attachments/assets/7939aa55-7958-49bb-a076-bceef6ca682c)

## 📋 系统要求 (Requirements)

- Windows 10 或更高版本
- 软件已静态链接所有必需库，无需额外安装任何依赖项

## 📦 快速开始 (Quick Start)

1.  从 **[Releases 页面](../../releases)** 下载最新版本
2.  解压并运行 `DownloadIntegrator.exe`
3.  启动软件，在搜索框输入中文游戏名即可开始搜索

## 🔧 开发环境 (Development)

- **Qt**: 6.6.3+
- **Compiler**: MSVC 2019+
- **Build System**: CMake 3.16+

## 📄 开源协议 (License)

本项目基于 MIT 协议开源，详情请参阅 [LICENSE](LICENSE) 文件。

## 📞 问题反馈 (Feedback)

如在使用过程中遇到任何问题，欢迎在 GitHub 创建 **[Issue](../../issues)**。

## 🛡️ 安全声明 (Security Notice)

### Windows Defender 误报问题

由于本软件具有网络下载和文件管理功能，可能会被 Windows Defender 误报为威胁（如 `Win32/Wacapew.C!ml`）。这是**误报**，软件是安全的。

**解决方案：**

1. **添加排除项**（推荐）：
   - 打开 Windows 安全中心
   - 转到"病毒和威胁防护"
   - 点击"病毒和威胁防护设置"
   - 在"排除项"中添加软件所在文件夹

2. **报告误报**：
   - 访问 [Microsoft 安全智能](https://www.microsoft.com/en-us/wdsi/filesubmission)
   - 提交误报文件以帮助改善检测

3. **源码验证**：
   - 本项目完全开源，可查看所有源代码
   - 支持自行编译验证安全性

> 📋 **详细解决方案**: 请参阅 **[杀毒软件误报问题 FAQ](./docs/ANTIVIRUS_FAQ.md)** 获取完整的解决步骤和其他杀毒软件的设置方法。

**为什么会误报：**
- 软件包含网络下载功能
- 自动文件管理和解压缩操作  
- 静态链接所有依赖库
- Windows Defender 的启发式检测机制

### 隐私保护

- 软件不收集任何个人信息
- 所有网络请求仅用于修改器搜索和下载
- 配置文件仅存储在本地

---

<p align="center">
  <strong>Made with ❤️ using Qt and C++</strong>
</p>
