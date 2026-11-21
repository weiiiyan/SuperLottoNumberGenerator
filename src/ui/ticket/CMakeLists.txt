# 票据风格组件: Widgets + QSS 实现
# 只被 Main 与票据视图测试链接; 核心库 lotto_ui 不依赖本目标

add_library(lotto_ui_ticket STATIC
    lottoticketview.h lottoticketview.cpp
    lottoballlabel.h lottoballlabel.cpp
    lottotickettheme.h
    lottostyledrawer.h lottostyledrawer.cpp
    lottoticketdecor.h lottoticketdecor.cpp
    resources.qrc
)

target_include_directories(lotto_ui_ticket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_ui_ticket PUBLIC lotto_ui
    Qt${QT_VERSION_MAJOR}::Widgets)
