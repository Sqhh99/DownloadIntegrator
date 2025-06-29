#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include "ConfigManager.h"

/**
 * @brief 设置对话框类
 * 
 * 提供用户界面来修改应用程序的各种设置
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit SettingsDialog(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~SettingsDialog();
    
private slots:
    /**
     * @brief 浏览下载目录
     */
    void onBrowseDownloadDirectory();
    
    /**
     * @brief 重置所有设置为默认值
     */
    void onResetToDefaults();
    
    /**
     * @brief 保存设置
     */
    void onSaveSettings();
    
    /**
     * @brief 取消设置
     */
    void onCancelSettings();
    
private:
    /**
     * @brief 初始化UI组件
     */
    void initUI();
    
    /**
     * @brief 加载当前设置
     */
    void loadSettings();
    
    // UI组件
    QLineEdit* m_downloadDirEdit;
    QLineEdit* m_userAgentEdit;
    QSpinBox* m_timeoutSpinBox;
    QCheckBox* m_autoCheckUpdatesCheckBox;
    
    // 按钮
    QPushButton* m_browseButton;
    QPushButton* m_resetButton;
    QDialogButtonBox* m_buttonBox;
}; 