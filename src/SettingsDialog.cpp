#include "SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    setMinimumWidth(450);
    
    // 初始化UI
    initUI();
    
    // 加载当前设置
    loadSettings();
    
    // 连接信号和槽
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseDownloadDirectory);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::onResetToDefaults);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveSettings);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onCancelSettings);
}

SettingsDialog::~SettingsDialog()
{
    // 所有QObject子对象会自动删除，无需手动清理
}

void SettingsDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 下载设置组
    QGroupBox* downloadGroup = new QGroupBox(tr("下载设置"), this);
    QFormLayout* downloadLayout = new QFormLayout(downloadGroup);
    
    // 下载目录
    QHBoxLayout* dirLayout = new QHBoxLayout();
    m_downloadDirEdit = new QLineEdit(this);
    m_browseButton = new QPushButton(tr("浏览..."), this);
    dirLayout->addWidget(m_downloadDirEdit);
    dirLayout->addWidget(m_browseButton);
    downloadLayout->addRow(tr("下载目录:"), dirLayout);
    
    // 网络设置组
    QGroupBox* networkGroup = new QGroupBox(tr("网络设置"), this);
    QFormLayout* networkLayout = new QFormLayout(networkGroup);
    
    // 用户代理
    m_userAgentEdit = new QLineEdit(this);
    networkLayout->addRow(tr("用户代理:"), m_userAgentEdit);
    
    // 超时设置
    m_timeoutSpinBox = new QSpinBox(this);
    m_timeoutSpinBox->setRange(5000, 120000);
    m_timeoutSpinBox->setSingleStep(1000);
    m_timeoutSpinBox->setSuffix(tr(" 毫秒"));
    networkLayout->addRow(tr("网络超时:"), m_timeoutSpinBox);
    
    // 更新设置组
    QGroupBox* updateGroup = new QGroupBox(tr("更新设置"), this);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    
    // 自动检查更新
    m_autoCheckUpdatesCheckBox = new QCheckBox(tr("启动时自动检查修改器更新"), this);
    updateLayout->addWidget(m_autoCheckUpdatesCheckBox);
    
    // 重置按钮
    m_resetButton = new QPushButton(tr("重置为默认设置"), this);
    
    // 确定/取消按钮
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    // 添加所有组件到主布局
    mainLayout->addWidget(downloadGroup);
    mainLayout->addWidget(networkGroup);
    mainLayout->addWidget(updateGroup);
    mainLayout->addWidget(m_resetButton);
    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::loadSettings()
{
    // 从ConfigManager获取当前设置
    ConfigManager& config = ConfigManager::getInstance();
    
    m_downloadDirEdit->setText(config.getDownloadDirectory());
    m_userAgentEdit->setText(config.getUserAgent());
    m_timeoutSpinBox->setValue(config.getNetworkTimeout());
    m_autoCheckUpdatesCheckBox->setChecked(config.getAutoCheckUpdates());
}

void SettingsDialog::onBrowseDownloadDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, 
        tr("选择下载目录"), 
        m_downloadDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        m_downloadDirEdit->setText(dir);
    }
}

void SettingsDialog::onResetToDefaults()
{
    // 询问用户是否确定重置
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("确认重置"), 
        tr("确定要将所有设置重置为默认值吗？"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        ConfigManager::getInstance().resetToDefaults();
        loadSettings(); // 重新加载默认设置
    }
}

void SettingsDialog::onSaveSettings()
{
    // 获取设置值
    QString downloadDir = m_downloadDirEdit->text();
    QString userAgent = m_userAgentEdit->text();
    int timeout = m_timeoutSpinBox->value();
    bool autoCheckUpdates = m_autoCheckUpdatesCheckBox->isChecked();
    
    // 验证下载目录
    QDir dir(downloadDir);
    if (!dir.exists()) {
        // 询问是否创建目录
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            tr("目录不存在"), 
            tr("下载目录不存在，是否创建？"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            if (!dir.mkpath(downloadDir)) {
                QMessageBox::critical(this, tr("错误"), tr("无法创建下载目录"));
                return;
            }
        } else {
            return; // 用户取消，不保存设置
        }
    }
    
    // 保存设置
    ConfigManager& config = ConfigManager::getInstance();
    config.setDownloadDirectory(downloadDir);
    config.setUserAgent(userAgent);
    config.setNetworkTimeout(timeout);
    config.setAutoCheckUpdates(autoCheckUpdates);
    
    accept(); // 关闭对话框并返回Accepted
}

void SettingsDialog::onCancelSettings()
{
    reject(); // 关闭对话框并返回Rejected
} 