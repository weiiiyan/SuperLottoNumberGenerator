# 架构设计

基于 Qt 6 Widgets 的大乐透随机号码生成器(桌面端 + Android),按整洁架构(Clean Architecture)分层,**依赖只指向内层**。

## 图表与文档

> 注意: `module-dependency.svg` 与 `control-flow.svg` 是 PlantUML 渲染产物,
> 本次分层调整后尚未重新渲染。以 `.puml` 源文件为准; 可用 VS Code PlantUML
> 插件或 kroki.io 重新导出同名 SVG。

| 图/文档 | PlantUML 源文件 | 渲染结果 |
| --- | --- | --- |
| 模块依赖图(分层/目标/测试) | [module-dependency.puml](module-dependency.puml) | [module-dependency.svg](module-dependency.svg) |
| 控制流图(恢复/生成/锁定) | [control-flow.puml](control-flow.puml) | [control-flow.svg](control-flow.svg) |
| Web 化迁移分析(边界划分/替换清单/关键决策) | — | [web-migration.md](web-migration.md) |

## 分层结构

```text
src/domain/        → lotto_core 库: 业务实体(Entities, 仅 Qt::Core)
src/application/   → lotto_application 库: 用例(Use Cases) + 三个 DIP 端口(仅 Qt::Core)
src/adapters/      → lotto_adapters 库: 接口适配器(Interface Adapters, 仅 Qt::Core)
                     存储侧 QSettingsTicketRepository + QSettingsStyleStore +
                     输入侧 LottoController + 输出侧 LottoPresenter +
                     展示状态 DTO lottoviewstate.h + 风格偏好端口 lottostylestore.h
src/ui/            → lotto_ui 库: 框架与驱动(UI 核心)
                     视图端口 + 风格端口 + 宿主 LottoViewShell + 过渡 + 窗口壳 MainWindow
                     不含任何具体风格, 不依赖 Qt Quick
src/ui/ticket/     → lotto_ui_ticket 库: 票据风格组件(Widgets + QSS)
src/ui/neumorphic/ → lotto_ui_neumorphic 库: 新拟物风格组件(Qt6 QML + QQuickWidget)
src/app/           → lotto_main 库: Main 组件(具体风格目录 + 接线 + 组合根)
src/app/main.cpp   → 组合根: 装配具体依赖(唯一接触 QSettings 具体类的点)
```

依赖方向(全部指向内层, 无环):

```text
lotto_ui_ticket ─┐
lotto_ui_neumorphic ┴─► lotto_ui ─► lotto_adapters ─► lotto_application ─► lotto_core
lotto_main ─► 以上全部(运行期无人依赖它)
```

**运行期没有任何组件依赖 `lotto_main`**, 只有可执行目标与 Main 级测试链接它。
因此它不是一层, 而是 ch26 所说的 Main 组件——最脏的一层, 所有具体类名与
配置都集中于此。

与《架构整洁之道》洋葱图的映射:

| 洋葱图层 | 目录 | 内容 |
| --- | --- | --- |
| 业务实体(Entities) | src/domain/ | LottoGroup/LottoTicket/LottoEngine |
| 用例(Use Cases) | src/application/ | LottoInteractor + TicketRepository 端口 |
| 接口适配器(Interface Adapters) | src/adapters/ | QSettingsTicketRepository(存储)、LottoController(输入)、LottoPresenter(输出) |
| 框架与驱动(Frameworks & Drivers) | src/ui/ 及其风格子目录 | LottoView/LottoViewShell/MainWindow/QSS/QML |
| Main 组件(洋葱外) | src/app/ | buildStyleCatalog()/wireStyleCatalog()/QSettingsStyleStore/main() |

**为何按层封装而非按用例/功能划分目录(有意权衡)**:第 21 章"尖叫的架构"主张顶层目录反映用例(如 `patients/`、`diagnoses/`);第 34 章也指出按层封装的隐患——不同业务领域的分层代码看起来惊人相似,无法展现领域信息。本项目仍按层划分,理由:(1) 本系统只有**一个用例**(生成/锁定/恢复一注大乐透号码),用例即系统本身,层目录只是洋葱壳;(2) "尖叫"由类名承担——全项目 `Lotto*` 前缀(LottoGroup/LottoTicket/LottoEngine/LottoInteractor/LottoPresenter),打开任何文件都知道这是什么系统;(3) 本项目用于教学,目录与洋葱图一一对应本身就是教学目标。**触发条件**:出现第二个可区分用例(历史记录/统计分析/多玩法等)时,层目录将开始埋没用例,应改为按用例纵向切分——每用例一个组件,自带交互器/展示器/仓储端口(参考第 33 章案例分析)。详见 CLAUDE.md。

