# 视图接线归位: 宿主只切换, 接线归组合根 设计文档

日期: 2026-11-21
状态: 已实施(2026-11-21)
范围: 把 `LottoViewShell` 中"视图 ↔ 控制器/展示器"的接线迁移到 Main;
展示器缓存最近一次展示状态; 宿主只负责承载与切换。视图实现与端口契约不变。

## 1. 意图与动机

现状 `LottoViewShell` 同时承担三件事:

1. 风格切换: 目录、懒创建、栈、过渡、偏好持久化、失败回退;
2. 请求转发: 把视图的 `generateRequested` / `lockRequested` 转给
   `LottoController`;
3. 状态缓存与重放: 缓存 `LottoPresenter` 的最近一次 `LottoViewState`,
   在创建/切换视图时重放。

这带来两个问题:

- 一个机制组件有两个变化原因: 切换策略变化、展示契约变化都会改它;
- 展示状态的"当前值"存在宿主里, 而不是存在产出它的展示器里。

本设计把职责切为三块:

- **视图**(`LottoView` 实现): 只渲染状态、只发意图信号;
- **宿主**(`LottoViewShell`): 只承载与切换;
- **组合根**(Main): 负责把视图信号接到控制器、把展示状态接到视图。

这样 `lotto_ui` 核心库不再知道任何业务端口, `LottoView` 抽象保持最小;
未来增加终端版/网页版时, 各产品在自己的 Main 里装配, 不把"所有视图共享
一个展示器"这条只对当前 Qt 产品成立的局部事实固化进宿主或视图基类。

## 2. 目标 / 非目标 / 反目标

目标:

1. `LottoViewShell` 构造不再接收 `LottoController` / `LottoPresenter` /
   `LottoViewState`。
2. `LottoViewShell` 不再缓存或重放展示状态, 不再转发请求信号。
3. `LottoPresenter` 持有最近一次展示状态并提供 `state()`, 作为懒创建视图的
   首帧来源; 交互器仍是领域状态的唯一权威。
4. Main 提供 `wireStyleCatalog(...)`, 把目录中的每个视图工厂包装为
   "创建 + 接线 + 首帧渲染"。
5. 既有行为不变: 切换保留票据状态、偏好持久化、失败回退、过渡动画、
   两套风格。
6. 现有 14 个测试 target 全部通过。

非目标:

- 不新增视图侧端口(不引入 `LottoViewFeed` / `LottoViewInputPort`)。
- 不改 `LottoView`、`LottoController`、`LottoTicketView`、
  `LottoNeumorphicView` 的接口与实现。
- 不做同进程多前端同时运行的复合输出端口。
- 不改持久化键、QSS、QML、视觉与交互。

反目标(即使技术可行也算失败):

- 让视图 include `lottopresenter.h`(会把 `LottoTicket` 拖进所有视图的
  编译作用域)。
- 让 `LottoViewShell` 继续持有任何业务端口。
- 把接线逻辑复制到每个具体风格。
- 在 `LottoView` 基类里内置 feed/input 依赖。

## 3. 约束

- 依赖方向: `lotto_main → lotto_ui → lotto_adapters → lotto_application →
  lotto_core`, 不得出现反向依赖或环。
- `lotto_ui` 不依赖 Qt Quick; `LottoUiIsolationTest` 继续只链接 `lotto_ui`。
- 视图只感知 `LottoViewState` 与 `LottoStyleHost`。
- C++17; 代码行宽 ≤ 100, 注释 ≤ 80; 注释中文; 无 emoji。
- 测试可改, 但覆盖面不得缩水: 接线、首帧、状态保留、偏好持久化、失败回退
  都必须仍被覆盖。

## 4. 已定决策

