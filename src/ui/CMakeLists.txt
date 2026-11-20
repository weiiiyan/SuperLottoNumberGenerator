# UI 层: 视图组件 + 视图宿主 + 风格目录(lotto_ui)
# 依赖只指向内层: app → ui → adapters
# 经典视图为 Widgets 实现; 新拟物视图为 Qt6 QML 实现(QQuickWidget 承载)

add_library(lotto_ui STATIC
    lottoview.h lottoview.cpp
    lottostylehost.h
    lottostylestore.h
    qsettingsstylestore.h qsettingsstylestore.cpp
    lottostylecatalog.h lottostylecatalog.cpp
    lottoviewshell.h lottoviewshell.cpp
    classic/lottoclassicview.h classic/lottoclassicview.cpp
    classic/resources.qrc
)

target_include_directories(lotto_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_ui PUBLIC lotto_adapters
    Qt${QT_VERSION_MAJOR}::Widgets)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Qml QuickWidgets)

    # 经典视图的样式表资源前缀为 /classic/
    # 单例需在加入模块前标记
    set_source_files_properties(LottoUi/NeumoTheme.qml PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE)

    # 静态库 QML 模块: qmldir/资源随 lotto_ui 一起链接
    qt_add_qml_module(lotto_ui
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
            LottoUi/NeumorphicView.qml
    )

    target_sources(lotto_ui PRIVATE
        neumorphic/lottoneumorphicview.h neumorphic/lottoneumorphicview.cpp
    )

    target_link_libraries(lotto_ui PUBLIC Qt6::Qml Qt6::QuickWidgets)
endif()
