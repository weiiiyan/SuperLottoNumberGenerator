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

# Android 调试构建（用 Qt 自带的 qt.toolchain.cmake）
# 注意: 直接用 NDK 的 android.toolchain.cmake 会让 Qt 的 QML 插件导入目标缺失,
# qt_finalize_executable 报 "No target Qt6::QDebugMessageServiceFactoryPlugin"
cmake -B build/Qt_6_11_1_for_Android_arm64_v8a-Debug -G Ninja \
  -DCMAKE_MAKE_PROGRAM=D:/Software/Qt6.11/Tools/Ninja/ninja.exe \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=D:/Software/Qt6.11/6.11.1/android_arm64_v8a/lib/cmake/Qt6/qt.toolchain.cmake \
  -DQT_HOST_PATH=D:/Software/Qt6.11/6.11.1/mingw_64 \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_SDK_ROOT=D:/Software/AndroidSDK \
  -DANDROID_NDK_ROOT=D:/Software/AndroidSDK/ndk/27.2.12479018 .
cmake --build build/Qt_6_11_1_for_Android_arm64_v8a-Debug

# 部署到 Android 设备（APK 落点以构建日志末尾的 File: 行为准）
adb install -r build/Qt_6_11_1_for_Android_arm64_v8a-Debug/src/app/android-build/build/outputs/apk/debug/android-build-debug.apk

# 清理构建
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target clean
```

构建产物位于 `build/`。

### 测试

```bash
# 运行全部测试（14 个 target：LottoEngineTest / LottoInteractorTest /
# QSettingsRepositoryTest / LottoControllerTest / LottoPresenterTest /
# MainWindowTest / LottoUiTest / LottoUiIsolationTest / LottoTicketViewTest /
# LottoViewShellTest / LottoNeumorphicViewTest / LottoQmlSmokeTest /
# LottoMainCatalogTest / QSettingsStyleStoreTest）
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

测试 target 链接各层库（`lotto_core` / `lotto_application` / `lotto_adapters` / `lotto_ui` / `lotto_ui_ticket` / `lotto_ui_neumorphic` / `lotto_main`），不手工编译被测源码。Android 构建自动跳过测试。

#### 各层测试的替身策略（与下游实现解耦）

每个测试 target 只测本层行为，**跨层协作者一律注入接口替身**（见 [tests/testhelpers.h](tests/testhelpers.h)），使缺陷能精确定位到某一层——下游模块行为变化不再连带本层测试变红：

- `LottoControllerTest`（输入侧适配器）注入 `FakeInputBoundary`，只断言"点击 → 用例调用"的翻译，不装配真交互器/引擎
- `LottoInteractorTest`（用例层）注入 `FakeTicketRepository` + `FakePresenterPort` + `FakeLottoEngine`，断言编排（引擎输出写入票据 / 锁定守卫 / 状态变化保存 / 经输出端口推送）；号码合法性属引擎契约，由 `LottoEngineTest` 承担
- `LottoPresenterTest`（输出侧适配器）只装配 presenter + `QSignalSpy`，断言 `present(票据) → viewStateChanged` 与 `state()` 缓存，不依赖用例/引擎
- `LottoEngineTest`（领域层）纯逻辑，无替身
- `LottoUiTest`（UI 核心端口）验证视图基类默认行为
- `LottoUiIsolationTest`（UI 核心）只链接 `lotto_ui`，是"核心库不拖入 Qt Quick"的守门测试
- `LottoMainCatalogTest` / `QSettingsStyleStoreTest`（Main）验证具体目录条目、偏好存储与 `wireStyleCatalog` 接线（首帧/请求/重渲染）
- `LottoTicketViewTest`（票据视图）注入 `FakeStyleHost`，验证状态渲染、请求信号、票根文案与风格抽屉
- `LottoViewShellTest`（视图宿主）注入假视图目录（`FakeLottoView`）+ 假偏好，只断言默认/恢复风格、非法 id、失败不写偏好、过渡覆盖层生命周期；不装配业务端口
- `LottoNeumorphicViewTest` / `LottoQmlSmokeTest`（QML）验证模块进资源、属性推送与信号回环；平台 Quick 不可用时 `QSKIP`
- `MainWindowTest` 是唯一的集成测试：真实装配全链（真交互器/引擎/仓储 + 同一临时文件），验证信号管线、布局与持久化 round-trip；原始目录先经 `wireStyleCatalog` 再注入窗口，切换用例注入「票据视图 + 假视图」混合目录，另有一条真实目录冒烟用例

配套：`LottoEngine::generate/generateBatch` 为 `virtual`，供 `FakeLottoEngine` 继承覆写返回固定号码——用例层断言从"号码看起来合法"变为精确确定性的"引擎输出原样写入票据"。

## 架构

**SuperLottoNumberGenerator** — 基于 Qt 6 Widgets 的大乐透随机号码生成器，同时面向桌面端与 Android。

### 分层

按整洁架构（Clean Architecture）分为**四个圆 + Main**，**依赖只指向内层**：

