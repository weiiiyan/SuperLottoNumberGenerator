# 架构对齐：四个圆 + Main，风格可插拔 设计文档

日期: 2026-11-20
状态: 已实施(2026-11-20)
范围: 仅调整分层与组件边界, 不改业务行为、不改视觉
依据: 《架构整洁之道》ch22(同心圆/依赖规则)、ch23(谦逊对象/ViewModel)、
ch26(Main 组件)

## 1. 意图

现有实现的功能正确, 但**分层叙事与依赖图不一致**: 声称五层
(`app → ui → adapters → application → domain`), 实际只有三个圆
(Entities / Use Cases / Interface Adapters), 而 `ui` 与 `app` 同属最外圈
(Frameworks & Drivers)。把一个圆拆成两层, 让层数不再对应"距 I/O 的距离",
是"乱"的根源。本次把分层改回书上的四个圆, 并让"新增一套 UI"由组件边界
强制, 而不是靠约定。

行为、视觉、交互一律不变; 现有 11 个测试目标覆盖的行为必须全部保持。

## 2. 目标 / 非目标 / 反目标

目标:

- 目录与依赖图回到四个圆: Entities / Use Cases / Interface Adapters /
  Frameworks & Drivers; `main.cpp` 作为 Main 组件位于圆外。
- 每套风格是一个可插拔组件: 链接 UI 核心不被迫拖入另一套风格的依赖
  (Qt Quick 与 Widgets 互不牵连)。
- 具体工厂与配置细节集中在 Main, 不散落在视图类声明里。
- 视图只消费视图模型, 不生产它, 也不引用展示器类。
- 行为零变化: 现有测试语义不变, 仅迁移链接目标与引用路径。

非目标:

- 不引入 DI 容器(项目规模不需要, `main.cpp` 手工装配即 ch26 的做法)。
- 不重写视图、不改 QSS/QML 观感、不改端口方法签名。
- 不为 Qt 5 新增风格(新拟物继续 Qt6-only)。

反目标(即使技术可用也算失败):

- `ui` 核心组件反向依赖某个具体风格组件。
- 某个风格组件之间互相引用。
- Main 之外的代码出现具体风格类名(如 `LottoClassicView`)。
- 为分层而分层: 增加一层只做转发的空壳。

## 3. 约束

- Qt 6.11.1 桌面 + Android; Qt 5 仍须能配置构建(仅经典风格)。
- 静态库, 依赖只指向内层; Qt 依赖必须 PUBLIC。
- 现有端口语义不变: `LottoView` / `LottoStyleHost` / `LottoStyleStore` /
  `LottoStyleDefinition` 的成员与方法签名保持兼容。
- 注释中文, 代码 ≤100 列。
- 测试全部保持绿色, 目标数量可增不可减。

## 4. 现状与问题(事实, 均有代码佐证)

| # | 问题 | 书上依据 |
|---|------|---------|
| 1 | `app` 被当作第五层, 实际与 `ui` 同圆 | ch22: 展示器/视图/控制器在同一个圆; 层数须对应策略层级 |
| 2 | `lotto_ui` 一个组件同时装: 抽象契约、UI 策略、Widgets 细节、QML 细节、`QSettings` 配置细节 | ch13 CCP/CRP: 不一起改的东西不该同组件; 链接者被迫依赖用不到的细节 |
| 3 | 视图端口头文件包含具体展示器头文件, 传递性把领域实体带入视图作用域 | ch22: 契约应依赖 DTO, 不依赖具体实现 |
| 4 | 经典视图构造时调用 `LottoPresenter::initialState()` 生产 ViewModel | ch23: 视图只渲染视图模型 |
| 5 | `MainWindow` 构造默认参数直接调用应用风格目录 | ch26: 具体类名与配置集中在 Main |
| 6 | `QSettings` 具体存储实现位于 UI 库内 | ch26/ch30: 配置与存储是 Main 装配的细节 |
| 7 | 静态库 qrc 需由入口与测试显式 `Q_INIT_RESOURCE` | ch23: 组件的实现细节不应泄漏给调用方 |
| 8 | `doc/architecture.md` / `doc/module-dependency.puml` 仍描述五层 | 教学文档与代码叙事必须一致 |

## 5. 目标结构

```text
圆(内 → 外):
  src/domain/        lotto_core                 Entities
  src/application/   lotto_application          Use Cases
  src/adapters/      lotto_adapters             Interface Adapters
                     └ LottoViewState(DTO) 独立成头文件
  src/ui/            lotto_ui                   Frameworks & Drivers (UI 核心)
                     ├ LottoView / LottoStyleHost / LottoStyleStore  (端口)
                     ├ LottoStyleDefinition / 目录类型              (注册表类型)
                     ├ LottoViewShell / LottoStyleSwitchOverlay     (宿主与过渡)
                     └ MainWindow(窗口壳) + mainwindow.ui
  src/ui/classic/    lotto_ui_classic           Frameworks (Widgets 细节)
  src/ui/neumorphic/ lotto_ui_neumorphic        Frameworks (Qt6 QML 细节)
  src/app/           —— 不是一层, 是 Main 组件
                     ├ main.cpp(装配 + 具体目录 + 初始视图模型)
                     └ (Main 只做装配; 存储实现在 adapters)
```

