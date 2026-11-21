import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import LottoUi

// 新拟物主场景: 由 C++ 侧推送属性, 交互以信号回传
// 骨架与票据版逐像素对齐: 顶栏 → 时间条 → 号码带 → 骑缝线
// → 两行票根 → 操作区
Item {
    id: root

    // ── C++ 推送的展示状态 ──
    property var groups: []
    property string timeLabelText: ""
    property string timeValueText: ""
    property string generateButtonText: ""
    property bool generateEnabled: true
    property string lockButtonText: ""
    property bool lockButtonEnabled: false
    property bool lockButtonChecked: false
    property var styles: []
    property string currentStyleId: ""
    property string numberPlaceholder: "?"
    property string separatorText: "+"
    property string appTitleText: ""
    property string headerMetaText: ""
    property string serialText: ""
    property string stubHintText: ""
    property string stubCountText: ""
    property string stubCountValueText: ""
    property string stampText: ""

    // ── 回传给 C++ 的交互信号 ──
    signal generateClicked()
    signal lockClicked()
    signal styleSwitchRequested(string styleId)

    // 供 QML 内部与测试直接触发风格切换
    function requestStyleSwitch(styleId) {
        if (styleId === currentStyleId)
            return
        root.styleSwitchRequested(styleId)
    }

    // 当前风格的显示名(风格按钮文案)
    function styleButtonLabel() {
        for (let i = 0; i < styles.length; ++i) {
            if (styles[i].id === currentStyleId)
                return styles[i].displayName
        }
        return ""
    }

    // 展示底
    Rectangle {
        anchors.fill: parent
        color: NeumoTheme.stage
    }

    // 票面卡片投影: 双向柔影(满幅时不可见)
    RectangularShadow {
        anchors.fill: card
        radius: card.radius
        color: NeumoTheme.darkShadow
        blur: 34
        offset: Qt.vector2d(14, 14)
        visible: card.radius > 0
    }

    RectangularShadow {
        anchors.fill: card
        radius: card.radius
        color: NeumoTheme.lightShadow
        blur: 34
        offset: Qt.vector2d(-14, -14)
        visible: card.radius > 0
    }

    Rectangle {
        id: card
        width: Math.min(root.width, NeumoTheme.canvasWidth)
        height: Math.min(root.height, NeumoTheme.canvasHeight)
        anchors.centerIn: parent
        radius: (width < root.width || height < root.height) ? 36 : 0
        color: NeumoTheme.background
        clip: true

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.leftMargin: NeumoTheme.padX
            anchors.rightMargin: NeumoTheme.padX
            anchors.topMargin: NeumoTheme.padTop
            anchors.bottomMargin: NeumoTheme.padBottom
            spacing: 0

            // ── 顶栏: 品牌 + 副标题 + 风格按钮 ──
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: NeumoTheme.topBarHeight

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    radius: 1
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: "#4DA3B1C6" }
                        GradientStop { position: 1.0; color: "#B3FFFFFF" }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.bottomMargin: 24
                    spacing: 10

                    Item {
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 6
                        width: 11
                        height: 11

                        Rectangle {
                            anchors.fill: parent
                            radius: 5.5
                            color: NeumoTheme.background
                            border.width: 1
                            border.color: "#66A3B1C6"
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 5
                            height: 5
                            radius: 2.5
                            color: NeumoTheme.frontColor
                        }
                    }

                    Column {
                        Layout.alignment: Qt.AlignTop
                        spacing: 4

                        Text {
                            text: root.appTitleText
                            color: "#3D4A63"
                            font.pixelSize: 18
                            font.bold: true
                            font.letterSpacing: 0.54
                        }
                        Text {
                            text: root.headerMetaText
                            color: NeumoTheme.textMuted
                            font.family: "Bahnschrift"
                            font.pixelSize: 11
                            font.letterSpacing: 1.5
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Item {
                        id: styleButton
                        objectName: "styleButton"
                        Layout.alignment: Qt.AlignTop
                        implicitWidth: Math.max(64, styleButtonRow.implicitWidth + 20)
                        implicitHeight: 32
                        Layout.preferredWidth: implicitWidth
                        Layout.preferredHeight: implicitHeight

                        // 设计稿: :active / [aria-expanded=true] 切换为内陷阴影
                        readonly property bool sunk: styleButtonArea.pressed
                                                     || styleOverlay.opened

                        RectangularShadow {
                            anchors.fill: styleButtonFace
                            radius: styleButtonFace.radius
                            color: NeumoTheme.darkShadow
                            blur: 7
                            offset: Qt.vector2d(styleButton.sunk ? -3 : 3,
                                                styleButton.sunk ? -3 : 3)
                            cached: true
                        }
                        RectangularShadow {
                            anchors.fill: styleButtonFace
                            radius: styleButtonFace.radius
                            color: NeumoTheme.lightShadow
                            blur: 7
                            offset: Qt.vector2d(styleButton.sunk ? 3 : -3,
                                                styleButton.sunk ? 3 : -3)
                            cached: true
                        }

                        Rectangle {
                            id: styleButtonFace
                            objectName: "styleButtonFace"
                            anchors.fill: parent
                            radius: 8
                            color: styleButton.sunk ? NeumoTheme.facePressed
                                                    : NeumoTheme.background
                        }

                        Row {
                            id: styleButtonRow
                            anchors.centerIn: parent
                            spacing: 6

                            NeumoIcon {
                                objectName: "styleIcon"
                                kind: "style"
                                color: NeumoTheme.text
                                width: 13
                                height: 13
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.styleButtonLabel()
                                color: NeumoTheme.text
                                font.pixelSize: 11
                                font.letterSpacing: 0.7
                            }
                        }

                        MouseArea {
                            id: styleButtonArea
                            anchors.fill: parent
                            onClicked: styleOverlay.open()
                        }
                    }
                }
            }

            // ── 时间条 ──
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: NeumoTheme.metaHeight

                NeumoValueRow {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 22
                    label: root.timeLabelText
                    value: root.timeValueText
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.15; color: "#A6A3B1C6" }
                        GradientStop { position: 0.85; color: "#A6A3B1C6" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }
            }

            // ── 号码带: 5 行, 行距 0 ──
            Item {
                id: rowsArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                readonly property int rowCount: root.groups.length
                readonly property real rowHeight: Math.max(NeumoTheme.rowHeightMin,
                    Math.min(NeumoTheme.rowHeight,
                             height / Math.max(1, rowCount)))

                Column {
                    anchors.centerIn: parent
                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: rowsArea.rowCount

                        delegate: NeumoTicketRow {
                            width: rowsArea.width
                            height: rowsArea.rowHeight
                            rowIndex: index + 1
                            showTopHairline: index > 0
                            numbers: root.groups[index] !== undefined ? root.groups[index] : []
                            placeholder: root.numberPlaceholder
                            separator: root.separatorText
                            ballSize: Math.min(NeumoTheme.ballSize, rowsArea.rowHeight - 28)
                        }
                    }
                }
            }

            // 骑缝线占位(自绘的骑缝线贴在卡片全宽上, 见下方 perf)
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: NeumoTheme.perfHeight
            }

            // ── 票根: 上行票号 + 条形码, 下行提示 + 注数 / 印章 ──
            NeumoStub {
                id: stubItem
                Layout.fillWidth: true
                Layout.preferredHeight: NeumoTheme.stubHeight
                serialText: root.serialText
                hintText: root.stubHintText
                countText: root.stubCountText
                countValueText: root.stubCountValueText
                stampText: root.stampText
                locked: root.lockButtonChecked
            }

            // ── 操作区: 左锁定 / 右生成 ──
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: NeumoTheme.actionsHeight

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.15; color: "#8CA3B1C6" }
                        GradientStop { position: 0.85; color: "#8CA3B1C6" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 24
                    height: NeumoTheme.actionHeight
                    spacing: 10

                    NeumoButton {
                        objectName: "lockButton"
                        width: (parent.width - 10) * 100 / 232
                        height: NeumoTheme.actionHeight
                        text: root.lockButtonText
                        iconKind: "lock"
                        interactive: root.lockButtonEnabled
                        checked: root.lockButtonChecked
                        onClicked: root.lockClicked()
                    }

                    NeumoButton {
                        objectName: "generateButton"
                        width: (parent.width - 10) * 132 / 232
                        height: NeumoTheme.actionHeight
                        text: root.generateButtonText
                        iconKind: "generate"
                        interactive: root.generateEnabled
                        onClicked: root.generateClicked()
                    }
                }
            }
        }

        // 骑缝线: 卡片全宽, 缺口落在卡片左右边缘
        // 用属性绑定推算 y: mapToItem 不是可观察调用, 绑定会在首帧后失效
        NeumoPerf {
            anchors.left: card.left
            anchors.right: card.right
            height: NeumoTheme.perfHeight
            y: content.y + stubItem.y - NeumoTheme.perfHeight
        }

        // 风格切换抽屉(覆盖整张卡片)
        NeumoStyleSheet {
            id: styleOverlay
            objectName: "styleOverlay"
            anchors.fill: parent
            styles: root.styles
            currentStyleId: root.currentStyleId
            onStyleChosen: (id) => root.requestStyleSwitch(id)
        }
    }
}
