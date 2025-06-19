# DownloadIntegrator

A Qt-based desktop application for downloading and managing game modifiers/trainers.

## 🚀 Features

- 🔍 **智能搜索**: Search and browse game modifiers from online sources
- 📥 **自动下载**: Automatic download with file type detection and extension correction
- 📂 **文件管理**: Manage downloaded modifiers with organized list view
- 🔧 **智能识别**: Automatic detection of executable files vs. archive files
- 🌍 **多语言支持**: Support for multiple languages (English, Chinese, Japanese)
- 🎨 **主题切换**: Different themes (Light, Windows 11, Classic, Colorful)
- 🔄 **自动更新**: Automatic update checking for downloaded modifiers
- 🛡️ **安全提示**: Safety warnings for archive files with detailed instructions

## 📋 Requirements

- Qt 6.6.3 or higher
- C++17 or higher compiler
- Windows 10 or higher (currently only Windows is supported)
- CMake 3.16 or higher

## 📦 Installation

### Option 1: Download Release
1. Download the latest release from the [Releases page](../../releases)
2. Extract the zip file to a location of your choice
3. Run the `DownloadIntegrator.exe` file

### Option 2: Build from Source
1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/DownloadIntegrator.git
   cd DownloadIntegrator
   ```

2. Create build directory and configure:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. Build the project:
   ```bash
   cmake --build . --config Release
   ```

4. Run the application:
   ```bash
   ./Release/DownloadIntegrator.exe
   ```

## 🛠️ Development

### Project Structure
```
DownloadIntegrator/
├── src/                    # Source code
│   ├── DownloadIntegrator.*  # Main application
│   ├── DownloadManager.*    # Download management
│   ├── ModifierManager.*    # Modifier management
│   └── ...
├── resources/              # Resources (icons, styles, translations)
├── translations/           # Language files
├── build/                  # Build output
└── release-package/        # Release packaging
```

### Key Components
- **DownloadManager**: Handles file downloads with automatic type detection
- **ModifierManager**: Manages downloaded modifiers and metadata
- **NetworkManager**: Network operations and HTTP requests
- **FileSystem**: File operations and path management
- **SearchManager**: Search functionality and result parsing

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### MIT License Summary
- ✅ Commercial use
- ✅ Modification
- ✅ Distribution
- ✅ Private use
- ❌ Liability
- ❌ Warranty

## ⚠️ Disclaimer

This application is designed for educational and personal use only. Users are responsible for ensuring they comply with all applicable laws and the terms of service of the websites they access. The developers are not responsible for any misuse of this software.

## 📞 Support

If you encounter any issues or have questions:
- Open an [Issue](../../issues) on GitHub
- Check the [Wiki](../../wiki) for documentation
- Review existing [Discussions](../../discussions)

## 🎯 Roadmap

- [ ] Support for more platforms (macOS, Linux)
- [ ] Enhanced search filters and sorting
- [ ] Batch download functionality
- [ ] Plugin system for custom sources
- [ ] Advanced file organization features

---

**Made with ❤️ using Qt and C++**