pragma Singleton
import QtQuick

// 新拟物主题令牌: 低对比底色 + 双向柔影
// 骨架尺寸与 src/ui/lottolayoutmetrics.h 镜像, 两处不一致时以设计规范为准
QtObject {
    // ── 颜色令牌 ──
    readonly property color background: "#E0E5EC"
    readonly property color lightShadow: "#FFFFFF"
    readonly property color darkShadow: "#A3B1C6"
    readonly property color facePressed: "#D6DCE5"
    readonly property color text: "#4A5568"
    readonly property color textMuted: "#7A8699"
    readonly property color label: "#5A677D"
    readonly property color frontColor: "#C86A6A"
    readonly property color backColor: "#6A8FC8"
    readonly property color stage: "#D3DAE4"

    // ── 圆角 / 柔影 ──
    readonly property real radius: 16
    readonly property real shadowBlur: 12
    readonly property real shadowOffset: 5
    readonly property real ballShadowBlur: 8
    readonly property real ballShadowOffset: 3
    readonly property real buttonRadius: 16

    // ── 骨架尺寸(390 × 844 基准)──
    readonly property int canvasWidth: 390
    readonly property int canvasHeight: 844
    readonly property int padX: 22
    readonly property int padTop: 40
    readonly property int padBottom: 34
    readonly property int topBarHeight: 66
    readonly property int metaHeight: 60
    readonly property int perfHeight: 26
    readonly property int stubHeight: 105
    readonly property int actionsHeight: 81
    readonly property int actionHeight: 56
    readonly property int ballSize: 40
    readonly property int ballGap: 4
    readonly property int separatorWidth: 10
    readonly property int rowIndexWidth: 18
    readonly property int rowHeight: 76
    readonly property int rowHeightMin: 68
    readonly property int barcodeWidth: 92
    readonly property int barcodeHeight: 20
}
