#include "DownloadIntegrator.h"
#include "ui_DownloadIntegrator.h"
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTableWidgetItem>
#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QTimer>
#include <QCompleter>
#include <QStringListModel>
#include <QProcess>
#include <QProgressDialog>
#include <QScrollBar>
#include <QClipboard>
// 添加ModifierParser类引用
#include "ModifierParser.h"
#include "ModifierManager.h"
#include "NetworkManager.h"
#include "ConfigManager.h"
#include "FileSystem.h"
#include "SearchManager.h"
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "CoverExtractor.h"

// HTML解析库
#include <pugixml.hpp>

// 构造函数
DownloadIntegrator::DownloadIntegrator(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::DownloadIntegrator)
    , currentModifier(nullptr)
    , currentDownloadId("")
    , currentDownloadedModifier(nullptr)
    , m_tabWidget(nullptr)
    , m_searchTab(nullptr)
    , m_downloadedTab(nullptr)
    , m_downloadedTable(nullptr)
    , m_runButton(nullptr)
    , m_deleteButton(nullptr)
    , m_coverExtractor(nullptr)
{
    qDebug() << "开始初始化界面...";
    
    // 设置UI
    ui->setupUi(this);
    
    // 设置分割条初始比例为2:1（左边比右边稍多一些）
    ui->mainSplitter->setSizes({600, 400}); // 2:1的比例
    ui->mainSplitter->setStretchFactor(0, 2); // 左侧拉伸因子为2
    ui->mainSplitter->setStretchFactor(1, 1); // 右侧拉伸因子为1
    
    // 设置主题菜单
    setupThemeMenu();
    setupLanguageMenu();
    
    // 添加工具菜单
    QMenu* toolsMenu = menuBar()->addMenu(tr("工具"));
    
    // 如果工具菜单现在为空，删除它
    if (toolsMenu->actions().isEmpty()) {
        menuBar()->removeAction(toolsMenu->menuAction());
    }
    
    // 添加标签页
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    // 创建搜索标签页
    m_searchTab = new QWidget(m_tabWidget);
    m_tabWidget->addTab(m_searchTab, tr("搜索修改器"));
    m_searchTab->setLayout(ui->centralwidget->layout());
    ui->centralwidget->setParent(nullptr);
    
    // 创建已下载标签页
    m_downloadedTab = new QWidget(m_tabWidget);
    m_tabWidget->addTab(m_downloadedTab, tr("已下载修改器"));
    
    // 设置已下载标签页布局
    QVBoxLayout* downloadedLayout = new QVBoxLayout(m_downloadedTab);
    
    // 添加标题标签
    QLabel* titleLabel = new QLabel(tr("已下载的游戏修改器"), m_downloadedTab);
    titleLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   color: #333333;"
        "   padding: 5px 0px 10px 0px;"
        "}"
    );
    downloadedLayout->addWidget(titleLabel);
    
    // 创建已下载修改器表格
    m_downloadedTable = new QTableWidget(m_downloadedTab);
    m_downloadedTable->setColumnCount(4);
    QStringList headers;
    headers << tr("修改器名称") << tr("版本") << tr("游戏版本") << tr("下载日期");
    m_downloadedTable->setHorizontalHeaderLabels(headers);
    m_downloadedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_downloadedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_downloadedTable->setAlternatingRowColors(true);
    m_downloadedTable->verticalHeader()->setVisible(false);
    m_downloadedTable->setShowGrid(true); // 显示网格线以提高可读性
    m_downloadedTable->setGridStyle(Qt::DotLine); // 使用点线样式的网格，看起来更现代
    m_downloadedTable->horizontalHeader()->setStretchLastSection(true);
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // 第一列可交互调整
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 其他列固定大小
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_downloadedTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter); // 居中对齐表头
    m_downloadedTable->horizontalHeader()->setHighlightSections(true); // 高亮显示选中的表头
    m_downloadedTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "   background-color: #3c4b64;"
        "   color: white;"
        "   padding: 8px;"
        "   font-weight: bold;"
        "   border: 1px solid #2c3e50;"
        "   font-size: 14px;"
        "}"
        "QHeaderView::section:hover {"
        "   background-color: #34495e;"
        "}"
    );
    
    m_downloadedTable->setColumnWidth(0, 300); // 修改器名称列宽
    m_downloadedTable->setColumnWidth(1, 150); // 版本列宽
    m_downloadedTable->setColumnWidth(2, 200); // 游戏版本列宽
    m_downloadedTable->setColumnWidth(3, 200); // 下载日期列宽
    
    // 固定表格大小策略，防止忽大忽小
    m_downloadedTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_downloadedTable->setMinimumHeight(300); // 设置最小高度
    m_downloadedTable->horizontalHeader()->setMinimumHeight(30); // 设置表头最小高度
    m_downloadedTable->horizontalHeader()->setFixedHeight(30); // 固定表头高度
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // 第一列可交互调整
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 其他列固定大小
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_downloadedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    
    // 设置表格样式
    m_downloadedTable->setStyleSheet(
        "QTableWidget {"
        "   border: 1px solid #dcdcdc;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "   selection-background-color: #e7f4ff;"
        "   selection-color: #000000;"
        "}"
        "QTableWidget::item {"
        "   padding: 5px;"
        "   border-bottom: 1px solid #f0f0f0;"
        "}"
        "QTableWidget::item:selected {"
        "   background-color: #e7f4ff;"
        "   color: #000000;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f5f5f5;"
        "   padding: 5px;"
        "   border: 1px solid #dcdcdc;"
        "   font-weight: bold;"
        "}"
    );
    
    // 设置行高
    m_downloadedTable->verticalHeader()->setDefaultSectionSize(36); // 增加行高以提高可读性
    
    // 创建按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 10, 0, 0); // 增加上边距，使按钮与表格分隔开
    
    // 创建按钮并设置样式
    m_runButton = new QPushButton(tr("运行"), m_downloadedTab);
    m_runButton->setIcon(QIcon(":/icons/play.png")); // 如果有图标资源
    m_runButton->setMinimumWidth(100);
    m_runButton->setCursor(Qt::PointingHandCursor);
    m_runButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   padding: 6px 12px;"
        "   border-radius: 4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    
    m_deleteButton = new QPushButton(tr("删除"), m_downloadedTab);
    m_deleteButton->setIcon(QIcon(":/icons/delete.png")); // 如果有图标资源
    m_deleteButton->setMinimumWidth(100);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336;"
        "   color: white;"
        "   border: none;"
        "   padding: 6px 12px;"
        "   border-radius: 4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d32f2f;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    
    // 添加"检查更新"按钮
    m_checkUpdateButton = new QPushButton(tr("检查更新"), m_downloadedTab);
    m_checkUpdateButton->setIcon(QIcon(":/icons/update.png")); // 如果有图标资源
    m_checkUpdateButton->setMinimumWidth(100);
    m_checkUpdateButton->setCursor(Qt::PointingHandCursor);
    m_checkUpdateButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   border: none;"
        "   padding: 6px 12px;"
        "   border-radius: 4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    
    // 初始禁用按钮
    m_runButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_checkUpdateButton->setEnabled(true); // 始终启用"检查更新"按钮
    
    // 添加按钮到布局
    buttonLayout->addWidget(m_runButton);
    buttonLayout->addSpacing(10); // 按钮之间的间距
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addSpacing(10); // 按钮之间的间距
    buttonLayout->addWidget(m_checkUpdateButton);
    buttonLayout->addStretch();
    
    // 添加到布局
    downloadedLayout->addWidget(m_downloadedTable);
    downloadedLayout->addLayout(buttonLayout);
    
    // 注释掉重复的样式表加载，由ThemeManager统一管理
    // 如果需要特定样式，应该在CSS中使用objectName选择器
    /*
    QFile styleFile(":/style/main.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        setStyleSheet(style);
        styleFile.close();
        qDebug() << "已加载样式表";
    } else {
        qDebug() << "无法加载样式表文件: " << styleFile.errorString();
    }
    */
    
    // 设置表格属性
    ui->modifierTable->setColumnCount(4);
    ui->modifierTable->setColumnWidth(0, 250); // 游戏名称列宽
    ui->modifierTable->setColumnWidth(1, 100); // 更新日期列宽
    ui->modifierTable->setColumnWidth(2, 150); // 支持版本列宽
    ui->modifierTable->setColumnWidth(3, 80);  // 选项数量列宽
    
    // 固定表格大小策略，防止忽大忽小
    ui->modifierTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->modifierTable->setMinimumHeight(300); // 设置最小高度
    ui->modifierTable->horizontalHeader()->setMinimumHeight(30); // 设置表头最小高度
    ui->modifierTable->horizontalHeader()->setFixedHeight(30); // 固定表头高度
    ui->modifierTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // 第一列可交互调整
    ui->modifierTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 其余列固定大小
    ui->modifierTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->modifierTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    
    ui->modifierTable->horizontalHeader()->setStretchLastSection(true);
    ui->modifierTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->modifierTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->modifierTable->setAlternatingRowColors(true); // 交替行颜色
    ui->modifierTable->verticalHeader()->setVisible(false); // 隐藏行号
    ui->modifierTable->setShowGrid(false); // 隐藏网格线
    
    // 设置窗口标题和图标
    setWindowTitle("游戏修改器下载集成工具");
    // 设置窗口图标（如果有的话）
    // setWindowIcon(QIcon(":/icons/app_icon.png"));
    
    // 设置搜索框自动完成功能
    setupSearchCompleter();
    
    // 优化状态栏显示
    QLabel* statusLabel = new QLabel("就绪");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setMargin(3);
    ui->statusbar->addWidget(statusLabel);
    
    // 初始禁用下载按钮
    ui->downloadButton->setEnabled(false);
    ui->versionSelect->setEnabled(false);
    
    // 设置信号和槽连接
    connect(ui->searchButton, &QPushButton::clicked, this, &DownloadIntegrator::onSearchClicked);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &DownloadIntegrator::onSearchClicked);
    
    // 添加搜索框文本变化的监听，支持清除搜索回到初始列表
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            // 当搜索框被清空时，显示初始的修改器列表
            qDebug() << "搜索框已清空，恢复到初始修改器列表";
            showStatusMessage(tr("正在加载初始修改器列表..."));
            SearchManager::getInstance().fetchRecentlyUpdatedModifiers(this, &DownloadIntegrator::onSearchCompleted);
        }
    });
    connect(ui->modifierTable, &QTableWidget::cellClicked, this, &DownloadIntegrator::onModifierItemClicked);
    connect(ui->downloadButton, &QPushButton::clicked, this, &DownloadIntegrator::onDownloadButtonClicked);
    connect(ui->openFolderButton, &QPushButton::clicked, this, &DownloadIntegrator::onOpenFolderButtonClicked);
    connect(ui->settingsButton, &QPushButton::clicked, this, &DownloadIntegrator::onSettingsButtonClicked);
    connect(ui->versionSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &DownloadIntegrator::onVersionSelectionChanged);
    connect(ui->sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DownloadIntegrator::onSortOrderChanged);
    // 重新启用刷新按钮连接
    connect(ui->refreshButton, &QPushButton::clicked, this, &DownloadIntegrator::onRefreshButtonClicked);
            
    // 连接菜单动作
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionSettings, &QAction::triggered, this, &DownloadIntegrator::onSettingsButtonClicked);
    
    // 连接已下载修改器管理相关信号
    connect(m_downloadedTable, &QTableWidget::cellDoubleClicked, this, &DownloadIntegrator::onDownloadedModifierDoubleClicked);
    connect(m_runButton, &QPushButton::clicked, this, &DownloadIntegrator::onRunButtonClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &DownloadIntegrator::onDeleteButtonClicked);
    connect(m_checkUpdateButton, &QPushButton::clicked, this, &DownloadIntegrator::onCheckUpdateButtonClicked);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &DownloadIntegrator::onTabChanged);
    connect(m_downloadedTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_downloadedTable->selectedItems().isEmpty();
        m_runButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
    });
    
    qDebug() << "界面初始化完成，准备加载修改器列表";
    
    // 初始化封面提取器
    m_coverExtractor = new CoverExtractor(this);
    
    // 加载已下载修改器列表
    updateDownloadedModifiersList();
    
    // 加载修改器列表 - 使用SearchManager获取最新修改器列表
    qDebug() << "正在调用SearchManager.fetchRecentlyUpdatedModifiers获取最新修改器列表...";
    SearchManager::getInstance().fetchRecentlyUpdatedModifiers(this, &DownloadIntegrator::onSearchCompleted);
    
    // 初始化翻译
    retranslateUi();
    
    qDebug() << "DownloadIntegrator构造函数执行完毕";
}