```text
src/domain/       → lotto_core 库：业务实体（Entities，仅 Qt::Core）
src/application/  → lotto_application 库：用例（Use Cases）+ 三个 DIP 端口
                    TicketRepository / LottoPresenterPort / LottoInputBoundary
src/adapters/     → lotto_adapters 库：接口适配器（Interface Adapters）
                    存储侧 QSettingsTicketRepository + QSettingsStyleStore +
                    输入侧 LottoController + 输出侧 LottoPresenter +
                    展示状态 DTO lottoviewstate.h + 风格偏好端口 lottostylestore.h（仅 Qt::Core）
src/ui/           → lotto_ui 库：框架与驱动（Frameworks & Drivers）的 UI 核心
                    视图端口 LottoView + 风格端口 LottoStyleHost + 宿主 LottoViewShell
                    (只承载与切换) + 过渡覆盖层 + 窗口壳 MainWindow。
                    不含任何具体风格, 不依赖 Qt Quick, 不感知业务端口
src/ui/ticket/    → lotto_ui_ticket 库：票据风格组件（Widgets + QSS）
src/ui/neumorphic/→ lotto_ui_neumorphic 库：新拟物风格组件（Qt6 QML + QQuickWidget）
src/app/          → lotto_main 库 + main.cpp：Main 组件（圆外）
                    具体风格目录 buildStyleCatalog() + 接线 wireStyleCatalog() + 组合根
tests/            → 14 个测试 target
```

**依赖图（全部指向内层, 无环）**：`lotto_ui_ticket → lotto_ui`,
`lotto_ui_neumorphic → lotto_ui`, `lotto_ui → lotto_adapters →
lotto_application → lotto_core`, `lotto_main → 全部`。**运行期没有任何组件
依赖 `lotto_main`**，只有可执行目标与 Main 级测试链接它——所以它不是一层，
而是 ch26 所说的"最脏的一层"。

**为何风格是独立组件（可插拔）**：链接 `lotto_ui` 的目标不被迫拖入 Qt Quick；
只想用票据视图的产品可以只链接 `lotto_ui_ticket`。新增一种风格 = 新建一个
只依赖 `lotto_ui` 的组件 + 在 `src/app/maincatalog.cpp` 追加一行，核心库与
宿主零改动。守门测试 `LottoUiIsolationTest` 只链接 `lotto_ui`，它的链接行里
一旦出现 `Quick` 就说明边界破了。

与《架构整洁之道》洋葱图的映射：`domain`=业务实体（Entities）、`application`=用例（Use Cases）、`adapters`=接口适配器层（Interface Adapters，存储/输入/输出三个端口：Gateway/Controller/Presenter）、`ui`（含窗口壳与各风格组件）=框架与驱动程序（Frameworks & Drivers）、`src/app/main.cpp` 与 `lotto_main`=Main 组件（洋葱图外）。书的图 22.2 把展示器、视图、控制器画在同一个圆里——本项目的 `src/ui/` 就是那个圆里"视图"的部分。

**为何没有"app 层"（有意权衡）**：早期版本把 `MainWindow` 放在单独的
`lotto_app` 库，声称它是第五层。按 ch22，层数必须对应"距 I/O 的距离"；
`MainWindow` 与视图宿主同属最外圈，单独成层只会让依赖图失真。现在
`MainWindow` 在 `lotto_ui` 内，`src/app/` 只是 Main 组件的物理位置。

**为何按层封装而非按用例/功能划分目录（有意权衡）**：第 21 章"尖叫的架构"主张顶层目录反映用例（如 `patients/`、`diagnoses/`），第 34 章也指出按层封装（Package by Layer）的隐患——不同业务领域的分层代码看起来惊人相似，无法展现领域信息。本项目仍按层划分，理由：(1) 本系统只有**一个用例**（生成/锁定/恢复一注大乐透号码），用例即系统本身，层目录只是洋葱壳，不会被误读为别的系统；(2) "尖叫"由类名承担——全项目 `Lotto*` 前缀（`LottoGroup`/`LottoTicket`/`LottoEngine`/`LottoInteractor`/`LottoPresenter`），新程序员打开任何文件都知道这是什么系统；(3) 本项目用于教学，目录与洋葱图一一对应本身就是教学目标。**触发条件**：若未来出现第二个可区分用例（历史记录、统计分析、多玩法等），层目录将开始埋没用例，届时应按用例纵向切分——每个用例一个组件，自带交互器/展示器/仓储端口（参考第 33 章案例分析），而不是继续在层内堆叠。

**输入/输出侧适配器为何统一归入 `adapters/`**：按洋葱图，Controller（输入侧）与 Presenter（输出侧）都是接口适配器，与存储侧 Gateway（QSettingsTicketRepository）同层对称。物理上统一放 `src/adapters/`，`src/app/` 只保留框架与驱动（MainWindow/QSS）。曾有意让展示器与谦卑视图同库配对（ch24 不完全边界），本次已按"严格整洁架构"收口：适配器不依赖 Widget、依赖只指向内层的约束由库边界强制，视图与适配器解耦后各自可独立测试与替换。

跨层数据只传 `LottoTicket`（值类型）：`LottoController`（输入侧）经输入边界 `LottoInputBoundary` 调用 `LottoInteractor`（用例层）；`LottoInteractor` 经 `TicketRepository` 接口（用例层）→ `QSettingsTicketRepository`（adapters 层）持久化；状态经输出边界 `LottoPresenterPort::present(LottoTicket)` 由 `LottoPresenter` 适配为 `LottoViewState` 再交视图——**视图只感知 `LottoViewState`，不感知领域实体 `LottoTicket`**。

领域层使用 Qt::Core 类型（`QDateTime`/`QVector`/`QObject`）是**有意权衡**：本项目以 Qt 为应用语言，不追求与框架无关的纯领域，但绝不依赖任何 UI/存储框架。

### 核心领域：[src/domain/](src/domain/)

