import QtQuick
import QtQuick.Layouts
import "../components"
import "../themes"

/**
 * SearchPage - 搜索标签页
 * 包含搜索栏、排序选项和修改器列表
 */
Item {
    id: searchPage
    
    // 后端接口和模型 - 使用直接属性而不是alias避免绑定链断裂
    property var modifierModel: null
    
    // 信号
    signal modifierSelected(int index)
    signal searchRequested(string keyword)
    signal sortChanged(int sortIndex)
    signal refreshRequested()
    
    // 监控模型变化
    Connections {
        target: modifierModel
        function onCountChanged() {
            console.log("SearchPage: 模型数据变化，当前行数:", modifierModel ? modifierModel.rowCount() : 0)
        }
    }
    
    // 当模型变化时打印调试信息
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
                onClicked: searchRequested(searchInput.text)
            }
            
            StyledComboBox {
                id: sortComboBox
                implicitWidth: 120
                model: [qsTr("最近更新"), qsTr("按名称"), qsTr("下载次数")]
                onCurrentIndexChanged: sortChanged(currentIndex)
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
            
            // 绑定模型到外部传入的 modifierModel
            model: searchPage.modifierModel
            
            headers: [qsTr("游戏名称"), qsTr("更新日期"), qsTr("支持版本"), qsTr("选项数量")]
            columnWidths: [250, 100, 150, 80]
            
            delegate: Rectangle {
                width: modifierTable.width
                height: modifierTable.rowHeight
                color: {
                    if (modifierTable.currentIndex === index) 
                        return ThemeProvider.selectedColor
                    if (itemMouseArea.containsMouse)
                        return ThemeProvider.hoverColor
                    if (index % 2 === 1)
                        return ThemeProvider.alternateRowColor
                    return "transparent"
                }
                
                Row {
                    anchors.fill: parent
                    
                    // 游戏名称
                    Text {
                        width: modifierTable.columnWidths[0]
                        height: parent.height
                        leftPadding: 10
                        text: model.name || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textPrimary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    
                    // 更新日期
                    Text {
                        width: modifierTable.columnWidths[1]
                        height: parent.height
                        leftPadding: 10
                        text: model.lastUpdate || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    
                    // 支持版本
                    Text {
                        width: modifierTable.columnWidths[2]
                        height: parent.height
                        leftPadding: 10
                        text: model.gameVersion || ""
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    
                    // 选项数量
                    Text {
                        width: modifierTable.columnWidths[3]
                        height: parent.height
                        leftPadding: 10
                        text: model.optionsCount || "0"
                        font.pixelSize: ThemeProvider.fontSizeMedium
                        color: ThemeProvider.textSecondary
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                
                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    
                    onClicked: {
                        modifierTable.currentIndex = index
                        modifierSelected(index)
                    }
                }
            }
        }
    }
}
