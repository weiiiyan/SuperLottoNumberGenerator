import QtQuick
import LottoUi

// 骑缝线: 横向虚线 + 两端半圆缺口(圆心落在左右边界上)
Item {
    id: perf
    objectName: "perfLine"

    implicitHeight: NeumoTheme.perfHeight

    Canvas {
        anchors.fill: parent

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cy = height / 2

            ctx.strokeStyle = "#BFA3B1C6"
            ctx.lineWidth = 2
            ctx.setLineDash([3, 2])
            ctx.beginPath()
            ctx.moveTo(0, cy)
            ctx.lineTo(width, cy)
            ctx.stroke()
            ctx.setLineDash([])

            // 半圆缺口: 底色 + inset 1px 1px 3px rgba(120,135,158,.75)
            function drawNotch(cx) {
                ctx.save()
                ctx.beginPath()
                ctx.arc(cx, cy, 8.5, 0, Math.PI * 2)
                ctx.clip()

                ctx.fillStyle = NeumoTheme.stage
                ctx.fillRect(cx - 8.5, cy - 8.5, 17, 17)

                // 顶部垂直内阴影: 左右缺口镜像对称
                const shade = ctx.createLinearGradient(0, cy - 8.5,
                                                       0, cy - 8.5 + 4.5)
                shade.addColorStop(0, "rgba(120,135,158,0.55)")
                shade.addColorStop(1, "rgba(120,135,158,0)")
                ctx.fillStyle = shade
                ctx.fillRect(cx - 8.5, cy - 8.5, 17, 17)
                ctx.restore()
            }

            drawNotch(0)
            drawNotch(width)
        }
    }
}