| 决策 | 结论 | 依据 |
|------|------|------|
| 接线归属 | Main 的 `wireStyleCatalog()` | ch26: Main 负责创建、协调、装配; 具体类名与配置集中在最外层 |
| 宿主职责 | 只承载与切换 | SRP: 切换策略与展示契约是两个变化原因 |
| 展示状态最近值 | 由 `LottoPresenter` 缓存 | 输出适配器持有自己的最近产出; 避免宿主成为第二份状态 |
| 懒创建视图的首帧 | 工厂在构造后调 `view->renderState(presenter->state())` | Qt 信号不重放; 首帧必须从缓存拉取 |
| 视图侧端口 | 不新增 | Main 可以依赖具体类; 视图保持最小抽象; 只有 Main 需要 `state()` |
| 视图实现 | 零改动 | 视图仍然只做 `renderState` + 发信号 |
| 隐藏视图是否继续渲染 | 是 | 它们仍是应用视图; 切回时天然最新; 性能另议 |
| 多前端扩展 | 每产品一份 Main + presenter; 同进程多前端需复合输出端口 | 共享 presenter 是组合根的局部事实, 不是视图/宿主的不变量 |
| 失败回退 | 保持现状(宿主校验目录/id/isReady, 失败不写偏好) | 与本设计正交, 不引入回归 |

## 5. 备选方案

- **主选: D(Main 接线)**。宿主只切换; 视图只渲染/发信号; Main 装配。
  组件级职责彻底分开, `lotto_ui` 不再知道业务端口。
- A(视图基类接线): 类级 SRP 改善, 但 `lotto_ui` 仍同时承担切换与接线两个
  变化原因, 且把"每个视图恰好接一个 feed/input"写进视图抽象。否决。
- C(每个风格自己接线): 同 A 的抽象问题, 且重复; 视图不再是可独立复用的
  渲染器。否决。
- 现状(宿主接线): 可读作"复合视图"因而并非错误, 但作为切换机制仍有两个
  变化原因, 且状态缓存错位。否决。

## 6. 设计

### 6.1 职责与数据流

```text
                  ┌──────────────── Main(组合根) ────────────────┐
                  │ wireStyleCatalog(catalog, controller,        │
                  │                  presenter)                  │
                  │   每个工厂: create → connect → 首帧渲染       │
                  └───────────────┬──────────────────────────────┘
                                  │ 返回已接线的 LottoStyleDefinition
                                  ▼
 用户点击 ──▶ LottoView(具体风格) ──信号──▶ LottoController ──▶ LottoInteractor
                    ▲                                              │
                    │ renderState(state)                           │ present(ticket)
                    │                                              ▼
                    └──────── LottoPresenter(viewStateChanged + state() 缓存)

  LottoViewShell: 目录 / 栈 / 懒创建 / 过渡 / 偏好 / LottoStyleHost
  (不知道 LottoController、LottoPresenter、LottoViewState)
```

### 6.2 LottoPresenter

文件: `src/adapters/lottopresenter.h/.cpp`

新增:

```cpp
/*! 最近一次推送的展示状态(懒创建视图的首帧来源) */
const LottoViewState &state() const;

private:
    LottoViewState m_lastState;   /*!< 最近一次展示状态, 构造时以 initialState() 播种 */
```

- 构造函数: `m_lastState = initialState();`。
- `present(ticket)`:
  `m_lastState = buildViewState(ticket); emit viewStateChanged(m_lastState);`。
- `initialState()` 保持公开静态函数(现有测试仍使用)。
- 不新增端口, 不新增 QObject 基类, 信号位置不变。

### 6.3 LottoViewShell

文件: `src/ui/lottoviewshell.h/.cpp`

构造签名:

```cpp
LottoViewShell(LottoStyleStore *store,
               QVector<LottoStyleDefinition> catalog,
               int transitionMs = 200,
               QWidget *parent = nullptr);
```

删除:

- 成员 `m_controller`、`m_presenter`、`m_lastState`;
- 构造函数中对 `presenter->viewStateChanged` 的连接;
- `initialState` 参数与"groups 不完整"的告警;
- `createView()` 中的两个 `connect`(请求信号 → controller);
- 首帧与切换时的 `view->renderState(...)` 调用;
- 头文件中的 `lottopresenter.h` / `lottocontroller.h` 依赖。

