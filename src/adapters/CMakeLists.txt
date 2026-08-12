# 适配器库: 输入/输出侧适配器(实现用例层端口, 仅依赖 Qt::Core)
# 存储侧: QSettingsTicketRepository; 输入侧: LottoController(事件→用例);
# 输出侧: LottoPresenter(用例状态→视图展示状态)

add_library(lotto_adapters STATIC
    qsettingsrepository.h qsettingsrepository.cpp
    lottocontroller.h lottocontroller.cpp
    lottopresenter.h lottopresenter.cpp
)

target_include_directories(lotto_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_adapters PUBLIC lotto_application
    Qt${QT_VERSION_MAJOR}::Core)
