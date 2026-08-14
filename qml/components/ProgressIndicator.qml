import QtQuick
import QtQuick.Controls.Basic
import FLiNGDownloader

/**
 * ProgressIndicator - 下载进度条组件
 * indeterminate 为未知总大小时的从左到右扫过动画
 */
ProgressBar {
    id: control
    
    property string statusText: ""
    property bool showText: true
    property bool flat: false
    property color barColor: ThemeProvider.primaryColor
    property real sweep: 0
    
    from: 0
    to: 1
    
    implicitWidth: 200
    implicitHeight: showText ? 24 : 8
    
    background: Rectangle {
        implicitWidth: 200
        implicitHeight: showText ? 24 : 8
        radius: control.flat ? 2 : ThemeProvider.radiusMedium
        color: ThemeProvider.backgroundColor
        border.width: control.flat ? 0 : 1
        border.color: ThemeProvider.borderColor
    }
    
    contentItem: Item {
        implicitWidth: 200
        implicitHeight: showText ? 24 : 8
        clip: true
        
        Rectangle {
            visible: !control.indeterminate
            width: Math.max(0, control.visualPosition * parent.width)
            height: parent.height
            radius: control.flat ? 2 : ThemeProvider.radiusMedium
            color: control.barColor
            
            Behavior on width {
                enabled: !control.indeterminate
                NumberAnimation { duration: 80 }
            }
        }
        
        Rectangle {
            visible: control.indeterminate
            width: Math.max(20, parent.width * 0.3)
            height: parent.height
            radius: control.flat ? 2 : ThemeProvider.radiusMedium
            color: control.barColor
            x: -width + (parent.width + width) * control.sweep
        }
        
        Text {
            visible: showText && !control.indeterminate
            anchors.centerIn: parent
            text: statusText.length > 0 ? statusText : Math.round(control.value * 100) + "%"
            font.pixelSize: ThemeProvider.fontSizeSmall
            font.bold: true
            color: control.value > 0.5 ? "white" : ThemeProvider.textPrimary
        }
    }
    
    Timer {
        interval: 16
        repeat: true
        running: control.indeterminate && control.visible
        onTriggered: control.sweep = (control.sweep + interval / 1200) % 1
    }
}