保留:

- 目录、`QStackedWidget`、懒创建、当前视图与 `currentStyleId`;
- 偏好恢复/保存、未知 id 与工厂空值/`isReady()` 失败的回退;
- 切换排队执行、过渡覆盖层、`styleChanged()` 通知;
- `LottoStyleHost` 端口实现。

视图创建后由工厂负责首帧; 宿主只检查 `isReady()` 并 `addWidget` /
`setCurrentWidget`。

### 6.4 MainWindow

文件: `src/ui/mainwindow.h/.cpp`

构造签名:

```cpp
MainWindow(LottoStyleStore *store,
           QVector<LottoStyleDefinition> catalog,
           QWidget *parent = nullptr);
```

删除 `controller` / `presenter` / `initialState` 参数与相关头文件; 内部改为
`new LottoViewShell(store, std::move(catalog), 200, this)`。Android 全屏与
安全区逻辑不变。

### 6.5 Main: 目录与接线

文件: `src/app/maincatalog.h/.cpp`

```cpp
/*! 原始目录: 只含具体风格类名与工厂 */
QVector<LottoStyleDefinition> buildStyleCatalog();

/*!
 * \brief 为目录中的每个视图工厂接上展示器与控制器
 *
 * 每个工厂被包装为: 创建视图 → 连接 viewStateChanged → 连接两个请求信号
 * → 用 presenter->state() 渲染首帧。接线只允许出现在 Main。
 */
QVector<LottoStyleDefinition> wireStyleCatalog(
    QVector<LottoStyleDefinition> catalog,
    LottoController *controller,
    LottoPresenter *presenter);
```

`wireStyleCatalog` 的实现要点:

```cpp
for (LottoStyleDefinition definition : catalog) {
    auto create = definition.create;
    definition.create = [create, controller, presenter](LottoStyleHost *host,
                                                        QWidget *parent) -> LottoView * {
        LottoView *view = create ? create(host, parent) : nullptr;
        if (!view)
            return nullptr;
        QObject::connect(presenter, &LottoPresenter::viewStateChanged,
                         view, &LottoView::renderState);
        QObject::connect(view, &LottoView::generateRequested,
                         controller, &LottoController::onGenerateRequested);
        QObject::connect(view, &LottoView::lockRequested,
                         controller, &LottoController::onLockRequested);
        view->renderState(presenter->state());
        return view;
    };
    wired.append(definition);
}
```

- `Q_ASSERT(controller && presenter)`; 生产与测试都传真实对象。
- `buildStyleCatalog()` 保持无参、只含具体风格类名; 新增风格仍然只改这一处,
  接线由循环统一完成, 不会漏接。

### 6.6 main.cpp

```cpp
MainWindow window(&styleStore,
                  wireStyleCatalog(buildStyleCatalog(), &controller, &presenter));
```

删除 `seed` 变量与 `LottoPresenter::initialState()` 调用; `interactor.load()`
仍在窗口构造后由组合根发起。装配顺序不变: 仓储/引擎 → presenter → interactor
→ controller → 风格偏好 → 接线后的目录 → 窗口。

### 6.7 运行时序

启动:

1. Main 构造 `LottoPresenter`; 构造时以 `initialState()` 播种缓存;
2. Main 构造控制器与目录, 经 `wireStyleCatalog` 得到已接线目录;
3. `MainWindow` → `LottoViewShell` 按偏好创建首个视图; 工厂创建视图、接线、
   用 `presenter.state()` 渲染首帧;
4. Main 调 `interactor.load()`; 异步恢复后 `present()` 更新缓存并向所有已
   连接视图广播; 隐藏视图也更新。

点击与切换:

1. 视图发 `generateRequested` / `lockRequested` → 控制器 → 交互器;
2. 交互器 `present(ticket)` → 展示器更新缓存并广播 → 所有视图 `renderState`;
3. 视图调 `LottoStyleHost::requestSwitchTo` → 宿主排队切换 → 工厂懒创建
   (首帧即最新状态) → 入栈 → `styleChanged` → 保存偏好 → 过渡。

