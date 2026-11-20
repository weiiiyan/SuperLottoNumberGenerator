# UI 核心组件(lotto_ui): 视图端口 + 风格端口 + 视图宿主 + 过渡 + 注册表类型
# 依赖只指向内层: ui → adapters → application → domain
# 本目标不链接任何具体风格, 也不依赖 Qt Quick —— 风格由子目录各自构建

add_library(lotto_ui STATIC
    lottoview.h lottoview.cpp
    lottostylehost.h
    lottostylecatalog.h
    lottoviewshell.h lottoviewshell.cpp
    lottostyleswitchoverlay.h lottostyleswitchoverlay.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
)

target_include_directories(lotto_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_ui PUBLIC lotto_adapters
    Qt${QT_VERSION_MAJOR}::Widgets)

# 具体风格组件: 每个风格一个可独立链接的目标
add_subdirectory(classic)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    add_subdirectory(neumorphic)
endif()