**为何风格是独立组件**:书图 22.2 把展示器、视图、控制器画在同一个圆里。实现"UI 可替换"的关键不是把视图放得"更外层", 而是让**每一套风格成为可独立链接的组件**:链接 `lotto_ui` 不被迫拖入 Qt Quick, 只想用票据视图的产品可以只链接 `lotto_ui_ticket`。守门测试 `LottoUiIsolationTest` 只链接核心库。

**输入/输出侧适配器为何统一归入 adapters/**:按洋葱图 Controller(输入侧)与 Presenter(输出侧)都是接口适配器,与存储侧 Gateway 同层对称,因此统一物理上放 src/adapters/。曾有意让展示器与谦卑视图同库配对(ch24 不完全边界),本次已按"严格整洁架构"收口:适配器不依赖 Widget、依赖只指向内层的约束由库边界强制,视图与适配器解耦后各自可独立测试与替换。

领域层使用 Qt::Core 类型(QDateTime/QVector/QObject)是有意权衡:本项目以 Qt 为应用语言,不追求与框架无关的纯领域,但绝不依赖任何 UI/存储框架。

## 各层职责

### 领域层(domain)

- `LottoGroup` — 单组号码(前区 5 个 [1,35] + 后区 2 个 [1,12],已排序);组级游戏规则常量 `FRONT_COUNT`/`BACK_COUNT`/`FRONT_MIN`/`FRONT_MAX`/`BACK_MIN`/`BACK_MAX` 定义于此(全项目唯一来源)
- `LottoTicket` — 领域实体:5 组号码 + 生成时间 + 锁定标志;票据级常量 `GROUP_COUNT` 定义于此(`FRONT_COUNT`/`BACK_COUNT` 为 `LottoGroup::*` 别名);`hasNumbers()`/`isValid()` 校验在此定义
- `LottoEngine` — 领域服务:随机生成号码(QRandomGenerator 洗牌,无状态)

### 用例层(application)

- `LottoInteractor` — 用例交互器(use case interactor),而非 MVC 控制器:不做输入/输出格式适配,只执行应用特定业务规则与编排。持有唯一状态源 `LottoTicket`;提供生成(锁定时拒绝)、锁定切换(状态守卫防重复保存)、异步恢复(`load()` 由组合根在窗口构造后调用,视图不驱动用例启动)。实现输入边界 `LottoInputBoundary`,状态变化后经输出边界 `LottoPresenterPort::present()` 把票据推送给出输出适配器——**用 DIP 端口主动调用,而非 Qt 信号广播**。仅依赖 Qt::Core,不接触任何 UI/存储具体类
  - **Controller 与 Interactor 的角色分离(已落实)**:书图 22.2 中 Controller(接口适配器层)与 UseCaseInteractor(用例层)是两个角色,前者把外部输入打包成 InputData,后者执行业务编排。本项目 `LottoController`(输入侧适配器)依赖用例层输入边界 `LottoInputBoundary`(由交互器实现),把按钮点击翻译为边界调用;`LottoInteractor` 保持零 I/O 格式适配。锁定时拒绝生成等应用规则仍在用例层(按钮禁用只是辅助,用例层判定权威),Controller 不重复业务规则
- `TicketRepository` — 用例层持久化接口(纯抽象,DIP 端口):`save/load/clear`,由适配器层实现
- `LottoPresenterPort` — 用例层输出边界端口(纯抽象,DIP 端口):`present(LottoTicket)`,由展示器实现,交互器在状态变化后调用
- `LottoInputBoundary` — 用例层输入边界接口(纯抽象):`generateNewTicket/toggleLock/setLocked/load`,由交互器实现,Controller 依赖

### 适配器层(adapters)

- `QSettingsTicketRepository` — TicketRepository 的 QSettings 实现。键名与旧版本兼容;时间存 ISO 字符串,读取时兼容旧版显示格式;锁定时写全量,未锁定仅写标志
- `LottoController` — 输入侧适配器(控制器):依赖 `LottoInputBoundary`,把外部输入翻译为用例调用。`onGenerateRequested → generateNewTicket()`、`onLockRequested → toggleLock()`,不含业务判断。不依赖任何 Widget,可独立测试
- `LottoPresenter` — 输出侧适配器(展示器,谦卑对象模式):实现输出边界 `LottoPresenterPort`,交互器调用 `present(LottoTicket)` 时经静态纯函数 `buildViewState(LottoTicket) → LottoViewState`(号码格式化/占位符/按钮互斥状态/时间显示格式)推导,更新 `state()` 缓存并重发 `viewStateChanged(LottoViewState)` 信号,不依赖任何 Widget。UI 展示字符串及视图结构常量单来源;`LottoViewState` 已注册 Qt 元类型。`state()` 是懒创建视图的首帧来源

### 框架与驱动(ui)

- `LottoView` — 视图端口:只消费 `LottoViewState`(定义在 adapters 的 `lottoviewstate.h`), 把用户操作翻译为 `generateRequested`/`lockRequested` 信号; 信号由 Main 的 `wireStyleCatalog()` 接到控制器, 视图不感知领域实体与本用例层
- `LottoViewShell` — 视图宿主: 按目录懒创建视图、承载当前视图、执行切换过渡、保存偏好、失败回退。**不缓存展示状态、不转发请求、不感知业务端口**; 视图的创建与接线由 Main 的 `wireStyleCatalog()` 在工厂中完成
- `MainWindow` — 窗口壳: `setupUi` + 把宿主设为 central widget, 与宿主同圆
- `lotto_ui_ticket` — 票据风格组件: `LottoTicketView` + QSS + qrc(自带资源初始化)
- `lotto_ui_neumorphic` — 新拟物风格组件: `LottoNeumorphicView`(组合 `QQuickWidget`)+ `LottoUi` QML 模块; Qt6-only

### Main(src/app)

- `buildStyleCatalog()` — 具体风格目录: 全仓唯一允许出现具体风格类名的地方
- `wireStyleCatalog()` — 把目录中的每个视图工厂包装为"创建 + 接线 + 首帧渲染": 连接 `viewStateChanged → renderState`、两个请求信号 → 控制器槽, 并用 `presenter->state()` 渲染首帧。全仓唯一允许接线的地方
- `QSettingsStyleStore` — 风格偏好端口的 QSettings 实现
- `main.cpp` — 组合根: 装配具体依赖(唯一接触 QSettings 具体类的点), 装配顺序为 仓储/引擎 → 输出适配器(实现输出端口) → 交互器(注入输出端口) → 输入适配器(依赖输入边界) → 风格偏好 → `wireStyleCatalog(buildStyleCatalog(), ...)` 接线 → MainWindow(注入接线后的目录), 启动用例(`interactor.load()`), 栈分配, 移交控制权

## 数据流

```text
用户点击(生成/锁定) → 当前 LottoView ──请求信号──► LottoController(输入侧适配器)
    (请求信号 → 控制器的接线由 Main 的 wireStyleCatalog 在创建视图时完成)
                                                      │
                                                      └─经 LottoInputBoundary──► LottoInteractor 用例
                                                                                    ↓ 状态变化
                                          present(ticket) 经 LottoPresenterPort(输出边界)
                                                                                    ↓
                                          LottoPresenter: buildViewState() 推导 + 更新 state() 缓存
                                                                                    ↓
                                                              viewStateChanged(LottoViewState)
                                                                                    ↓
                                          所有已接线 LottoView 各自 renderState()
                                          懒创建视图由目录工厂用 state() 渲染首帧
                                                                                    ↑
        QSettingsTicketRepository(仅锁定/解锁时读写, 恢复时异步加载, 加载结果经 presenter 适配)
```

跨层只传 `LottoTicket` 值类型(存储/用例间)与 `LottoViewState`(展示侧);**视图只感知 `LottoViewState`,不感知领域实体**,也不传递 widget 或存储内部结构。

## 测试

14 个测试 target 链接各层库,不手工编译被测源码:

| target | 覆盖 |
| --- | --- |
| LottoEngineTest | 引擎随机性/边界/分布 |
| LottoInteractorTest | 状态机 + 输出端口推送(注入 Fake 仓储/Fake 输出端口,无 GUI 依赖) |
| QSettingsRepositoryTest | 持久化 round-trip 与旧 INI 兼容 |
| LottoControllerTest | 输入侧翻译(点击 → 用例调用,用例守卫仍生效,无 GUI 依赖) |
| LottoPresenterTest | 展示推导 + 输出端口桥接 + `state()` 缓存(无 GUI 依赖) |
| LottoUiTest | 视图基类默认行为 |
| LottoUiIsolationTest | 只链接 lotto_ui: 核心库不拖入 Qt Quick 的守门测试 |
| LottoTicketViewTest | 票据视图渲染/请求信号/票根文案/风格抽屉 |
| LottoViewShellTest | 宿主: 默认与恢复风格/失败回退/过渡覆盖层(不装配业务端口) |
| LottoNeumorphicViewTest / LottoQmlSmokeTest | QML 视图与模块加载(Qt6) |
| LottoMainCatalogTest | Main 的具体风格目录条目 + `wireStyleCatalog` 接线(首帧/请求/重渲染) |
| QSettingsStyleStoreTest | 风格偏好存储往返 |
| MainWindowTest | GUI 集成(注入式夹具,目录经 `wireStyleCatalog` 接线,offscreen) |
