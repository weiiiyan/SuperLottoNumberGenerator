import QtQuick
import QtQuick.Layouts
import LottoUi

// 时间行: 标签左 / 值右, 整行两端对齐
RowLayout {
    id: row

    property string label: ""
    property string value: ""

    spacing: 12

    Text {
        text: row.label
        color: NeumoTheme.textMuted
        font.pixelSize: 11
        font.letterSpacing: 1.7
        Layout.alignment: Qt.AlignBottom
    }

    Item { Layout.fillWidth: true }

    Text {
        text: row.value
        color: NeumoTheme.text
        font.family: "Bahnschrift"
        font.pixelSize: 13
        font.letterSpacing: 0.5
        Layout.alignment: Qt.AlignBottom
    }
}