依赖图(全部指向内层, 无环):

```text
lotto_ui             → lotto_adapters → lotto_application → lotto_core
lotto_ui_classic     → lotto_ui
lotto_ui_neumorphic  → lotto_ui            (+ Qt6 Qml/QuickWidgets)
main (exe)           → lotto_ui, lotto_ui_classic, lotto_ui_neumorphic,
                       lotto_adapters, lotto_application, lotto_core
tests                → 按被测层选择, 不经过 lotto_ui 取具体风格
```

关键点: **`lotto_ui` 不链接任何风格组件**; `lotto_app` 作为库被取消,
`src/app/` 只剩 Main 的源文件。

## 6. 决策

| 决策 | 结论 | 理由 |
|------|------|------|
| 分层 | 四个圆 + Main, 取消 `app` 层 | 用户选定; ch22 |
| 风格 | 每风格一个静态库组件 | 用户选定"可插拔"; ch13 CRP |
| `MainWindow` 归属 | 并入 `lotto_ui` | 与宿主同圆, 且用户明确要求 |
| ViewModel | 独立 DTO 头文件, 仍在 adapters | 由展示器生产, 由视图消费; 契约只依赖 DTO |
| 初始 ViewModel | 由 Main 提供, 经 `MainWindow`/宿主注入 | ch23: 视图不生产 ViewModel |
| 具体风格目录 | 移到 Main(`main.cpp` 内的 `buildStyleCatalog()`) | ch26: 具体类名集中在 Main |
| 测试用真实目录 | 测试自行构造 | 避免测试依赖 Main |
| `QSettingsStyleStore` | 留在 `src/adapters/`, 与端口同层 | 实现属存储适配器, 非 Main 配置 |
| 经典 qrc 初始化 | 由 `lotto_ui_classic` 内部调用 | 实现细节不外泄 |
| QML 模块 | 挂在 `lotto_ui_neumorphic` 目标上 | 模块与风格组件同生命周期 |
| 初始 ViewModel | 宿主构造参数必选, 注入后即视为"已有状态" | 视图任何时刻都不会渲染空白; ch23 |
| 展示常量归属 | 随 `LottoViewState` 迁入 `lottoviewstate.h`(aliases `LottoGroup`) | 单一来源, 视图与测试不再引用展示器 |
| Main 的辅助代码 | `lotto_main` 静态库(仅 exe 与 Main 级测试链接) | 可测且不构成一层: 运行期无人依赖它 |
| `doc/` 架构文档 | 本次同步 `.md` 与 `.puml`; `.svg` 需手工重新渲染 | 本机无 PlantUML/kroki |

## 7. 各组件职责

- **lotto_ui(核心)**: 端口三件套 + 注册表类型 + 宿主 + 过渡 + 窗口壳。
  不含任何具体风格、不含 QSettings、不含 QML。对 Widgets 有依赖(宿主与
  窗口壳是 Widgets), 对 Quick 无依赖。端口头只包含 `lottoviewstate.h`。
- **lotto_ui_classic**: `LottoClassicView`、`style.qss`、`resources.qrc`;
  构造时自行 `Q_INIT_RESOURCE(resources)`。仅 Qt::Widgets。
- **lotto_ui_neumorphic**: `LottoNeumorphicView` + `LottoUi` QML 模块。
  仅 Qt6 构建; 加载失败时 `isReady()` 为假, 由宿主回退(现有行为)。
- **Main**: 装配顺序不变; 额外职责: 构造具体目录、提供初始 ViewModel、
  创建 `QSettingsStyleStore`。

### 7.1 初始 ViewModel 的注入契约(补充)

`LottoViewShell` 构造函数接受一个必选的 `LottoViewState initialState`, 并在
内部立即置位"已有状态"(`m_lastState = initialState; m_hasState = true`)。

后果与取舍:

- 任何时刻创建或切换到某个视图, 都至少能渲染出占位符与初始按钮状态;
  §8.7 的"启动首帧"与"切换首帧"由同一契约保证。
- `tst_lottoviewshell.cpp::switchBeforeAnyState_shouldNotCrash` 的语义改为
  "在 `load()` 之前切换, 新视图收到的是注入的初始状态而非 0 次渲染", 这是
  本次唯一一处**有意调整**的现有测试断言, 记为裁定。
