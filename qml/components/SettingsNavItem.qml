import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import FLiNGDownloader

/**
 * SettingsNavItem - 设置对话框左侧导航项
 *
 * 原本是四份几乎相同的 Rectangle + MouseArea，键盘用户既无法 Tab 到
 * 这些导航项，读屏软件也读不出它们是什么。用 AbstractButton 统一实现，
 * 顺带去掉四份重复。
 */
AbstractButton {
    id: navItem

    property string iconSource: ""
    property bool selected: false

    implicitHeight: 40
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    Accessible.role: Accessible.Button
    Accessible.name: text

    background: Rectangle {
        radius: ThemeProvider.radiusSmall
        color: {
            if (navItem.selected) return ThemeProvider.selectedColor
            if (navItem.hovered) return ThemeProvider.hoverColor
            return "transparent"
        }
        border.width: navItem.visualFocus ? 2 : 0
        border.color: ThemeProvider.primaryColor
    }

    contentItem: RowLayout {
        spacing: 8

        Image {
            source: navItem.iconSource
            Layout.leftMargin: 10
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            sourceSize: Qt.size(18, 18)
        }

        Text {
            text: navItem.text
            font.pixelSize: ThemeProvider.fontSizeMedium
            color: ThemeProvider.textPrimary
            Layout.fillWidth: true
            verticalAlignment: Text.AlignVCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: Qt.PointingHandCursor
    }
}
