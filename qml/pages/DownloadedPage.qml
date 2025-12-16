import QtQuick
import QtQuick.Layouts
import "../components"
import "../themes"

/**
 * DownloadedPage - 已下载修改器标签页
 */
Item {
    id: downloadedPage
    
    property alias downloadedModel: downloadedTable.model
    
    signal runModifier(int index)
    signal deleteModifier(int index)
    signal checkUpdates()
    signal modifierDoubleClicked(int index)
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeProvider.spacingMedium
        spacing: ThemeProvider.spacingMedium
        
        // 标题
        Text {
            text: qsTr("已下载的游戏修改器")
            font.pixelSize: ThemeProvider.fontSizeTitle
            font.bold: true
            color: ThemeProvider.textPrimary
        }
        
        // 已下载表格
        StyledTable {
            id: downloadedTable
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            headers: [qsTr("修改器名称"), qsTr("版本"), qsTr("游戏版本"), qsTr("下载日期")]
            columnWidths: [300, 150, 200, 200]
            
            delegate: Rectangle {
                width: downloadedTable.width
                height: downloadedTable.rowHeight
                color: {
                    if (downloadedTable.currentIndex === index)
                        return ThemeProvider.selectedColor
                    if (rowMouseArea.containsMouse)
                        return ThemeProvider.hoverColor
                    if (index % 2 === 1)
                        return ThemeProvider.alternateRowColor
                    return "transparent"
                }
                
                Row {
                    anchors.fill: parent
                    
                    Text {
                        width: downloadedTable.columnWidths[0]
                        height: parent.height
                        leftPadding: 10
                        text: model.name || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textPrimary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    
                    Text {
                        width: downloadedTable.columnWidths[1]
                        height: parent.height
                        leftPadding: 10
                        text: model.version || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    Text {
                        width: downloadedTable.columnWidths[2]
                        height: parent.height
                        leftPadding: 10
                        text: model.gameVersion || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    Text {
                        width: downloadedTable.columnWidths[3]
                        height: parent.height
                        leftPadding: 10
                        text: model.downloadDate || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                MouseArea {
                    id: rowMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: downloadedTable.currentIndex = index
                    onDoubleClicked: modifierDoubleClicked(index)
                }
            }
        }
        
        // 操作按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: ThemeProvider.spacingMedium
            
            StyledButton {
                text: qsTr("运行")
                buttonType: "success"
                enabled: downloadedTable.currentIndex >= 0
                onClicked: runModifier(downloadedTable.currentIndex)
            }
            
            StyledButton {
                text: qsTr("删除")
                buttonType: "danger"
                enabled: downloadedTable.currentIndex >= 0
                onClicked: deleteModifier(downloadedTable.currentIndex)
            }
            
            StyledButton {
                text: qsTr("检查更新")
                buttonType: "info"
                onClicked: checkUpdates()
            }
            
            Item { Layout.fillWidth: true }
        }
    }
}