- 视图构造函数不再调用 `LottoPresenter::initialState()`; 该函数的唯一调用
  方是 Main(它把结果传给窗口与宿主)。
- Main 需要 `LottoPresenter` 才能拿到初始 ViewModel —— 合法: Main 本就装配
  展示器, 且这是 ch26 允许的"最脏的一层知道所有具体类"。

### 7.2 展示常量的新家(补充)

新建 `src/adapters/lottoviewstate.h`, 内容:

- `struct LottoViewState`(从 `lottopresenter.h` 迁出, 字段不变);
- 展示文案常量(`TIME_LABEL` / `TIME_PLACEHOLDER` / `TIME_FORMAT` /
  `LOCK_TEXT_*` / `GENERATE_TEXT` / `NUMBER_PLACEHOLDER` / `SEPARATOR_TEXT`);
- 视图结构常量 `GROUP_COUNT` / `FRONT_COUNT` / `BACK_COUNT`, 直接别名
  `LottoGroup`, 不再经过 `LottoPresenter`。

`LottoPresenter` 保留 `formatNumber()` 与 `buildViewState()`, 并 `#include`
该头以使用同一批常量; 其原有的同名单例常量**删除**, 不保留别名(避免两份
名字指向同一值)。全部 `LottoPresenter::GROUP_COUNT` 等引用改为新头。

### 7.3 Main 的可测性(补充)

`src/app/` 不再产出 `lotto_app` 层库, 改为产出 `lotto_main` 静态库, 内含:

- (无存储实现: 风格偏好存储已归 adapters 层)
- `buildStyleCatalog()`(唯一知道具体风格类名的地方);
- 其余 Main 专有的装配辅助(如有)。

约束: **运行期没有任何组件依赖 `lotto_main`**, 只有可执行目标与 Main 级测试
链接它 —— 因此它不是一层, 只是把 Main 组件打包成可链接单元(ch26 的
"多个 Main"做法)。`main.cpp` 保留在可执行目标内。

`tst_lottoclassicview.cpp` 的 `catalogShouldContainClassic` 与
`tst_mainwindow.cpp` 的 `realCatalog_neuomorphicEntryCanBeActivated` 改为
调用 `lotto_main` 的 `buildStyleCatalog()`, 保住"真实目录"覆盖。

## 8. 行为保持清单

迁移后必须逐条成立(由现有测试保证, 必要时补测):

1. 默认风格为目录首项(经典); 偏好可恢复; 未知偏好回退且不覆盖原值。
2. 切换时重放最近状态; 切换失败不写偏好并保持当前视图。
3. 过渡覆盖层在切换期间存在、随尺寸变化、结束后释放。
4. 视图只收 `LottoViewState`; 空票据显示占位符与禁用状态。
5. 生成/锁定/恢复全链路不变; 持久化 round-trip 不变。
6. Qt 5 配置构建可用, 目录中只有经典风格。
7. 经典视图首帧即有占位符(初始 ViewModel 注入生效)。
8. `load()` 返回前切换风格, 新视图同样显示占位符而非空白。

## 9. 风险与回退

| 风险 | 影响 | 回退 |
|------|------|------|
| QML 模块改挂到风格组件后静态链接资源被裁剪 | 新拟物空白 | 冒烟测试先跑; 必要时改用 qrc 路径加载 |
| 拆分 target 后测试链接遗漏 | 构建失败 | 逐目标迁移, 每步跑全量 |
| 初始 ViewModel 注入改动影响首帧 | 启动闪现空控件 | 宿主构造时即渲染注入状态; 用测试断言首帧 |
| Android 部署漏掉新组件 | 运行期缺库 | 构建后检查 APK 组件清单 |

## 10. 留给实施者的决定

- 拆 target 的具体 CMake 组织(目录内 CMakeLists 还是父级统一)。
- 端口头文件拆分粒度(一个 `lottoviewports.h` 还是保持三个独立头)。
- `doc/module-dependency.svg` 与 `doc/control-flow.svg` 的重新渲染方式
  (本机无 PlantUML; 可留占位说明或由用户用 VS Code 插件渲染)。

## 11. 落地顺序建议

1. 抽出 `lottoviewstate.h`(DTO + 常量), 端口不再包含展示器头。
2. 初始 ViewModel 改为宿主构造注入; 视图不再引用展示器符号; 调整一条测试断言。
3. 拆出 `lotto_ui_classic` 与 `lotto_ui_neumorphic`, 核心库去掉具体风格。
4. `MainWindow` 移入核心库; `lotto_app` 改为 `lotto_main`(仅 exe/测试链接)。
5. `QSettingsStyleStore` 与 `buildStyleCatalog()` 移入 `lotto_main`; 经典 qrc 初始化内置。
6. 更新 CLAUDE.md 与 `doc/architecture.md`、`doc/module-dependency.puml`。
