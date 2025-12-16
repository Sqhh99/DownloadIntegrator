import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../themes"

/**
 * SettingsDialog - 设置对话框
 * 侧边栏导航布局，设置主题和语言
 */
Dialog {
    id: settingsDialog
    
    property int currentTheme: 0
    property int currentLanguage: 0
    property string downloadPath: ""
    
    signal themeChanged(int index)
    signal languageChanged(int index)
    signal browseDownloadPath()  // 打开目录选择对话框
    signal settingsApplied()
    
    title: qsTr("设置")
    modal: true
    anchors.centerIn: parent
    width: 600
    height: 450
    
    // 自定义背景
    background: Rectangle {
        color: ThemeProvider.surfaceColor
        radius: ThemeProvider.radiusMedium
        border.color: ThemeProvider.borderColor
        border.width: 1
    }
    
    // 自定义标题
    header: Rectangle {
        height: 50
        color: "transparent"
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: ThemeProvider.spacingMedium
            anchors.rightMargin: ThemeProvider.spacingSmall
            
            Text {
                text: qsTr("设置")
                font.pixelSize: ThemeProvider.fontSizeTitle
                font.bold: true
                color: ThemeProvider.textPrimary
                Layout.fillWidth: true
            }
            
            IconButton {
                iconSource: "qrc:/icons/exit.png"
                iconSize: 16
                tooltip: qsTr("关闭")
                onClicked: settingsDialog.close()
            }
        }
        
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: ThemeProvider.borderColor
        }
    }
    
    // 主内容
    contentItem: RowLayout {
        spacing: 0
        
        // 侧边栏导航
        Rectangle {
            Layout.preferredWidth: 150
            Layout.fillHeight: true
            color: ThemeProvider.backgroundColor
            
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: ThemeProvider.borderColor
            }
            
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
                
                // 语言设置
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
                        onClicked: settingsStack.currentIndex = 1
                    }
                }
                
                // 关于
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
                        onClicked: settingsStack.currentIndex = 2
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
                    
                    // 下载目录设置
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: ThemeProvider.spacingMedium
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
                                    id: downloadPathText
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    text: settingsDialog.downloadPath || qsTr("默认位置")
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
                            Layout.preferredWidth: 200
                            model: [qsTr("简体中文"), qsTr("English"), qsTr("日本語")]
                            currentIndex: currentLanguage
                            
                            onCurrentIndexChanged: {
                                currentLanguage = currentIndex
                                languageChanged(currentIndex)
                            }
                            
                            background: Rectangle {
                                color: ThemeProvider.inputBackground
                                border.color: parent.hovered ? ThemeProvider.primaryColor : ThemeProvider.borderColor
                                radius: ThemeProvider.radiusSmall
                            }
                            
                            contentItem: Text {
                                leftPadding: 10
                                text: parent.displayText
                                font.pixelSize: ThemeProvider.fontSizeMedium
                                color: ThemeProvider.textPrimary
                                verticalAlignment: Text.AlignVCenter
                            }
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
                    
                    Image {
                        source: "qrc:/icons/app_icon.png"
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        sourceSize: Qt.size(64, 64)
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: qsTr("游戏修改器下载集成工具")
                        font.pixelSize: ThemeProvider.fontSizeTitle
                        font.bold: true
                        color: ThemeProvider.textPrimary
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: qsTr("版本 1.0.0")
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: qsTr("一个用于下载和管理游戏修改器的工具")
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
    
    // 没有底部按钮
    footer: Item { height: 0 }
}
