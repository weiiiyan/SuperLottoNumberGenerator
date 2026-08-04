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

构建产物位于 `build/`。

### 测试

```bash
# 运行全部测试（4 个 target：LottoEngineTest / LottoControllerTest /
# QSettingsRepositoryTest / MainWindowTest）
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

测试 target 链接各层库（`lotto_core` / `lotto_adapters` / `lotto_app`），不手工编译被测源码。Android 构建自动跳过测试。

## 架构

**SuperLottoNumberGenerator** — 基于 Qt 6 Widgets 的大乐透随机号码生成器，同时面向桌面端与 Android。

### 分层

按整洁架构（Clean Architecture）分为三层，**依赖只指向内层**（`app → adapters → domain`）：

```text
src/domain/   → lotto_core 库：实体 + 用例 + 用例层接口（仅 Qt::Core）
src/adapters/ → lotto_adapters 库：QSettings 仓储实现（仅 Qt::Core）
src/app/      → lotto_app 库：控制器 + 谦卑视图（Qt::Widgets）
src/app/main.cpp → 组合根：装配具体依赖（唯一接触 QSettings 具体类的点）
tests/        → 4 个测试 target
```

跨层数据只传 `LottoTicket`（值类型）：`MainWindow`（框架层）→ `LottoController`（app 层）→ `TicketRepository` 接口（domain 层）→ `QSettingsTicketRepository`（adapters 层）。

### 核心领域：[src/domain/](src/domain/)

- `LottoResult`（[lottoresult.h](src/domain/lottoresult.h)）— 纯数据 struct，两个已排序向量：`front`（5 个号码，范围 1–35）和 `back`（2 个号码，范围 1–12）。通过 `Q_DECLARE_METATYPE` 注册为 Qt 元类型。
- `LottoTicket`（[lottoticket.h](src/domain/lottoticket.h)）— 领域实体：5 组号码 + `QDateTime` 生成时间戳 + 锁定标志。**游戏规则常量** `GROUP_COUNT`/`FRONT_COUNT`/`BACK_COUNT` 定义于此。
- `LottoEngine`（[lottoengine.h](src/domain/lottoengine.h)）— 用例：使用 `std::shuffle` 对 `std::iota` 填充的号码池随机洗牌，随机引擎为 `QRandomGenerator::global()`。不使用 `rand()`，不手动 seed。方法标记为 `Q_INVOKABLE`（为未来可能的 QML 使用做准备，当前仅 Widgets）。
- `TicketRepository`（[ticketrepository.h](src/domain/ticketrepository.h)）— 用例层持久化接口（DIP）：`save/load/clear` 纯抽象，由适配器层实现。

### 应用层：[src/app/lottocontroller.h](src/app/lottocontroller.h)

`LottoController` 是应用编排器，持有**唯一状态源** `LottoTicket`：`generateNewTicket()`（锁定时拒绝）、`toggleLock()`/`setLocked()`（状态变化守卫防重复保存）、`load()`（`singleShot(0)` 异步恢复，不阻塞 Android 启动）。通过唯一信号 `ticketChanged(LottoTicket)` 推送状态，视图只依赖此信号渲染。

### UI：[src/app/mainwindow.cpp](src/app/mainwindow.cpp)

UI **完全通过代码构建**于 `MainWindow::init()` 中。`.ui` 文件仅为最小骨架（central widget、menu bar、status bar）——**不要在此项目中使用 Qt Designer 设计 UI**。MainWindow 是谦卑视图：`onTicketChanged()` 纯渲染（号码/时间/按钮互斥状态），按钮 `clicked` 转发给 controller（不用 `toggled`，避免程序化 `setChecked` 触发保存回路）。

#### 布局（仅竖屏）

项目已锁定为竖屏（Android `screenOrientation="portrait"`）。布局在 `buildLayout()` 中一次性构建（`init()` 调用），不再做横/竖屏切换。

- **竖屏**：5 行纵向排列，通过 `spacingForWidth()` 按可用宽度分四档（XS/SM/MD/LG）动态计算间距和分隔符宽度
- `resizeEvent()` 仅更新标签尺寸和字体；若宽度跨越间距档位阈值，调用 `rebuildGroupRows()` 轻量重建组行
- `clearLayout()` 静态辅助函数递归销毁布局项但保留 widget

#### 标签尺寸计算

`updateLabelSizes(int availableWidth)` 根据可用宽度计算标签尺寸：

- 从容器的实际宽度反推，`std::clamp` 在 `LABEL_MIN_SIZE_PORTRAIT`(22px) 到 `LABEL_MAX_SIZE`(44px) 之间，乘以 `LABEL_ROW_WIDTH_FRACTION`(0.94) 安全系数消化内边距误差
- 字号 = 标签尺寸的 50%，不窄于 10px

所有布局尺寸常量定义在 [mainwindow.h](src/app/mainwindow.h) 的 `public` 区：`WIDTH_TIER_*`、`LABEL_*`。彩票格式常量（`GROUP_COUNT` 等）为 `LottoTicket::*` 的别名，实际定义于领域层。

#### 号码显示格式

`formatNumber()` 工具函数将号码零填充为两位（如 3 → "03"），前区号码红色、后区号码蓝色，区之间用 "+" 分隔符。

### 样式系统：[style.qss](src/app/style.qss)

所有样式集中于此 QSS 文件，通过 Qt 资源系统加载（`:/style.qss`）。使用 ID 选择器（`#btnGenerate`）、动态属性选择器（`[area="front"]`）和通配前缀选择器（`[objectName^="separator_"]`）区分控件，避免代码中散布样式。

### 资源文件：[resources.qrc](src/app/resources.qrc)