// 设置搜索框自动完成功能
void DownloadIntegrator::setupSearchCompleter()
{
    qDebug() << "设置搜索框自动完成功能";
    
    // 使用SearchManager获取自动完成器
    QCompleter *completer = SearchManager::getInstance().getCompleter();
    
    // 设置匹配模式为包含匹配，而不仅是前缀匹配
    completer->setFilterMode(Qt::MatchContains);
    
    // 设置完成模式为弹出式列表
    completer->setCompletionMode(QCompleter::PopupCompletion);
    
    // 设置最小补全前缀长度为1个字符
    completer->setCompletionPrefix("");
    
    // 设置自动完成弹出框样式
    QAbstractItemView *popup = completer->popup();
    popup->setStyleSheet(
        "background-color: white;"
        "border: 1px solid #bdc3c7;"
        "border-radius: 4px;"
        "selection-background-color: #16a085;"
        "selection-color: white;"
        "padding: 8px;"  // 增加内边距
        "font-size: 14px;"
    );
    
    // 设置弹出框尺寸自适应
    popup->setMinimumHeight(100);  // 设置最小高度
    popup->setMaximumHeight(300);  // 设置最大高度，避免太多项目时过长
    
    // 设置弹出框宽度自适应内容
    popup->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
    popup->setMinimumWidth(200);   // 最小宽度
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏水平滚动条
    
    // 确保可以看到完整内容
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(popup->model());
    if (model) {
        // 在数据变化时重新计算合适的宽度
        connect(model, &QStandardItemModel::dataChanged, this, [popup]() {
            // 使用通用方法调整大小，而不是表格特有的方法
            popup->adjustSize();
            popup->update();
        });
    }
    
    // 应用到搜索框
    ui->searchEdit->setCompleter(completer);
    
    // 连接搜索框文本变化信号和搜索框获取焦点信号
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this, popup, completer](const QString &text) {
        // 如果文本长度大于等于1，显示建议
        if (text.length() >= 1) {
            // 显示自动完成弹出框
            completer->complete();
            
            // 计算最佳宽度：搜索框宽度的1.5倍与内容宽度中的较大值
            int contentWidth = 0;
            
            // 获取所有项目，找出最宽的文本
            QAbstractItemModel* model = completer->model();
            if (model) {
                QFontMetrics fm = popup->fontMetrics();
                for (int i = 0; i < model->rowCount(); i++) {
                    QString itemText = model->data(model->index(i, 0)).toString();
                    int textWidth = fm.horizontalAdvance(itemText) + 40; // 加上一些边距
                    contentWidth = qMax(contentWidth, textWidth);
                }
            }
            
            // 设置最佳宽度，但不小于搜索框宽度
            int bestWidth = qMax(ui->searchEdit->width(), 
                              qMax(contentWidth, 
                                 static_cast<int>(ui->searchEdit->width() * 1.5)));
            
            // 限制最大宽度，避免太宽
            int maxWidth = static_cast<int>(this->width() * 3/4); // 使用主窗口宽度的3/4
            bestWidth = qMin(bestWidth, maxWidth);
            
            popup->setMinimumWidth(bestWidth);
            popup->resize(bestWidth, popup->height());
            
            // 调整位置，使其水平居中于搜索框
            QPoint globalPos = ui->searchEdit->mapToGlobal(QPoint(0, ui->searchEdit->height()));
            int x = globalPos.x() + (ui->searchEdit->width() - popup->width()) / 2;
            popup->move(x, globalPos.y());
        }
    });
    
    // 搜索框获得焦点时直接显示建议
    connect(ui->searchEdit, &QLineEdit::cursorPositionChanged, this, [this, completer]() {
        if (ui->searchEdit->hasFocus() && !ui->searchEdit->text().isEmpty()) {
            completer->complete();
        }
    });
    
    qDebug() << "搜索框自动完成功能已设置";
}

// 析构函数
DownloadIntegrator::~DownloadIntegrator()
{
    delete currentModifier;
    delete ui;
}

// 更新修改器列表
void DownloadIntegrator::updateModifierList(const QList<ModifierInfo>& modifiers)
{
    qDebug() << "更新修改器列表，共" << modifiers.size() << "个修改器";
    
    // 更新内部修改器列表
    modifierList = modifiers;
    
    // 记录当前表格滚动位置
    int scrollValue = ui->modifierTable->verticalScrollBar()->value();
    
    // 关闭表格重排功能，防止在填充时频繁布局
    ui->modifierTable->setUpdatesEnabled(false);
    
    // 清空现有表格
    ui->modifierTable->clearContents();
    ui->modifierTable->setRowCount(modifierList.size());
    
    // 填充数据
    for (int i = 0; i < modifierList.size(); i++) {
        const ModifierInfo& info = modifierList[i];
        
        // 设置修改器名称
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.name);
        ui->modifierTable->setItem(i, 0, nameItem);
        
        // 设置游戏版本
        QTableWidgetItem* versionItem = new QTableWidgetItem(info.gameVersion);
        ui->modifierTable->setItem(i, 1, versionItem);
        
        // 设置最后更新时间
        QTableWidgetItem* updateItem = new QTableWidgetItem(info.lastUpdate);
        ui->modifierTable->setItem(i, 2, updateItem);
        
        // 设置选项数量
        QTableWidgetItem* optionsItem = new QTableWidgetItem(QString::number(info.optionsCount));
        ui->modifierTable->setItem(i, 3, optionsItem);
        
        qDebug() << "  添加修改器到表格: " << info.name << " (" << info.gameVersion << ")";
    }
    
    // 仅在数据为空时添加空行以保持表格高度
    if (modifierList.isEmpty()) {
        ui->modifierTable->setRowCount(10); // 添加10个空行
    }
    
    // 重新启用表格更新
    ui->modifierTable->setUpdatesEnabled(true);
    
    // 恢复滚动位置
    ui->modifierTable->verticalScrollBar()->setValue(scrollValue);
    
    // 清空详情区域
    ui->gameTitle->clear();
    ui->versionInfo->clear();
    ui->optionsCount->clear();
    ui->lastUpdate->clear();
    ui->modifierOptions->clear();
    ui->downloadButton->setEnabled(false);
    
    qDebug() << "修改器列表UI更新完成";
}

// 更新修改器列表（无参数版本）
void DownloadIntegrator::updateModifierList()
{
    updateModifierList(modifierList);
}

