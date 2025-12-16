pragma Singleton
import QtQuick

/**
 * ThemeProvider - 主题管理单例
 * 提供统一的主题属性供所有QML组件使用
 */
QtObject {
    id: themeProvider
    
    // 当前主题索引: 0=Light, 1=Win11, 2=Classic, 3=Colorful
    property int currentTheme: 0
    
    // ========== 主色调 ==========
    readonly property color primaryColor: {
        switch(currentTheme) {
            case 0: return "#6cb7fb"  // Light - 清新蓝
            case 1: return "#0078d4"  // Win11 - Windows蓝
            case 2: return "#1a73e8"  // Classic - 经典蓝
            case 3: return "#ff6b6b"  // Colorful - 活力红
            default: return "#6cb7fb"
        }
    }
    
    readonly property color secondaryColor: {
        switch(currentTheme) {
            case 0: return "#59ccb6"  // Light - 薄荷绿
            case 1: return "#005a9e"  // Win11 - 深蓝
            case 2: return "#34a853"  // Classic - 经典绿
            case 3: return "#4ecdc4"  // Colorful - 青绿
            default: return "#59ccb6"
        }
    }
    
    // ========== 背景色 ==========
    readonly property color backgroundColor: {
        switch(currentTheme) {
            case 0: return "#f8f9fa"  // Light
            case 1: return "#f3f3f3"  // Win11
            case 2: return "#ffffff"  // Classic
            case 3: return "#2d3436"  // Colorful (暗色)
            default: return "#f8f9fa"
        }
    }
    
    readonly property color surfaceColor: {
        switch(currentTheme) {
            case 0: return "#ffffff"
            case 1: return "#ffffff"
            case 2: return "#f5f5f5"
            case 3: return "#3d4449"
            default: return "#ffffff"
        }
    }
    
    readonly property color cardColor: {
        switch(currentTheme) {
            case 0: return "#ffffff"
            case 1: return "#ffffff"
            case 2: return "#ffffff"
            case 3: return "#45525a"
            default: return "#ffffff"
        }
    }
    
    // ========== 文本色 ==========
    readonly property color textPrimary: {
        switch(currentTheme) {
            case 0: return "#333333"
            case 1: return "#1a1a1a"
            case 2: return "#202124"
            case 3: return "#ffffff"
            default: return "#333333"
        }
    }
    
    readonly property color textSecondary: {
        switch(currentTheme) {
            case 0: return "#505a68"
            case 1: return "#5f6368"
            case 2: return "#5f6368"
            case 3: return "#b2bec3"
            default: return "#505a68"
        }
    }
    
    readonly property color textDisabled: {
        switch(currentTheme) {
            case 0: return "#a9aeb2"
            case 1: return "#a0a0a0"
            case 2: return "#9aa0a6"
            case 3: return "#636e72"
            default: return "#a9aeb2"
        }
    }
    
    // ========== 边框色 ==========
    readonly property color borderColor: {
        switch(currentTheme) {
            case 0: return "#e6e7e8"
            case 1: return "#d1d1d1"
            case 2: return "#dadce0"
            case 3: return "#4a5568"
            default: return "#e6e7e8"
        }
    }
    
    // ========== 悬停/选中色 ==========
    readonly property color hoverColor: {
        switch(currentTheme) {
            case 0: return "#e1efff"
            case 1: return "#e5f1fb"
            case 2: return "#e8f0fe"
            case 3: return "#4a5568"
            default: return "#e1efff"
        }
    }
    
    readonly property color selectedColor: {
        switch(currentTheme) {
            case 0: return "#e1efff"
            case 1: return "#cce4f7"
            case 2: return "#d2e3fc"
            case 3: return "#5a6c7d"
            default: return "#e1efff"
        }
    }
    
    // ========== 交替行色 ==========
    readonly property color alternateRowColor: {
        switch(currentTheme) {
            case 0: return "#fafbfc"
            case 1: return "#fafafa"
            case 2: return "#f8f9fa"
            case 3: return "#3a4248"
            default: return "#fafbfc"
        }
    }
    
    // ========== 功能色 ==========
    readonly property color successColor: "#4CAF50"
    readonly property color warningColor: "#ff9800"
    readonly property color errorColor: "#f44336"
    readonly property color infoColor: "#2196F3"
    
    // ========== 尺寸 ==========
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 6
    readonly property int radiusLarge: 8
    
    readonly property int spacingSmall: 5
    readonly property int spacingMedium: 10
    readonly property int spacingLarge: 15
    
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeMedium: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 18
    
    // ========== 主题名称 ==========
    readonly property var themeNames: [
        qsTr("浅色主题"),
        qsTr("Windows 11主题"),
        qsTr("经典主题"),
        qsTr("多彩主题")
    ]
    
    function getThemeName(index) {
        return themeNames[index] || themeNames[0]
    }
}
