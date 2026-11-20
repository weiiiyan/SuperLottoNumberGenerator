import QtQuick
import QtQuick.Layouts
import LottoUi

// 一行号码: 前区 5 球 + "+" + 后区 2 球
Item {
    id: row

    property var numbers: []
    property int frontCount: 5
    property int backCount: 2
    implicitHeight: ballSize + 2 * NeumoTheme.shadowOffset
    implicitWidth: layout.implicitWidth

    readonly property real ballSize: Math.max(22, Math.min(44,
        (width - 6 * spacing - separator.implicitWidth) / (frontCount + backCount)))

    Row {
        id: layout
        anchors.centerIn: parent
        spacing: row.spacing

        Repeater {
            model: row.frontCount
            delegate: NeumoBall {
                width: row.ballSize
                height: row.ballSize
                text: row.numbers[index] !== undefined ? row.numbers[index] : "?"
                ballColor: NeumoTheme.frontColor
            }
        }

        Text {
            id: separator
            anchors.verticalCenter: parent.verticalCenter
            text: "+"
            color: NeumoTheme.textMuted
            font.pixelSize: Math.max(10, Math.round(row.ballSize * 0.4))
        }

        Repeater {
            model: row.backCount
            delegate: NeumoBall {
                width: row.ballSize
                height: row.ballSize
                text: row.numbers[row.frontCount + index] !== undefined
                      ? row.numbers[row.frontCount + index] : "?"
                ballColor: NeumoTheme.backColor
            }
        }
    }

    readonly property real spacing: 6
}