// 更新修改器详情UI
void DownloadIntegrator::updateModifierDetail()
{
    if (!currentModifier) {
        qDebug() << "当前没有选择的修改器";
        return;
    }
    
    // 更新界面
    ui->gameTitle->setText(currentModifier->name);
    ui->versionInfo->setText(QString("<b>%1</b> %2").arg(tr("游戏版本:")).arg(currentModifier->gameVersion));
    ui->optionsCount->setText(QString("<b>%1</b> %2").arg(tr("修改器选项:")).arg(currentModifier->options.size()));
    ui->lastUpdate->setText(QString("<b>%1</b> %2").arg(tr("最后更新:")).arg(currentModifier->lastUpdate));
    
    // 更新修改器选项列表
    ui->modifierOptions->clear();
    
    // 检查是否有选项
    if (currentModifier->options.isEmpty()) {
        ui->modifierOptions->setHtml(QString("<div style='padding: 15px;'>"
                                           "<h3 style='color: #3c4b64; margin-top: 0;'>%1</h3>"
                                           "<p style='color: #666; font-style: italic;'>%2</p></div>")
                                    .arg(tr("可用功能:"))
                                    .arg(tr("暂无功能信息")));
        
        // 添加获取信息中的提示，如果选项数计数不为0但没有实际选项
        if (currentModifier->optionsCount > 0) {
            ui->modifierOptions->append(QString("<div style='text-align: center; padding: 20px;'>"
                                             "<p style='color: #666;'>%1</p>"
                                             "<img src=':/resources/loading.gif' width='32' height='32'/>"
                                             "</div>")
                                      .arg(tr("正在尝试从网络获取选项信息，请稍后...")));
            
            qDebug() << "修改器" << currentModifier->name << "有" << currentModifier->optionsCount 
                    << "个选项，但未能提取具体信息";
            
            // 如果有URL，尝试重新加载详情
            if (!currentModifier->url.isEmpty()) {
                QTimer::singleShot(500, this, [this, url=currentModifier->url]() {
                    ModifierManager::getInstance().getModifierDetail(url, [this](ModifierInfo* modifier) {
                        delete currentModifier;
                        currentModifier = modifier;
                        updateModifierDetail();
                    });
                });
            }
        }
    } else {
        // 使用更美观的HTML布局显示选项
        QString optionsHtml = QString(
            "<div style='padding: 15px;'>"
            "<h3 style='color: #3c4b64; margin-top: 0; margin-bottom: 15px;'>%1</h3>")
            .arg(tr("可用功能:"));
        
        // 统计实际选项数量（不包括分类标题）
        int actualOptionCount = 0;
        for (const QString& option : currentModifier->options) {
            if (!option.startsWith("●")) {
                actualOptionCount++;
            }
        }
        
        // 添加选项计数信息
        optionsHtml += QString("<p style='color: #666; margin-bottom: 15px;'>%1 <b>%2</b> %3</p>")
            .arg(tr("共")).arg(actualOptionCount).arg(tr("个功能选项"));
        
        // 选项内容容器
        optionsHtml += "<div style='background-color: #f9f9f9; border: 1px solid #e0e0e0; "
                       "border-radius: 6px; padding: 15px; max-height: 400px; overflow-y: auto;'>";
        
        bool inCategory = false;
        QString currentCategory;
        bool isFirstCategory = true;
        
        for (const QString& option : currentModifier->options) {
            // 检查是否是分类标题
            if (option.startsWith("●")) {
                // 如果已经在一个分类中，结束上一个分类
                if (inCategory) {
                    optionsHtml += "</div>";
                }
                
                // 提取分类名称
                currentCategory = option.mid(1).trimmed();
                
                // 添加分类标题
                optionsHtml += QString("<div class='category' style='%1'>"
                                    "<h4 style='margin: 0; padding: 8px 10px; background-color: #3c4b64; color: white; "
                                    "border-radius: 4px; font-size: 14px;'>%2</h4>"
                                    "<div style='padding: 10px 5px 5px 5px;'>")
                            .arg(isFirstCategory ? "" : "margin-top: 20px;")
                            .arg(currentCategory);
                
                inCategory = true;
                isFirstCategory = false;
            }
            else {
                // 如果没有任何类别标题但遇到了选项，创建一个默认类别
                if (!inCategory) {
                    optionsHtml += "<div class='category'>"
                                 "<h4 style='margin: 0; padding: 8px 10px; background-color: #3c4b64; color: white; "
                                 "border-radius: 4px; font-size: 14px;'>基本选项</h4>"
                                 "<div style='padding: 10px 5px 5px 5px;'>";
                    inCategory = true;
                }
                
                // 格式化选项文本
                QString formattedOption = option.trimmed();
                
                // 移除前导•符号（如果有）
                if (formattedOption.startsWith("•")) {
                    formattedOption = formattedOption.mid(1).trimmed();
                }
                
                // 检测并高亮按键组合
                QRegularExpression keyRegex("(Num\\s*\\d+|Ctrl\\+Num\\s*\\d+|Alt\\+Num\\s*\\d+|Shift\\+[^\\s\\–-]+|F\\d+)");
                QRegularExpressionMatch keyMatch = keyRegex.match(formattedOption);
                
                QString keyPart;
                QString descPart = formattedOption;
                
                if (keyMatch.hasMatch()) {
                    keyPart = keyMatch.captured(1);
                    // 在第一个 - 或 – 处分割
                    int dashPos = formattedOption.indexOf(QRegularExpression("[\\–-]"), keyMatch.capturedEnd(1));
                    if (dashPos != -1) {
                        descPart = formattedOption.mid(dashPos + 1).trimmed();
                    }
                }
                
                // 创建带样式的选项项
                optionsHtml += "<div style='margin-bottom: 8px; display: flex; align-items: center;'>";
                
                // 添加项目符号
                optionsHtml += "<div style='color: #3498db; margin-right: 8px; flex-shrink: 0;'>•</div>";
                
                // 如果有按键部分
                if (!keyPart.isEmpty()) {
                    // 按键部分使用特殊样式
                    optionsHtml += QString("<div style='background-color: #e7f4ff; border: 1px solid #bde0fe; "
                                        "border-radius: 3px; padding: 3px 8px; color: #0069c0; font-family: monospace; "
                                        "font-weight: bold; margin-right: 10px; flex-shrink: 0; min-width: 80px; "
                                        "text-align: center;'>%1</div>")
                                 .arg(keyPart);
                    
                    // 功能描述使用普通样式
                    optionsHtml += QString("<div style='flex-grow: 1;'>%1</div>").arg(descPart);
                } else {
                    // 如果没有识别出按键部分，使用整个选项文本
                    optionsHtml += QString("<div style='flex-grow: 1;'>%1</div>").arg(formattedOption);
                }
                
                optionsHtml += "</div>"; // 关闭选项项div
            }
        }
        
        // 关闭最后一个分类（如果有）
        if (inCategory) {
            optionsHtml += "</div></div>";
        }
        
        optionsHtml += "</div></div>"; // 关闭选项容器和主容器
        
        ui->modifierOptions->setHtml(optionsHtml);
        
        // 如果实际提取的选项数量小于网站标称的选项数量，显示提示信息
        if (currentModifier->optionsCount > 0 && actualOptionCount < currentModifier->optionsCount * 0.5) {
            QString warningInfo = QString("<div style='margin-top: 10px; padding: 8px 15px; background-color: #fff3cd; "
                                        "border-left: 4px solid #e67e22; color: #856404;'>"
                                        "<p style='margin: 0;'>%1 %2 %3 %4 %5</p></div>")
                                 .arg(tr("注意：网站显示该修改器有"))
                                 .arg(currentModifier->optionsCount)
                                 .arg(tr("个选项，但仅提取到"))
                                 .arg(actualOptionCount)
                                 .arg(tr("个，可能有部分选项未能正确识别。"));
            ui->modifierOptions->append(warningInfo);
        }
    }
    
    // 更新版本选择下拉框
    ui->versionSelect->clear();
    for (const auto& version : currentModifier->versions) {
        ui->versionSelect->addItem(version.first);
    }
    
    // 启用下载按钮和版本选择框
    ui->downloadButton->setEnabled(!currentModifier->versions.isEmpty());
    ui->versionSelect->setEnabled(!currentModifier->versions.isEmpty());
    
    // 提取游戏封面
    if (!currentModifier->screenshotUrl.isEmpty()) {
        qDebug() << "开始提取游戏封面：" << currentModifier->screenshotUrl;
        
        // 生成游戏ID用于缓存
        QString gameId = currentModifier->name.toLower().replace(QRegularExpression("[^a-z0-9]"), "_");
        
        // 先检查缓存
        QPixmap cachedCover = CoverExtractor::getCachedCover(gameId);
        if (!cachedCover.isNull()) {
            // 设置封面并保持纵横比
            setGameCoverWithAspectRatio(cachedCover);
            qDebug() << "从缓存加载游戏封面";
        } else {
            // 异步提取封面
            m_coverExtractor->extractCoverFromTrainerImage(currentModifier->screenshotUrl, 
                [this, gameId](const QPixmap& cover, bool success) {
                    if (success && !cover.isNull()) {
                        // 设置封面并保持纵横比
                        setGameCoverWithAspectRatio(cover);
                        // 保存到缓存
                        CoverExtractor::saveCoverToCache(gameId, cover);
                        qDebug() << "游戏封面提取成功";
                    } else {
                        // 失败时显示空白
                        ui->gameCoverLabel->clear();
                        qDebug() << "游戏封面提取失败";
                    }
                });
        }
    } else {
        // 没有截图URL时显示空白
        ui->gameCoverLabel->clear();
        qDebug() << "修改器没有截图URL，无法提取游戏封面";
    }
    
    qDebug() << "修改器详情更新完成";
}

// 显示状态消息的辅助函数
void DownloadIntegrator::showStatusMessage(const QString& message, int timeout)
{
    // 使用UIHelper显示状态消息
    UIHelper::showStatusMessage(ui->statusbar, message, timeout);
}

// 设置游戏封面并保持图片原始大小
void DownloadIntegrator::setGameCoverWithAspectRatio(const QPixmap& cover)
{
    if (cover.isNull()) {
        ui->gameCoverLabel->clear();
        ui->gameCoverLabel->setMinimumSize(120, 160); // 设置一个默认的最小尺寸
        ui->gameCoverLabel->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        return;
    }
    
    // 设置合理的最大尺寸限制（进一步缩小）
    const int MAX_WIDTH = 160;
    const int MAX_HEIGHT = 240;
    
    QPixmap finalPixmap = cover;
    QSize originalSize = cover.size();
    
    // 如果图片超过最大尺寸，按比例缩放
    if (originalSize.width() > MAX_WIDTH || originalSize.height() > MAX_HEIGHT) {
        finalPixmap = cover.scaled(MAX_WIDTH, MAX_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << QString("封面过大，已缩放: 原始尺寸(%1x%2) -> 缩放后(%3x%4)")
                    .arg(originalSize.width()).arg(originalSize.height())
                    .arg(finalPixmap.width()).arg(finalPixmap.height());
    }
    
    // 设置Label的尺寸为最终图片的尺寸
    QSize finalSize = finalPixmap.size();
    ui->gameCoverLabel->setFixedSize(finalSize);
    
    // 设置像素图
    ui->gameCoverLabel->setPixmap(finalPixmap);
    
    qDebug() << QString("设置封面: 最终尺寸(%1x%2)，Label尺寸已调整为相同大小")
                .arg(finalSize.width()).arg(finalSize.height());
}

// 获取下载目录
QString DownloadIntegrator::getDownloadDirectory()
{
    // 使用ConfigManager获取下载目录，该方法内部已经使用了FileSystem
    return ConfigManager::getInstance().getDownloadDirectory();
}

// 从URL推断文件扩展名
QString DownloadIntegrator::getFileExtensionFromUrl(const QString& url)
{
    QString cleanUrl = url;
    
    // 移除查询参数和fragment
    if (cleanUrl.contains("?")) {
        cleanUrl = cleanUrl.split("?").first();
    }
    if (cleanUrl.contains("#")) {
        cleanUrl = cleanUrl.split("#").first();
    }
    
    // 获取URL路径的最后部分（文件名）
    QString fileName = cleanUrl.split("/").last();
    
    // 如果没有扩展名，返回空字符串
    if (!fileName.contains(".")) {
        return QString();
    }
    
    // 处理特殊的双扩展名
    if (fileName.contains(".tar.")) {
        if (fileName.endsWith(".gz")) return ".tar.gz";
        if (fileName.endsWith(".bz2")) return ".tar.bz2";
        if (fileName.endsWith(".xz")) return ".tar.xz";
    }
    
    // 处理常见的缩写扩展名
    if (fileName.endsWith(".tgz")) return ".tgz";
    if (fileName.endsWith(".tbz2")) return ".tbz2";
    if (fileName.endsWith(".txz")) return ".txz";
    
    // 获取普通扩展名
    QString extension = "." + fileName.split(".").last().toLower();
    
    qDebug() << "从URL推断文件扩展名：" << url << " -> " << extension;
    
    return extension;
}

// 检测文件的实际格式（通过文件魔数）
QString DownloadIntegrator::detectFileFormat(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件进行格式检测：" << filePath;
        return QString();
    }
    
    // 读取文件头部字节（魔数）
    QByteArray header = file.read(32); // 读取前32字节足够检测大多数格式
    file.close();
    
    if (header.isEmpty()) {
        return QString();
    }
    
    // 检测各种文件格式的魔数
    // ZIP 格式: 50 4B 03 04 或 50 4B 05 06 或 50 4B 07 08
    if (header.startsWith("\x50\x4B\x03\x04") || 
        header.startsWith("\x50\x4B\x05\x06") ||
        header.startsWith("\x50\x4B\x07\x08")) {
        return "zip";
    }
    
    // RAR 格式: 52 61 72 21 1A 07 00 (RAR!\x1A\x07\x00)
    if (header.startsWith("Rar!\x1A\x07\x00") || header.startsWith("Rar!\x1A\x07\x01")) {
        return "rar";
    }
    
    // 7Z 格式: 37 7A BC AF 27 1C
    if (header.startsWith("\x37\x7A\xBC\xAF\x27\x1C")) {
        return "7z";
    }
    
    // PE 可执行文件 (EXE): MZ (4D 5A)
    if (header.startsWith("MZ")) {
        return "exe";
    }
    
    // GZIP 格式: 1F 8B
    if (header.startsWith("\x1F\x8B")) {
        return "gz";
    }
    
    // BZIP2 格式: 42 5A 68 (BZh)
    if (header.startsWith("BZh")) {
        return "bz2";
    }
    
    qDebug() << "无法识别文件格式，文件头：" << header.left(16).toHex();
    return QString();
}

// 检查文件是否为压缩包格式
bool DownloadIntegrator::isArchiveFile(const QString& filePath)
{
    QString detectedFormat = detectFileFormat(filePath);
    
    // 基于魔数检测
    if (detectedFormat == "zip" || detectedFormat == "rar" || detectedFormat == "7z" || 
        detectedFormat == "gz" || detectedFormat == "bz2") {
        return true;
    }
    
    // 如果魔数检测失败，回退到扩展名检测
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    return (extension == "zip" || extension == "rar" || extension == "7z" || 
            extension == "tar" || extension == "gz" || extension == "bz2" || 
            extension == "xz" || extension == "tgz" || extension == "tbz2");
}

// 检查文件是否为可执行文件
bool DownloadIntegrator::isExecutableFile(const QString& filePath)
{
    QString detectedFormat = detectFileFormat(filePath);
    
    // 基于魔数检测
    if (detectedFormat == "exe") {
        return true;
    }
    
    // 如果魔数检测失败，回退到扩展名检测
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    return (extension == "exe" || extension == "msi" || extension == "bat" || 
            extension == "cmd" || extension == "com");
}

