# 适配器库: 仓储实现(实现用例层端口, 仅依赖 Qt::Core)

add_library(lotto_adapters STATIC
    qsettingsrepository.h qsettingsrepository.cpp
)

target_include_directories(lotto_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_adapters PUBLIC lotto_application)