- `LottoGroup`（[lottogroup.h](src/domain/lottogroup.h)）— 纯数据 struct，两个已排序向量：`front`（5 个号码，范围 1–35）和 `back`（2 个号码，范围 1–12）。**组级游戏规则常量** `FRONT_COUNT`/`BACK_COUNT`/`FRONT_MIN`/`FRONT_MAX`/`BACK_MIN`/`BACK_MAX` 定义于此（全项目唯一来源）。通过 `Q_DECLARE_METATYPE` 注册为 Qt 元类型。
- `LottoTicket`（[lottoticket.h](src/domain/lottoticket.h)）— 领域实体：5 组号码 + `QDateTime` 生成时间戳 + 锁定标志。票据级常量 `GROUP_COUNT` 定义于此；`FRONT_COUNT`/`BACK_COUNT` 为 `LottoGroup::*` 的别名（组级规则唯一来源是 LottoGroup）。

**锁定标志为何随实体存储（有意权衡）**：按整洁架构 ch20 定义，`isLocked` 属"因自动化而生的"应用特定规则——现实彩票无锁定概念（买票即固定），严格应归用例层；"锁定时拒绝生成"的判定也确已位于 `LottoInteractor::generateNewTicket()`。物理上随 `LottoTicket` 存储是为了保持跨层传输唯一值类型载体，避免 save/渲染时拼装「票据 + 锁标志」；实体仅持有数据、不含锁定逻辑，污染限于数据层面。

- `LottoEngine`（[lottoengine.h](src/domain/lottoengine.h)）— 领域服务：使用 `std::shuffle` 对 `std::iota` 填充的号码池随机洗牌，随机引擎为 `QRandomGenerator::global()`。不使用 `rand()`，不手动 seed。`generate()`/`generateBatch()` 为 `virtual`，供测试替身 `FakeLottoEngine`（[tests/testhelpers.h](tests/testhelpers.h)）覆写返回固定号码。

### 用例层：[src/application/lottointeractor.h](src/application/lottointeractor.h)

`LottoInteractor` 是用例交互器（use case interactor），而非 MVC 控制器——它不做任何输入/输出格式适配，只执行应用特定业务规则与编排：`generateNewTicket()`（锁定时拒绝）、`toggleLock()`/`setLocked()`（状态变化守卫防重复保存）、`load()`（`singleShot(0)` 异步恢复，不阻塞 Android 启动，**由组合根在窗口构造后调用**）。持有**唯一状态源** `LottoTicket`，实现输入边界 `LottoInputBoundary`，状态变化后经输出边界 `LottoPresenterPort::present()` 把票据推送给出输出适配器——**用 DIP 端口主动调用，而非 Qt 信号广播**（教学上更贴近书图 22.2 的请求/响应）。仅依赖 Qt::Core，不接触任何 UI/存储具体类。

**Controller 与 Interactor 的角色分离（已落实）**：《架构整洁之道》图 22.2 中 Controller（接口适配器层）与 UseCaseInteractor（用例层）是两个角色——前者把外部输入（HTTP 请求/UI 事件）打包成用例理解的输入，后者执行业务编排。本项目 `LottoController`（[lottocontroller.h](src/adapters/lottocontroller.h)）= 输入侧适配器，依赖用例层输入边界 `LottoInputBoundary`（由交互器实现），把按钮点击翻译为边界调用（`onGenerateRequested → generateNewTicket()`、`onLockRequested → toggleLock()`）；`LottoInteractor` = 用例层交互器，实现输入边界、保持零 I/O 格式适配。锁定时拒绝生成等应用规则仍在用例层（按钮禁用只是辅助，用例层判定权威），Controller 不重复业务规则。

`TicketRepository`（[ticketrepository.h](src/application/ticketrepository.h)）— 用例层持久化接口（DIP 端口）：`save/load/clear` 纯抽象，由适配器层实现。`LottoPresenterPort`（[lottopresenterport.h](src/application/lottopresenterport.h)）— 用例层输出边界端口：`present(LottoTicket)` 纯抽象，由展示器实现，交互器在状态变化后调用。两个端口均归用例层所有（严格整洁架构），物理上位于 `src/application/`。

### UI 组件层：[src/ui/](src/ui/)

视图是可替换组件：每套风格一个 `LottoView` 子类，视图之间互不认识，由 `LottoViewShell` 宿主承载并可在运行时切换。

```text
src/ui/lottoview.h/.cpp          视图端口：renderState / isReady / styleChanged / snapshot + 两个请求信号
                                 只包含 lottoviewstate.h, 不包含展示器, 不感知领域实体
src/ui/lottostylehost.h          宿主端口：availableStyles / currentStyleId / requestSwitchTo(视图自行调用)
src/ui/lottostylecatalog.h       仅 LottoStyleDefinition 类型（具体目录在 Main）
src/ui/lottoviewshell.*          宿主：栈/懒创建/切换/过渡/偏好保存/失败回退(不接线)
src/ui/lottostyleswitchoverlay.* 切换过渡覆盖层(旧画面淡出)
src/ui/mainwindow.*              窗口壳：setupUi + 挂宿主（并入 UI 核心, 不是单独一层）
src/ui/ticket/                   lotto_ui_ticket: LottoTicketView + 自绘控件(球/骑缝线/条形码/印章)
                                 + LottoStyleDrawer + style.qss + resources.qrc
src/ui/neumorphic/               lotto_ui_neumorphic: LottoNeumorphicView + LottoUi/*.qml
```

Main 侧（`src/app/`）有 `buildStyleCatalog()` 与 `wireStyleCatalog()`——前者是唯一
允许出现具体风格类名的地方，后者是唯一允许把视图接到控制器/展示器的地方。