// 更新已下载修改器列表
void DownloadIntegrator::updateDownloadedModifiersList()
{
    qDebug() << "更新已下载修改器列表UI";
    
    // 使用ModifierManager获取下载列表
    QList<DownloadedModifierInfo> downloadedModifiers = ModifierManager::getInstance().getDownloadedModifiers();
    
    // 记录当前表格滚动位置
    int scrollValue = m_downloadedTable->verticalScrollBar()->value();
    
    // 关闭表格重排功能，防止在填充时频繁布局
    m_downloadedTable->setUpdatesEnabled(false);
    
    // 清空表格
    m_downloadedTable->clearContents();
    m_downloadedTable->setRowCount(downloadedModifiers.size());
    
    // 检查是否有结果
    if (downloadedModifiers.isEmpty()) {
        // 显示无结果提示
        showStatusMessage(tr("没有已下载的修改器"), 3000);
        
        // 添加空行以保持表格高度
        m_downloadedTable->setRowCount(10); // 添加10个空行
        
        // 重新启用表格更新
        m_downloadedTable->setUpdatesEnabled(true);
        return;
    }
    
    // 设置行数
    m_downloadedTable->setRowCount(downloadedModifiers.size());
    
    // 填充表格数据
    for (int i = 0; i < downloadedModifiers.size(); ++i) {
        const DownloadedModifierInfo& info = downloadedModifiers[i];
        
        // 创建单元格项
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.name);
        QTableWidgetItem* versionItem = new QTableWidgetItem(info.version);
        QTableWidgetItem* gameVersionItem = new QTableWidgetItem(info.gameVersion);
        QTableWidgetItem* dateItem = new QTableWidgetItem(info.downloadDate.toString("yyyy-MM-dd"));
        
        // 设置工具提示
        nameItem->setToolTip(info.name);
        versionItem->setToolTip(info.version);
        gameVersionItem->setToolTip(info.gameVersion);
        dateItem->setToolTip(info.downloadDate.toString("yyyy-MM-dd hh:mm:ss"));
        
        // 如果有更新，设置背景色
        if (info.hasUpdate) {
            QBrush updateBrush(QColor(255, 243, 205)); // 淡黄色背景
            nameItem->setBackground(updateBrush);
            nameItem->setToolTip(nameItem->toolTip() + " " + tr("(有新版本可用)"));
        }
        
        // 设置字体和对齐方式
        QFont nameFont = nameItem->font();
        nameFont.setBold(true);
        nameItem->setFont(nameFont);
        
        dateItem->setTextAlignment(Qt::AlignCenter);
        versionItem->setTextAlignment(Qt::AlignCenter);
        
        // 添加到表格
        m_downloadedTable->setItem(i, 0, nameItem);
        m_downloadedTable->setItem(i, 1, versionItem);
        m_downloadedTable->setItem(i, 2, gameVersionItem);
        m_downloadedTable->setItem(i, 3, dateItem);
        
        // 存储索引数据
        nameItem->setData(Qt::UserRole, i);
    }
    
    // 确保表格高度稳定
    if (downloadedModifiers.size() < 10) {
        int currentRows = m_downloadedTable->rowCount();
        m_downloadedTable->setRowCount(qMax(10, currentRows));
    }
    
    // 重新启用表格更新
    m_downloadedTable->setUpdatesEnabled(true);
    
    // 恢复滚动位置
    m_downloadedTable->verticalScrollBar()->setValue(scrollValue);
}

// 标签页切换事件
void DownloadIntegrator::onTabChanged(int index)
{
    qDebug() << "切换到标签页:" << index;
    
    if (index == 1) { // 已下载修改器标签页
        // 刷新已下载修改器列表
        updateDownloadedModifiersList();
        
        showStatusMessage(tr("已下载修改器列表已刷新"), 3000);
    }
}

// 检查更新按钮点击事件
void DownloadIntegrator::onCheckUpdateButtonClicked()
{
    // 获取选中的行
    QList<QTableWidgetItem*> selectedItems = m_downloadedTable->selectedItems();
    if (selectedItems.isEmpty()) {
        // 如果没有选中行，检查所有修改器的更新
        
        // 创建进度对话框
        QProgressDialog* progressDialog = new QProgressDialog(tr("正在检查更新..."), tr("取消"), 0, 100, this);
        progressDialog->setWindowTitle(tr("检查更新"));
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(500);
        progressDialog->setAutoClose(true);
        progressDialog->setValue(0);
        
        // 使用批量更新检查方法
        ModifierManager::getInstance().batchCheckForUpdates(
            // 进度回调
            [progressDialog](int current, int total, bool hasUpdates) {
                if (progressDialog->wasCanceled()) {
                    return;
                }
                
                int percent = (current * 100) / total;
                progressDialog->setValue(percent);
                progressDialog->setLabelText(tr("正在检查更新... (%1/%2)").arg(current).arg(total));
            },
            // 完成回调
            [this, progressDialog](int updatesCount) {
                // 关闭进度对话框
                progressDialog->close();
                progressDialog->deleteLater();
                
                // 更新UI以反映更新状态
                updateDownloadedModifiersList();
                
                if (updatesCount > 0) {
                    QMessageBox::information(this, tr("检查更新完成"), 
                                          tr("找到 %1 个可用更新！").arg(updatesCount));
                    showStatusMessage(tr("找到 %1 个更新").arg(updatesCount), 3000);
                } else {
                    QMessageBox::information(this, tr("检查更新完成"), tr("所有修改器都是最新的"));
                    showStatusMessage(tr("所有修改器都是最新的"), 3000);
                }
            }
        );
    } else {
        int row = selectedItems.first()->row();
        // 检查选中修改器的更新
        ModifierManager::getInstance().checkForUpdates(
            row,
            [this, row](bool hasUpdate) {
                updateDownloadedModifiersList();
                if (hasUpdate) {
                    showStatusMessage(tr("发现新版本"), 3000);
                } else {
                    showStatusMessage(tr("已是最新版本"), 3000);
                }
            }
        );
    }
}

// 搜索按钮点击
void DownloadIntegrator::onSearchClicked()
{
    QString searchTerm = ui->searchEdit->text().trimmed();
    
    if (searchTerm.isEmpty()) {
        // 如果搜索框为空，提示用户输入搜索内容
        ui->searchEdit->setFocus();
        ui->searchEdit->setStyleSheet("border: 1px solid #e74c3c;");
        
        // 2秒后恢复样式
        QTimer::singleShot(2000, this, [this]() {
            ui->searchEdit->setStyleSheet("");
        });
        
        showStatusMessage(tr("请输入要搜索的游戏名称"), 2000);
        return;
    }
    
    qDebug() << "执行搜索：" << searchTerm;
    
    // 显示搜索中的动画或提示
    ui->modifierTable->setRowCount(0);
    ui->modifierTable->setEnabled(false);
    showStatusMessage(tr("正在搜索 \"%1\"...").arg(searchTerm));
    
    // 显示进度条
    ui->downloadProgress->setValue(0);
    ui->downloadProgress->setVisible(true);
    ui->downloadProgress->setFormat(tr("搜索中..."));
    ui->downloadProgress->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #bdc3c7;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   background-color: #f5f5f5;"
        "   height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #2ecc71;"
        "   width: 10px;"
        "   margin: 0.5px;"
        "   border-radius: 3px;"
        "}"
    );
    
    // 禁用搜索按钮，防止重复点击
    ui->searchButton->setEnabled(false);
    ui->searchEdit->setEnabled(false);
    
    // 创建一个定时器来模拟搜索进度
    QTimer* searchTimer = new QTimer(this);
    int progress = 0;
    
    connect(searchTimer, &QTimer::timeout, [this, searchTimer, progress]() mutable {
        progress += 10;
        if (progress <= 90) {
            ui->downloadProgress->setValue(progress);
        } else {
            searchTimer->stop();
            searchTimer->deleteLater();
        }
    });
    
    // 开始模拟搜索进度
    searchTimer->start(100);
    
    // 使用SearchManager进行搜索，并连接到onSearchCompleted槽
    SearchManager::getInstance().searchModifiers(searchTerm, this, &DownloadIntegrator::onSearchCompleted);
}

// 添加搜索完成处理方法
void DownloadIntegrator::onSearchCompleted(const QList<ModifierInfo>& modifiers) {
    // 保存到本地修改器列表
    modifierList = modifiers;
    
    // 更新UI
    updateModifierList(modifiers);
    
    // 恢复UI状态
    ui->searchButton->setEnabled(true);
    ui->searchEdit->setEnabled(true);
    ui->modifierTable->setEnabled(true);
    ui->downloadProgress->setVisible(false);
    
    // 显示搜索结果提示
    if (modifiers.isEmpty()) {
        showStatusMessage(tr("未找到符合条件的修改器"), 5000);
    } else {
        showStatusMessage(tr("成功获取到 %1 个修改器").arg(modifiers.size()), 3000);
    }
}

// 修改器表格项点击
void DownloadIntegrator::onModifierItemClicked(int row, int column)
{
    // 获取当前显示的修改器列表
    if (row >= 0 && row < modifierList.size()) {
        const ModifierInfo& selectedModifier = modifierList[row];
        QString url = selectedModifier.url;
        
        qDebug() << "选择修改器：" << selectedModifier.name << "，URL：" << url;
        
        if (url.isEmpty()) {
            showStatusMessage("修改器URL为空，无法获取详情", 3000);
            return;
        }
        
        // 显示加载中状态
        showStatusMessage("正在加载修改器详情...");
        ui->gameTitle->setText(selectedModifier.name);
        ui->versionInfo->setText("游戏版本：加载中...");
        ui->optionsCount->setText("修改器选项：加载中...");
        ui->lastUpdate->setText("最后更新：加载中...");
        ui->modifierOptions->clear();
        ui->modifierOptions->setPlainText("加载中...");
        
        // 设置游戏封面加载状态
        ui->gameCoverLabel->clear();
        
        // 加载详细信息
        ModifierManager::getInstance().getModifierDetail(url, [this](ModifierInfo* modifier) {
            // 替换旧的详情
            delete currentModifier;
            currentModifier = modifier;
            
            // 更新UI
            updateModifierDetail();
            showStatusMessage("修改器详情获取成功", 3000);
        });
    }
}

