import QtQuick
import QtQuick.Layouts
import LottoUi

// 一行号码: 前区 5 球 + "+" + 后区 2 球
Item {
    id: row

    property var numbers: []
    property string placeholder: "?"
    property string separator: "+"
    property int frontCount: 5
    property int backCount: 2
    implicitHeight: ballSize + 2 * NeumoTheme.shadowOffset
    implicitWidth: layout.implicitWidth

    // 分隔符槽位用固定宽度参与计算, 不得回读 separator.implicitWidth:
    // 否则 ballSize ← 分隔符字号 ← ballSize 构成绑定环
    readonly property real separatorWidth: 18
    readonly property real ballSize: Math.max(22, Math.min(44,
        (width - 6 * spacing - separatorWidth) / (frontCount + backCount)))

    Row {
        id: layout
        anchors.centerIn: parent
        spacing: row.spacing

        Repeater {
            model: row.frontCount
            delegate: NeumoBall {
                width: row.ballSize
                height: row.ballSize
                text: row.numbers[index] !== undefined ? row.numbers[index] : row.placeholder
                ballColor: NeumoTheme.frontColor
            }
        }

        Text {
            id: separatorLabel
            anchors.verticalCenter: parent.verticalCenter
            width: row.separatorWidth
            horizontalAlignment: Text.AlignHCenter
            text: row.separator
            color: NeumoTheme.textMuted
            font.pixelSize: Math.max(10, Math.round(row.ballSize * 0.4))
        }

        Repeater {
            model: row.backCount
            delegate: NeumoBall {
                width: row.ballSize
                height: row.ballSize
                text: row.numbers[row.frontCount + index] !== undefined
                      ? row.numbers[row.frontCount + index] : row.placeholder
                ballColor: NeumoTheme.backColor
            }
        }
    }

    readonly property real spacing: 6
}
