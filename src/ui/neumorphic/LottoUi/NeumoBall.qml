import QtQuick
import QtQuick.Effects
import LottoUi

// 新拟物号码球: 圆角面板 + 左上亮影 / 右下暗影
Item {
    id: ball

    property string text: ""
    property color ballColor: NeumoTheme.frontColor
    property bool raised: true

    implicitWidth: 44
    implicitHeight: 44

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.darkShadow
        blur: NeumoTheme.shadowBlur
        offset: Qt.vector2d(ball.raised ? NeumoTheme.shadowOffset
                                        : -NeumoTheme.shadowOffset,
                            ball.raised ? NeumoTheme.shadowOffset
                                        : -NeumoTheme.shadowOffset)
        cached: true
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.lightShadow
        blur: NeumoTheme.shadowBlur
        offset: Qt.vector2d(ball.raised ? -NeumoTheme.shadowOffset
                                        : NeumoTheme.shadowOffset,
                            ball.raised ? -NeumoTheme.shadowOffset
                                        : NeumoTheme.shadowOffset)
        cached: true
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: NeumoTheme.radius
        color: ball.raised ? NeumoTheme.background : NeumoTheme.facePressed

        Text {
            anchors.centerIn: parent
            text: ball.text
            color: ball.ballColor
            font.pixelSize: Math.max(10, Math.round(parent.height * 0.42))
            font.bold: true
        }
    }
}