// 下载按钮点击
void DownloadIntegrator::onDownloadButtonClicked()
{
    qDebug() << "下载按钮点击";
    
    // 确保选择了要下载的修改器
    if (!currentModifier) {
        QMessageBox::warning(this, "错误", "请先选择一个修改器！");
        return;
    }
    
    // 获取修改器名称
    QString modifierName = currentModifier->name;
    if (modifierName.isEmpty()) {
        QMessageBox::warning(this, "错误", "修改器名称为空！");
        return;
    }
    
    // 获取下载路径
    QString downloadPath = getDownloadDirectory();
    
    // 检查是否使用预设下载路径
    bool useCustomPath = ConfigManager::getInstance().getUseCustomDownloadPath();
    if (useCustomPath) {
        downloadPath = ConfigManager::getInstance().getCustomDownloadPath();
    }
    
    // 确保下载目录存在
    QDir dir(downloadPath);
    if (!dir.exists()) {
        if (!dir.mkpath(downloadPath)) {
            QMessageBox::warning(this, "错误", "无法创建下载目录：" + downloadPath);
            return;
        }
    }
    
    // 构建文件名 - 使用FileSystem清理文件名
    QString sanitizedName = FileSystem::getInstance().sanitizeFileName(modifierName);
    QString fileName = sanitizedName + "_v";
    
    // 添加版本号
    if (ui->versionSelect->currentIndex() >= 0 && ui->versionSelect->currentIndex() < currentModifier->versions.size()) {
        // 使用选定版本
        int index = ui->versionSelect->currentIndex();
        fileName += currentModifier->versions[index].first;
    } else {
        fileName += "latest";
    }
    
    // 确定下载URL
    QString downloadUrl;
    
    if (ui->versionSelect->currentIndex() >= 0 && ui->versionSelect->currentIndex() < currentModifier->versions.size()) {
        // 使用选定版本的URL
        int index = ui->versionSelect->currentIndex();
        downloadUrl = currentModifier->versions[index].second;
    } else if (!currentModifier->versions.isEmpty()) {
        // 默认使用最新版本的URL
        downloadUrl = currentModifier->versions.first().second;
    }
    
    // 检查URL是否有效
    if (downloadUrl.isEmpty()) {
        QMessageBox::warning(this, "下载失败", "无法获取下载链接，请重试或选择其他版本。");
        return;
    }
    
    // 清理URL，移除末尾的逗号
    while (downloadUrl.endsWith(",")) {
        downloadUrl.chop(1);
    }
    
    // 智能推断文件扩展名
    QString fileExtension = getFileExtensionFromUrl(downloadUrl);
    if (fileExtension.isEmpty()) {
        // 对于FLiNG网站的动态下载链接，根据URL特征和修改器名称判断
        if (downloadUrl.contains("flingtrainer.com/downloads/")) {
            // FLiNG网站的修改器通常是压缩包格式
            // 但我们先假设是exe，让后续的文件格式检测来修正
            fileExtension = ".exe";
            qDebug() << "检测到FLiNG下载链接，使用临时.exe扩展名，将在下载后自动检测并修正";
        } else {
            // 其他情况默认使用.exe
            fileExtension = ".exe";
            qDebug() << "无法从URL推断扩展名，使用默认.exe扩展名";
        }
    }
    
    qDebug() << "下载URL:" << downloadUrl;
    qDebug() << "推断的文件扩展名:" << fileExtension;
    
    // 添加正确的扩展名
    fileName += fileExtension;
    
    // 确认下载
    QString message = QString("确认下载以下修改器？\n\n名称: %1\n版本: %2\n存储位置: %3")
                    .arg(modifierName)
                    .arg(ui->versionSelect->currentText())
                    .arg(downloadPath + "/" + fileName);
    
    if (QMessageBox::question(this, "下载确认", message, 
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        // 完整下载路径
        QString fullDownloadPath = downloadPath + "/" + fileName;
        
        // 确保进度条可见并设置初始值
        ui->downloadProgress->setValue(0);
        ui->downloadProgress->setVisible(true);
        ui->downloadProgress->setStyleSheet(
            "QProgressBar {"
            "   border: 1px solid #bdc3c7;"
            "   border-radius: 5px;"
            "   text-align: center;"
            "   background-color: #f5f5f5;"
            "   height: 20px;"
            "}"
            "QProgressBar::chunk {"
            "   background-color: #3498db;"
            "   width: 10px;"
            "   margin: 0.5px;"
            "   border-radius: 3px;"
            "}"
        );
        
        // 执行下载
        if (ConfigManager::getInstance().getUseMockDownload()) {
            // 使用模拟下载方法作为备用方案
            simulateDownload(downloadUrl, fileName);
        } else {
            // 使用ModifierManager下载修改器
            ModifierManager::getInstance().downloadModifier(
                *currentModifier,
                ui->versionSelect->currentText(),
                fullDownloadPath,
                // 下载完成回调
                [this, fileName, fullDownloadPath](bool success, const QString& errorMsg, const QString& actualPath, const ModifierInfo& modifier, bool isArchive) {
                    ui->downloadProgress->setVisible(false);
                    ui->downloadButton->setEnabled(true);
                    
                    if (success) {
                        showStatusMessage("下载完成", 5000);
                        
                        QString displayMessage;
                        QString finalFileName = fileName;
                        
                        // 检查是否有扩展名修正的信息
                        if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                            // 从错误消息中提取新文件名
                            QRegularExpression regex("文件格式已自动检测并修正为: (.+)");
                            QRegularExpressionMatch match = regex.match(errorMsg);
                            if (match.hasMatch()) {
                                finalFileName = match.captured(1);
                                qDebug() << "文件扩展名已修正为：" << finalFileName;
                            }
                        }
                        
                        if (isArchive) {
                            displayMessage = QString("修改器 %1 下载完成。\n\n"
                                            "⚠️ 注意：此修改器为压缩包格式，可能包含反作弊程序和使用说明。\n"
                                            "请手动解压文件并仔细阅读其中的使用说明，\n"
                                            "确保遵循相关操作约定后再使用。").arg(finalFileName);
                            
                            if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                                displayMessage += "\n\n✅ 文件格式已自动检测并修正。";
                            }
                            
                            QMessageBox::information(this, "下载完成 - 压缩包格式", displayMessage);
                        } else {
                            displayMessage = "修改器 " + finalFileName + " 下载完成。";
                            
                            if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                                displayMessage += "\n\n✅ 文件格式已自动检测并修正。";
                            }
                            
                            QMessageBox::information(this, "下载完成", displayMessage);
                        }
                                               
                        // 获取版本信息
                        QString version;
                        if (ui->versionSelect->currentIndex() >= 0 && ui->versionSelect->currentIndex() < modifier.versions.size()) {
                            version = ui->versionSelect->currentText();
                        } else if (!modifier.versions.isEmpty()) {
                            version = modifier.versions.first().first;
                        }
                        
                        // 添加到已下载修改器列表 - 使用实际的文件路径
                        addDownloadedModifier(modifier, version, actualPath);
                        
                        // 切换到已下载标签页
                        m_tabWidget->setCurrentIndex(1);
                    } else {
                        showStatusMessage(QString("下载失败: %1").arg(errorMsg), 5000);
                        QMessageBox::warning(this, "下载失败", 
                                           QString("下载修改器 %1 时发生错误: %2").arg(fileName).arg(errorMsg));
                    }
                },
                // 进度回调
                [this](int progress) {
                    ui->downloadProgress->setValue(progress);
                    ui->downloadProgress->setFormat("%p%");
                    showStatusMessage(QString("下载中... %1%").arg(progress));
                }
            );
        }
    }
}

// 模拟下载过程 - 作为备用方案
void DownloadIntegrator::simulateDownload(const QString& url, const QString& fileName)
{
    qDebug() << "模拟下载：" << fileName << "，URL：" << url;
    
    // 显示进度条
    ui->downloadProgress->setValue(0);
    ui->downloadProgress->setVisible(true);
    ui->downloadProgress->setFormat("%p%");
    ui->downloadProgress->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #bdc3c7;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   background-color: #f5f5f5;"
        "   height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #3498db;"
        "   width: 10px;"
        "   margin: 0.5px;"
        "   border-radius: 3px;"
        "}"
    );
    
    showStatusMessage("正在下载 " + fileName + "...");
    ui->downloadButton->setEnabled(false);
    
    // 创建一个定时器来模拟下载进度
    QTimer* timer = new QTimer(this);
    int progress = 0;
    
    // 使用非捕获this的方式连接信号槽
    connect(timer, &QTimer::timeout, [this, timer, progress, fileName]() mutable {
        progress += 5;
        if (progress <= 100) {
            ui->downloadProgress->setValue(progress);
            showStatusMessage(QString("下载中... %1%").arg(progress));
        } else {
            // 下载完成
            timer->stop();
            timer->deleteLater();
            
            ui->downloadProgress->setVisible(false);
            ui->downloadButton->setEnabled(true);
            showStatusMessage("下载完成", 5000);
            
            QMessageBox::information(this, "下载完成", 
                                   "修改器 " + fileName + " 下载完成。\n\n" + 
                                   "注意：这只是一个模拟下载，没有实际下载文件。");
        }
    });
    
    // 开始模拟下载
    timer->start(100);
}

// 打开下载文件夹
void DownloadIntegrator::onOpenFolderButtonClicked()
{
    // 使用用户配置的下载目录，而不是系统默认下载目录
    QString downloadPath = ConfigManager::getInstance().getDownloadDirectory();
    qDebug() << "打开用户配置的下载文件夹：" << downloadPath;
    
    // 确保目录存在
    QDir downloadDir(downloadPath);
    if (!downloadDir.exists()) {
        if (downloadDir.mkpath(".")) {
            qDebug() << "创建下载目录：" << downloadPath;
        } else {
            qDebug() << "无法创建下载目录：" << downloadPath;
            showStatusMessage("无法创建下载目录：" + downloadPath, 5000);
            return;
        }
    }
    
    QDesktopServices::openUrl(QUrl::fromLocalFile(downloadPath));
    showStatusMessage("打开文件夹：" + downloadPath, 3000);
}

// 设置按钮点击
void DownloadIntegrator::onSettingsButtonClicked()
{
    qDebug() << "打开设置对话框";
    
    // 创建并显示设置对话框
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 设置已保存，更新应用程序设置
        qDebug() << "设置已保存";
        showStatusMessage("设置已更新", 3000);
    }
}

// 版本类型改变
void DownloadIntegrator::onVersionTypeChanged(bool checked)
{
    // 版本选择框总是启用的
    ui->versionSelect->setEnabled(true);
    qDebug() << "版本选择框已启用";
}

// 版本选择改变
void DownloadIntegrator::onVersionSelectionChanged(int index)
{
    if (index >= 0) {
        qDebug() << "版本选择更改为索引：" << index;
    }
}

// 排序顺序改变
void DownloadIntegrator::onSortOrderChanged(int index)
{
    // 获取ModifierManager的当前修改器列表
    QList<ModifierInfo> currentModifiers = ModifierManager::getInstance().getSortedModifierList();
    
    // 使用SearchManager进行排序
    SearchManager& searchManager = SearchManager::getInstance();
    QList<ModifierInfo> sortedModifiers;
    
    // 根据选择的排序方式进行排序
    switch (index) {
        case 0: // 最近更新
            sortedModifiers = searchManager.sortByDate(currentModifiers);
            break;
            
        case 1: // 按名称
            std::sort(currentModifiers.begin(), currentModifiers.end(), [](const ModifierInfo &a, const ModifierInfo &b) {
                return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
            });
            sortedModifiers = currentModifiers;
            break;
            
        case 2: // 按功能数量
            sortedModifiers = searchManager.sortByPopularity(currentModifiers);
            break;
            
        default:
            sortedModifiers = currentModifiers;
            break;
    }
    
    // 更新ModifierManager的列表
    searchManager.updateModifierManagerList(sortedModifiers);
    
    // 更新UI显示
    updateModifierList(sortedModifiers);
    
    // 显示排序提示
    QString sortType;
    switch (index) {
        case 0:
            sortType = "最近更新";
            break;
        case 1:
            sortType = "名称";
            break;
        case 2:
            sortType = "功能数量";
            break;
    }
    
    showStatusMessage(QString("已按%1排序").arg(sortType), 2000);
}

// 获取修改器列表
void DownloadIntegrator::fetchModifierList(const QString& searchTerm)
{
    showStatusMessage("正在从网络获取修改器列表...");
    
    // 使用ModifierManager进行搜索
    ModifierManager::getInstance().searchModifiers(
        searchTerm,
        [this, searchTerm](const QList<ModifierInfo>& results) {
            // 搜索结果回调
            modifierList = results;
            
            // 如果未找到结果，但在标题中包含"Red Dead Redemption"，尝试特殊处理
            if (modifierList.isEmpty() && searchTerm.contains("Red", Qt::CaseInsensitive)) {
                qDebug() << "针对'Red Dead Redemption'系列的特殊处理";
                
                // 强制添加Red Dead Redemption 2的结果
                if (searchTerm.contains("2", Qt::CaseInsensitive) || 
                    searchTerm.contains("Redemption", Qt::CaseInsensitive)) {
                ModifierInfo rdr2;
                rdr2.name = "Red Dead Redemption 2";
                rdr2.url = "https://flingtrainer.com/trainer/red-dead-redemption-2-trainer/";
                rdr2.gameVersion = "Offline/story mode only";
                rdr2.lastUpdate = "2024";
                rdr2.optionsCount = 561; // 从网站看到的值
                    modifierList.append(rdr2);
                    
                    qDebug() << "强制添加RDR2结果";
                }
                
                // 可能还需要添加Red Dead Redemption 1
                ModifierInfo rdr1;
                rdr1.name = "Red Dead Redemption";
                rdr1.url = "https://flingtrainer.com/trainer/red-dead-redemption-trainer/";
                rdr1.gameVersion = "Latest";
                rdr1.lastUpdate = "2024";
                rdr1.optionsCount = 47;
                modifierList.append(rdr1);
                qDebug() << "强制添加RDR1结果";
            }
            
            // 更新UI
            updateModifierList();
            showStatusMessage(QString("成功获取到 %1 个修改器").arg(modifierList.size()), 3000);
        }
    );
}