偏好端口 `LottoStyleStore`（`src/adapters/lottostylestore.h`）与其 QSettings 实现
（`src/adapters/qsettingsstylestore.*`，键 `ui/styleId`）位于**适配器层**，与
`QSettingsTicketRepository` 同级——风格偏好是呈现层偏好，不是应用业务规则，但
它的实现是存储适配器，不属于 Main 的装配职责，也不与 UI 共库。

**视图只消费 `LottoViewState`**：按钮点击只发 `generateRequested` / `lockRequested`，由 Main 在 `wireStyleCatalog()` 中接到 `LottoController`；视图不持有控制器/展示器、不感知 `LottoInteractor` / `LottoTicket` / 仓储。宿主也不感知 `LottoController` / `LottoPresenter`。

**切换交互由视图自己提供**：视图经构造注入的 `LottoStyleHost` 枚举风格（两套主题统一为右上角按钮 + 底部抽屉），点击后调用 `requestSwitchTo(id)`；宿主排队执行切换、通知两视图 `styleChanged`、播放过渡并保存偏好。新视图由目录工厂懒创建，Main 的接线以展示器 `state()` 缓存渲染首帧。**切换必须在队列中执行**（`QMetaObject::invokeMethod(..., Qt::QueuedConnection)`），否则会在视图自身事件处理中隐藏自己。

**失败路径**：目标 id 不在目录、工厂返回空、或 `isReady()` 为假（如 QML 加载出错）时，宿主保持当前视图、`qWarning` 并发出 `styleSwitchFailed(id)`，**不写偏好**；启动时未知/空偏好回退首项且不覆盖原值。

**新增一种风格**：新建一个只依赖 `lotto_ui` 的组件（例如 `src/ui/retro/`），
在其中实现 `LottoView` 子类（提供 `styleId` / `renderState` / 自己的切换交互），
然后在 `src/app/maincatalog.cpp` 的 `buildStyleCatalog()` 追加一行。**核心库、
宿主、业务层零改动**；若新风格只用于某些产品形态，该产品只需多链接一个组件。

#### 窗口壳：[src/ui/mainwindow.cpp](src/ui/mainwindow.cpp)

`MainWindow` 只剩窗口骨架：`setupUi` → 标题 → `setCentralWidget(new LottoViewShell(...))`；
它位于 UI 核心库，与宿主同圆。`.ui` 文件仅为最小骨架（central widget、menu bar、status bar）——**不要在此项目中使用 Qt Designer 设计 UI**。所有实际控件在视图内部通过代码构建（票据视图）或 QML 声明（新拟物视图）。

#### 票据视图：[src/ui/ticket/lottoticketview.cpp](src/ui/ticket/lottoticketview.cpp)

Widgets + QSS 实现，按 390 × 844 竖屏骨架排布：顶栏（品牌 + 副标题 + 风格按钮）→ 时间条 → 5 行号码带 → 骑缝线 → 两行票根 → 操作区。保留既有控件 objectName（`ticketRoot` / `timeLabel` / `timeValueLabel` / `frontLabel_g_i` / `backLabel_g_i` / `separator_g` / `btnGenerate` / `btnLock` / `btnStyleSwitch` / `groupsContainer`），并新增 `serialLabel` / `stubHint` / `stubCount` / `stampLabel` / `rowIndex_g`。展示推导仍由 `LottoPresenter` 承担，本视图只做机械写入；按钮用 `clicked`（不用 `toggled`，避免程序化 `setChecked` 触发保存回路）。

性能：票面背景（舞台渐变 + 纸纹 + 顶部柔光）只与尺寸有关，缓存进
`m_backdrop`（QPixmap，按 `devicePixelRatio` 渲染），尺寸/DPR 变化才重建；
抽屉等子控件的动画重绘因此只做位块拷贝（实机单帧重绘 6–16ms → 1–2ms）。

自绘控件（QSS 无法表达多重 inset 阴影与条纹）：
- [lottoballlabel.cpp](src/ui/ticket/lottoballlabel.cpp) — 号码球：`QRadialGradient` 双点 + 底部内阴影 + 1px 内描边；`popScale` 属性驱动换号弹入（0.72 → 1，按球序 16ms 错峰）
- [lottoticketdecor.cpp](src/ui/ticket/lottoticketdecor.cpp) — 骑缝线（虚线 + 两端半圆缺口）、条形码、锁定印章（斜 7°，220ms 淡入）
- [lottostyledrawer.cpp](src/ui/ticket/lottostyledrawer.cpp) — 风格切换底部抽屉（遮罩 + 上滑面板）
配色令牌见 [lottotickettheme.h](src/ui/ticket/lottotickettheme.h)（与 `style.qss` 同名色值保持一致）。

#### 新拟物视图：[src/ui/neumorphic/lottoneumorphicview.cpp](src/ui/neumorphic/lottoneumorphicview.cpp)

`LottoNeumorphicView` **组合**一个 `QQuickWidget`（QQuickWidget 自身是 QWidget，无法与 `LottoView` 多重继承），QML 场景在 `src/ui/neumorphic/LottoUi/`。C++ 侧把 `LottoViewState` 写成根对象属性（`groups` / 时间与按钮文案 / 三个布尔 / 票根文案 / `styles` / `currentStyleId`），QML 侧以 `generateClicked` / `lockClicked` / `styleSwitchRequested(string)` 信号回传。骨架与票据版逐像素对齐：`NeumoPerf.qml`（骑缝线）、`NeumoStub.qml`（两行票根 + 印章）、`NeumoStyleSheet.qml`（底部抽屉）；`NeumoPill.qml` 已删除。视觉由 `RectangularShadow`（QtQuick.Effects）做双向柔影，按下/选中改为阴影反向 + 底色加深形成内陷感；图标用 `Canvas` 自绘，不引入 Qt6::Svg。骨架常量在 `NeumoTheme.qml` 中镜像 [lottolayoutmetrics.h](src/ui/lottolayoutmetrics.h)。

