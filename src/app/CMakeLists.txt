# Main 组件(圆外): 配置细节 + 具体风格目录 + 可执行目标
# 运行期没有任何组件依赖 lotto_main —— 只有可执行目标与 Main 级测试链接它,
# 因此它不是一层, 而是 ch26 所说的"最脏的一层"

add_library(lotto_main STATIC
    maincatalog.h maincatalog.cpp
)

target_include_directories(lotto_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_main PUBLIC
    lotto_ui lotto_ui_ticket lotto_adapters lotto_core
    Qt${QT_VERSION_MAJOR}::Widgets)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_link_libraries(lotto_main PUBLIC lotto_ui_neumorphic)
endif()

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
    lotto_main lotto_adapters lotto_application lotto_core)

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