// 获取修改器详情
void DownloadIntegrator::fetchModifierDetail(const QString& url)
{
    showStatusMessage("正在获取修改器详情...");
    
    // 使用ModifierManager获取详情
    ModifierManager::getInstance().getModifierDetail(
        url,
        [this, url](ModifierInfo* modifier) {
            if (modifier) {
                qDebug() << "修改器详情获取成功：" << modifier->name 
                        << "，提取了" << modifier->options.size() << "个选项"
                        << "，官方标记选项数量：" << modifier->optionsCount;
                
                // 替换旧的详情
                delete currentModifier;
                currentModifier = modifier;
                
                // 保存URL以便重新获取
                currentModifier->url = url;
                
                // 更新UI
                updateModifierDetail();
                showStatusMessage("修改器详情获取成功", 3000);
            } else {
                qDebug() << "获取修改器详情失败";
                showStatusMessage("获取修改器详情失败", 5000);
            }
        }
    );
}

// 获取修改器详情 - 示例数据版本
void DownloadIntegrator::loadSampleData()
{
    qDebug() << "加载示例数据...";
    
    modifierList.clear();
    
    // 添加一些示例修改器
    ModifierInfo info1;
    info1.name = "赛博朋克 2077";
    info1.gameVersion = "2.0+";
    info1.lastUpdate = "2023-12-15";
    info1.optionsCount = 15; // 示例数据
    info1.url = "https://flingtrainer.com/trainer/cyberpunk-2077-trainer/";
    modifierList.append(info1);
    
    ModifierInfo info2;
    info2.name = "巫师 3：狂猎";
    info2.gameVersion = "GOTY 1.32";
    info2.lastUpdate = "2023-05-20";
    info2.optionsCount = 12; // 示例数据
    info2.url = "https://flingtrainer.com/trainer/the-witcher-3-wild-hunt-trainer/";
    modifierList.append(info2);
    
    ModifierInfo info3;
    info3.name = "刺客信条：起源";
    info3.gameVersion = "1.5.0";
    info3.lastUpdate = "2022-11-30";
    info3.optionsCount = 8; // 示例数据
    info3.url = "https://flingtrainer.com/trainer/assassins-creed-origins-trainer/";
    modifierList.append(info3);
    
    // 更新表格UI
    updateModifierList();
    
    qDebug() << "示例数据加载完成。";
}

// 获取修改器详情
void DownloadIntegrator::loadModifierDetail(const QString& modifierName)
{
    qDebug() << "加载修改器详情：" << modifierName;
    
    // 创建示例详情
    ModifierInfo* modifier = new ModifierInfo();
    modifier->name = modifierName;
    modifier->optionsCount = 0; // 确保初始化为0
    
    // 查找对应的基础信息
    for (const ModifierInfo& info : modifierList) {
        if (info.name == modifierName) {
            modifier->gameVersion = info.gameVersion;
            modifier->lastUpdate = info.lastUpdate;
            modifier->optionsCount = info.optionsCount;
            break;
        }
    }
    
    // 添加示例选项
    if (modifierName == "赛博朋克 2077" || modifierName.toLower() == "cyberpunk 2077") {
        // 添加基本选项类别
        modifier->options.append("● 基本选项");
        modifier->options.append("• Num 1 – Infinite Health");
        modifier->options.append("• Num 2 – Infinite Stamina");
        modifier->options.append("• Num 3 – Infinite Items/Ammo");
        modifier->options.append("• Num 4 – Items Won't Decrease");
        modifier->options.append("• Num 5 – Healing Items No Cooldown");
        modifier->options.append("• Num 6 – Grenades No Cooldown");
        modifier->options.append("• Num * – Projectile Launch System No Cooldown");
        modifier->options.append("• Num 7 – No Reload");
        modifier->options.append("• Num 8 – Super Accuracy");
        modifier->options.append("• Num 9 – No Recoil");
        modifier->options.append("• Num 0 – One Hit Kill");
        modifier->options.append("• Num . – Damage Multiplier");
        modifier->options.append("• Num + – Defense Multiplier");
        modifier->options.append("• Num – – Stealth Mode");
        
        // 添加控制选项类别
        modifier->options.append("● 控制选项");
        modifier->options.append("• Ctrl+Num 1 – Edit Money");
        modifier->options.append("• Ctrl+Num 2 – Infinite XP");
        modifier->options.append("• Ctrl+Num 3 – XP Multiplier");
        modifier->options.append("• Ctrl+Num 4 – Infinite Street Cred");
        modifier->options.append("• Ctrl+Num 5 – Street Cred Multiplier");
        modifier->options.append("• Ctrl+Num 6 – Max Skill XP/Progression");
        modifier->options.append("• Ctrl+Num 7 – Skill XP Multiplier");
        modifier->options.append("• Ctrl+Num 8 – Edit Attribute Points");
        modifier->options.append("• Ctrl+Num 9 – Edit Perk Points");
        modifier->options.append("• Ctrl+Num 0 – Edit Relic Points");
        modifier->options.append("• Ctrl+Num . – Ignore Cyberware Capacity");
        modifier->options.append("• Ctrl+Num + – Set Game Speed");
        
        // 添加高级选项类别
        modifier->options.append("● 高级选项");
        modifier->options.append("• Alt+Num 1 – Infinite RAM");
        modifier->options.append("• Alt+Num 2 – Freeze Breach Protocol Timer");
        modifier->options.append("• Alt+Num 3 – Infinite Components");
        modifier->options.append("• Alt+Num 4 – Infinite Quickhack Components");
        modifier->options.append("• Alt+Num 5 – Edit Max Carrying Weight");
        modifier->options.append("• Alt+Num 6 – Set Movement Speed");
        modifier->options.append("• Alt+Num 7 – Super Jump");
        modifier->options.append("• Alt+Num 8 – Infinite Double Jumps (Reinforced Tendons)");
        modifier->options.append("• Alt+Num 9 – Edit Player Level");
        modifier->options.append("• Alt+Num 0 – Edit Street Cred Level");
        modifier->options.append("• Alt+Num . – Freeze Daytime");
        modifier->options.append("• Alt+Num + – Daytime +1 Hour");
        
        // 添加特殊选项类别
        modifier->options.append("● 特殊选项");
        modifier->options.append("• Shift+F1 – Edit Headhunter Skill Level");
        modifier->options.append("• Shift+F2 – Edit Netrunner Skill Level");
        modifier->options.append("• Shift+F3 – Edit Shinobi Skill Level");
        modifier->options.append("• Shift+F4 – Edit Solo Skill Level");
        modifier->options.append("• Shift+F5 – Edit Engineer Skill Level");
        modifier->options.append("• Shift+PageUp – Enable Fly Mode");
        modifier->options.append("• Shift+Num + – Set Fly Height");
        modifier->options.append("• Shift+Num – – Set Fly Speed");
        
        // 更新选项计数
        modifier->optionsCount = 46;
        
        // 添加版本
        modifier->versions.append(qMakePair("自动更新版本", "https://example.com/cp2077-latest.exe"));
        modifier->versions.append(qMakePair("v2.0-v2.13+", "https://example.com/cp2077-v2.13.exe"));
    }
    else if (modifierName == "巫师 3：狂猎") {
        modifier->options.append("无限生命值");
        modifier->options.append("无限耐力");
        modifier->options.append("无限技能点");
        modifier->options.append("无限金钱");
        modifier->options.append("无限物品");
        modifier->options.append("一击必杀");
        modifier->options.append("无限符文槽");
        modifier->options.append("无限炼金材料");
        modifier->options.append("超级伊格尼法印");
        modifier->options.append("超级阿尔德法印");
        modifier->options.append("超级亚登法印");
        modifier->options.append("超级昆恩法印");
        
        // 添加示例版本
        modifier->versions.append(qMakePair("自动更新版本", "https://example.com/witcher3-latest.exe"));
        modifier->versions.append(qMakePair("GOTY v1.32", "https://example.com/witcher3-goty-v1.32.exe"));
        modifier->versions.append(qMakePair("v1.31", "https://example.com/witcher3-v1.31.exe"));
    }
    else if (modifierName == "刺客信条：起源") {
        modifier->options.append("无限生命值");
        modifier->options.append("无限金钱");
        modifier->options.append("无限经验值");
        modifier->options.append("无限技能点");
        modifier->options.append("无限弓箭");
        modifier->options.append("一击必杀");
        modifier->options.append("超级速度");
        modifier->options.append("隐身模式");
        
        // 添加示例版本
        modifier->versions.append(qMakePair("自动更新版本", "https://example.com/ac-origins-latest.exe"));
        modifier->versions.append(qMakePair("v1.5.0", "https://example.com/ac-origins-v1.5.0.exe"));
        modifier->versions.append(qMakePair("v1.4.0", "https://example.com/ac-origins-v1.4.0.exe"));
    }
    else {
        // 默认选项
        modifier->options.append("无限生命值");
        modifier->options.append("无限弹药");
        modifier->options.append("无限金钱");
        
        // 默认版本
        modifier->versions.append(qMakePair("自动更新版本", "https://example.com/default-latest.exe"));
    }
    
    // 替换旧的详情
    delete currentModifier;
    currentModifier = modifier;
    
    // 更新UI
    updateModifierDetail();
}

// 运行按钮点击事件
void DownloadIntegrator::onRunButtonClicked()
{
    // 获取选中的行
    QList<QTableWidgetItem*> selectedItems = m_downloadedTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    onDownloadedModifierDoubleClicked(row, 0);
}

// 删除按钮点击事件
void DownloadIntegrator::onDeleteButtonClicked()
{
    // 获取选中的行
    QList<QTableWidgetItem*> selectedItems = m_downloadedTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    
    // 确认删除
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", 
                                    "确定要删除选中的修改器吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        // 从ModifierManager中删除
        ModifierManager::getInstance().removeDownloadedModifier(row);
        updateDownloadedModifiersList();
    }
}

// 检查所有修改器的更新
void DownloadIntegrator::checkForUpdates(int index)
{
    if (index >= 0) {
        qDebug() << "检查单个修改器更新，索引:" << index;
        showStatusMessage("正在检查更新...");
        
        // 使用ModifierManager检查单个修改器更新
        ModifierManager::getInstance().checkForUpdates(
            index,
            [this](bool hasUpdate) {
                if (hasUpdate) {
                    showStatusMessage("发现更新！", 3000);
                } else {
                    showStatusMessage("没有可用更新", 3000);
                }
                
                // 更新UI以反映更新状态
                updateDownloadedModifiersList();
            }
        );
        } else {
        // 检查所有修改器的更新
        qDebug() << "检查所有修改器更新";
        showStatusMessage("正在检查更新...");
        
        // 使用ModifierManager检查所有修改器更新
        ModifierManager::getInstance().checkForUpdates(
            -1,
            [this](bool hasValidUrls) {
                if (!hasValidUrls) {
                    QMessageBox::information(this, "无法检查更新",
                                           "没有找到可以检查更新的修改器。请确保您已下载的修改器有关联的详情页URL。");
                }
                
                // 更新UI以反映更新状态
    updateDownloadedModifiersList();
}
        );
    }
}

