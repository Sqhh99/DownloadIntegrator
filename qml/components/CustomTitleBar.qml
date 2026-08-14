import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import FLiNGDownloader

/**
 * CustomTitleBar - 自定义无边框窗口标题栏
 * 包含应用标题和窗口控制按钮
 */
Rectangle {
    id: titleBar
    
    property string title: ""
    property var targetWindow: null
    property bool maximized: targetWindow ? (targetWindow.visibility === Window.Maximized) : false
    property int activeDownloads: 0  // 活动下载数
    
    signal minimizeClicked()
    signal maximizeClicked()
    signal closeClicked()
    signal settingsClicked()
    signal downloadClicked()  // 下载列表按钮
    
    height: 40
    color: ThemeProvider.surfaceColor
    
    // 底部边框
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: ThemeProvider.borderColor
    }
    
    // 拖动区域
    MouseArea {
        id: dragArea
        anchors.fill: parent
        anchors.rightMargin: windowControls.width
        
        property point clickPos: Qt.point(0, 0)
        
        onPressed: function(mouse) {
            clickPos = Qt.point(mouse.x, mouse.y)
        }
        
        onPositionChanged: function(mouse) {
            if (pressed && targetWindow) {
                var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
                if (maximized) {
                    // 从最大化状态拖动时还原窗口
                    targetWindow.showNormal()
                }
                targetWindow.x += delta.x
                targetWindow.y += delta.y
            }
        }
        
        onDoubleClicked: {
            if (maximized) {
                targetWindow.showNormal()
            } else {
                targetWindow.showMaximized()
            }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 5
        spacing: 0
        
        // 应用图标
        Image {
            source: ThemeProvider.assetUrl("icons/app_icon.png")
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            sourceSize: Qt.size(24, 24)
            fillMode: Image.PreserveAspectFit
        }
        
        // 标题
        Text {
            text: titleBar.title
            font.pixelSize: ThemeProvider.fontSizeMedium
            font.bold: true
            color: ThemeProvider.textPrimary
            Layout.fillWidth: true
            Layout.leftMargin: 10
            elide: Text.ElideRight
        }
        
        // 下载按钮（带徽章）
        Item {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            
            IconButton {
                anchors.centerIn: parent
                iconSource: ThemeProvider.assetUrl("icons/download.png")
                tooltip: qsTr("下载列表")
                iconSize: 16
                onClicked: downloadClicked()
            }
            
            // 活动下载数徽章
            Rectangle {
                visible: activeDownloads > 0
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 2
                anchors.rightMargin: 2
                width: 16
                height: 16
                radius: 8
                color: ThemeProvider.dangerColor
                
                Text {
                    anchors.centerIn: parent
                    text: activeDownloads > 9 ? "9+" : activeDownloads.toString()
                    font.pixelSize: 10
                    font.bold: true
                    color: "white"
                }
            }
        }
        
        // 设置按钮
        IconButton {
            iconSource: ThemeProvider.assetUrl("icons/settings.png")
            tooltip: qsTr("设置")
            iconSize: 16
            onClicked: settingsClicked()
        }
        
        // 窗口控制按钮：与下载/设置相同的圆角小矩形
        Row {
            id: windowControls
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 4
            spacing: 4

            IconButton {
                iconSource: ThemeProvider.assetUrl("icons/minimize.png")
                tooltip: qsTr("最小化")
                iconSize: 16
                onClicked: {
                    if (targetWindow) targetWindow.showMinimized()
                    minimizeClicked()
                }
            }

            IconButton {
                iconSource: maximized
                    ? ThemeProvider.assetUrl("icons/maximize_restoration.png")
                    : ThemeProvider.assetUrl("icons/maximize.png")
                tooltip: maximized ? qsTr("还原") : qsTr("最大化")
                iconSize: 16
                onClicked: {
                    if (targetWindow) {
                        if (maximized) {
                            targetWindow.showNormal()
                        } else {
                            targetWindow.showMaximized()
                        }
                    }
                    maximizeClicked()
                }
            }

            IconButton {
                iconSource: ThemeProvider.assetUrl("icons/exit.png")
                tooltip: qsTr("关闭")
                iconSize: 16
                hoverColor: ThemeProvider.isDark ? "#8D454C" : "#FFCDD2"
                pressedColor: ThemeProvider.isDark ? "#A3525A" : "#EF9A9A"
                onClicked: {
                    if (targetWindow) targetWindow.close()
                    closeClicked()
                }
            }
        }
    }
}
