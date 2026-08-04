# 领域核心库: 实体 + 用例 + 用例层接口(仅依赖 Qt::Core)

add_library(lotto_core STATIC
    lottoresult.h
    lottoengine.h lottoengine.cpp
    lottoticket.h lottoticket.cpp
    ticketrepository.h
)

target_include_directories(lotto_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_core PUBLIC Qt${QT_VERSION_MAJOR}::Core)
