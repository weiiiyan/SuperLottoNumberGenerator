import QtQuick
import LottoUi

// 自绘几何图标(不依赖 svg 模块): generate / lock / style
Canvas {
    id: icon

    property string kind: "generate"
    property color color: NeumoTheme.textMuted

    implicitWidth: 18
    implicitHeight: 18

    onKindChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = Math.max(1.2, width * 0.10)
        ctx.lineCap = "round"

        const w = width
        const h = height
        const pad = w * 0.18

        if (kind === "generate") {
            // 三点方阵: 随机生成
            const r = Math.max(1.2, w * 0.09)
            for (let row = 0; row < 3; ++row) {
                for (let col = 0; col < 3; ++col) {
                    ctx.beginPath()
                    ctx.arc(pad + col * (w - 2 * pad) / 2,
                            pad + row * (h - 2 * pad) / 2, r, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
        } else if (kind === "lock") {
            // 锁体 + 锁梁
            const bodyH = h * 0.45
            const bodyW = w * 0.62
            ctx.beginPath()
            ctx.roundedRect((w - bodyW) / 2, h - pad - bodyH, bodyW, bodyH,
                            w * 0.08, w * 0.08)
            ctx.fill()
            ctx.beginPath()
            ctx.arc(w / 2, h - pad - bodyH, bodyW * 0.34,
                    Math.PI, Math.PI * 2)
            ctx.stroke()
        } else {
            // 四角方块: 与设计稿 SVG 一致(5×5, rx=1, 1/.45/.45/.8)
            const sx = w / 13.0
            const sy = h / 13.0
            const positions = [
                [0.5, 0.5], [7.5, 0.5], [0.5, 7.5], [7.5, 7.5]
            ]
            const opacities = [1.0, 0.45, 0.45, 0.8]
            for (let i = 0; i < positions.length; ++i) {
                ctx.globalAlpha = opacities[i]
                ctx.beginPath()
                ctx.roundedRect(positions[i][0] * sx, positions[i][1] * sy,
                                5.0 * sx, 5.0 * sy, 1.0 * sx, 1.0 * sy)
                ctx.fill()
            }
            ctx.globalAlpha = 1.0
        }
    }
}
