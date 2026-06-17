# CLAUDE.md

向 Claude Code (claude.ai/code) 提供本仓库的上下文指引，帮助后续会话更准确地理解项目。

## 构建

```bash
# Desktop 调试构建（Qt 6.11.0 或 6.11.1，MinGW 64-bit）
# 版本号取决于当前安装的工具链，下面以 6.11.1 为例：
# 同时生成 compile_commands.json 供 clangd LSP 使用
cmake -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -G "MinGW Makefiles" .
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug

# 运行桌面版
./build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/SuperLottoNumberGenerator.exe

# Android 调试构建（需先设置 ANDROID_NDK_ROOT 环境变量）
cmake -B build/Qt_6_11_1_for_Android_arm64_v8a-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_ROOT%/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a .
cmake --build build/Qt_6_11_1_for_Android_arm64_v8a-Debug

# 部署到 Android 设备
adb install build/Qt_6_11_1_for_Android_arm64_v8a-Debug/android-build-SuperLottoNumberGenerator/build/outputs/apk/debug/android-build-SuperLottoNumberGenerator-debug.apk

# 清理构建
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target clean
```

构建产物位于 `build/`。项目无自动化测试。

## 架构

**SuperLottoNumberGenerator** — 基于 Qt 6 Widgets 的大乐透随机号码生成器，同时面向桌面端与 Android。

### 分层

```text
main.cpp          →  QApplication + MainWindow (showMaximized)
LottoEngine       →  纯号码生成逻辑（QObject, Q_INVOKABLE）
MainWindow        →  全部 UI 构建、事件处理、持久化
style.qss         →  统一样式表（Qt 资源系统加载）
```

### 核心领域：[lottoengine.h](lottoengine.h)

`LottoResult` 是一个纯数据 struct，包含两个已排序向量：`front`（5 个号码，范围 1–35）和 `back`（2 个号码，范围 1–12）。通过 `Q_DECLARE_METATYPE` 注册为 Qt 元类型。

`LottoEngine` 使用 `std::shuffle` 对 `std::iota` 填充的号码池进行随机洗牌，随机引擎为 `QRandomGenerator::global()`。不使用 `rand()`，不手动 seed。方法标记为 `Q_INVOKABLE`（为未来可能的 QML 使用做准备，当前仅 Widgets）。

### UI：[mainwindow.cpp](mainwindow.cpp)

UI **完全通过代码构建**于 `MainWindow::init()` 中。`.ui` 文件仅为最小骨架（central widget、menu bar、status bar）——**不要在此项目中使用 Qt Designer 设计 UI**。

#### 布局（仅竖屏）

项目已锁定为竖屏（Android `screenOrientation="portrait"`）。布局在 `buildLayout()` 中一次性构建（`init()` 调用），不再做横/竖屏切换。

- **竖屏**：5 行纵向排列，通过 `spacingForWidth()` 按可用宽度分四档（XS/SM/MD/LG）动态计算间距和分隔符宽度
- `resizeEvent()` 仅更新标签尺寸和字体；若宽度跨越间距档位阈值，调用 `rebuildGroupRows()` 轻量重建组行
- `clearLayout()` 静态辅助函数递归销毁布局项但保留 widget

#### 标签尺寸计算

`updateLabelSizes(int availableWidth)` 根据可用宽度计算标签尺寸：

- 从容器的实际宽度反推，`std::clamp` 在 `LABEL_MIN_SIZE_PORTRAIT`(22px) 到 `LABEL_MAX_SIZE`(44px) 之间，乘以 `LABEL_ROW_WIDTH_FRACTION`(0.94) 安全系数消化内边距误差
- 字号 = 标签尺寸的 50%，不窄于 10px

所有尺寸常量定义在 [mainwindow.h](mainwindow.h) 的 `public` 区：`GROUP_COUNT`、`FRONT_COUNT`、`BACK_COUNT`、`WIDTH_TIER_*`、`LABEL_*`。

#### 号码显示格式

