#pragma once

#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QProgressBar>
#include <QLabel>
#include <QStatusBar>
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QTimer>
#include <QProcess>
#include <QLineEdit>
#include <QDir>

/**
 * @brief UI辅助类，提供UI操作相关的静态方法
 * 
 * 这个类提供了常用的UI操作，如显示消息框、设置样式等，帮助简化UI代码
 */
class UIHelper {
public:
    /**
     * @brief 显示信息对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息内容
     * @return 用户点击的按钮
     */
    static QMessageBox::StandardButton showInformation(QWidget* parent, const QString& title, const QString& message) {
        return QMessageBox::information(parent, title, message);
    }
    
    /**
     * @brief 显示警告对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息内容
     * @return 用户点击的按钮
     */
    static QMessageBox::StandardButton showWarning(QWidget* parent, const QString& title, const QString& message) {
        return QMessageBox::warning(parent, title, message);
    }
    
    /**
     * @brief 显示错误对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息内容
     * @return 用户点击的按钮
     */
    static QMessageBox::StandardButton showError(QWidget* parent, const QString& title, const QString& message) {
        return QMessageBox::critical(parent, title, message);
    }
    
    /**
     * @brief 显示确认对话框
     * @param parent 父窗口
     * @param title 标题
     * @param message 消息内容
     * @param defaultButton 默认按钮
     * @return 用户点击的按钮
     */
    static QMessageBox::StandardButton showConfirmation(QWidget* parent, const QString& title, const QString& message, 
                                                       QMessageBox::StandardButton defaultButton = QMessageBox::Yes) {
        return QMessageBox::question(parent, title, message, 
                                   QMessageBox::Yes | QMessageBox::No, defaultButton);
    }
    
    /**
     * @brief 在状态栏显示消息
     * @param statusBar 状态栏对象
     * @param message 消息内容
     * @param timeout 显示时间(毫秒)，0表示一直显示
     */
    static void showStatusMessage(QStatusBar* statusBar, const QString& message, int timeout = 0) {
        if (!statusBar) return;
        
        // 查找或创建状态标签
        QLabel* statusLabel = statusBar->findChild<QLabel*>("statusLabel");
        if (!statusLabel) {
            statusLabel = new QLabel(statusBar);
            statusLabel->setObjectName("statusLabel");
            statusLabel->setMargin(3);
            statusBar->addWidget(statusLabel);
        }
        
        // 设置消息
        statusLabel->setText(message);
        
        // 如果设置了超时，添加定时器恢复默认消息
        if (timeout > 0) {
            QTimer::singleShot(timeout, statusLabel, [statusLabel]() {
                statusLabel->setText("就绪");
            });
        }
    }
    
    /**
     * @brief 加载并应用样式表
     * @param widget 要应用样式的控件
     * @param styleFile 样式表文件路径
     * @return 是否成功加载样式
     */
    static bool loadStyleSheet(QWidget* widget, const QString& styleFile) {
        if (!widget) return false;
        
        QFile file(styleFile);
        if (file.open(QFile::ReadOnly)) {
            QString style = QLatin1String(file.readAll());
            widget->setStyleSheet(style);
            file.close();
            qDebug() << "已加载样式表:" << styleFile;
            return true;
        } else {
            qDebug() << "无法加载样式表文件:" << styleFile << " - " << file.errorString();
            return false;
        }
    }
    
    /**
     * @brief 为搜索框设置自动完成功能
     * @param lineEdit 输入框
     * @param suggestions 推荐项列表
     */
    static void setupSearchCompleter(QLineEdit* lineEdit, const QStringList& suggestions) {
        if (!lineEdit) return;
        
        // 创建自动完成器 - 修复参数顺序，第二个参数应该是父对象
        QCompleter* completer = new QCompleter(suggestions, lineEdit);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);  // 允许在任何位置匹配
        
        // 设置自动完成弹出框样式
        QAbstractItemView* popup = completer->popup();
        popup->setStyleSheet(
            "background-color: white;"
            "border: 1px solid #bdc3c7;"
            "border-radius: 4px;"
            "selection-background-color: #16a085;"
            "selection-color: white;"
            "padding: 2px;"
            "font-size: 14px;"
        );
        
        // 应用到搜索框
        lineEdit->setCompleter(completer);
        
        // 当搜索框文本改变时显示提示
        QObject::connect(lineEdit, &QLineEdit::textChanged, [completer](const QString &text) {
            if (text.isEmpty()) {
                completer->setModel(new QStringListModel(dynamic_cast<QStringListModel*>(completer->model())->stringList(), completer));
                completer->complete();
            }
        });
    }
    
    /**
     * @brief 在默认浏览器中打开URL
     * @param url 要打开的URL
     * @return 是否成功打开
     */
    static bool openUrl(const QString& url) {
        return QDesktopServices::openUrl(QUrl(url));
    }
    
    /**
     * @brief 打开文件所在文件夹并选中文件
     * @param filePath 文件路径
     * @return 是否成功打开
     */
    static bool openFileLocation(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        QString folderPath = fileInfo.absolutePath();
        
        #ifdef Q_OS_WIN
            // 在Windows上，可以使用explorer命令选中文件
            return QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
        #else
            // 在其他平台上，只能打开包含该文件的文件夹
            return QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
        #endif
    }
    
    /**
     * @brief 打开文件夹
     * @param folderPath 文件夹路径
     * @return 是否成功打开
     */
    static bool openFolder(const QString& folderPath) {
        return QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    }
    
private:
    // 私有构造函数，防止实例化
    UIHelper() = delete;
}; 