### 6.8 失败与边界

- 工厂返回空、目标 id 未知、`isReady()` 为假: 宿主保持当前视图、告警、
  发 `styleSwitchFailed`、不写偏好(行为不变)。
- 已创建但不可用的视图: 宿主 `deleteLater`; 其连接随对象销毁自动断开。
- 隐藏视图继续渲染: 语义正确(仍是应用视图), 性能可接受; 若未来 QML 隐藏
  渲染成为瓶颈, 另行在视图内部做可见性节流, 不改接线位置。
- 展示器缓存只是派生值; 领域状态权威仍在 `LottoInteractor`。

### 6.9 测试改动

| 测试 target | 改动 |
|-------------|------|
| `LottoPresenterTest` | 新增: 构造后 `state()` 等于 `initialState()`; `present()` 后 `state()` 与信号参数一致 |
| `LottoViewShellTest` | 夹具去掉 controller/presenter/seed; 构造调用改为 `(store, catalog, transitionMs)`; 删除"信号转发到 controller"与"首帧收到种子/切换重放种子"三类用例(迁至 Main 级) |
| `LottoMainCatalogTest` | 新增 `wireStyleCatalog` 用例: 用 `FakeLottoView` + 真实 `LottoController`(注入 `FakeInputBoundary`) + 真实 `LottoPresenter` + `FakeStyleHost`; 断言首帧渲染、两个请求信号到达输入边界、`present()` 后视图重渲染 |
| `MainWindowTest` | `rebuildWindow` 在创建新 controller/presenter 之后对原始目录调用 `wireStyleCatalog`; 构造改为 `MainWindow(m_store, wired)`; 生产目录冒烟继续使用原始 `buildStyleCatalog()` |
| 视图测试(`LottoTicketViewTest` / `LottoNeumorphicViewTest` / `LottoQmlSmokeTest`) | 不变 |
| 其余 target | 不变 |

### 6.10 文档改动

- `CLAUDE.md`:
  - 分层描述中 `LottoViewShell` 的职责改为"只承载与切换";
  - "视图只消费 `LottoViewState`"段落改为"由 Main 的目录接线, 不再经宿主";
  - `LottoPresenter` 段落补充 `state()` 缓存;
  - "关键代码模式"表新增 `LottoPresenter::state()` 与 `wireStyleCatalog()`;
  - "避坑指南"删除"初始 ViewModel 由 Main 注入", 新增"接线只允许出现在
    `wireStyleCatalog`"与"宿主/视图不得持有业务端口";
  - 测试策略段落同步 shell/presenter/maincatalog/mainwindow 的替身与断言。
- 头文件文档注释: `lottoviewshell.h`、`lottopresenter.h`、`lottoview.h`、
  `maincatalog.h`、`mainwindow.h`。
- 本设计文档状态在实施后改为"已实施(日期)"。

### 6.11 验收标准

1. 桌面构建成功; `ctest` 14 个 target 全部通过, 无新增警告。
2. `grep -rn "lottopresenter.h\|lottocontroller.h" src/ui/` 无结果。
3. `LottoUiIsolationTest` 的链接行仍不含任何 `Quick` / `Qml`。
4. 手工: 生成 → 锁定 → 两种风格互切, 号码与锁定状态保留; 重启恢复偏好与
   锁定票据; 未知偏好回退首项且不覆盖原值。
5. `src/ui` 内不再出现 `LottoViewState` 的缓存成员(视图自身内部缓存 QML
   待推状态除外)。

### 6.12 留给实现者的决定

- `state()` 返回 `const LottoViewState &`(倾向)还是按值;
- `wireStyleCatalog` 是否加 `Q_ASSERT` 与失败时的告警文案;
- 是否给 `buildStyleCatalog()` 增加一个便捷重载
  `buildWiredStyleCatalog(controller, presenter)`(非必需);
- 测试用例的具体命名与拆分;
- 文档措辞与段落位置。
