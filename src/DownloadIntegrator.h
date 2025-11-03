#pragma once
#include <memory>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QTableWidget>
#include <QProcess>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QTabWidget>
#include <QActionGroup>
#include <algorithm> // 用于std::sort

// 添加设置相关头文件
#include "ConfigManager.h"
#include "SettingsDialog.h"
// 添加HTML解析器
#include "ModifierParser.h"
#include "ModifierManager.h"
// 添加文件系统工具
#include "FileSystem.h"
// 添加UI辅助工具
#include "UIHelper.h"
// 添加主题管理器
#include "ThemeManager.h"
// 添加语言管理器
#include "LanguageManager.h"
// 添加封面提取器
#include "CoverExtractor.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>

// 游戏修改器信息结构体已移动到ModifierParser.h

// DownloadedModifierInfo已在ModifierManager.h中定义，这里删除重复定义

class DownloadIntegratorUI {
public:
    void setupUi(QMainWindow* window);
    void retranslateUi(QMainWindow* window);

    // UI 更新接口
    void resetModifierDetails();
    void populateModifierList(const QList<ModifierInfo>& modifiers);
    void populateModifierListPlaceholder();
    void displayModifierDetail(const ModifierInfo& modifier);
    void setModifierOptionsHtml(const QString& html);
    void clearModifierOptions();
    void showModifierCoverLoading(const QString& message);
    void showModifierCoverPixmap(const QPixmap& pixmap);
    void showModifierCoverError(const QString& message);
    void clearModifierCover();
    void updateDownloadedModifiersTable(const QList<DownloadedModifierInfo>& modifiers);
    void setDownloadedModifiersPlaceholder(int minimumRows = 10);
    void setDownloadProgressVisible(bool visible);
    void setDownloadProgressValue(int value, const QString& format = QString());
    void setDownloadProgressStyle(const QString& stylesheet);
    void setDownloadButtonEnabled(bool enabled);
    void setVersionSelectEnabled(bool enabled);
    void setRightPanelVisible(bool visible);
    void setStatusMessage(const QString& message, int timeout = 0);

    // Widget 访问接口（用于信号连接）
    QTabWidget* tabWidget() const { return m_tabWidget; }
    QWidget* searchTab() const { return m_searchTab; }
    QWidget* downloadedTab() const { return m_downloadedTab; }
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_searchButton; }
    QComboBox* sortComboBox() const { return m_sortComboBox; }
    QPushButton* refreshButton() const { return m_refreshButton; }
    QSplitter* mainSplitter() const { return m_mainSplitter; }
    QTableWidget* modifierTable() const { return m_modifierTable; }
    QWidget* rightWidget() const { return m_rightWidget; }
    QLabel* gameTitle() const { return m_gameTitle; }
    QLabel* versionInfo() const { return m_versionInfo; }
    QLabel* optionsCount() const { return m_optionsCount; }
    QLabel* lastUpdate() const { return m_lastUpdate; }
    QLabel* gameCoverLabel() const { return m_gameCoverLabel; }
    QGroupBox* coverGroup() const { return m_coverGroup; }
    QGroupBox* downloadGroup() const { return m_downloadGroup; }
    QComboBox* versionSelect() const { return m_versionSelect; }
    QTextEdit* modifierOptions() const { return m_modifierOptions; }
    QProgressBar* downloadProgress() const { return m_downloadProgress; }
    QPushButton* downloadButton() const { return m_downloadButton; }
    QPushButton* openFolderButton() const { return m_openFolderButton; }
    QPushButton* settingsButton() const { return m_settingsButton; }
    QTableWidget* downloadedTable() const { return m_downloadedTable; }
    QPushButton* runButton() const { return m_runButton; }
    QPushButton* deleteButton() const { return m_deleteButton; }
    QPushButton* checkUpdateButton() const { return m_checkUpdateButton; }
    QAction* actionExit() const { return m_actionExit; }
    QAction* actionSettings() const { return m_actionSettings; }
    QMenu* fileMenu() const { return m_menuFile; }
    QMenu* themeMenu() const { return m_menuTheme; }
    QActionGroup* themeActionGroup() const { return m_themeActionGroup; }
    QAction* actionLightTheme() const { return m_actionLightTheme; }
    QAction* actionWin11Theme() const { return m_actionWin11Theme; }
    QAction* actionClassicTheme() const { return m_actionClassicTheme; }
    QAction* actionColorfulTheme() const { return m_actionColorfulTheme; }
    QMenu* languageMenu() const { return m_menuLanguage; }
    QActionGroup* languageActionGroup() const { return m_languageActionGroup; }
    QAction* actionChineseLanguage() const { return m_actionChineseLanguage; }
    QAction* actionEnglishLanguage() const { return m_actionEnglishLanguage; }
    QAction* actionJapaneseLanguage() const { return m_actionJapaneseLanguage; }
    QStatusBar* statusbar() const { return m_statusbar; }
    QLabel* statusLabel() const { return m_statusLabel; }

