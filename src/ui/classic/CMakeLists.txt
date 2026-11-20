# 经典风格组件: Widgets + QSS 实现
# 只被 Main 与经典视图测试链接; 核心库 lotto_ui 不依赖本目标

add_library(lotto_ui_classic STATIC
    lottoclassicview.h lottoclassicview.cpp
    resources.qrc
)

target_include_directories(lotto_ui_classic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_ui_classic PUBLIC lotto_ui
    Qt${QT_VERSION_MAJOR}::Widgets)
