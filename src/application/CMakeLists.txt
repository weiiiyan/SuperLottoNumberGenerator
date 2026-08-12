# 用例层库: 应用特定业务逻辑(仅依赖 Qt::Core)
# LottoInteractor 在此实现用例编排与业务规则, 并实现输入边界 LottoInputBoundary;
# 三个 DIP 端口归用例层所有, 由适配器层实现:
#   TicketRepository(存储) / LottoPresenterPort(输出) / LottoInputBoundary(输入)

add_library(lotto_application STATIC
    lottoinputboundary.h
    lottointeractor.h lottointeractor.cpp
    lottopresenterport.h
    ticketrepository.h
)

target_include_directories(lotto_application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Qt 依赖必须 PUBLIC: 静态库的 PRIVATE 链接不会向下游传递 Qt 头文件路径
target_link_libraries(lotto_application PUBLIC lotto_core)