// 添加已下载修改器
void DownloadIntegrator::addDownloadedModifier(const ModifierInfo& info, const QString& version, const QString& filePath)
{
    qDebug() << "添加已下载修改器:" << info.name << "版本:" << version;
    
    // 使用ModifierManager添加修改器
    ModifierManager::getInstance().addDownloadedModifier(info, version, filePath);
    
    // 更新UI
    updateDownloadedModifiersList();
}

// 删除已下载修改器
void DownloadIntegrator::removeDownloadedModifier(int index)
{
    if (index < 0) {
        return;
    }
    
    qDebug() << "删除修改器，索引:" << index;
    
    // 使用ModifierManager删除修改器
    ModifierManager::getInstance().removeDownloadedModifier(index);
    
    // 更新UI
    updateDownloadedModifiersList();
}

// 保存已下载修改器列表
void DownloadIntegrator::saveDownloadedModifiers()
{
    // 使用ModifierManager保存列表
    ModifierManager::getInstance().saveDownloadedModifiers();
}

// 已下载修改器双击事件
void DownloadIntegrator::onDownloadedModifierDoubleClicked(int row, int column)
{
    QList<DownloadedModifierInfo> downloadedModifiers = ModifierManager::getInstance().getDownloadedModifiers();
    
    if (row >= 0 && row < downloadedModifiers.size()) {
        // 双击运行修改器
        const DownloadedModifierInfo& info = downloadedModifiers[row];
        qDebug() << "运行修改器:" << info.name;
        
        QFileInfo fileInfo(info.filePath);
        if (!fileInfo.exists()) {
            // 处理文件不存在的情况（保持原有逻辑不变）
            QString message = QString("修改器文件不存在：\n%1\n\n"
                                    "可能的原因：\n"
                                    "• 文件被移动或删除\n"
                                    "• 文件被杀毒软件误删\n"
                                    "• 存储路径发生变化\n\n"
                                    "是否从列表中移除此项？")
                                    .arg(info.filePath);
            
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, 
                "文件不存在", 
                message,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            
            if (reply == QMessageBox::Yes) {
                // 从列表中删除
                ModifierManager::getInstance().removeDownloadedModifier(row);
                updateDownloadedModifiersList();
            }
            return;
        }
        
        // 文件存在，检测文件类型
        if (isExecutableFile(info.filePath)) {
            // 可执行文件，直接运行
            QProcess::startDetached(info.filePath, QStringList());
            showStatusMessage("已启动 " + info.name, 3000);
        } else if (isArchiveFile(info.filePath)) {
            // 压缩包文件，提供详细的安全提示
            QString detectedFormat = detectFileFormat(info.filePath);
            if (detectedFormat.isEmpty()) {
                // 如果魔数检测失败，使用扩展名
                detectedFormat = fileInfo.suffix().toUpper();
            } else {
                detectedFormat = detectedFormat.toUpper();
            }
            
            QString message = QString("修改器 \"%1\" 是压缩包格式 (.%2)。\n\n"
                                    "🔒 安全提示：\n"
                                    "此修改器以压缩包形式分发，可能包含：\n"
                                    "• 反作弊绕过程序\n"
                                    "• 详细的使用说明\n"
                                    "• 特殊的运行要求\n\n"
                                    "⚠️ 重要操作步骤：\n"
                                    "1. 右键点击文件选择解压\n"
                                    "2. 仔细阅读解压后的 README 或说明文件\n"
                                    "3. 关闭游戏和反作弊软件（如需要）\n"
                                    "4. 按照说明文档的要求操作\n"
                                    "5. 确保遵循所有使用约定\n\n"
                                    "是否打开文件所在文件夹进行手动操作？")
                                    .arg(info.name)
                                    .arg(detectedFormat);
            
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, 
                "压缩包修改器 - 需要手动操作", 
                message,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            
            if (reply == QMessageBox::Yes) {
                // 打开文件所在文件夹并选中文件
                QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(info.filePath));
                showStatusMessage("已打开文件夹，请手动解压并按说明操作", 5000);
            }
        } else {
            // 其他格式文件，提示用户
            QString extension = fileInfo.suffix();
            if (extension.isEmpty()) {
                extension = "未知";
            }
            
            QString message = QString("修改器 \"%1\" 是 %2 格式文件。\n\n"
                                    "这不是常见的可执行文件或压缩包格式。\n"
                                    "可能需要特殊的程序来打开此文件。\n\n"
                                    "是否在文件管理器中打开查看？")
                                    .arg(info.name)
                                    .arg(extension.isEmpty() ? "未知" : ("." + extension.toUpper()));
            
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, 
                "未知文件格式", 
                message,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            
            if (reply == QMessageBox::Yes) {
                QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(info.filePath));
            }
        }
    }
}

// 下载修改器
void DownloadIntegrator::downloadModifier(const QString& url, const QString& fileName)
{
    showStatusMessage("正在下载 " + fileName + "...");
    ui->downloadProgress->setValue(0);
    ui->downloadProgress->setVisible(true);
    ui->downloadButton->setEnabled(false);
    
    // 清理URL，移除末尾的逗号
    QString cleanUrl = url;
    while (cleanUrl.endsWith(",")) {
        cleanUrl.chop(1);
    }
    
    qDebug() << "原始URL: " << url;
    qDebug() << "清理后URL: " << cleanUrl;
    
    // 检查URL是否有效
    if (cleanUrl.isEmpty() || !cleanUrl.startsWith("http")) {
        QMessageBox::warning(this, "下载失败", 
                           "无效的下载链接: " + cleanUrl);
        ui->downloadProgress->setVisible(false);
        ui->downloadButton->setEnabled(true);
        return;
    }
    
    // 构建下载路径
    QString downloadPath = getDownloadDirectory() + "/" + fileName;
    
    // 使用ModifierManager下载修改器
    if (currentModifier) {
        // 使用ModifierManager下载
        ModifierManager::getInstance().downloadModifier(
            *currentModifier,
            ui->versionSelect->currentText(),
            downloadPath,
            // 下载完成回调 - 修复捕获
            [this, fileName, downloadPath](bool success, const QString& errorMsg, const QString& actualPath, const ModifierInfo& modifier, bool isArchive) {
                ui->downloadProgress->setVisible(false);
                ui->downloadButton->setEnabled(true);
                
                if (success) {
                    showStatusMessage("下载完成", 5000);
                    
                    QString finalFileName = fileName;
                    
                    // 检查是否有扩展名修正的信息
                    if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                        QRegularExpression regex("文件格式已自动检测并修正为: (.+)");
                        QRegularExpressionMatch match = regex.match(errorMsg);
                        if (match.hasMatch()) {
                            finalFileName = match.captured(1);
                            qDebug() << "文件扩展名已修正为：" << finalFileName;
                        }
                    }
                    
                    QString message;
                    if (isArchive) {
                        message = QString("修改器 %1 下载完成。\n\n"
                                        "⚠️ 注意：此修改器为压缩包格式，可能包含反作弊程序和使用说明。\n"
                                        "请手动解压文件并仔细阅读其中的使用说明，\n"
                                        "确保遵循相关操作约定后再使用。").arg(finalFileName);
                        
                        if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                            message += "\n\n✅ 文件格式已自动检测并修正。";
                        }
                        
                        QMessageBox::information(this, "下载完成 - 压缩包格式", message);
                    } else {
                        message = "修改器 " + finalFileName + " 下载完成。";
                        
                        if (!errorMsg.isEmpty() && errorMsg.contains("文件格式已自动检测并修正为")) {
                            message += "\n\n✅ 文件格式已自动检测并修正。";
                        }
                        
                        QMessageBox::information(this, "下载完成", message);
                    }
                                           
                    // 获取版本信息
                    QString version;
                    if (ui->versionSelect->currentIndex() >= 0 && ui->versionSelect->currentIndex() < modifier.versions.size()) {
                        version = ui->versionSelect->currentText();
                    } else if (!modifier.versions.isEmpty()) {
                        version = modifier.versions.first().first;
                    }
                    
                    // 添加到已下载修改器列表 - 使用实际的文件路径
                    addDownloadedModifier(modifier, version, actualPath);
                    
                    // 切换到已下载标签页
                    m_tabWidget->setCurrentIndex(1);
            } else {
                    showStatusMessage(QString("下载失败: %1").arg(errorMsg), 5000);
                    QMessageBox::warning(this, "下载失败", 
                                       QString("下载修改器 %1 时发生错误: %2").arg(fileName).arg(errorMsg));
                }
            },
            // 进度回调
            [this](int progress) {
                ui->downloadProgress->setValue(progress);
                showStatusMessage(QString("下载中... %1%").arg(progress));
            }
        );
    } else {
        QMessageBox::warning(this, "下载失败", "未选择有效的修改器");
        ui->downloadProgress->setVisible(false);
        ui->downloadButton->setEnabled(true);
    }
}

// 网络错误处理
void DownloadIntegrator::onNetworkError(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        qDebug() << "网络错误: " << static_cast<int>(code) << " - " << reply->errorString();
    }
}

// 在DownloadIntegrator构造函数中添加主题菜单设置
void DownloadIntegrator::setupThemeMenu() {
    QMenuBar* menuBar = this->menuBar();
    
    // 创建主题菜单
    QMenu* themeMenu = menuBar->addMenu("主题");
    
    // 创建动作组，确保只有一个主题被选中
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    
    // 创建浅色主题动作
    QAction* lightThemeAction = new QAction("浅色主题", this);
    lightThemeAction->setCheckable(true);
    connect(lightThemeAction, &QAction::triggered, this, &DownloadIntegrator::onLightThemeSelected);
    themeGroup->addAction(lightThemeAction);
    themeMenu->addAction(lightThemeAction);
    
    // 创建Windows 11主题动作
    QAction* win11ThemeAction = new QAction("Windows 11主题", this);
    win11ThemeAction->setCheckable(true);
    connect(win11ThemeAction, &QAction::triggered, this, &DownloadIntegrator::onWin11ThemeSelected);
    themeGroup->addAction(win11ThemeAction);
    themeMenu->addAction(win11ThemeAction);
    
    // 创建经典主题动作
    QAction* classicThemeAction = new QAction("经典主题", this);
    classicThemeAction->setCheckable(true);
    connect(classicThemeAction, &QAction::triggered, this, &DownloadIntegrator::onClassicThemeSelected);
    themeGroup->addAction(classicThemeAction);
    themeMenu->addAction(classicThemeAction);
    
    // 创建多彩主题动作
    QAction* colorfulThemeAction = new QAction("多彩主题", this);
    colorfulThemeAction->setCheckable(true);
    connect(colorfulThemeAction, &QAction::triggered, this, &DownloadIntegrator::onColorfulThemeSelected);
    themeGroup->addAction(colorfulThemeAction);
    themeMenu->addAction(colorfulThemeAction);
    
    // 根据当前设置选中对应主题
    ConfigManager::Theme currentTheme = ConfigManager::getInstance().getCurrentTheme();
    switch (currentTheme) {
        case ConfigManager::Theme::Light:
            lightThemeAction->setChecked(true);
            break;
        case ConfigManager::Theme::Win11:
            win11ThemeAction->setChecked(true);
            break;
        case ConfigManager::Theme::Classic:
            classicThemeAction->setChecked(true);
            break;
        case ConfigManager::Theme::Colorful:
            colorfulThemeAction->setChecked(true);
            break;
        default:
            lightThemeAction->setChecked(true);
            break;
    }
}

