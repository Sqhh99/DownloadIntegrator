import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../themes"

/**
 * SettingsDialog - 设置对话框
 * 侧边栏导航布局，设置主题、语言、下载路径
 */
Dialog {
    id: settingsDialog
    
    property int currentTheme: 0
    property int currentLanguage: 0
    property string downloadPath: ""
    
    signal themeChanged(int index)
    signal languageChangedSignal(int index)  // 重命名避免冲突
    signal browseDownloadPath()
    signal settingsApplied()
    
    title: qsTr("设置")
    modal: true
    anchors.centerIn: parent
    width: 600
    height: 500
    
    // 自定义背景
    background: Rectangle {
        color: ThemeProvider.surfaceColor
        radius: ThemeProvider.radiusMedium
        border.color: ThemeProvider.borderColor
    }
    
    // 自定义头部
    header: Rectangle {
        height: 50
        color: ThemeProvider.surfaceColor
        radius: ThemeProvider.radiusMedium
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: ThemeProvider.spacingMedium
            anchors.rightMargin: ThemeProvider.spacingMedium
            
            Text {
                text: qsTr("设置")
                font.pixelSize: ThemeProvider.fontSizeTitle
                font.bold: true
                color: ThemeProvider.textPrimary
                Layout.fillWidth: true
            }
            
            // 关闭按钮
            Rectangle {
                width: 30
                height: 30
                radius: ThemeProvider.radiusSmall
                color: closeArea.containsMouse ? ThemeProvider.dangerColor : "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 16
                    color: closeArea.containsMouse ? "#fff" : ThemeProvider.textSecondary
                }
                
                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: settingsDialog.close()
                }
            }
        }
        
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: ThemeProvider.borderColor
        }
    }
    
    contentItem: RowLayout {
        spacing: 0
        
        // 侧边栏
        Rectangle {
            Layout.preferredWidth: 140
            Layout.fillHeight: true
            color: ThemeProvider.backgroundColor
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ThemeProvider.spacingSmall
                spacing: 4
                
                // 外观设置
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: ThemeProvider.radiusSmall
                    color: settingsStack.currentIndex === 0 ? ThemeProvider.selectedColor : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        spacing: 8
                        
                        Image {
                            source: "qrc:/icons/settings.png"
                            width: 18
                            height: 18
                            sourceSize: Qt.size(18, 18)
                        }
                        
                        Text {
                            text: qsTr("外观")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsStack.currentIndex = 0
                    }
                }
                
                // 下载设置
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: ThemeProvider.radiusSmall
                    color: settingsStack.currentIndex === 1 ? ThemeProvider.selectedColor : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        spacing: 8
                        
                        Image {
                            source: "qrc:/icons/download.png"
                            width: 18
                            height: 18
                            sourceSize: Qt.size(18, 18)
                        }
                        
                        Text {
                            text: qsTr("下载")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsStack.currentIndex = 1
                    }
                }
                
                // 语言设置
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: ThemeProvider.radiusSmall
                    color: settingsStack.currentIndex === 2 ? ThemeProvider.selectedColor : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        spacing: 8
                        
                        Image {
                            source: "qrc:/icons/language.png"
                            width: 18
                            height: 18
                            sourceSize: Qt.size(18, 18)
                        }
                        
                        Text {
                            text: qsTr("语言")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsStack.currentIndex = 2
                    }
                }
                
                // 关于
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: ThemeProvider.radiusSmall
                    color: settingsStack.currentIndex === 3 ? ThemeProvider.selectedColor : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        spacing: 8
                        
                        Image {
                            source: "qrc:/icons/about.png"
                            width: 18
                            height: 18
                            sourceSize: Qt.size(18, 18)
                        }
                        
                        Text {
                            text: qsTr("关于")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: settingsStack.currentIndex = 3
                    }
                }
                
                Item { Layout.fillHeight: true }
            }
        }
        
        // 内容区
        StackLayout {
            id: settingsStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0
            
            // 外观设置页
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeProvider.spacingMedium
                    spacing: ThemeProvider.spacingMedium
                    
                    Text {
                        text: qsTr("外观设置")
                        font.pixelSize: ThemeProvider.fontSizeTitle
                        font.bold: true
                        color: ThemeProvider.textPrimary
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: ThemeProvider.borderColor
                    }
                    
                    // 主题选择
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ThemeProvider.spacingSmall
                        
                        Text {
                            text: qsTr("主题")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                        
                        RowLayout {
                            spacing: ThemeProvider.spacingMedium
                            
                            Repeater {
                                model: [
                                    {name: qsTr("Dark"), color: "#1e1e1e"},
                                    {name: qsTr("Light"), color: "#ffffff"},
                                    {name: qsTr("经典"), color: "#2b2b2b"},
                                    {name: qsTr("多彩"), color: "#1a1a2e"}
                                ]
                                
                                delegate: Rectangle {
                                    width: 80
                                    height: 60
                                    radius: ThemeProvider.radiusSmall
                                    color: modelData.color
                                    border.width: currentTheme === index ? 2 : 1
                                    border.color: currentTheme === index ? ThemeProvider.primaryColor : ThemeProvider.borderColor
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.name
                                        font.pixelSize: ThemeProvider.fontSizeSmall
                                        color: index === 1 ? "#333" : "#fff"
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            currentTheme = index
                                            themeChanged(index)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
            
            // 下载设置页
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeProvider.spacingMedium
                    spacing: ThemeProvider.spacingMedium
                    
                    Text {
                        text: qsTr("下载设置")
                        font.pixelSize: ThemeProvider.fontSizeTitle
                        font.bold: true
                        color: ThemeProvider.textPrimary
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: ThemeProvider.borderColor
                    }
                    
                    // 下载目录设置
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ThemeProvider.spacingSmall
                        
                        Text {
                            text: qsTr("下载目录")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: ThemeProvider.spacingSmall
                            
                            Rectangle {
                                Layout.fillWidth: true
                                height: 36
                                radius: ThemeProvider.radiusSmall
                                color: ThemeProvider.inputBackground
                                border.color: ThemeProvider.borderColor
                                
                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    text: settingsDialog.downloadPath || qsTr("未设置")
                                    font.pixelSize: ThemeProvider.fontSizeMedium
                                    color: ThemeProvider.textPrimary
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideMiddle
                                }
                            }
                            
                            StyledButton {
                                text: qsTr("浏览...")
                                buttonType: "secondary"
                                onClicked: {
                                    browseDownloadPath()
                                }
                            }
                        }
                        
                        Text {
                            text: qsTr("修改器将下载到此目录")
                            font.pixelSize: ThemeProvider.fontSizeSmall
                            color: ThemeProvider.textDisabled
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
            
            // 语言设置页
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeProvider.spacingMedium
                    spacing: ThemeProvider.spacingMedium
                    
                    Text {
                        text: qsTr("语言设置")
                        font.pixelSize: ThemeProvider.fontSizeTitle
                        font.bold: true
                        color: ThemeProvider.textPrimary
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: ThemeProvider.borderColor
                    }
                    
                    // 语言选择
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ThemeProvider.spacingSmall
                        
                        Text {
                            text: qsTr("界面语言")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                        
                        ComboBox {
                            id: languageComboBox
                            Layout.preferredWidth: 200
                            model: [qsTr("简体中文"), qsTr("English"), qsTr("日本語")]
                            currentIndex: settingsDialog.currentLanguage
                            
                            onActivated: function(index) {
                                console.log("语言切换:", index)
                                settingsDialog.currentLanguage = index
                                settingsDialog.languageChangedSignal(index)
                            }
                            
                            background: Rectangle {
                                color: ThemeProvider.inputBackground
                                border.color: parent.hovered ? ThemeProvider.primaryColor : ThemeProvider.borderColor
                                radius: ThemeProvider.radiusSmall
                            }
                            
                            contentItem: Text {
                                leftPadding: 10
                                text: languageComboBox.displayText
                                font.pixelSize: ThemeProvider.fontSizeMedium
                                color: ThemeProvider.textPrimary
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        Text {
                            text: qsTr("切换语言后需要重启应用才能完全生效")
                            font.pixelSize: ThemeProvider.fontSizeSmall
                            color: ThemeProvider.textDisabled
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
            
            // 关于页
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeProvider.spacingMedium
                    spacing: ThemeProvider.spacingMedium
                    
                    Text {
                        text: qsTr("关于")
                        font.pixelSize: ThemeProvider.fontSizeTitle
                        font.bold: true
                        color: ThemeProvider.textPrimary
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: ThemeProvider.borderColor
                    }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ThemeProvider.spacingSmall
                        
                        Text {
                            text: qsTr("游戏修改器下载集成工具")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                        
                        Text {
                            text: qsTr("版本: 1.0.0")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                        
                        Text {
                            text: qsTr("作者: Sqhh99")
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
    
    footer: Rectangle {
        height: 50
        color: ThemeProvider.surfaceColor
        radius: ThemeProvider.radiusMedium
        
        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: ThemeProvider.borderColor
        }
    }
}
