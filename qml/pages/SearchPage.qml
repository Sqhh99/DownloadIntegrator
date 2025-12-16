import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../components"
import "../themes"

/**
 * SearchPage - 搜索标签页
 * 包含搜索栏、排序选项和修改器列表（带详情按钮）
 * 下载功能已移至详情面板
 */
Item {
    id: searchPage
    
    // 后端接口和模型
    property var modifierModel: null
    
    // 信号
    signal modifierSelected(int index)
    signal searchRequested(string keyword)
    signal sortChanged(int sortIndex)
    signal refreshRequested()
    signal detailsRequested(int index)
    
    // 设置选中的行
    function selectRow(index) {
        modifierTable.currentIndex = index
    }
    
    // 监控模型变化
    Connections {
        target: modifierModel
        function onCountChanged() {
            console.log("SearchPage: 模型数据变化，当前行数:", modifierModel ? modifierModel.rowCount() : 0)
        }
    }
    
    onModifierModelChanged: {
        console.log("SearchPage: modifierModel 属性变化:", modifierModel)
        if (modifierModel) {
            console.log("SearchPage: 模型行数:", modifierModel.rowCount())
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeProvider.spacingMedium
        spacing: ThemeProvider.spacingMedium
        
        // 搜索栏
        RowLayout {
            Layout.fillWidth: true
            spacing: ThemeProvider.spacingMedium
            
            StyledTextField {
                id: searchInput
                Layout.fillWidth: true
                placeholderText: qsTr("搜索游戏...")
                showClearButton: true
                
                Keys.onReturnPressed: searchRequested(text)
                Keys.onEnterPressed: searchRequested(text)
            }
            
            StyledButton {
                text: qsTr("搜索")
                buttonType: "secondary"
                onClicked: {
                    if (searchInput.text.trim() === "") {
                        refreshRequested()
                    } else {
                        searchRequested(searchInput.text)
                    }
                }
            }
            
            StyledComboBox {
                id: sortComboBox
                implicitWidth: 120
                model: [qsTr("最近更新"), qsTr("按名称"), qsTr("选项数量")]
                onActivated: function(index) {
                    console.log("排序方式改变:", index)
                    sortChanged(index)
                }
            }
            
            StyledButton {
                text: qsTr("显示全部")
                buttonType: "primary"
                onClicked: {
                    searchInput.clear()
                    refreshRequested()
                }
            }
        }
        
        // 修改器表格
        StyledTable {
            id: modifierTable
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            model: searchPage.modifierModel
            
            headers: [qsTr("游戏名称"), qsTr("更新日期"), qsTr("支持版本"), qsTr("选项数量"), qsTr("操作")]
            columnWidths: [240, 100, 130, 80, 80]
            
            delegate: Rectangle {
                id: delegateRoot
                width: modifierTable.width
                height: modifierTable.rowHeight
                
                // 存储当前行索引
                property int rowIndex: index
                
                color: {
                    if (modifierTable.currentIndex === rowIndex) 
                        return ThemeProvider.selectedColor
                    if (rowMouseArea.containsMouse)
                        return ThemeProvider.hoverColor
                    if (rowIndex % 2 === 1)
                        return ThemeProvider.alternateRowColor
                    return "transparent"
                }
                
                // 行选择区域（不包括操作按钮列）
                MouseArea {
                    id: rowMouseArea
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width - 80  // 减去操作列宽度
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        modifierTable.currentIndex = delegateRoot.rowIndex
                        searchPage.modifierSelected(delegateRoot.rowIndex)
                    }
                    
                    onDoubleClicked: {
                        // 双击打开详情
                        searchPage.detailsRequested(delegateRoot.rowIndex)
                    }
                }
                
                // 数据行
                Row {
                    anchors.fill: parent
                    
                    // 游戏名称
                    Item {
                        width: modifierTable.columnWidths[0]
                        height: parent.height
                        
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            text: model.name || ""
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textPrimary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                    
                    // 更新日期
                    Item {
                        width: modifierTable.columnWidths[1]
                        height: parent.height
                        
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            text: model.lastUpdate || ""
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                    
                    // 支持版本
                    Item {
                        width: modifierTable.columnWidths[2]
                        height: parent.height
                        
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            text: model.gameVersion || ""
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                    
                    // 选项数量
                    Item {
                        width: modifierTable.columnWidths[3]
                        height: parent.height
                        
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            text: model.optionsCount || "0"
                            font.pixelSize: ThemeProvider.fontSizeMedium
                            color: ThemeProvider.textSecondary
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                    
                    // 操作按钮区 - 只有详情图标按钮
                    Item {
                        width: modifierTable.columnWidths[4]
                        height: parent.height
                        
                        // 详情按钮 - 仅图标
                        Rectangle {
                            anchors.centerIn: parent
                            width: 28
                            height: 28
                            radius: ThemeProvider.radiusSmall
                            color: detailsMouseArea.containsMouse ? ThemeProvider.hoverColor : "transparent"
                            
                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/details.png"
                                width: 18
                                height: 18
                                sourceSize: Qt.size(18, 18)
                            }
                            
                            MouseArea {
                                id: detailsMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                
                                onClicked: {
                                    console.log("详情按钮点击, rowIndex:", delegateRoot.rowIndex)
                                    searchPage.detailsRequested(delegateRoot.rowIndex)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
