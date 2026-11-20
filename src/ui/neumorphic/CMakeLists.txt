# 新拟物风格组件: Qt6 QML + QQuickWidget 实现
# 只被 Main 与新拟物视图测试链接; 核心库 lotto_ui 不依赖本目标
# 本目录只在 Qt6 下被 add_subdirectory
#
# QML 源文件位于本目录下的 LottoUi/(与模块 URI 的 target path 一致),
# 资源落点 /qt/qml/LottoUi/<文件名>。

set_source_files_properties(LottoUi/NeumoTheme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE)

add_library(lotto_ui_neumorphic STATIC
    lottoneumorphicview.h lottoneumorphicview.cpp
)

target_include_directories(lotto_ui_neumorphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(lotto_ui_neumorphic
    URI LottoUi
    VERSION 1.0
    NO_PLUGIN
    RESOURCE_PREFIX /qt/qml/
    QML_FILES
        LottoUi/NeumoTheme.qml
        LottoUi/NeumoBall.qml
        LottoUi/NeumoButton.qml
        LottoUi/NeumoTicketRow.qml
        LottoUi/NeumoValueRow.qml
        LottoUi/NeumoPill.qml
        LottoUi/NeumoIcon.qml
        LottoUi/NeumorphicView.qml
)

target_link_libraries(lotto_ui_neumorphic PUBLIC lotto_ui
    Qt6::Qml Qt6::QuickWidgets)