#### 输出侧适配器（展示器）：[lottopresenter.h](src/adapters/lottopresenter.h)

`LottoPresenter` 按谦卑对象模式提取所有可测试的展示逻辑（不依赖任何 Widget）：实现用例层输出边界 `LottoPresenterPort`，交互器调用 `present(LottoTicket)` 时，经静态纯函数 `buildViewState(LottoTicket) → LottoViewState` 推导展示文本（号码格式化/`?` 占位符/时间标签/按钮文字与互斥状态），更新 `state()` 缓存并重发 `viewStateChanged(LottoViewState)` 信号供视图消费。展示**文案与结构常量不在此定义**：它们住在 `src/adapters/lottoviewstate.h` 的
`LottoViewText::*` 与 `LottoViewShape::*`（见下文"展示状态与文案约定"）。
本类只保留 `formatNumber()` / `buildSerial()` / `buildViewState()` / `initialState()` / `state()`。
`initialState()` 由构造函数用于播种 `state()`；懒创建视图由 Main 的接线以 `state()` 渲染首帧，**视图不得调用它**。

#### 共用骨架与尺寸令牌

两套主题共用同一张骨架（设计文档 [design-spec.html](doc/界面设计/design-spec.html)）：顶栏 66 → 时间条 60 → 号码带（5 行 × 76，行距 0）→ 骑缝线 26 → 两行票根 105 → 操作区 81（按钮高 56）。左右安全边距 22、上 40、下 34；球径上限 40、球距 4、行号列宽 18。C++ 侧令牌集中在 [lottolayoutmetrics.h](src/ui/lottolayoutmetrics.h)，QML 侧镜像在 `NeumoTheme.qml`，两处不一致时以设计文档为准。

`LottoTicketView::recomputeMetrics()` 只在尺寸变化（50ms 防抖后）重算：票面矩形（窄屏即宽 ≤ 390 时宽高都满幅；宽屏居中成 390 × 844 卡片）、行高（先压到 68，再压上下边距）、球径（按宽度反算并钳在 16–40）。**号码行不重建**，只调整控件尺寸，因此 `rebuildGroupRows()` / `clearLayout()` / 四档间距逻辑已删除。主布局设 `QLayout::SetNoConstraint`，窗口变小时压缩而不是撑出滚动条。

彩票格式常量（`GROUP_COUNT` 等）别名 `LottoViewShape::*`（定义于 `lottoviewstate.h`，其 `GROUP_COUNT` 由 `static_assert` 与 `LottoTicket::GROUP_COUNT` 绑定），视图不直接依赖领域常量。

#### 号码显示格式

号码零填充为两位（如 3 → "03"）由 `LottoPresenter::formatNumber()`（[lottopresenter.h](src/adapters/lottopresenter.h)）承担。配色由各风格自行决定：票据视图前区红色、后区蓝色；新拟物视图取同色系的低饱和变体。

#### 展示文案约定（去 emoji）

`LottoViewState` 只下发语义化文案，**不得包含 emoji 或图标字符**：`timeLabelText`（"生成时间："）与 `timeValueText`（时间值/占位）分开下发，另有 `generateButtonText`、`lockButtonText`；票根的 `serialText` / `stubHintText` / `stubCountText` / `stampText` 与顶栏 `appTitleText` / `headerMetaText` 同样由展示器推导（票号取生成时间的确定性 `NO. yyMMdd NNNN`，重绘与重启不跳变）。图标属装饰，由各视图自选（票据视图纯文字，新拟物视图用 `Canvas` 自绘）。测试 `LottoPresenterTest::constants_shouldNotContainEmoji` 守住该约定。

### 样式系统：[ticket/style.qss](src/ui/ticket/style.qss)

票据视图的样式集中于此 QSS 文件，通过 Qt 资源系统加载（`:/ticket/style.qss`）。只放颜色/描边/背景：使用 ID 选择器（`#btnGenerate` / `#btnLock`）、动态属性选择器（`[last="true"]` / `[current="true"]`）和通配前缀选择器（`[objectName^="ticketRow_"]`）。**字号、字重与字距在代码中按设计稿设置**（QSS 不支持 letter-spacing）；号码球、骑缝线、条形码与印章是自绘控件，配色见 `lottotickettheme.h`。

### 资源文件：[ticket/resources.qrc](src/ui/ticket/resources.qrc)

Qt 资源文件，将票据视图的 `style.qss` 打包到 `:/ticket/` 前缀下。**初始化由
`LottoTicketView` 构造函数内的 `Q_INIT_RESOURCE(resources)` 自己完成**——入口与
测试都不再手工初始化，资源属于组件实现细节。新拟物视图的 QML 由
`qt_add_qml_module` 打进 `:/qt/qml/LottoUi/`。

### 持久化：[src/adapters/qsettingsrepository.cpp](src/adapters/qsettingsrepository.cpp)

