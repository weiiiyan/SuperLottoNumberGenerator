# 用例层库: 应用特定业务逻辑(仅依赖 Qt::Core)
# LottoInteractor 在此实现用例编排与业务规则;
# TicketRepository 持久化端口(DIP)由用例层定义, 适配器层实现

add_library(lotto_application STATIC
    lottointeractor.h lottointeractor.cpp
    ticketrepository.h
)

target_include_directories(lotto_application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_application PUBLIC lotto_core)
