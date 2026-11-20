import QtQuick
import QtQuick.Effects
import LottoUi

// 风格胶囊: 当前项内陷(阴影反向), 非当前项凸起
Item {
    id: pill

    property string text: ""
    property bool current: false
    property bool down: mouseArea.pressed

    signal clicked()

    implicitWidth: Math.max(84, label.implicitWidth + 32)
    implicitHeight: 34

    readonly property bool sunk: current || down

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.darkShadow
        blur: NeumoTheme.shadowBlur * 0.7
        offset: Qt.vector2d(pill.sunk ? -3 : 3, pill.sunk ? -3 : 3)
        cached: true
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.lightShadow
        blur: NeumoTheme.shadowBlur * 0.7
        offset: Qt.vector2d(pill.sunk ? 3 : -3, pill.sunk ? 3 : -3)
        cached: true
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: height / 2
        color: pill.sunk ? NeumoTheme.facePressed : NeumoTheme.background

        Text {
            id: label
            anchors.centerIn: parent
            text: pill.text
            color: pill.current ? NeumoTheme.text : NeumoTheme.textMuted
            font.pixelSize: 13
            font.bold: pill.current
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: pill.clicked()
    }
}