// 添加主题切换槽函数
void DownloadIntegrator::onLightThemeSelected() {
    ThemeManager::getInstance().switchTheme(*qApp, ConfigManager::Theme::Light);
    showStatusMessage("已切换到浅色主题");
}

void DownloadIntegrator::onWin11ThemeSelected() {
    ThemeManager::getInstance().switchTheme(*qApp, ConfigManager::Theme::Win11);
    showStatusMessage("已切换到Windows 11主题");
}

// 实现新主题的槽函数
void DownloadIntegrator::onClassicThemeSelected() {
    ThemeManager::getInstance().switchTheme(*qApp, ConfigManager::Theme::Classic);
    showStatusMessage("已切换到经典主题");
}

void DownloadIntegrator::onColorfulThemeSelected() {
    ThemeManager::getInstance().switchTheme(*qApp, ConfigManager::Theme::Colorful);
    showStatusMessage("已切换到多彩主题");
}

// 设置语言菜单
void DownloadIntegrator::setupLanguageMenu() {
    QMenuBar* menuBar = this->menuBar();
    
    // 创建语言菜单
    QMenu* languageMenu = menuBar->addMenu(tr("语言"));
    
    // 创建动作组，确保只有一种语言被选中
    QActionGroup* languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);
    
    // 创建中文选项
    QAction* chineseAction = new QAction("中文", this);
    chineseAction->setCheckable(true);
    connect(chineseAction, &QAction::triggered, this, &DownloadIntegrator::onChineseLanguageSelected);
    languageGroup->addAction(chineseAction);
    languageMenu->addAction(chineseAction);
    
    // 创建英文选项
    QAction* englishAction = new QAction("English", this);
    englishAction->setCheckable(true);
    connect(englishAction, &QAction::triggered, this, &DownloadIntegrator::onEnglishLanguageSelected);
    languageGroup->addAction(englishAction);
    languageMenu->addAction(englishAction);
    
    // 创建日文选项
    QAction* japaneseAction = new QAction("日本語", this);
    japaneseAction->setCheckable(true);
    connect(japaneseAction, &QAction::triggered, this, &DownloadIntegrator::onJapaneseLanguageSelected);
    languageGroup->addAction(japaneseAction);
    languageMenu->addAction(japaneseAction);
    
    // 根据当前设置选中对应语言
    ConfigManager::Language currentLanguage = ConfigManager::getInstance().getCurrentLanguage();
    switch (currentLanguage) {
        case ConfigManager::Language::Chinese:
            chineseAction->setChecked(true);
            break;
        case ConfigManager::Language::English:
            englishAction->setChecked(true);
            break;
        case ConfigManager::Language::Japanese:
            japaneseAction->setChecked(true);
            break;
        default:
            chineseAction->setChecked(true);
            break;
    }
}

// 语言切换槽函数
void DownloadIntegrator::onChineseLanguageSelected() {
    LanguageManager::getInstance().switchLanguage(*qApp, LanguageManager::Language::Chinese);
    retranslateUi(); // 重新翻译UI
    qApp->processEvents(); // 确保UI更新
    showStatusMessage(tr("已切换到中文"));
}

void DownloadIntegrator::onEnglishLanguageSelected() {
    LanguageManager::getInstance().switchLanguage(*qApp, LanguageManager::Language::English);
    retranslateUi(); // 重新翻译UI
    qApp->processEvents(); // 确保UI更新
    showStatusMessage(tr("已切换到英文 / Switched to English"));
}

void DownloadIntegrator::onJapaneseLanguageSelected() {
    LanguageManager::getInstance().switchLanguage(*qApp, LanguageManager::Language::Japanese);
    retranslateUi(); // 重新翻译UI
    qApp->processEvents(); // 确保UI更新
    showStatusMessage(tr("已切换到日文 / 日本語に切り替えました"));
}

// 添加UI重新翻译方法
void DownloadIntegrator::retranslateUi() {
    // 更新窗口标题
    this->setWindowTitle(tr("游戏修改器下载集成工具"));
    
    // 更新菜单项
    QList<QMenu*> menus = this->menuBar()->findChildren<QMenu*>();
    for (QMenu* menu : menus) {
        if (menu->title() == "主题" || menu->title() == "Theme" || menu->title() == "テーマ") {
            menu->setTitle(tr("主题"));
        }
        else if (menu->title() == "语言" || menu->title() == "Language" || menu->title() == "言語") {
            menu->setTitle(tr("语言"));
        }
        else if (menu->objectName() == "menuFile") {
            menu->setTitle(tr("文件"));
        }
    }
    
    // 更新主题菜单项
    QList<QAction*> themeActions = this->menuBar()->findChildren<QAction*>();
    for (QAction* action : themeActions) {
        if (action->text() == "浅色主题" || action->text() == "Light Theme" || action->text() == "ライトテーマ") {
            action->setText(tr("浅色主题"));
        }
        else if (action->text() == "Windows 11主题" || action->text() == "Windows 11 Theme" || action->text() == "Windows 11テーマ") {
            action->setText(tr("Windows 11主题"));
        }
        else if (action->text() == "经典主题" || action->text() == "Classic Theme" || action->text() == "クラシックテーマ") {
            action->setText(tr("经典主题"));
        }
        else if (action->text() == "多彩主题" || action->text() == "Colorful Theme" || action->text() == "カラフルテーマ") {
            action->setText(tr("多彩主题"));
        }
        else if (action->objectName() == "actionSettings") {
            action->setText(tr("设置"));
        }
        else if (action->objectName() == "actionExit") {
            action->setText(tr("退出"));
        }
    }
    
    // 更新标签页标题
    if (m_tabWidget) {
        m_tabWidget->setTabText(0, tr("搜索修改器"));
        m_tabWidget->setTabText(1, tr("已下载修改器"));
    }
    
    // 更新搜索页面
    ui->searchEdit->setPlaceholderText(tr("搜索游戏..."));
    ui->searchButton->setText(tr("搜索"));
    
    // 更新排序下拉框
    ui->sortComboBox->setItemText(0, tr("最近更新"));
    ui->sortComboBox->setItemText(1, tr("按名称"));
    ui->sortComboBox->setItemText(2, tr("下载次数"));
    
    // 更新表格列标题
    QTableWidgetItem *item0 = ui->modifierTable->horizontalHeaderItem(0);
    if (item0) item0->setText(tr("游戏名称"));
    
    QTableWidgetItem *item1 = ui->modifierTable->horizontalHeaderItem(1);
    if (item1) item1->setText(tr("更新日期"));
    
    QTableWidgetItem *item2 = ui->modifierTable->horizontalHeaderItem(2);
    if (item2) item2->setText(tr("支持版本"));
    
    QTableWidgetItem *item3 = ui->modifierTable->horizontalHeaderItem(3);
    if (item3) item3->setText(tr("选项数量"));
    
    // 更新版本信息标签（现在在封面组中）
    ui->versionInfo->setText(tr("游戏版本："));
    ui->optionsCount->setText(tr("修改器选项："));
    ui->lastUpdate->setText(tr("最后更新："));
    
    // 更新下载选项区
    ui->downloadGroup->setTitle(tr("版本选择"));
    
    // 更新修改器选项文本
    ui->modifierOptions->setPlaceholderText(tr("修改器功能选项列表..."));
    
    // 更新按钮
    ui->downloadButton->setText(tr("下载"));
    ui->openFolderButton->setText(tr("打开下载目录"));
    ui->settingsButton->setText(tr("设置"));
    
    // 更新已下载标签页
    QLabel* titleLabel = m_downloadedTab->findChild<QLabel*>();
    if (titleLabel) {
        titleLabel->setText(tr("已下载的游戏修改器"));
    }
    
    // 更新已下载表格
    if (m_downloadedTable) {
        QStringList headers;
        headers << tr("修改器名称") << tr("版本") << tr("游戏版本") << tr("下载日期");
        m_downloadedTable->setHorizontalHeaderLabels(headers);
    }
    
    // 更新按钮
    if (m_runButton) {
        m_runButton->setText(tr("运行"));
    }
    
    if (m_deleteButton) {
        m_deleteButton->setText(tr("删除"));
    }
    
    if (m_checkUpdateButton) {
        m_checkUpdateButton->setText(tr("检查更新"));
    }

    // 刷新表格数据，以更新表格中的内容
    // 重新加载修改器列表和已下载修改器列表
    updateModifierList();
    updateDownloadedModifiersList();
    
    // 更新状态栏
    QLabel* statusLabel = statusBar()->findChild<QLabel*>("statusLabel");
    if(statusLabel) {
        statusLabel->setText(tr("就绪"));
    }
    
    // 强制更新布局
    this->update();
}

// 刷新按钮点击处理方法
void DownloadIntegrator::onRefreshButtonClicked()
{
    qDebug() << "刷新按钮被点击，正在获取最新修改器列表...";
    
    // 清空搜索框，触发回到初始列表
    ui->searchEdit->clear();
    
    // 显示加载中状态
    QApplication::setOverrideCursor(Qt::WaitCursor);
    showStatusMessage(tr("正在获取最新数据..."));
    
    // 调用SearchManager获取最新修改器列表
    SearchManager::getInstance().fetchRecentlyUpdatedModifiers(this, &DownloadIntegrator::onRefreshCompleted);
}

// 添加刷新完成处理方法
void DownloadIntegrator::onRefreshCompleted(const QList<ModifierInfo>& modifiers) {
    qDebug() << "获取到" << modifiers.size() << "个最新修改器";
    
    // 保存到本地修改器列表
    modifierList = modifiers;
    
    // 更新UI显示
    updateModifierList(modifierList);
    
    // 禁用刷新功能，直到修复UI元素问题
    
    // 恢复光标
    QApplication::restoreOverrideCursor();
    
    // 显示状态消息
    if (modifiers.isEmpty()) {
        showStatusMessage(tr("未能获取最新数据，请检查网络连接"));
    } else {
        showStatusMessage(tr("最新数据已更新，共获取 %1 个修改器").arg(modifiers.size()));
    }
}

// 从HTML内容中解析修改器选项
void DownloadIntegrator::parseOptionsFromHTMLContent(const QString& htmlContent) {
    if (htmlContent.isEmpty()) {
        QMessageBox::warning(this, tr("解析错误"), tr("HTML内容为空，无法解析选项"));
        return;
    }
    
    qDebug() << "从用户提供的HTML内容解析修改器选项";
    
    // 从HTML内容中检测游戏名称
    QString gameName = ModifierParser::detectGameNameFromHTML(htmlContent);
    
    // 从HTML内容中解析选项
    QStringList options = ModifierParser::parseOptionsFromHTML(htmlContent);
    
    if (options.isEmpty()) {
        QMessageBox::warning(this, tr("解析错误"), 
                          tr("未能从HTML内容中提取任何选项，请确认内容格式正确"));
        return;
    }
    
    // 创建或更新当前修改器
    if (!currentModifier) {
        currentModifier = new ModifierInfo();
        currentModifier->optionsCount = 0;
    }
    
    // 更新游戏名称（如果检测到）
    if (!gameName.isEmpty()) {
        currentModifier->name = gameName;
    } else if (currentModifier->name.isEmpty()) {
        // 如果没有检测到名称，且当前没有名称，使用默认名称
        currentModifier->name = tr("未知游戏");
    }
    
    // 更新选项列表
    currentModifier->options = options;
    currentModifier->optionsCount = options.size();
    
    // 更新UI
    updateModifierDetail();
    
    // 显示成功消息
    showStatusMessage(tr("成功从HTML内容中提取了 %1 个选项").arg(options.size()), 3000);
    
    QMessageBox::information(this, tr("解析完成"), 
                          tr("成功从HTML内容中提取了 %1 个选项。\n游戏名称: %2")
                          .arg(options.size())
                          .arg(currentModifier->name));
}
