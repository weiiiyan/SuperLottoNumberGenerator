import QtQuick
import QtQuick.Effects
import LottoUi

// 新拟物按钮: 常态凸起(左上亮/右下暗), 按下或选中时内陷(阴影反向)
Item {
    id: control

    property string text: ""
    property bool interactive: true
    property bool checked: false
    property string iconKind: ""
    property bool down: mouseArea.pressed && control.interactive
    property bool sunk: down || checked

    signal clicked()

    implicitWidth: Math.max(120, content.implicitWidth + 36)
    implicitHeight: 44

    opacity: interactive ? 1.0 : 0.45

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.darkShadow
        blur: NeumoTheme.shadowBlur
        offset: Qt.vector2d(control.sunk ? -NeumoTheme.shadowOffset
                                         : NeumoTheme.shadowOffset,
                            control.sunk ? -NeumoTheme.shadowOffset
                                         : NeumoTheme.shadowOffset)
        cached: true
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        color: NeumoTheme.lightShadow
        blur: NeumoTheme.shadowBlur
        offset: Qt.vector2d(control.sunk ? NeumoTheme.shadowOffset
                                         : -NeumoTheme.shadowOffset,
                            control.sunk ? NeumoTheme.shadowOffset
                                         : -NeumoTheme.shadowOffset)
        cached: true
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: NeumoTheme.radius
        color: control.sunk ? NeumoTheme.facePressed : NeumoTheme.background

        Row {
            id: content
            anchors.centerIn: parent
            spacing: 8

            NeumoIcon {
                kind: control.iconKind
                visible: control.iconKind !== ""
                color: control.checked ? NeumoTheme.frontColor : NeumoTheme.textMuted
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: label
                objectName: "buttonText"
                text: control.text
                color: control.checked ? NeumoTheme.frontColor : NeumoTheme.text
                font.pixelSize: 15
                font.bold: control.checked
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: control.interactive
        onClicked: control.clicked()
    }
}
