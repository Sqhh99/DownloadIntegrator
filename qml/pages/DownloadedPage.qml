import QtQuick
import QtQuick.Layouts
import "../components"
import "../themes"

/**
 * DownloadedPage - 已下载修改器标签页
 * 列表每行右侧有打开文件夹和删除按钮
 */
Item {
    id: downloadedPage
    
    property alias downloadedModel: downloadedTable.model
    
    signal openFolderRequested(int index)
    signal deleteModifier(int index)
    signal checkUpdates()
    signal modifierDoubleClicked(int index)
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeProvider.spacingMedium
        spacing: ThemeProvider.spacingMedium
        
        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: qsTr("已下载的游戏修改器")
                font.pixelSize: ThemeProvider.fontSizeTitle
                font.bold: true
                color: ThemeProvider.textPrimary
                Layout.fillWidth: true
            }
            
            StyledButton {
                text: qsTr("检查更新")
                buttonType: "info"
                onClicked: checkUpdates()
            }
        }
        
        // 已下载表格
        StyledTable {
            id: downloadedTable
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            headers: [qsTr("修改器名称"), qsTr("版本"), qsTr("游戏版本"), qsTr("下载日期"), qsTr("操作")]
            columnWidths: [250, 120, 150, 150, 100]
            
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
                
                RowLayout {
                    anchors.fill: parent
                    spacing: 0
                    
                    // 修改器名称
                    Text {
                        Layout.preferredWidth: downloadedTable.columnWidths[0]
                        Layout.fillHeight: true
                        leftPadding: 10
                        text: model.name || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textPrimary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    
                    // 版本
                    Text {
                        Layout.preferredWidth: downloadedTable.columnWidths[1]
                        Layout.fillHeight: true
                        leftPadding: 10
                        text: model.version || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    // 游戏版本
                    Text {
                        Layout.preferredWidth: downloadedTable.columnWidths[2]
                        Layout.fillHeight: true
                        leftPadding: 10
                        text: model.gameVersion || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    // 下载日期
                    Text {
                        Layout.preferredWidth: downloadedTable.columnWidths[3]
                        Layout.fillHeight: true
                        leftPadding: 10
                        text: model.downloadDate || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    // 操作按钮
                    Item {
                        Layout.preferredWidth: downloadedTable.columnWidths[4]
                        Layout.fillHeight: true
                        
                        Row {
                            anchors.centerIn: parent
                            spacing: 8
                            
                            // 打开文件夹按钮
                            IconButton {
                                iconSource: "qrc:/icons/folder.png"
                                iconSize: 18
                                tooltip: qsTr("打开文件夹")
                                onClicked: openFolderRequested(index)
                            }
                            
                            // 删除按钮
                            IconButton {
                                iconSource: "qrc:/icons/delete.png"
                                iconSize: 18
                                tooltip: qsTr("删除")
                                onClicked: deleteModifier(index)
                            }
                        }
                    }
                }
                
                MouseArea {
                    id: rowMouseArea
                    anchors.fill: parent
                    anchors.rightMargin: downloadedTable.columnWidths[4]  // 排除操作按钮区域
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: downloadedTable.currentIndex = index
                    onDoubleClicked: modifierDoubleClicked(index)
                }
            }
        }
    }
}
