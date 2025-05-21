#pragma once
// 移除对ui_DownloadIntegrator.h的依赖
#include "ui_DownloadIntegrator.h"
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

// 使用自动生成的UI类
namespace Ui {
class DownloadIntegrator;
}

// 游戏修改器信息结构体已移动到ModifierParser.h

// DownloadedModifierInfo已在ModifierManager.h中定义，这里删除重复定义

// 创建UI相关结构体，代替ui_DownloadIntegrator.h
struct DownloadIntegratorUI {
    // 主布局
    QVBoxLayout* mainLayout;
    
    // 标签页控件
    QTabWidget* tabWidget;
    QWidget* searchTab;
    QWidget* downloadedTab;
    
    // 搜索区域
    QLineEdit* searchEdit;
    QPushButton* searchButton;
    QComboBox* sortComboBox;
    
    // 主分割区
    QSplitter* mainSplitter;
    
    // 左侧修改器列表
    QTableWidget* modifierTable;
    
    // 右侧详细信息区
    QLabel* gameTitle;
    QLabel* versionInfo;
    QLabel* optionsCount;
    QLabel* lastUpdate;
    
    // 版本选择区
    QRadioButton* autoUpdateVersion;
    QRadioButton* standaloneVersion;
    QComboBox* versionSelect;
    
    // 修改器选项显示区
    QTextEdit* modifierOptions;
    
    // 下载进度条
    QProgressBar* downloadProgress;
    
    // 按钮区
    QPushButton* downloadButton;
    QPushButton* openFolderButton;
    QPushButton* settingsButton;
    
    // 已下载修改器管理
    QTableWidget* downloadedTable;
    QPushButton* runButton;
    QPushButton* deleteButton;
    
    // 菜单操作
    QAction* actionExit;
    QAction* actionSettings;
    
    // 主题菜单
    QMenu* menuTheme;
    QAction* actionLightTheme;
    QAction* actionWin11Theme;
    QActionGroup* themeActionGroup;
    
    // 语言菜单
    QMenu* menuLanguage;
    QAction* actionChineseLanguage;
    QAction* actionEnglishLanguage;
    QAction* actionJapaneseLanguage;
    QActionGroup* languageActionGroup;
    
    // 状态栏
    QStatusBar* statusbar;
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
    Ui::DownloadIntegrator* ui;
    
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
};