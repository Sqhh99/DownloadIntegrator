import QtQuick
import QtQuick.Controls.Basic
import FLiNGDownloader

/**
 * StyledButton - 统一样式按钮组件
 * 支持主题切换，自动应用 ThemeProvider 中的样式
 */
Button {
    id: control
    
    // 按钮类型: "primary", "secondary", "success", "danger", "info"
    // secondary 使用主题主色的浅底描边，避免再引入一套冲突色相
    property string buttonType: "primary"
    property bool rounded: true
    
    implicitWidth: Math.max(80, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 36
    
    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    
    font.pixelSize: ThemeProvider.fontSizeMedium
    font.bold: true

    readonly property bool isTonal: buttonType === "secondary"

    readonly property color accentColor: {
        switch (buttonType) {
            case "success": return ThemeProvider.successColor
            case "danger": return ThemeProvider.errorColor
            case "info": return ThemeProvider.infoColor
            default: return ThemeProvider.primaryColor
        }
    }

    readonly property color labelColor: {
        if (!control.enabled)
            return ThemeProvider.textDisabled
        if (isTonal)
            return ThemeProvider.isDark ? accentColor : Qt.darker(accentColor, 1.15)
        return ThemeProvider.contrastOn(accentColor)
    }

    function fillWithAlpha(base, alpha) {
        return Qt.rgba(base.r, base.g, base.b, alpha)
    }

    readonly property color idleFill: {
        if (isTonal)
            return fillWithAlpha(accentColor, ThemeProvider.isDark ? 0.18 : 0.12)
        return accentColor
    }

    readonly property color hoverFill: {
        if (isTonal)
            return fillWithAlpha(accentColor, ThemeProvider.isDark ? 0.28 : 0.20)
        return Qt.darker(accentColor, 1.1)
    }

    readonly property color pressedFill: {
        if (isTonal)
            return fillWithAlpha(accentColor, ThemeProvider.isDark ? 0.36 : 0.28)
        return Qt.darker(accentColor, 1.2)
    }
    
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.labelColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    
    background: Rectangle {
        implicitWidth: 80
        implicitHeight: 36
        radius: rounded ? ThemeProvider.radiusMedium : 0
        color: {
            if (!control.enabled) return ThemeProvider.disabledColor
            if (control.pressed) return control.pressedFill
            if (control.hovered) return control.hoverFill
            return control.idleFill
        }
        border.width: control.isTonal && control.enabled ? 1 : 0
        border.color: control.fillWithAlpha(control.accentColor, ThemeProvider.isDark ? 0.50 : 0.38)

        Behavior on color {
            ColorAnimation { duration: 100 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 100 }
        }
    }
}
