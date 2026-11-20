import QtQuick
import QtQuick.Layouts
import LottoUi

// 新拟物主场景: 由 C++ 侧推送属性, 交互以信号回传
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

    Rectangle {
        anchors.fill: parent
        color: NeumoTheme.background

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            // ── 顶部: 时间 + 风格切换 ──
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                NeumoValueRow {
                    Layout.fillWidth: true
                    label: root.timeLabelText
                    value: root.timeValueText
                }

                NeumoButton {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 34
                    implicitHeight: 34
                    iconKind: "style"
                    onClicked: {
                        // 在可用风格中循环切换
                        if (!root.styles || root.styles.length < 2)
                            return
                        for (let i = 0; i < root.styles.length; ++i) {
                            if (root.styles[i].id === root.currentStyleId) {
                                const next = root.styles[(i + 1) % root.styles.length]
                                root.requestStyleSwitch(next.id)
                                return
                            }
                        }
                    }
                }
            }

            // ── 号码区: 5 行 ──
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                Repeater {
                    model: root.groups.length

                    delegate: NeumoTicketRow {
                        Layout.fillWidth: true
                        numbers: root.groups[index] !== undefined ? root.groups[index] : []
                        placeholder: root.numberPlaceholder
                        separator: root.separatorText
                    }
                }
            }

            // ── 按钮行 ──
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Item { Layout.fillWidth: true }

                NeumoButton {
                    Layout.preferredWidth: 140
                    text: root.lockButtonText
                    iconKind: "lock"
                    interactive: root.lockButtonEnabled
                    checked: root.lockButtonChecked
                    onClicked: root.lockClicked()
                }

                NeumoButton {
                    Layout.preferredWidth: 170
                    text: root.generateButtonText
                    iconKind: "generate"
                    interactive: root.generateEnabled
                    onClicked: root.generateClicked()
                }

                Item { Layout.fillWidth: true }
            }

            // ── 底部: 风格胶囊 ──
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Repeater {
                    model: root.styles

                    delegate: NeumoPill {
                        text: modelData.displayName
                        current: modelData.id === root.currentStyleId
                        onClicked: root.requestStyleSwitch(modelData.id)
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
