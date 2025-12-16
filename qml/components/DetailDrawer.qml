import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../themes"

/**
 * DetailDrawer - 右侧弹出式详情面板
 * 从右侧滑入显示修改器详细信息
 */
Drawer {
    id: detailDrawer
    
    // 属性
    property string gameName: ""
    property string gameVersion: ""
    property int optionsCount: 0
    property string lastUpdate: ""
    property string optionsHtml: ""
    property var versions: []
    property int selectedVersionIndex: 0
    property string coverUrl: ""
    
    // 信号
    signal versionChanged(int index)
    signal closed()
    
    edge: Qt.RightEdge
    width: Math.min(400, parent.width * 0.4)
    height: parent.height
    modal: false
    interactive: true
    
    background: Rectangle {
        color: ThemeProvider.surfaceColor
        
        // 左侧边框
        Rectangle {
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: ThemeProvider.borderColor
        }
    }
    
    onClosed: detailDrawer.closed()
    
    // 内容
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeProvider.spacingMedium
        spacing: ThemeProvider.spacingMedium
        
        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: gameName
                font.pixelSize: ThemeProvider.fontSizeTitle
                font.bold: true
                color: ThemeProvider.textPrimary
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            // 关闭按钮
            IconButton {
                iconSource: "qrc:/icons/exit.png"
                iconSize: 14
                tooltip: qsTr("关闭")
                onClicked: detailDrawer.close()
            }
        }
        
        // 分割线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: ThemeProvider.borderColor
        }
        
        // 封面和信息区
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            spacing: ThemeProvider.spacingMedium
            
            // 游戏封面
            Rectangle {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 140
                radius: ThemeProvider.radiusSmall
                color: ThemeProvider.backgroundColor
                border.width: 1
                border.color: ThemeProvider.borderColor
                
                Image {
                    id: coverImage
                    anchors.fill: parent
                    anchors.margins: 2
                    source: coverUrl
                    fillMode: Image.PreserveAspectFit
                    
                    Text {
                        visible: coverImage.status === Image.Null || coverImage.status === Image.Error || coverUrl.length === 0
                        anchors.centerIn: parent
                        text: qsTr("暂无封面")
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textDisabled
                    }
                }
            }
            
            // 信息列表
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: ThemeProvider.spacingSmall
                
                // 游戏版本
                RowLayout {
                    spacing: 8
                    Text {
                        text: qsTr("游戏版本:")
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textSecondary
                    }
                    Text {
                        text: gameVersion || "-"
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textPrimary
                    }
                }
                
                // 选项数量
                RowLayout {
                    spacing: 8
                    Text {
                        text: qsTr("选项数量:")
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textSecondary
                    }
                    Text {
                        text: optionsCount > 0 ? optionsCount.toString() : "-"
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textPrimary
                    }
                }
                
                // 最后更新
                RowLayout {
                    spacing: 8
                    Text {
                        text: qsTr("最后更新:")
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textSecondary
                    }
                    Text {
                        text: lastUpdate || "-"
                        font.pixelSize: ThemeProvider.fontSizeSmall
                        color: ThemeProvider.textPrimary
                    }
                }
                
                Item { Layout.fillHeight: true }
            }
        }
        
        // 版本选择
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("可用版本")
            
            background: Rectangle {
                y: parent.topPadding - parent.padding
                width: parent.width
                height: parent.height - parent.topPadding + parent.padding
                color: "transparent"
                border.color: ThemeProvider.borderColor
                radius: ThemeProvider.radiusSmall
            }
            
            label: Text {
                x: parent.leftPadding
                text: parent.title
                font.pixelSize: ThemeProvider.fontSizeSmall
                color: ThemeProvider.textSecondary
            }
            
            ComboBox {
                width: parent.width
                model: versions.map(function(v) { return v.name || v; })
                currentIndex: selectedVersionIndex
                onCurrentIndexChanged: versionChanged(currentIndex)
                
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
                    elide: Text.ElideRight
                }
            }
        }
        
        // 选项列表（可滚动）
        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("修改器选项")
            
            background: Rectangle {
                y: parent.topPadding - parent.padding
                width: parent.width
                height: parent.height - parent.topPadding + parent.padding
                color: "transparent"
                border.color: ThemeProvider.borderColor
                radius: ThemeProvider.radiusSmall
            }
            
            label: Text {
                x: parent.leftPadding
                text: parent.title
                font.pixelSize: ThemeProvider.fontSizeSmall
                color: ThemeProvider.textSecondary
            }
            
            ScrollView {
                anchors.fill: parent
                clip: true
                
                TextArea {
                    readOnly: true
                    text: optionsHtml
                    font.pixelSize: ThemeProvider.fontSizeSmall
                    color: ThemeProvider.textPrimary
                    wrapMode: Text.WordWrap
                    
                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }
}
