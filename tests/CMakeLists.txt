# tests/CMakeLists.txt
# 大乐透号码生成器 — 单元测试构建配置
#
# 注意事项：
#   - 测试仅在桌面平台构建（MinGW/MSVC/Linux/macOS），Android 跳过
#   - 测试目标链接各层库（lotto_core / lotto_application / lotto_adapters /
#     lotto_app），
#     不再手工编译被测源码
#   - 使用 CTest 集成，可通过 ctest 或 cmake --build . --target test 运行

# ── 查找 Qt Test（复用父级 CMake 的 QT_VERSION_MAJOR，避免版本不一致）──
set(TEST_QT_MAJOR ${QT_VERSION_MAJOR})
find_package(Qt${TEST_QT_MAJOR} REQUIRED COMPONENTS Test)
message(STATUS "测试: 使用 Qt${TEST_QT_MAJOR}::Test")

if(NOT ANDROID)
    # ── 辅助：推导 Qt 和编译器的 bin 目录，加入 PATH ──
    # 解决 Windows 上运行测试时找不到 DLL 的问题

    # Qt 的 bin 目录（通过 Qt${TEST_QT_MAJOR}_DIR 推导，消除 Qt6/Qt5 分支重复）
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

    # ── 统一测试目标辅助函数 ──
    # 建可执行 + 链接 Qt::Test + 非 GUI 子系统(注册 CTest 用 register_lotto_test)
    function(add_lotto_test target)
        add_executable(${target} ${ARGN})
        target_link_libraries(${target} PRIVATE Qt${TEST_QT_MAJOR}::Test)
        set_target_properties(${target} PROPERTIES
            WIN32_EXECUTABLE FALSE
            MACOSX_BUNDLE FALSE
        )
    endfunction()

    # 注册 CTest + PATH 注入(命令参数可附加, 如 -platform offscreen)
    function(register_lotto_test target)
        add_test(NAME ${target} COMMAND ${ARGN})
        set_tests_properties(${target} PROPERTIES
            ENVIRONMENT "PATH=${RUNTIME_PATH}\;$ENV{PATH}")
    endfunction()

    # ── 领域层: 引擎单元测试(纯逻辑, 无 GUI)──
    add_lotto_test(LottoEngineTest tst_lottoengine.cpp)
    target_link_libraries(LottoEngineTest PRIVATE lotto_core)
    register_lotto_test(LottoEngineTest LottoEngineTest)
    message(STATUS "测试目标 LottoEngineTest 已创建")

    # ── 用例层: 控制器状态机测试(注入内存仓储, 无 GUI 依赖)──
    add_lotto_test(LottoInteractorTest tst_lottointeractor.cpp)
    target_link_libraries(LottoInteractorTest PRIVATE lotto_application lotto_core)
    register_lotto_test(LottoInteractorTest LottoInteractorTest)
    message(STATUS "测试目标 LottoInteractorTest 已创建")

    # ── 适配器层: 持久化测试(真文件隔离)──
    add_lotto_test(QSettingsRepositoryTest tst_qsettingsrepository.cpp)
    target_link_libraries(QSettingsRepositoryTest PRIVATE lotto_adapters lotto_core)
    register_lotto_test(QSettingsRepositoryTest QSettingsRepositoryTest)
    message(STATUS "测试目标 QSettingsRepositoryTest 已创建")

    # ── 适配器层: 输入侧控制器测试(事件→用例翻译, 注入内存仓储, 无 GUI 依赖)──
    add_lotto_test(LottoControllerTest tst_lottocontroller.cpp)
    target_link_libraries(LottoControllerTest PRIVATE lotto_adapters lotto_application lotto_core)
    register_lotto_test(LottoControllerTest LottoControllerTest)
    message(STATUS "测试目标 LottoControllerTest 已创建")

    # ── 适配器层: 输出侧展示器测试(纯展示推导 + 订阅桥接, 无 GUI 依赖)──
    add_lotto_test(LottoPresenterTest tst_lottopresenter.cpp)
    target_link_libraries(LottoPresenterTest PRIVATE lotto_adapters lotto_application lotto_core)
    register_lotto_test(LottoPresenterTest LottoPresenterTest)
    message(STATUS "测试目标 LottoPresenterTest 已创建")

    # ── GUI 集成测试: 需要 Qt::Widgets, 运行时需 -platform offscreen ──
    find_package(Qt${TEST_QT_MAJOR} REQUIRED COMPONENTS Widgets)
    add_lotto_test(MainWindowTest tst_mainwindow.cpp)
    target_link_libraries(MainWindowTest PRIVATE
        lotto_app lotto_adapters lotto_application lotto_core
        Qt${TEST_QT_MAJOR}::Widgets)
    register_lotto_test(MainWindowTest MainWindowTest -platform offscreen)
    message(STATUS "测试目标 MainWindowTest 已创建")
else()
    message(STATUS "跳过测试目标：当前为 Android 构建（测试仅支持桌面平台）")
endif()