`formatNumber()` 工具函数将号码零填充为两位（如 3 → "03"），前区号码红色、后区号码蓝色，区之间用 "+" 分隔符。

### 样式系统：[style.qss](style.qss)

所有样式集中于此 QSS 文件，通过 Qt 资源系统加载（`:/style.qss`）。使用 ID 选择器（`#btnGenerate`）、动态属性选择器（`[area="front"]`）和通配前缀选择器（`[objectName^="separator_"]`）区分控件，避免代码中散布样式。

### 资源文件：[resources.qrc](resources.qrc)

Qt 资源文件，将 `style.qss` 打包到 `:/` 前缀下，编译时嵌入二进制。

### 持久化

通过 `QSettings`（INI 格式，文件 `SuperLottoNumberGenerator.ini`）保存/恢复。锁定按钮点击时保存。恢复通过 `QTimer::singleShot(0, ...)` 异步执行，避免阻塞 Android 启动 UI。锁定状态保存：5 组前/后区号码 + 生成时间戳。

### Android 支持

`android/` 目录包含标准 Qt Android 部署文件：`AndroidManifest.xml`、`build.gradle`、Gradle wrapper 以及 `res/mipmap-*` 中的启动图标。CMake 通过 `QT_ANDROID_PACKAGE_SOURCE_DIR` 指向此目录。

### LSP / 工具链

`scripts/clangd-uri-fixer.js` — JSON-RPC 代理，修复 Claude Code 在 Windows 下传给 clangd 的文件 URI 格式问题（`file://c:/...` → `file:///C:/...`）。LSP 配置位于 `.lsp.json`。构建时通过 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 生成 `compile_commands.json` 供 clangd 使用。

**注意**：修改 CMakeLists.txt（增删源文件、更改编译选项）后需重新运行 cmake 配置以刷新 `compile_commands.json`，否则 clangd 可能报错找不到文件或索引过期。

### 关键代码模式

| 模式 | 位置 | 说明 |
| ---- | ---- | ---- |
| `formatNumber()` | mainwindow.cpp | 零填充两位数字 |
| `clearLayout()` | mainwindow.cpp | 递归清空布局项，保留 widget |
| `spacingForWidth()` | mainwindow.cpp | 竖屏四档间距分档（XS/SM/MD/LG） |
| `buildLayout()` | mainwindow.cpp | 构建固定竖屏布局（仅 init 调用一次） |
| `rebuildGroupRows()` | mainwindow.cpp | 间距档位变化时轻量重建组行 |
| `setObjectName()` | mainwindow.cpp | 所有控件均命名，供 QSS 和无障碍访问 |
| `m_layoutDebounceTimer` | mainwindow.cpp | 50ms 防抖定时器，解决 Android resizeEvent 自激震荡 |

### 避坑指南

- **不要删除 resizeEvent 中的 50ms 防抖定时器**：Android 端 centralWidget 宽度可能在相近值间反复跳动（如 448↔800），导致 `resizeEvent → updateLabelSizes → rebuildGroupRows → resizeEvent` 死循环。`m_layoutDebounceTimer`（singleShot, 50ms）将多次 resize 合并为一次布局更新。
- **新增 .cpp/.h 文件需同时加入两处**：`lottoengine.h`/`lottoengine.cpp` 不在 `PROJECT_SOURCES` 中，而是直接传给 `qt_add_executable()`。新增源文件时需同时更新 `PROJECT_SOURCES`（Qt 5 回退路径）和 `qt_add_executable()` 调用（Qt 6 路径）。
- **`.ui` 文件仅做骨架**：`mainwindow.ui` 只定义 central widget、menu bar、status bar。所有实际 UI 控件在 `MainWindow::init()` 中通过代码构建，不要用 Qt Designer 添加业务控件。

## 代码风格

- C++17（CMake 中设定）
- Qt 6 优先，CMake 中 Fallback 到 Qt 5
- `CMAKE_AUTOMOC` / `AUTOUIC` / `AUTORCC` 已启用，MOC 自动运行
- 源码中注释使用中文
