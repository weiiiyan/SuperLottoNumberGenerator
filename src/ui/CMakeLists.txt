# UI 层: 视图组件 + 视图宿主 + 风格目录(lotto_ui)
# 依赖只指向内层: app → ui → adapters
# 经典视图为 Widgets 实现; 新拟物视图为 Qt6 QML 实现(QQuickWidget 承载)

add_library(lotto_ui STATIC
    lottoview.h lottoview.cpp
    lottostylehost.h
    lottostylestore.h
    qsettingsstylestore.h qsettingsstylestore.cpp
)

target_include_directories(lotto_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_ui PUBLIC lotto_adapters
    Qt${QT_VERSION_MAJOR}::Widgets)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Qml QuickWidgets)

    # 静态库 QML 模块: 生成的 qrc/qmldir 需在链接期保留(见 lottoview.cpp 的注册调用)
    qt_add_qml_module(lotto_ui
        URI LottoUi
        VERSION 1.0
        NO_PLUGIN
        RESOURCE_PREFIX /qt/qml/
        QML_FILES
            LottoUi/NeumorphicView.qml
    )

    target_link_libraries(lotto_ui PUBLIC Qt6::Qml Qt6::QuickWidgets)
endif()
