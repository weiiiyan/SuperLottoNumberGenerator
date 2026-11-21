import QtQuick
import QtQuick.Effects
import LottoUi

// 新拟物号码球: 与背景同色的球面, 靠双向柔影浮起; 数字用主题色
Item {
    id: ball

    property string text: ""
    property color ballColor: NeumoTheme.frontColor
    property bool raised: true
    property int popDelay: 0

    implicitWidth: NeumoTheme.ballSize
    implicitHeight: NeumoTheme.ballSize

    // 换号时逐个弹入; 占位符不参与动效
    onTextChanged: {
        if (text !== "" && text !== "?")
            popAnimation.restart()
    }

    SequentialAnimation {
        id: popAnimation
        PauseAnimation { duration: ball.popDelay }
        NumberAnimation {
            target: ball
            property: "scale"
            from: 0.72
            to: 1.0
            duration: 340
            easing.type: Easing.OutBack
            easing.amplitude: 0.62
        }
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.darkShadow
        blur: NeumoTheme.ballShadowBlur
        offset: Qt.vector2d(ball.raised ? NeumoTheme.ballShadowOffset
                                        : -NeumoTheme.ballShadowOffset,
                            ball.raised ? NeumoTheme.ballShadowOffset
                                        : -NeumoTheme.ballShadowOffset)
        cached: true
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.lightShadow
        blur: NeumoTheme.ballShadowBlur
        offset: Qt.vector2d(ball.raised ? -NeumoTheme.ballShadowOffset
                                        : NeumoTheme.ballShadowOffset,
                            ball.raised ? -NeumoTheme.ballShadowOffset
                                        : NeumoTheme.ballShadowOffset)
        cached: true
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: width / 2
        color: ball.raised ? NeumoTheme.background : NeumoTheme.facePressed

        Text {
            anchors.centerIn: parent
            text: ball.text
            color: ball.ballColor
            font.family: "Bahnschrift"
            font.pixelSize: Math.max(11, Math.round(parent.height * 0.425))
            font.bold: true
        }
    }
}