`QSettingsTicketRepository` 实现 `TicketRepository` 接口。键名与旧版本完全兼容：`isLocked`（bool）、`frontNumbers`（25 个扁平 int）、`backNumbers`（10 个扁平 int）、`generateTime`（**ISO 字符串**，读取时兼容旧版显示字符串 "🕐 生成时间：..."）。锁定时写全量 + `sync()`；未锁定仅写 `isLocked=false`。恢复流程保持 `singleShot(0)` 异步语义（由 `LottoInteractor::load()` 承担，加载结果经 presenter 适配后渲染）。QSettings 路径由组合根注入（相对路径 `SuperLottoNumberGenerator.ini`），测试注入 `QTemporaryDir` 文件隔离。

### QML 构建约束（Qt6）

`lotto_ui_neumorphic` 用 `qt_add_qml_module(lotto_ui_neumorphic URI LottoUi ...
QML_FILES LottoUi/*.qml)` 定义 QML 模块；QML 源文件位于
`src/ui/neumorphic/LottoUi/`（与 URI 的 target path 一致，qmlcachegen 才不会
生成带 `..` 的路径）。单例 `NeumoTheme.qml` 需在加入模块前
`set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)`。由于是静态库，
qrc/qmldir 随该目标一起被链接；`LottoNeumorphicView` 在 `loadFromModule` 前
调用 `engine()->addImportPath(":/")`。

新拟物视图相关代码全部包在 `if(QT_VERSION_MAJOR GREATER_EQUAL 6)` 内：CMake
只在 Qt6 `add_subdirectory(neumorphic)`，`buildStyleCatalog()` 在 Qt 5 下只
注册票据视图。根 `CMakeLists.txt` 在 Qt6 下显式 `find_package(Qt6 REQUIRED COMPONENTS Qml QuickWidgets)`——**缺少这一行时 Android 的 `qt_finalize_executable` 会因找不到 QML 插件导入目标而失败**。

### Android 支持

`android/` 目录包含标准 Qt Android 部署文件：`AndroidManifest.xml`、`build.gradle`、Gradle wrapper 以及 `res/mipmap-*` 中的启动图标。CMake 通过 `QT_ANDROID_PACKAGE_SOURCE_DIR` 指向此目录。

**全屏（edge-to-edge）**：`MainWindow` 在 Android（Qt 6.9+）上做三件事：
`setWindowFlag(Qt::ExpandedClientAreaHint, true)` 让窗口铺满整屏（Android QPA 走
`showExpanded`）；`setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false)`
关闭 Qt 6.9 起的"安全区并入 contentsMargins"——否则中央控件会被状态栏/手势条
再缩一次（实机日志：窗口 375×834，central widget 只有 375×752）；`statusBar()->hide()`
——设计骨架没有状态栏，否则底部再白占 24dp。之后两套视图把窗口安全区并入
自己的上/下边距：`LottoView::windowSafeArea()` 读 `QWindow::safeAreaMargins()`，
`LottoLayout::effectivePadTop/PadBottom()` 取 `max(设计令牌, 安全区)`（40/34 是设计
下限，系统栏更高时自动让位）；窗口安全区变化时经 `safeAreaMarginsChanged` 重算
（票据视图 `recomputeMetrics()`，QML 侧由 C++ 推送 `safeTop`/`safeBottom` 属性，
QML 内取 `Math.max`）。票面在窄屏下必须**宽高都满幅**，不得按画布高 844 截断。

APK 现状（Qt 6.11.1，arm64-v8a，debug）：约 24 MB，已包含 `libQt6Quick`、`libQt6QuickWidgets`、`libQt6QuickEffects`、`libQt6QuickLayouts`；`LottoUi` 的 QML 以资源形式编入应用库（已验证 APK 内 `.so` 含 `LottoUi/Neumo*` 资源名）。低端设备上 `QQuickWidget` 的性能与包体是否可接受仍需真机实测——若不可接受，新拟物视图可退回 Widgets 自绘，`LottoView` 端口以上无需改动。

### LSP / 工具链

`scripts/clangd-uri-fixer.js` — JSON-RPC 代理，修复 Claude Code 在 Windows 下传给 clangd 的文件 URI 格式问题（`file://c:/...` → `file:///C:/...`）。LSP 配置位于 `.lsp.json`。构建时通过 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 生成 `compile_commands.json` 供 clangd 使用。

**注意**：修改 CMakeLists.txt（增删源文件、更改编译选项）后需重新运行 cmake 配置以刷新 `compile_commands.json`，否则 clangd 可能报错找不到文件或索引过期。

### 关键代码模式