private:
    // 主布局
    QTabWidget* m_tabWidget = nullptr;
    QWidget* m_searchTab = nullptr;
    QWidget* m_downloadedTab = nullptr;

    // 搜索区域
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_searchButton = nullptr;
    QComboBox* m_sortComboBox = nullptr;
    QPushButton* m_refreshButton = nullptr;

    // 主分割区
    QSplitter* m_mainSplitter = nullptr;

    // 左侧修改器列表
    QTableWidget* m_modifierTable = nullptr;

    // 右侧详细信息区
    QWidget* m_rightWidget = nullptr;
    QLabel* m_gameTitle = nullptr;
    QLabel* m_versionInfo = nullptr;
    QLabel* m_optionsCount = nullptr;
    QLabel* m_lastUpdate = nullptr;
    QLabel* m_gameCoverLabel = nullptr;
    QGroupBox* m_coverGroup = nullptr;
    QGroupBox* m_downloadGroup = nullptr;

    // 版本选择区
    QComboBox* m_versionSelect = nullptr;

    // 修改器选项显示区
    QTextEdit* m_modifierOptions = nullptr;

    // 下载进度条
    QProgressBar* m_downloadProgress = nullptr;

    // 按钮区
    QPushButton* m_downloadButton = nullptr;
    QPushButton* m_openFolderButton = nullptr;
    QPushButton* m_settingsButton = nullptr;

    // 已下载修改器管理
    QTableWidget* m_downloadedTable = nullptr;
    QPushButton* m_runButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_checkUpdateButton = nullptr;

    // 菜单操作
    QMenuBar* m_menubar = nullptr;
    QMenu* m_menuFile = nullptr;
    QAction* m_actionExit = nullptr;
    QAction* m_actionSettings = nullptr;

    // 主题菜单
    QMenu* m_menuTheme = nullptr;
    QAction* m_actionLightTheme = nullptr;
    QAction* m_actionWin11Theme = nullptr;
    QAction* m_actionClassicTheme = nullptr;
    QAction* m_actionColorfulTheme = nullptr;
    QActionGroup* m_themeActionGroup = nullptr;

    // 语言菜单
    QMenu* m_menuLanguage = nullptr;
    QAction* m_actionChineseLanguage = nullptr;
    QAction* m_actionEnglishLanguage = nullptr;
    QAction* m_actionJapaneseLanguage = nullptr;
    QActionGroup* m_languageActionGroup = nullptr;

    // 状态栏
    QStatusBar* m_statusbar = nullptr;
    QLabel* m_statusLabel = nullptr;
};

class DownloadIntegrator : public QMainWindow {
    Q_OBJECT
    
public:
    explicit DownloadIntegrator(QWidget* parent = nullptr);
    ~DownloadIntegrator();

private slots:
    // 搜索相关
    void onSearchClicked();
    void onSearchCompleted(const QList<ModifierInfo>& modifiers);
    void onModifierItemClicked(int row, int column);
    void onVersionTypeChanged(bool checked);
    void onVersionSelectionChanged(int index);
    void onSortOrderChanged(int index);
    
    // 下载相关
    void onDownloadButtonClicked();
    void onOpenFolderButtonClicked();
    
    // 设置相关
    void onSettingsButtonClicked();
    
    // 已下载修改器管理
    void onDownloadedModifierDoubleClicked(int row, int column);
    void onRunButtonClicked();
    void onDeleteButtonClicked();
    void onCheckUpdateButtonClicked();
    void onTabChanged(int index);
    
    // 主题相关
    void onLightThemeSelected();
    void onWin11ThemeSelected();
    void onClassicThemeSelected();
    void onColorfulThemeSelected();
    
    // 语言相关
    void onChineseLanguageSelected();
    void onEnglishLanguageSelected();
    void onJapaneseLanguageSelected();
    
    // 刷新按钮点击
    void onRefreshButtonClicked();
    
    // 刷新完成槽
    void onRefreshCompleted(const QList<ModifierInfo>& modifiers);
    
    // 解析选项HTML
    void parseOptionsFromHTMLContent(const QString& htmlContent);

private:
    // UI初始化
    void setupSearchCompleter();
    void setupThemeMenu();
    void setupLanguageMenu();
    void retranslateUi();
    
    // UI更新方法
    void updateModifierList();
    void updateModifierList(const QList<ModifierInfo>& modifiers);
    void updateModifierDetail();
    void updateDownloadedModifiersList();
    
    // 辅助方法
    void showStatusMessage(const QString& message, int timeout = 0);
    QString getDownloadDirectory();
    QString getFileExtensionFromUrl(const QString& url);
    QString detectFileFormat(const QString& filePath);
    bool isArchiveFile(const QString& filePath);
    bool isExecutableFile(const QString& filePath);
    void setGameCoverWithAspectRatio(const QPixmap& cover);
    
    // 网络操作方法 - 通过ModifierManager实现
    void simulateDownload(const QString& url, const QString& fileName);
    void downloadModifier(const QString& url, const QString& fileName);
    void fetchModifierList(const QString& searchTerm);
    void fetchModifierDetail(const QString& url);
    void onNetworkError(QNetworkReply::NetworkError code);
    
    // 数据加载方法
    void loadSampleData();
    void loadModifierDetail(const QString& modifierName);
    
    // 下载和更新管理
    void checkForUpdates(int index = -1);
    void addDownloadedModifier(const ModifierInfo& info, const QString& version, const QString& filePath);
    void removeDownloadedModifier(int index);
    void saveDownloadedModifiers();
    
private:
    std::unique_ptr<DownloadIntegratorUI> ui;
    
    // 当前选中的修改器
    ModifierInfo* currentModifier;
    
    // 下载相关
    QString currentDownloadId;
    ModifierInfo* currentDownloadedModifier;
    
    // 修改器列表
    QList<ModifierInfo> modifierList;
    
    // 已下载修改器列表
    QList<DownloadedModifierInfo> downloadedModifiers;
    
    // 已下载修改器标签页组件
    QTabWidget* m_tabWidget;
    QWidget* m_searchTab;
    QWidget* m_downloadedTab;
    QTableWidget* m_downloadedTable;
    QPushButton* m_runButton;
    QPushButton* m_deleteButton;
    QPushButton* m_checkUpdateButton;
    
    // 封面提取器
    CoverExtractor* m_coverExtractor;
};
