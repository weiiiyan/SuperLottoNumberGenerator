import QtQuick
import QtQuick.Layouts
import LottoUi

// 一行号码: 行号列 + 前区 5 球 + "+" + 后区 2 球
// 行高固定 76, 行间 spacing 0, 行边界由 1px 渐变 hairline 表达
Item {
    id: row

    property var numbers: []
    property string placeholder: "?"
    property string separator: "+"
    property int rowIndex: 1
    property bool showTopHairline: false
    property int frontCount: 5
    property int backCount: 2
    property int ballSize: NeumoTheme.ballSize

    implicitHeight: NeumoTheme.rowHeight

    // 行边界: 中间实、两端透明的 1px 渐变
    Rectangle {
        visible: row.showTopHairline
        anchors.top: parent.top
        width: parent.width
        height: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.15; color: "#8CA3B1C6" }
            GradientStop { position: 0.85; color: "#8CA3B1C6" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Text {
        id: idx
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: NeumoTheme.rowIndexWidth
        text: String(row.rowIndex).padStart(2, "0")
        color: NeumoTheme.textMuted
        font.family: "Bahnschrift"
        font.pixelSize: 11
        font.letterSpacing: 1.3
    }

    Item {
        id: cell
        anchors.left: idx.right
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Row {
            id: line
            anchors.centerIn: parent
            spacing: NeumoTheme.ballGap

            Repeater {
                model: row.frontCount
                delegate: NeumoBall {
                    width: row.ballSize
                    height: row.ballSize
                    text: row.numbers[index] !== undefined ? row.numbers[index]
                                                           : row.placeholder
                    ballColor: NeumoTheme.frontColor
                    popDelay: index * 16
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: NeumoTheme.separatorWidth
                horizontalAlignment: Text.AlignHCenter
                text: row.separator
                color: NeumoTheme.textMuted
                font.family: "Bahnschrift"
                font.pixelSize: Math.max(10, Math.round(row.ballSize * 0.325))
            }

            Repeater {
                model: row.backCount
                delegate: NeumoBall {
                    width: row.ballSize
                    height: row.ballSize
                    text: row.numbers[row.frontCount + index] !== undefined
                          ? row.numbers[row.frontCount + index] : row.placeholder
                    ballColor: NeumoTheme.backColor
                    popDelay: (row.frontCount + index) * 16
                }
            }
        }
    }
}