Qt 资源文件，将 `style.qss` 打包到 `:/` 前缀下，编译时嵌入二进制。

### 持久化：[src/adapters/qsettingsrepository.cpp](src/adapters/qsettingsrepository.cpp)

`QSettingsTicketRepository` 实现 `TicketRepository` 接口。键名与旧版本完全兼容：`isLocked`（bool）、`frontNumbers`（25 个扁平 int）、`backNumbers`（10 个扁平 int）、`generateTime`（**ISO 字符串**，读取时兼容旧版显示字符串 "🕐 生成时间：..."）。锁定时写全量 + `sync()`；未锁定仅写 `isLocked=false`。恢复流程保持 `singleShot(0)` 异步语义（由 `LottoController::load()` 承担）。QSettings 路径由组合根注入（相对路径 `SuperLottoNumberGenerator.ini`），测试注入 `QTemporaryDir` 文件隔离。

### Android 支持

`android/` 目录包含标准 Qt Android 部署文件：`AndroidManifest.xml`、`build.gradle`、Gradle wrapper 以及 `res/mipmap-*` 中的启动图标。CMake 通过 `QT_ANDROID_PACKAGE_SOURCE_DIR` 指向此目录。

### LSP / 工具链

`scripts/clangd-uri-fixer.js` — JSON-RPC 代理，修复 Claude Code 在 Windows 下传给 clangd 的文件 URI 格式问题（`file://c:/...` → `file:///C:/...`）。LSP 配置位于 `.lsp.json`。构建时通过 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 生成 `compile_commands.json` 供 clangd 使用。

**注意**：修改 CMakeLists.txt（增删源文件、更改编译选项）后需重新运行 cmake 配置以刷新 `compile_commands.json`，否则 clangd 可能报错找不到文件或索引过期。

### 关键代码模式

| 模式 | 位置 | 说明 |
| ---- | ---- | ---- |
| `formatNumber()` | mainwindow.h | 零填充两位数字（inline，UI 与测试复用） |
| `ticketChanged(LottoTicket)` | lottocontroller.h | 唯一状态信号，视图唯一渲染入口 |
| `onTicketChanged()` | mainwindow.cpp | 谦卑视图纯渲染（号码/时间/按钮互斥） |
| `setLocked()` 状态守卫 | lottocontroller.cpp | 相同状态不重复保存 |
| `parseGenerateTime()` | qsettingsrepository.cpp | 时间 ISO 优先、旧显示串正则回退、无效兜底 |
| `clearLayout()` | mainwindow.cpp | 递归清空布局项，保留 widget |
| `spacingForWidth()` | mainwindow.cpp | 竖屏四档间距分档（XS/SM/MD/LG） |
| `buildLayout()` | mainwindow.cpp | 构建固定竖屏布局（仅 init 调用一次） |
| `m_layoutDebounceTimer` | mainwindow.cpp | 50ms 防抖定时器，解决 Android resizeEvent 自激震荡 |

### 避坑指南

- **不要删除 resizeEvent 中的 50ms 防抖定时器**：Android 端 centralWidget 宽度可能在相近值间反复跳动（如 448↔800），导致 `resizeEvent → updateLabelSizes → rebuildGroupRows → resizeEvent` 死循环。`m_layoutDebounceTimer`（singleShot, 50ms）将多次 resize 合并为一次布局更新。
- **新增源文件加入所属层的 CMakeLists.txt**：`src/domain/`、`src/adapters/`、`src/app/` 各有独立 `CMakeLists.txt`，新增 .cpp/.h 加入对应 `add_library` 的源列表即可（可执行目标只含 `main.cpp`）。改 CMake 后需重新运行 cmake 配置以刷新 `compile_commands.json`。
- **静态库的 Qt 依赖必须 PUBLIC**：`lotto_core`/`lotto_app` 用 `target_link_libraries(... PUBLIC Qt::Core)`，PRIVATE 不会向下游传递 Qt 头文件路径，导致测试/可执行编译找不到 Qt 头。
- **视图用 `clicked` 而非 `toggled` 转发按钮动作**：程序化 `setChecked`（恢复/渲染时）会触发 `toggled`，若接 `toggled` 会导致保存回路。按钮 checked 状态完全由 `ticketChanged` 信号驱动。
- **`.ui` 文件仅做骨架**：`mainwindow.ui` 只定义 central widget、menu bar、status bar。所有实际 UI 控件在 `MainWindow::init()` 中通过代码构建，不要用 Qt Designer 添加业务控件。
- **测试夹具为注入式**：`MainWindowTest` 每次通过 `makeWindow()` 重建「控制器 + 指向同一临时文件的仓储」，恢复断言前必须 `QTest::qWait(50~100)` 等待 `singleShot(0)` 异步恢复。

## 代码风格

- C++17（CMake 中设定）
- Qt 6 优先，CMake 中 Fallback 到 Qt 5
- `CMAKE_AUTOMOC` / `AUTOUIC` / `AUTORCC` 已启用，MOC 自动运行
- 源码中注释使用中文
- **对用户的输出使用中文**：所有面向用户的解释、说明、询问、报告等内容均使用中文回复
- **行长度上限 100 字符**（遵循 [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style)），注释/文档 80 字符。不要过早换行——短于 100 字符的代码行保持单行
- **注释遵循 [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style) 和 [C++ Documentation Style](https://wiki.qt.io/C%2B%2B_Documentation_Style)**：
  - 类和公共函数使用 `/*! ... */` QDoc 风格文档块，以 `\brief` 开头
  - 成员变量行内文档使用 `/*!< ... */`
  - 不使用装饰性分隔符（`────`、`════` 等）
  - 不在代码注释中使用 emoji（UI 字符串除外）
  - 注释在 80 列以内
