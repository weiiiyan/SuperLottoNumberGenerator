cmake_minimum_required(VERSION 3.16)

project(SuperLottoNumberGenerator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 测试开关 ──
option(BUILD_TESTS "构建单元测试" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Qt6: UI 层的新拟物视图使用 QML; 在根目录解析 Qml 组件, 使 QML 插件
# 导入目标对 Android 部署(finalize_executable)可见
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Qml QuickWidgets)
endif()

# ── 分层架构: 领域核心 / 用例层 / 适配器 / 视图 ──
# 依赖只指向内层: app → adapters → application → domain
add_subdirectory(src/domain)
add_subdirectory(src/application)
add_subdirectory(src/adapters)
add_subdirectory(src/ui)
add_subdirectory(src/app)

include(GNUInstallDirs)
install(TARGETS SuperLottoNumberGenerator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── 单元测试子目录 ──
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
