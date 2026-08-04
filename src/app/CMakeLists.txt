# 应用层: lotto_app 静态库(控制器 + 视图)与可执行目标
# 拆分为库+薄 exe: 测试可链接库而非手工编译视图源码

add_library(lotto_app STATIC
    lottocontroller.h lottocontroller.cpp
    mainwindow.h mainwindow.cpp
    mainwindow.ui
    resources.qrc
)

target_include_directories(lotto_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_app PUBLIC lotto_core Qt${QT_VERSION_MAJOR}::Widgets)

# ── 可执行目标(Qt6 / Qt5 双路径, Android manifest 只在 exe)──
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SuperLottoNumberGenerator
        MANUAL_FINALIZATION
        main.cpp
        ${CMAKE_SOURCE_DIR}/android/AndroidManifest.xml
    )

    set_target_properties(SuperLottoNumberGenerator PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")
else()
    if(ANDROID)
        add_library(SuperLottoNumberGenerator SHARED main.cpp)
    else()
        add_executable(SuperLottoNumberGenerator main.cpp)
    endif()
endif()

target_link_libraries(SuperLottoNumberGenerator PRIVATE
    lotto_app lotto_adapters lotto_core)

# 可执行输出到构建根目录(子目录默认会把 exe 放到 src/app/, 改变 QSettings INI 落地位置)
set_target_properties(SuperLottoNumberGenerator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Qt for iOS 自 6.1 起自动设置 MACOSX_BUNDLE_GUI_IDENTIFIER
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SuperLottoNumberGenerator)
endif()
set_target_properties(SuperLottoNumberGenerator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SuperLottoNumberGenerator)
endif()
