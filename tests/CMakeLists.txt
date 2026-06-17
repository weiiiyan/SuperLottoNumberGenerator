# tests/CMakeLists.txt
# 大乐透号码生成器 — 单元测试构建配置
#
# 注意事项：
#   - 测试仅在桌面平台构建（MinGW/MSVC/Linux/macOS），Android 跳过
#   - LottoEngine 源码直接编译到测试目标中（非库链接）
#   - 使用 CTest 集成，可通过 ctest 或 cmake --build . --target test 运行

# ── 查找 Qt Test（复用父级 CMake 的 QT_VERSION_MAJOR，避免版本不一致）──
set(TEST_QT_MAJOR ${QT_VERSION_MAJOR})
find_package(Qt${TEST_QT_MAJOR} REQUIRED COMPONENTS Test)
message(STATUS "测试: 使用 Qt${TEST_QT_MAJOR}::Test")

# CTest 集成由父级 CMakeLists.txt 的 enable_testing() 负责，此处不重复调用

if(NOT ANDROID)
    # ═══════════════════════════════════════════════════
    # 辅助：推导 Qt 和编译器的 bin 目录，加入 PATH
    # 解决 Windows 上运行测试时找不到 DLL 的问题
    # ═══════════════════════════════════════════════════

    # Qt 的 bin 目录（通过 Qt${TEST_QT_MAJOR}_DIR 推导，消除 Qt6/Qt5 分支重复）
    # Qt6_DIR / Qt5_DIR 典型值: <prefix>/lib/cmake/Qt6
    if(DEFINED Qt${TEST_QT_MAJOR}_DIR)
        cmake_path(GET Qt${TEST_QT_MAJOR}_DIR PARENT_PATH _qt_lib_cmake_dir)
        cmake_path(GET _qt_lib_cmake_dir PARENT_PATH _qt_lib_dir)
        cmake_path(GET _qt_lib_dir PARENT_PATH _qt_prefix)
        set(QT_BIN_DIR "${_qt_prefix}/bin")
    else()
        set(QT_BIN_DIR "")
    endif()

    # 编译器的 bin 目录（MinGW runtime DLL 需要）
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _compiler_dir)
    set(RUNTIME_PATH "${_compiler_dir}")

    if(QT_BIN_DIR)
        set(RUNTIME_PATH "${QT_BIN_DIR}\;${RUNTIME_PATH}")
    endif()

    # ═══════════════════════════════════════════════════
    # 测试目标 1: LottoEngineTest (纯逻辑，无 GUI 依赖)
    # ═══════════════════════════════════════════════════

    set(LOTTO_TEST_SOURCES
        tst_lottoengine.cpp
        ${CMAKE_SOURCE_DIR}/lottoengine.h
        ${CMAKE_SOURCE_DIR}/lottoengine.cpp
    )

    add_executable(LottoEngineTest ${LOTTO_TEST_SOURCES})
    target_include_directories(LottoEngineTest PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(LottoEngineTest PRIVATE Qt${TEST_QT_MAJOR}::Test)
    add_test(NAME LottoEngineTest COMMAND LottoEngineTest)
    set_tests_properties(LottoEngineTest PROPERTIES
        ENVIRONMENT "PATH=${RUNTIME_PATH}\;$ENV{PATH}")
    set_target_properties(LottoEngineTest PROPERTIES
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE FALSE
    )
    message(STATUS "测试目标 LottoEngineTest 已创建")

    # ═══════════════════════════════════════════════════
    # 测试目标 2: MainWindowTest (GUI 集成测试)
    # 需要 Qt::Widgets，运行时需 -platform offscreen
    # ═══════════════════════════════════════════════════

    find_package(Qt${TEST_QT_MAJOR} REQUIRED COMPONENTS Widgets)

    set(GUI_TEST_SOURCES
        tst_mainwindow.cpp
        ${CMAKE_SOURCE_DIR}/mainwindow.h
        ${CMAKE_SOURCE_DIR}/mainwindow.cpp
        ${CMAKE_SOURCE_DIR}/mainwindow.ui
        ${CMAKE_SOURCE_DIR}/lottoengine.h
        ${CMAKE_SOURCE_DIR}/lottoengine.cpp
        ${CMAKE_SOURCE_DIR}/resources.qrc
    )

    add_executable(MainWindowTest ${GUI_TEST_SOURCES})
    target_include_directories(MainWindowTest PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(MainWindowTest PRIVATE
        Qt${TEST_QT_MAJOR}::Test
        Qt${TEST_QT_MAJOR}::Widgets
    )
    add_test(NAME MainWindowTest COMMAND MainWindowTest -platform offscreen)
    set_tests_properties(MainWindowTest PROPERTIES
        ENVIRONMENT "PATH=${RUNTIME_PATH}\;$ENV{PATH}")
    set_target_properties(MainWindowTest PROPERTIES
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE FALSE
    )
    message(STATUS "测试目标 MainWindowTest 已创建")
else()
    message(STATUS "跳过测试目标：当前为 Android 构建（测试仅支持桌面平台）")
endif()
