# 适配器库: 输入/输出/存储侧适配器(仅依赖 Qt::Core)
# 存储侧: QSettingsTicketRepository(票据) + QSettingsStyleStore(风格偏好);
# 输入侧: LottoController(事件→用例); 输出侧: LottoPresenter(用例状态→展示状态)
# 风格偏好端口 LottoStyleStore 也在此: 它由存储侧实现、由 UI 消费,
# 放在实现所在的层可避免 adapters → ui 的反向依赖

add_library(lotto_adapters STATIC
    qsettingsrepository.h qsettingsrepository.cpp
    lottocontroller.h lottocontroller.cpp
    lottopresenter.h lottopresenter.cpp
    lottoviewstate.h
    lottostylestore.h
    qsettingsstylestore.h qsettingsstylestore.cpp
)

target_include_directories(lotto_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_adapters PUBLIC lotto_application
    Qt${QT_VERSION_MAJOR}::Core)
