import QtQuick
import QtQuick.Layouts
import LottoUi

// 标题 + 值的一行文本(时间显示用)
RowLayout {
    id: row

    property string label: ""
    property string value: ""

    spacing: 6

    Text {
        text: row.label
        color: NeumoTheme.textMuted
        font.pixelSize: 14
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        text: row.value
        color: NeumoTheme.text
        font.pixelSize: 14
        Layout.alignment: Qt.AlignVCenter
    }
}
