import QtQuick
import QtQuick.Layouts
import LottoUi

// 票根: 上行票号 + 条形码, 下行提示 + 注数 / 锁定印章
Item {
    id: stub

    property string serialText: ""
    property string hintText: ""
    property string countText: ""
    property string countValueText: ""
    property string stampText: ""
    property bool locked: false

    implicitHeight: NeumoTheme.stubHeight

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.topMargin: 18
        anchors.bottomMargin: 16
        spacing: 0

        // ── 上行: 票号 / 条形码 ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: stub.serialText
                color: NeumoTheme.textMuted
                font.family: "Bahnschrift"
                font.pixelSize: 11
                font.letterSpacing: 0.7
                Layout.alignment: Qt.AlignBottom
            }

            Item { Layout.fillWidth: true }

            Canvas {
                id: barcode
                Layout.preferredWidth: NeumoTheme.barcodeWidth
                Layout.preferredHeight: NeumoTheme.barcodeHeight
                Layout.alignment: Qt.AlignBottom
                opacity: 0.5

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    ctx.fillStyle = NeumoTheme.textMuted
                    for (let x = 0; x < width; x += 12) {
                        ctx.fillRect(x + 0, 0, 1, height)
                        ctx.fillRect(x + 3, 0, 2, height)
                        ctx.fillRect(x + 7, 0, 1, height)
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 15 }

        // 两行之间的 hairline
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.15; color: "#8CA3B1C6" }
                GradientStop { position: 0.85; color: "#8CA3B1C6" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Item { Layout.preferredHeight: 9 }

        // ── 下行: 提示 / 注数 · 印章 ──
        RowLayout {
            id: bottomRow
            objectName: "stubBottomRow"
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: stub.hintText
                color: NeumoTheme.textMuted
                font.pixelSize: 10
                font.letterSpacing: 0.6
                Layout.alignment: Qt.AlignBottom
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                id: countRow
                spacing: 0
                Layout.alignment: Qt.AlignBottom
                // 锁定后只淡出, 保留布局高度, 避免票根下行变高
                opacity: stub.locked ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 180 } }

                readonly property var parts: stub.countValueText !== ""
                    ? stub.countText.split(stub.countValueText)
                    : [stub.countText]

                Text {
                    text: countRow.parts[0] || ""
                    color: NeumoTheme.textMuted
                    font.family: "Bahnschrift"
                    font.pixelSize: 11
                    font.letterSpacing: 1.5
                    Layout.alignment: Qt.AlignBaseline
                }
                Text {
                    text: stub.countValueText
                    color: NeumoTheme.text
                    font.family: "Bahnschrift"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 0.5
                    Layout.alignment: Qt.AlignBaseline
                }
                Text {
                    text: countRow.parts[1] || ""
                    color: NeumoTheme.textMuted
                    font.family: "Bahnschrift"
                    font.pixelSize: 11
                    font.letterSpacing: 1.5
                    Layout.alignment: Qt.AlignBaseline
                }
            }
        }
    }

    // 锁定印章: 绝对定位(设计稿 right:-2px; bottom:-4px), 不参与布局
    // 用属性绑定而非 anchors: 锚点目标不是父/兄弟项, anchors 会被忽略
    Rectangle {
        id: stamp
        objectName: "stubStamp"
        x: column.x + bottomRow.x + bottomRow.width - width + 2
        y: column.y + bottomRow.y + bottomRow.height - height + 4
        width: 74
        height: 26
        radius: 5
        rotation: -7
        visible: false
        opacity: 0
        color: "#80F7F2E6"
        border.width: 2
        border.color: "#80C1121F"

        Text {
            anchors.centerIn: parent
            text: stub.stampText
            color: "#CCC1121F"
            font.pixelSize: 12
            font.bold: true
            font.letterSpacing: 2
        }
    }

    onLockedChanged: {
        if (locked) {
            stamp.visible = true
            stampIn.restart()
        } else {
            stamp.visible = false
            stamp.opacity = 0
            stamp.scale = 0.94
        }
    }

    // 设计稿: 印章 rotate(-7deg) 微放大淡入, 220ms
    ParallelAnimation {
        id: stampIn
        NumberAnimation { target: stamp; property: "opacity"; from: 0; to: 1; duration: 220 }
        NumberAnimation {
            target: stamp
            property: "scale"
            from: 0.94
            to: 1
            duration: 220
            easing.type: Easing.OutCubic
        }
    }
}