| 模式 | 位置 | 说明 |
| ---- | ---- | ---- |
| `LottoPresenter::buildViewState()` | src/adapters/lottopresenter.h | 展示推导（号码格式化/占位符/按钮互斥/时间格式），纯函数供单测 |
| `LottoPresenter::present()` | src/adapters/lottopresenter.cpp | 实现输出边界：推导后更新 `state()` 缓存并重发 viewStateChanged 信号 |
| `LottoPresenter::state()` | src/adapters/lottopresenter.h | 最近一次展示状态缓存，懒创建视图的首帧来源 |
| `LottoPresenterPort` | src/application/lottopresenterport.h | 用例层输出边界端口，交互器状态变化后调用 |
| `LottoInputBoundary` | src/application/lottoinputboundary.h | 用例层输入边界接口，Controller 依赖、交互器实现 |
| `LottoPresenter::viewStateChanged` | src/adapters/lottopresenter.h | 输出侧适配器重发信号，视图唯一渲染入口 |
| `LottoController::onGenerateRequested/onLockRequested` | src/adapters/lottocontroller.cpp | 输入侧适配器：按钮点击 → 用例调用翻译 |
| `LottoViewState` / `LottoViewText` / `LottoViewShape` | src/adapters/lottoviewstate.h | 展示状态 DTO + 展示文案 + 视图结构常量（单一来源，Q_DECLARE_METATYPE） |
| `formatNumber()` | src/adapters/lottopresenter.h | 零填充两位数字（inline，UI 与测试复用） |
| `LottoView` | src/ui/lottoview.h | 视图端口（renderState/isReady/styleChanged/snapshot + 请求信号） |
| `LottoStyleHost` | src/ui/lottostylehost.h | 宿主端口：风格清单/当前风格/切换请求，视图自行调用 |
| `LottoStyleStore` / `QSettingsStyleStore` | src/adapters/lottostylestore.h | 风格偏好端口与 INI 实现（键 `ui/styleId`） |
| `buildStyleCatalog()` | src/app/maincatalog.cpp | 具体风格目录（未接线），首项默认；新增风格只改这里 |
| `wireStyleCatalog()` | src/app/maincatalog.cpp | Main：创建 + 接线 + 首帧渲染的唯一位置 |
| `LottoViewShell::requestSwitchTo()` | src/ui/lottoviewshell.cpp | 切换排队执行，防在视图事件中隐藏自己 |
| `LottoViewShell::applySwitch()` | src/ui/lottoviewshell.cpp | 查找/懒创建视图、通知、存偏好、失败回退 |
| `LottoStyleSwitchOverlay` | src/ui/lottostyleswitchoverlay.* | 切换过渡：旧视图快照淡出，快速连击时先结束上一次 |
| `renderState()` / `applyState()` | lottoticketview.cpp | 票据视图渲染唯一入口（初始渲染与信号渲染共用） |
| `NeumoIcon.kind` | src/ui/neumorphic/LottoUi/NeumoIcon.qml | Canvas 自绘图标（generate/lock/style），不依赖 Svg 模块 |
| `setLocked()` 状态守卫 | src/application/lottointeractor.cpp | 相同状态不重复保存 |
| `hasNumbers()` | src/domain/lottoticket.cpp | 实体层"是否含号码"判定，展示器据此推导锁定按钮可用性 |
| `parseGenerateTime()` | qsettingsrepository.cpp | 时间 ISO 优先、旧显示串正则回退、无效兜底 |
| `LottoBallLabel::paintEvent` | src/ui/ticket/lottoballlabel.cpp | 自绘号码球（径向渐变 + 内阴影 + 描边），`popScale` 驱动弹入 |
| `LottoPerfLine` / `LottoBarcode` / `LottoStamp` | src/ui/ticket/lottoticketdecor.* | 骑缝线（半圆缺口）/ 条形码 / 锁定印章 |
| `LottoStyleDrawer` | src/ui/ticket/lottostyledrawer.* | 底部风格抽屉（遮罩 + 上滑面板） |
| `NeumoPerf` / `NeumoStub` / `NeumoStyleSheet` | src/ui/neumorphic/LottoUi/*.qml | 新拟物侧的骑缝线 / 两行票根 / 底部抽屉 |
| `recomputeMetrics()` | src/ui/ticket/lottoticketview.cpp | 票面矩形、行高压缩、球径反算的唯一入口 |
| `windowSafeArea()` / `effectivePadTop/PadBottom()` | src/ui/lottoview.cpp / lottolayoutmetrics.h | 全屏安全区并入设计上/下边距，取较大者 |
| `m_layoutDebounceTimer` | src/ui/ticket/lottoticketview.cpp | 50ms 防抖定时器，解决 Android resizeEvent 自激震荡 |

### 避坑指南

- **不要删除票据视图 resizeEvent 中的 50ms 防抖定时器**：Android 端宽度可能在相近值间反复跳动（如 448↔800），导致 `resizeEvent → recomputeMetrics → setFixedSize → resizeEvent` 自激震荡。`m_layoutDebounceTimer`（singleShot, 50ms）将多次 resize 合并为一次布局更新。QML 视图无此问题。
- **新增源文件加入所属层的 CMakeLists.txt**：`src/domain/`、`src/application/`、`src/adapters/`、`src/ui/`、`src/ui/ticket/`、`src/ui/neumorphic/`、`src/app/` 各有独立 `CMakeLists.txt`。**QML 文件要加进 `qt_add_qml_module` 的 `QML_FILES`**，否则不会进资源。改 CMake 后需重新运行 cmake 配置以刷新 `compile_commands.json`。
- **静态库的 Qt 依赖必须 PUBLIC**：`lotto_core`/`lotto_application`/`lotto_adapters`/`lotto_ui`/`lotto_ui_ticket`/`lotto_ui_neumorphic`/`lotto_main` 用 `target_link_libraries(... PUBLIC Qt::...)`，PRIVATE 不会向下游传递 Qt 头文件路径，导致测试/可执行编译找不到 Qt 头。
- **Android 全屏要连 `WA_ContentsMarginsRespectsSafeArea` 一起关**：只设 `ExpandedClientAreaHint` 时窗口虽已是整屏，`QWidget::contentsMargins()` 仍会把 `safeAreaMargins()` 并进布局边距，中央控件再缩一圈，且 `QStatusBar` 在底部多占一行。属性必须在 `show()` 之前设置，否则自动置位已生效。
- **动画中的控件不要挂 `QGraphicsDropShadowEffect`**：票据抽屉原用 blurRadius 40 的投影，实机每帧绘制 20–30ms，240ms 上滑动画只跑出 5–6 帧（约 12fps）；现改为在抽屉 `paintEvent` 里画渐变柔影（`paintSheetShadow()`），同动画 14 帧（≈58fps）。需要柔影时优先缓存成 pixmap 或用渐变近似。
- **手绘柔影要自己维护失效区域**：柔影画在面板矩形之外，Qt 只按面板新旧矩形失效重绘，面板滑动时旧柔影会留在画面上（收起时出现拖影）。抽屉在动画每帧调 `updatePaintFrame()`，把上一帧与当前帧的「面板 + 柔影」范围一并 `update()`。
- **柔影裁剪要用正圆弧**：面板圆角是 QSS 的 20px 圆弧，柔影裁剪若用 `quadTo`（二次贝塞尔）会在角上偏外约 6px，圆角外留下月牙形浅色夹缝；`paintSheetShadow()` 用 `arcTo` 画圆角。
- **窄屏票面不要按画布高 844 截断**：Android 全屏后可用高可能超过 844，`qMin(height, 844)` 会让票面缩成上下留边的圆角卡片。窄屏（宽 ≤ 390）必须宽高都满幅，票据视图 `updateCardRect()` 与 `NeumorphicView.qml` 的 `card` 两处同步。
- **视图的最小尺寸不能随当前尺寸增长**：票据视图的左右居中边距由当前宽度推导，布局 `minimumSizeHint` 会≈当前宽度。运行时创建的页面（构造尺寸 390×844 → 最小宽 388dp）会把窗口/表面最小宽度抬到超过窄屏手机（如 375dp），整页被布局到屏幕之外、右侧内容被裁；该页隐藏后仍留在 `QStackedWidget` 中持续抬高下限，所以切回其它风格也继续错位。`LottoTicketView::minimumSizeHint()` 必须返回与尺寸无关的 `LottoLayout::MinimumWidth/MinimumHeight`（回归测试：`minimumSizeHintStaysBoundedWhenViewGrows`、`ticketMinimumWidthDoesNotBlockWindowShrink`）。
- **不要把具体风格链接进 `lotto_ui`**：核心库一旦依赖 `lotto_ui_ticket` 或 `lotto_ui_neumorphic`，"可插拔"就失效了。`LottoUiIsolationTest` 只链接 `lotto_ui`，它的 `linkLibs.rsp` 里出现任何 `Quick`/`Qml` 都说明边界被破坏。
- **具体风格类名只允许出现在 `src/app/maincatalog.cpp`**：核心库、宿主、窗口壳都不得 include `lottoticketview.h` / `lottoneumorphicview.h`。新增风格时改目录，不改宿主。
- **接线只允许出现在 Main 的 `wireStyleCatalog()`**：宿主与视图不得持有 `LottoController` / `LottoPresenter`；展示器 `state()` 是懒创建视图的首帧来源。
- **展示器/控制器生命周期必须长于窗口与目录工厂**：`wireStyleCatalog()` 的工厂闭包捕获裸指针；Main 中先构造依赖，再构造窗口。
- **新增风格只改 `buildStyleCatalog()`**：接线由 `wireStyleCatalog()` 统一完成，不要在每个风格里自己连。
- **视图用 `clicked` 而非 `toggled` 转发按钮动作**：程序化 `setChecked`（渲染时）会触发 `toggled`，若接 `toggled` 会导致保存回路。按钮 checked 状态完全由 `renderState()` 驱动。
- **`.ui` 文件仅做骨架**：`mainwindow.ui` 只定义 central widget、menu bar、status bar；所有实际 UI 由 `LottoView` 实现提供，不要用 Qt Designer 添加业务控件。
- **展示逻辑放 `LottoPresenter`，视图只调 `renderState()`**：号码格式化、占位符、按钮文字/互斥状态、时间拆分统一由 `LottoPresenter::buildViewState()` 推导；视图只做机械写入。展示文案与结构常量以 `LottoViewText::*` / `LottoViewShape::*`（[lottoviewstate.h](src/adapters/lottoviewstate.h)）为唯一来源，视图与测试不得重复魔法字符串，且**不得含 emoji**。
- **视图不感知领域实体与业务端口**：`LottoView` 实现只依赖 `LottoViewState`（+ `LottoStyleHost`），不得直接包含/引用 `LottoTicket`、`LottoInteractor`、`LottoController`、`LottoPresenter`。**端口头 `lottoview.h` 只包含 `lottoviewstate.h`**——加一个 `#include "lottopresenter.h"` 就会把领域实体带进所有视图的编译作用域。
- **切换视图必须排队执行**：`LottoViewShell::requestSwitchTo()` 用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 延后到事件循环，否则会在视图自身的事件处理中隐藏/销毁自己。切换时先 `finishTransition()` 结束上一个过渡，避免覆盖层叠加。
- **QML 属性不得覆盖基类成员**：例如 `Item` 已有 `enabled`，自定义按钮若声明同名属性会触发 `qmlcache` 警告并改变语义——本项目用 `interactive` 代替。
- **测试夹具为注入式**：`MainWindowTest` 每次通过 `rebuildWindow()` 重建「交互器 + 输入/输出适配器 + 指向同一临时文件的仓储 + 窗口」，原始目录在依赖重建后经 `wireStyleCatalog()` 接线再注入窗口；风格偏好存储 `FakeLottoStyleStore` **跨重建保留**以验证持久化；`rebuildWindow()` 内显式调用 `interactor->load()`（与生产一致：用例启动由组合根发起），恢复断言前必须 `QTest::qWait(50~100)` 等待 `singleShot(0)` 异步恢复。默认目录为「真实票据视图 + 假视图」混合目录，切换用例不依赖 QML 环境。

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
