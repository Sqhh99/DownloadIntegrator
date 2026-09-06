import QtQuick
import QtQuick.Controls.Basic
import FLiNGDownloader

/**
 * IconButton - 可复用的图标按钮组件
 * 用于列表项操作和标题栏按钮
 *
 * 基于 AbstractButton 而不是 Rectangle + MouseArea：后者只认鼠标，
 * 键盘用户既无法 Tab 到这些操作，也没有可见的焦点提示，读屏软件
 * 更是只能看到一个矩形。AbstractButton 自带空格/回车激活、
 * visualFocus 和 Accessible 角色。
 */
AbstractButton {
    id: iconButton

    property string iconSource: ""
    property string tooltip: ""
    property int iconSize: 16
    property color normalColor: "transparent"
    property color hoverColor: ThemeProvider.hoverColor
    property color pressedColor: ThemeProvider.selectedColor

    implicitWidth: iconSize + 12
    implicitHeight: iconSize + 12

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.5

    // 图标按钮没有可见文字，读屏软件只能靠这个名字。
    Accessible.role: Accessible.Button
    Accessible.name: tooltip

    background: Rectangle {
        radius: ThemeProvider.radiusSmall
        color: {
            if (!iconButton.enabled) return iconButton.normalColor
            if (iconButton.down) return iconButton.pressedColor
            if (iconButton.hovered) return iconButton.hoverColor
            return iconButton.normalColor
        }

        // 键盘焦点必须看得见，否则 Tab 过去等于什么都没发生。
        border.width: iconButton.visualFocus ? 2 : 0
        border.color: ThemeProvider.primaryColor
    }

    contentItem: Image {
        source: iconButton.iconSource
        sourceSize: Qt.size(iconButton.iconSize, iconButton.iconSize)
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
    }

    // 仅用于光标形状：AbstractButton 自己处理点击，这里不能吃掉事件。
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: iconButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    ToolTip {
        visible: iconButton.hovered && iconButton.tooltip.length > 0
        text: iconButton.tooltip
        delay: 500

        background: Rectangle {
            color: ThemeProvider.surfaceColor
            border.color: ThemeProvider.borderColor
            radius: ThemeProvider.radiusSmall
        }

        contentItem: Text {
            text: iconButton.tooltip
            font.pixelSize: ThemeProvider.fontSizeSmall
            color: ThemeProvider.textPrimary
        }
    }
}
