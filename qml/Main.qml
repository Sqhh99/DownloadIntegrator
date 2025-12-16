import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic
import QtQuick.Layouts

import "themes"
import "components"
import "pages"
import "layouts"

/**
 * Main.qml - 主窗口
 * 组合所有模块，作为应用程序入口
 */
ApplicationWindow {
    id: mainWindow
    
    width: 1200
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: qsTr("游戏修改器下载集成工具")
    
    color: ThemeProvider.backgroundColor
    
    // backend 由 main.cpp 通过 rootContext 注入
    // 使用 required property 确保绑定正确
    required property var backend
    
    Component.onCompleted: {
        console.log("Main.qml 加载完成")
        console.log("Backend 对象:", backend)
        console.log("ModifierListModel:", backend ? backend.modifierListModel : "null")
        if (backend && backend.modifierListModel) {
            console.log("模型行数:", backend.modifierListModel.rowCount())
        }
    }
    
    // 菜单栏
    menuBar: AppMenuBar {
        id: appMenuBar
        currentTheme: ThemeProvider.currentTheme
        currentLanguage: backend ? backend.currentLanguage : 0
        
        onThemeSelected: function(index) {
            ThemeProvider.currentTheme = index
            if (backend) backend.setTheme(index)
        }
        
        onLanguageSelected: function(index) {
            if (backend) backend.setLanguage(index)
        }
        
        onSettingsRequested: {
            if (backend) backend.openSettings()
        }
        
        onExitRequested: Qt.quit()
    }
    
    // 主内容区
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标签页
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            
            background: Rectangle {
                color: ThemeProvider.backgroundColor
                
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: ThemeProvider.borderColor
                }
            }
            
            TabButton {
                text: qsTr("搜索修改器")
                width: implicitWidth + 40
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: ThemeProvider.fontSizeMedium
                    font.bold: parent.checked
                    color: parent.checked ? ThemeProvider.primaryColor : ThemeProvider.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    color: parent.checked ? ThemeProvider.surfaceColor : "transparent"
                    
                    Rectangle {
                        visible: parent.parent.checked
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 2
                        color: ThemeProvider.primaryColor
                    }
                }
            }
            
            TabButton {
                text: qsTr("已下载修改器")
                width: implicitWidth + 40
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: ThemeProvider.fontSizeMedium
                    font.bold: parent.checked
                    color: parent.checked ? ThemeProvider.primaryColor : ThemeProvider.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    color: parent.checked ? ThemeProvider.surfaceColor : "transparent"
                    
                    Rectangle {
                        visible: parent.parent.checked
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 2
                        color: ThemeProvider.primaryColor
                    }
                }
            }
        }
        
        // 内容区
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            
            // 搜索页 - 带分割器
            SplitView {
                id: searchSplitView
                orientation: Qt.Horizontal
                
                SearchPage {
                    id: searchPage
                    SplitView.preferredWidth: parent.width * 0.6
                    SplitView.minimumWidth: 400
                    modifierModel: backend ? backend.modifierListModel : null
                    
                    onSearchRequested: function(keyword) {
                        if (backend) backend.searchModifiers(keyword)
                        statusBar.showMessage(qsTr("正在搜索: ") + keyword, 0)
                    }
                    
                    onModifierSelected: function(index) {
                        if (backend) backend.selectModifier(index)
                        detailPanel.visible = true
                    }
                    
                    onSortChanged: function(sortIndex) {
                        if (backend) backend.setSortOrder(sortIndex)
                    }
                    
                    onRefreshRequested: {
                        if (backend) backend.fetchRecentModifiers()
                        statusBar.showMessage(qsTr("正在刷新..."), 0)
                    }
                }
                
                ModifierDetailPanel {
                    id: detailPanel
                    SplitView.preferredWidth: parent.width * 0.4
                    SplitView.minimumWidth: 300
                    visible: false
                    
                    gameName: backend ? backend.selectedModifierName : ""
                    gameVersion: backend ? backend.selectedModifierVersion : ""
                    optionsCount: backend ? backend.selectedModifierOptionsCount : 0
                    lastUpdate: backend ? backend.selectedModifierLastUpdate : ""
                    optionsHtml: backend ? backend.selectedModifierOptions : ""
                    versions: backend ? backend.selectedModifierVersions : []
                    downloadProgress: backend ? backend.downloadProgress : 0
                    downloading: backend ? backend.isDownloading : false
                    coverUrl: backend ? backend.selectedModifierCoverPath : ""
                    
                    onDownloadRequested: function(versionIndex) {
                        if (backend) backend.downloadModifier(versionIndex)
                        statusBar.showMessage(qsTr("开始下载..."), 0)
                    }
                    
                    onOpenFolderRequested: {
                        if (backend) backend.openDownloadFolder()
                    }
                    
                    onSettingsRequested: {
                        if (backend) backend.openSettings()
                    }
                    
                    onVersionChanged: function(index) {
                        if (backend) backend.selectVersion(index)
                    }
                }
            }
            
            // 已下载页
            DownloadedPage {
                id: downloadedPage
                downloadedModel: backend ? backend.downloadedModifierModel : null
                
                onRunModifier: function(index) {
                    if (backend) backend.runModifier(index)
                }
                
                onDeleteModifier: function(index) {
                    if (backend) backend.deleteModifier(index)
                }
                
                onCheckUpdates: {
                    if (backend) backend.checkForUpdates()
                    statusBar.showMessage(qsTr("正在检查更新..."), 0)
                }
                
                onModifierDoubleClicked: function(index) {
                    if (backend) backend.runModifier(index)
                }
            }
        }
        
        // 状态栏
        AppStatusBar {
            id: statusBar
            Layout.fillWidth: true
        }
    }
    
    // 后端连接
    Connections {
        target: backend
        
        function onSearchCompleted() {
            statusBar.showMessage(qsTr("搜索完成"), 3000)
        }
        
        function onDownloadCompleted(success) {
            statusBar.showMessage(success ? qsTr("下载完成") : qsTr("下载失败"), 5000)
        }
        
        function onStatusMessage(message) {
            statusBar.showMessage(message, 0)
        }
    }
}
