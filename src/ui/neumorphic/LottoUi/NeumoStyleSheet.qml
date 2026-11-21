import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import LottoUi

// 风格切换底部抽屉: 遮罩 + 贴票面底部的面板
// 视觉沿用新拟物语言: 同色面板、双向柔影、当前项内陷
Item {
    id: overlay

    anchors.fill: parent
    visible: false

    property bool opened: false
    property var styles: []
    property string currentStyleId: ""

    signal styleChosen(string styleId)

    // 显式 show/hide: 初始不可见, 关闭动画结束后再隐藏
    function open() {
        visible = true
        opened = true
    }

    function close() {
        opened = false
        hideTimer.start()
    }

    // 选完风格立即隐藏: 紧接着就切走视图, 不能把可见的抽屉留在树上
    function closeImmediately() {
        hideTimer.stop()
        opened = false
        visible = false
    }

    Timer {
        id: hideTimer
        interval: 250
        onTriggered: if (!overlay.opened) overlay.visible = false
    }

    // 打开/关闭时驱动遮罩与面板
    onOpenedChanged: {
        if (opened) {
            scrim.opacity = 1
            sheet.y = overlay.height + 20 - sheet.height
        } else {
            scrim.opacity = 0
            sheet.y = overlay.height
        }
    }

    Rectangle {
        id: scrim
        anchors.fill: parent
        color: "#4733405A"   // rgba(51,64,90,.28)
        opacity: 0

        Behavior on opacity { NumberAnimation { id: fade; duration: 200 } }

        MouseArea {
            anchors.fill: parent
            onClicked: overlay.close()
        }
    }

    // 面板上沿柔影
    RectangularShadow {
        anchors.fill: sheet
        radius: sheet.radius
        color: "#731A2437"
        blur: 30
        offset: Qt.vector2d(0, -14)
        cached: true
    }

    Rectangle {
        id: sheet
        width: parent.width
        // 多出 20px 让底部圆角被卡片裁掉, 只留上圆角
        height: sheetColumn.implicitHeight + 36 + 20
        radius: 20
        color: NeumoTheme.background
        y: overlay.height

        Behavior on y {
            NumberAnimation { id: slide; duration: 240; easing.type: Easing.OutCubic }
        }

        Column {
            id: sheetColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 18
            spacing: 8

            Text {
                text: "界面风格"
                color: NeumoTheme.textMuted
                font.pixelSize: 11
                font.letterSpacing: 1.7
            }

            Repeater {
                model: overlay.styles

                delegate: Item {
                    objectName: "styleOption_" + index
                    width: sheetColumn.width
                    height: 46

                    readonly property bool current: modelData.id === overlay.currentStyleId

                    // 当前项阴影反向 + 底色加深, 形成内陷感
                    RectangularShadow {
                        anchors.fill: optionFace
                        radius: optionFace.radius
                        color: NeumoTheme.darkShadow
                        blur: 10
                        offset: Qt.vector2d(parent.current ? -4 : 4, parent.current ? -4 : 4)
                        cached: true
                    }

                    RectangularShadow {
                        anchors.fill: optionFace
                        radius: optionFace.radius
                        color: NeumoTheme.lightShadow
                        blur: 10
                        offset: Qt.vector2d(parent.current ? 4 : -4, parent.current ? 4 : -4)
                        cached: true
                    }

                    Rectangle {
                        id: optionFace
                        anchors.fill: parent
                        radius: 10
                        color: parent.current ? NeumoTheme.facePressed : NeumoTheme.background
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.displayName
                        color: parent.current ? "#3D4A63" : NeumoTheme.text
                        font.pixelSize: 15
                        font.bold: parent.current
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.current ? "当前" : "切换"
                        color: parent.current ? NeumoTheme.frontColor : NeumoTheme.textMuted
                        font.pixelSize: 11
                        font.letterSpacing: 0.9
                        font.bold: parent.current
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            overlay.closeImmediately()
                            overlay.styleChosen(modelData.id)
                        }
                    }
                }
            }
        }
    }
}
