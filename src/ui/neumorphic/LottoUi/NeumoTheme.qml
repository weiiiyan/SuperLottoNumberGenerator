pragma Singleton
import QtQuick

// 新拟物主题令牌: 低对比底色 + 双向柔影
QtObject {
    readonly property color background: "#E0E5EC"
    readonly property color lightShadow: "#FFFFFF"
    readonly property color darkShadow: "#A3B1C6"
    readonly property color facePressed: "#D6DCE5"
    readonly property color text: "#4A5568"
    readonly property color textMuted: "#7A8699"
    readonly property color frontColor: "#C86A6A"
    readonly property color backColor: "#6A8FC8"
    readonly property real radius: 16
    readonly property real shadowBlur: 12
    readonly property real shadowOffset: 5
}
