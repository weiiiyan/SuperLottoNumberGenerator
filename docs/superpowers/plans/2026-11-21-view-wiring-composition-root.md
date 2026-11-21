# 视图接线归位（组合根接线）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把视图与控制器/展示器的接线从 `LottoViewShell` 迁到 Main，让宿主只负责承载与切换。

**Architecture:** `LottoPresenter` 缓存最近一次 `LottoViewState`；Main 的 `wireStyleCatalog()` 为每个视图工厂接线并渲染首帧；`LottoViewShell` / `MainWindow` 不再接收业务端口；具体视图实现零改动。

**Tech Stack:** C++17、Qt 6.11（Widgets / Test）、CMake、CTest、MinGW 64-bit。

**Spec:** `docs/superpowers/specs/2026-11-21-view-wiring-composition-root-design.md`

## Global Constraints

- 依赖方向：`lotto_main → lotto_ui → lotto_adapters → lotto_application → lotto_core`，不得反向或成环。
- `lotto_ui` 不依赖 Qt Quick；`LottoUiIsolationTest` 只链接 `lotto_ui`。
- 视图只感知 `LottoViewState` 与 `LottoStyleHost`；不得 include `lottopresenter.h`。
- 宿主 `LottoViewShell` 与窗口 `MainWindow` 不得持有 `LottoController` / `LottoPresenter`。
- 接线只允许出现在 `src/app/maincatalog.cpp` 的 `wireStyleCatalog()`。
- C++17；代码行宽 ≤ 100，注释 ≤ 80；注释中文；无 emoji。
- 构建：`cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug`
- 测试：`ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure`
- 当前 14 个测试 target 必须全部通过。

## Review Focus

1. **隐藏视图在非当前状态时收到状态更新**：切到另一风格后，原视图仍在栈中；`present()` 后它必须已被重渲染，切回即显示最新号码。Task 3 的 `hiddenView_receivesStateUpdatesWhileInactive` 覆盖。
2. **懒创建视图的首帧必须是缓存的最新状态**：先 `present()` 再创建视图，首帧不能拿到构造前的旧值。Task 2 的接线用例覆盖。
3. **首帧早于 `interactor.load()` 的异步恢复**：先把占位符渲染出来，恢复完成后再更新。Task 3 的 `saveAndRestore_roundTrip` 覆盖。
4. **`isReady()` 为假的视图被丢弃**：它已经被接线并渲染过一次，删除后不得残留连接或影响当前视图。Task 3 保留的 shell 用例 `startupNotReadyView_shouldFallBackToFirst` 覆盖。
5. **生产入口必须使用接线后的目录**：`main.cpp` 若只传 `buildStyleCatalog()` 的原始目录，界面能显示但按钮无反应。Task 3 更新 `main.cpp`，`MainWindowTest` 的点击用例覆盖接线后的行为。

---

### Task 1: 展示器缓存最近一次展示状态

**Files:**
- Modify: `src/adapters/lottopresenter.h`
- Modify: `src/adapters/lottopresenter.cpp`
- Test: `tests/tst_lottopresenter.cpp`

**Interfaces:**
- Produces: `const LottoViewState &LottoPresenter::state() const;`
- Produces: `void LottoPresenter::present(const LottoTicket &ticket)` 先更新缓存再发信号。

- [ ] **Step 1: 写失败测试**

在 `tests/tst_lottopresenter.cpp` 的 `private slots:` 中加入：

```cpp
void state_shouldBeSeededWithInitialState()
{
    qDebug() << "验证展示器构造后缓存初始展示状态";
    const LottoViewState expected = LottoPresenter::initialState();
    QCOMPARE(m_presenter->state().generateButtonText, expected.generateButtonText);
    QCOMPARE(m_presenter->state().stubHintText, expected.stubHintText);
    QCOMPARE(m_presenter->state().groups.size(), LottoTicket::GROUP_COUNT);
}

void present_shouldUpdateCachedState()
{
    qDebug() << "验证 present() 同时更新缓存与信号";
    const LottoTicket ticket = makeTicket(
        LottoTicket::GROUP_COUNT, false, QDateTime(QDate(2026, 1, 1), QTime(12, 0, 0)));
    m_presenter->present(ticket);

    const LottoViewState expected = LottoPresenter::buildViewState(ticket);
    QCOMPARE(m_presenter->state().groups, expected.groups);
    QCOMPARE(m_presenter->state().timeValueText, expected.timeValueText);
    QCOMPARE(m_presenter->state().lockButtonEnabled, expected.lockButtonEnabled);
}
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target LottoPresenterTest`
Expected: 编译失败，`LottoPresenter` 没有 `state()`。

- [ ] **Step 3: 实现缓存**

`src/adapters/lottopresenter.h`：在 `initialState()` 声明之后加入

```cpp
/*! 最近一次推送的展示状态(懒创建视图的首帧来源) */
const LottoViewState &state() const;
```

在 `signals:` 之前加入私有成员：

```cpp
private:
    LottoViewState m_lastState;   /*!< 最近一次展示状态 */
```

`src/adapters/lottopresenter.cpp`：构造函数体中加入

```cpp
m_lastState = initialState();
```

把 `present()` 改为

```cpp
void LottoPresenter::present(const LottoTicket &ticket)
{
    m_lastState = buildViewState(ticket);
    emit viewStateChanged(m_lastState);
}
```

并在文件内加入

```cpp
const LottoViewState &LottoPresenter::state() const
{
    return m_lastState;
}
```

- [ ] **Step 4: 运行测试确认通过**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target LottoPresenterTest && ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug -R LottoPresenterTest --output-on-failure`
Expected: PASS。

- [ ] **Step 5: 提交**

```bash
git add src/adapters/lottopresenter.h src/adapters/lottopresenter.cpp tests/tst_lottopresenter.cpp
git commit -m "feat(adapters): 展示器缓存最近一次展示状态"
```

---

### Task 2: Main 统一接线目录

**Files:**
- Modify: `src/app/maincatalog.h`
- Modify: `src/app/maincatalog.cpp`
- Test: `tests/tst_maincatalog.cpp`

**Interfaces:**
- Consumes: `LottoPresenter::state()`（Task 1）。
- Produces: `QVector<LottoStyleDefinition> wireStyleCatalog(QVector<LottoStyleDefinition> catalog, LottoController *controller, LottoPresenter *presenter);`
- Keeps: `QVector<LottoStyleDefinition> buildStyleCatalog();` 返回未接线的原始目录。

- [ ] **Step 1: 写失败测试**

在 `tests/tst_maincatalog.cpp` 顶部加入

```cpp
#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottoview.h"
#include "testhelpers.h"
#include "uitesthelpers.h"
```

在 `private slots:` 中加入

```cpp
void wireStyleCatalog_shouldRenderCurrentStateAndForwardIntents()
{
    qDebug() << "验证接线: 首帧取展示器缓存, 请求信号到达输入边界, present 后重渲染";

    FakeInputBoundary input;
    LottoController controller(&input);
    LottoPresenter presenter;
    FakeStyleHost host;

    // 先推送一次, 确保视图创建时拿到的是最新缓存而不是初始种子
    presenter.present(makeTicket());

    QVector<LottoStyleDefinition> raw;
    raw.append({ {QStringLiteral("fake"), QStringLiteral("假风格")},
        [](LottoStyleHost *, QWidget *parent) -> LottoView * {
            return new FakeLottoView(nullptr, QStringLiteral("fake"), parent);
        } });

    const auto wired = wireStyleCatalog(raw, &controller, &presenter);
    QCOMPARE(wired.size(), 1);

    LottoView *view = wired.first().create(&host, nullptr);
    auto *fake = static_cast<FakeLottoView *>(view);
    QVERIFY(fake != nullptr);

    // 首帧: 展示器当前状态(含号码, 不是空种子)
    QCOMPARE(fake->stateCount(), 1);
    QCOMPARE(fake->lastState().groups, presenter.state().groups);
    QCOMPARE(fake->lastState().lockButtonEnabled, true);

    // 点击 → 输入边界
    fake->simulateGenerate();
    fake->simulateLock();
    QCOMPARE(input.generateCalls, 1);
    QCOMPARE(input.toggleLockCalls, 1);

    // present → 视图重渲染
    presenter.present(makeTicket());
    QCOMPARE(fake->stateCount(), 2);

    delete view;
}
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target LottoMainCatalogTest`
Expected: 编译失败，`wireStyleCatalog` 未声明。

- [ ] **Step 3: 实现 `wireStyleCatalog()`**

`src/app/maincatalog.h`：加入前向声明与函数声明

```cpp
class LottoController;
class LottoPresenter;

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

`src/app/maincatalog.cpp`：加入

```cpp
#include <QObject>

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottoview.h"
```

并实现

```cpp
QVector<LottoStyleDefinition> wireStyleCatalog(
    QVector<LottoStyleDefinition> catalog,
    LottoController *controller,
    LottoPresenter *presenter)
{
    Q_ASSERT(controller);
    Q_ASSERT(presenter);

    QVector<LottoStyleDefinition> wired;
    wired.reserve(catalog.size());
    for (const LottoStyleDefinition &definition : catalog) {
        LottoStyleDefinition entry = definition;
        const auto create = definition.create;
        entry.create = [create, controller, presenter](LottoStyleHost *host,
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
        wired.append(entry);
    }
    return wired;
}
```

- [ ] **Step 4: 运行测试确认通过**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target LottoMainCatalogTest && ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug -R LottoMainCatalogTest --output-on-failure`
Expected: PASS。

- [ ] **Step 5: 提交**

```bash
git add src/app/maincatalog.h src/app/maincatalog.cpp tests/tst_maincatalog.cpp
git commit -m "feat(main): 目录工厂统一接线视图与业务端口"
```

---

### Task 3: 宿主与窗口交出业务端口

**Files:**
- Modify: `src/ui/lottoviewshell.h`
- Modify: `src/ui/lottoviewshell.cpp`
- Modify: `src/ui/mainwindow.h`
- Modify: `src/ui/mainwindow.cpp`
- Modify: `src/app/main.cpp`
- Modify: `tests/tst_lottoviewshell.cpp`
- Modify: `tests/tst_mainwindow.cpp`

**Interfaces:**
- Consumes: `wireStyleCatalog()`（Task 2）、`LottoPresenter::state()`（Task 1）。
- Produces: `LottoViewShell(LottoStyleStore *store, QVector<LottoStyleDefinition> catalog, int transitionMs = 200, QWidget *parent = nullptr);`
- Produces: `MainWindow(LottoStyleStore *store, QVector<LottoStyleDefinition> catalog, QWidget *parent = nullptr);`

- [ ] **Step 1: 更新测试到新构造签名**

`tests/tst_lottoviewshell.cpp`：

1. 删除 `#include "lottocontroller.h"`、`#include "lottopresenter.h"` 与 `FakeInputBoundary` 相关成员。
2. 夹具只保留 `FakeLottoStyleStore *m_store` 与 `LottoViewShell *m_shell`；删除 `m_input` / `m_controller` / `m_presenter` / `m_seed`。
3. `init()` 改为：

```cpp
m_store = new FakeLottoStyleStore;
m_shell = new LottoViewShell(m_store, makeCatalog(), 0);
```

4. 所有 `LottoViewShell(m_controller, m_presenter, m_store, catalog, m_seed, N)` 改为 `LottoViewShell(m_store, catalog, N)`。
5. 删除三个用例：`generateSignalForwardsToController`、`switchBeforeAnyState_shouldRenderInjectedSeed`、`initialViewReceivesSeedImmediately`。

`tests/tst_mainwindow.cpp`：

1. `rebuildWindow()` 在创建 `m_controller` 之后、构造窗口之前加入：

```cpp
const auto wired = wireStyleCatalog(catalog, m_controller, m_presenter);
m_window = new MainWindow(m_store, wired);
```

2. 删除 `LottoPresenter::initialState()` 参数。
3. `realCatalog_neuomorphicEntryCanBeActivated()` 中 `rebuildWindow(buildStyleCatalog())` 保持不变（`buildStyleCatalog()` 现在返回原始目录，由 `rebuildWindow` 接线）。
4. 新增用例：

```cpp
void hiddenView_receivesStateUpdatesWhileInactive()
{
    qDebug() << "验证隐藏视图在非当前状态时仍收到状态更新, 切回即最新";
    auto *shell = m_window->viewShell();
    shell->requestSwitchTo(QStringLiteral("fake-b"));
    QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("fake-b"));

    auto *fakeB = static_cast<FakeLottoView *>(shell->currentView());
    QVERIFY(fakeB != nullptr);
    const int before = fakeB->stateCount();

    // 票据视图此时不可见, 但仍在栈中; 经控制器触发用例
    m_controller->onGenerateRequested();
    QTRY_VERIFY(frontLabel(0, 0)->text()
                != QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));

    // 两个视图都收到同一状态
    QCOMPARE(fakeB->stateCount(), before + 1);
    QCOMPARE(fakeB->lastState().groups.at(0).at(0), frontLabel(0, 0)->text());
}
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --target LottoViewShellTest MainWindowTest`
Expected: 编译失败，构造签名不匹配、`wireStyleCatalog` 未使用。

- [ ] **Step 3: 实现宿主瘦身**

`src/ui/lottoviewshell.h`：

- 删除 `#include "lottopresenter.h"`、`class LottoController;`。
- 构造函数改为

```cpp
LottoViewShell(LottoStyleStore *store,
               QVector<LottoStyleDefinition> catalog,
               int transitionMs = 200, QWidget *parent = nullptr);
```

- 删除成员 `m_controller`、`m_presenter`、`m_lastState`。
- 类文档改为"承载与切换; 接线由组合根完成, 本类不感知业务端口"。

`src/ui/lottoviewshell.cpp`：

- 删除 `#include "lottocontroller.h"`。
- 构造函数初始化列表删除 controller/presenter；删除连接 `presenter->viewStateChanged` 的整段代码；删除 `m_lastState = initialState` 与 groups 不完整的告警；删除创建首个视图后的 `view->renderState(m_lastState)`。
- `applySwitch()` 删除 `view->renderState(m_lastState);`。
- `createView()` 删除两个 `connect`，只保留

```cpp
if (!definition.create)
    return nullptr;
return definition.create(this, this);
```

- 保留 `isReady()`、入栈、`styleChanged()`、偏好保存、过渡与失败回退。

- [ ] **Step 4: 实现窗口与组合根改造**

`src/ui/mainwindow.h`：

- 删除 `#include "lottoviewstate.h"` 与 `class LottoController;` / `class LottoPresenter;`。
- 构造函数改为

```cpp
MainWindow(LottoStyleStore *store,
           QVector<LottoStyleDefinition> catalog,
           QWidget *parent = nullptr);
```

`src/ui/mainwindow.cpp`：

- 删除 `#include "lottocontroller.h"`、`#include "lottopresenter.h"`。
- 改为

```cpp
m_shell = new LottoViewShell(store, std::move(catalog), 200, this);
```

`src/app/main.cpp`：

```cpp
MainWindow window(&styleStore,
                  wireStyleCatalog(buildStyleCatalog(), &controller, &presenter));
```

删除 `const LottoViewState seed = LottoPresenter::initialState();`。

- [ ] **Step 5: 运行测试确认通过**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug`
Expected: 全部目标编译通过。

Run: `ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug -R "LottoViewShellTest|MainWindowTest" --output-on-failure`
Expected: PASS。

- [ ] **Step 6: 提交**

```bash
git add src/ui/lottoviewshell.h src/ui/lottoviewshell.cpp src/ui/mainwindow.h src/ui/mainwindow.cpp src/app/main.cpp tests/tst_lottoviewshell.cpp tests/tst_mainwindow.cpp
git commit -m "refactor(ui): 宿主只承载切换, 接线迁至组合根"
```

---

### Task 4: 文档与注释同步

**Files:**
- Modify: `CLAUDE.md`
- Modify: `src/ui/lottoview.h`（文档注释）
- Modify: `src/ui/lottoviewshell.h`（若 Task 3 未改完文档注释）
- Modify: `src/adapters/lottopresenter.h`（若 Task 1 未改完文档注释）
- Modify: `src/app/maincatalog.h`（若 Task 2 未改完文档注释）
- Modify: `docs/superpowers/specs/2026-11-21-view-wiring-composition-root-design.md`（状态行）

**Interfaces:**
- Consumes: 无。
- Produces: 文档与代码一致。

- [ ] **Step 1: 更新 CLAUDE.md**

按下述位置逐段改：

1. 分层说明中 `src/ui/` 的 `LottoViewShell` 描述：改为"视图宿主（只承载与切换）"。
2. "跨层数据只传 `LottoTicket`"段落补一句："`LottoController` / `LottoPresenter` 与视图的接线由 Main 的 `wireStyleCatalog()` 完成，宿主不感知二者。"
3. "输出侧适配器（展示器）"段落补一句："`present()` 同时更新 `state()` 缓存；懒创建视图以该缓存渲染首帧。"
4. "视图只消费 `LottoViewState`"段落：把"由宿主接给 `LottoController`"改为"由 Main 的目录接线接给 `LottoController`；视图不持有控制器/展示器"。
5. "切换交互由视图自己提供"段落：删除"重放最近状态"，改为"工厂懒创建时由 Main 的接线以展示器缓存渲染首帧"。
6. "关键代码模式"表：新增

```markdown
| `LottoPresenter::state()` | src/adapters/lottopresenter.h | 最近一次展示状态缓存, 懒创建视图的首帧来源 |
| `wireStyleCatalog()` | src/app/maincatalog.cpp | Main: 创建 + 接线 + 首帧渲染的唯一位置 |
```

7. "避坑指南"：删除"初始 ViewModel 由 Main 注入"整条，替换为

```markdown
- **接线只允许出现在 Main 的 `wireStyleCatalog()`**：宿主与视图不得持有 `LottoController` / `LottoPresenter`；展示器 `state()` 是懒创建视图的首帧来源。
- **展示器/控制器生命周期必须长于窗口与目录工厂**：`wireStyleCatalog()` 的工厂闭包捕获裸指针；Main 中先构造依赖，再构造窗口。
- **新增风格只改 `buildStyleCatalog()`**：接线由 `wireStyleCatalog()` 统一完成，不要在每个风格里自己连。
```

8. "各层测试的替身策略"：
   - `LottoViewShellTest` 描述改为"假目录 + 假偏好，只断言切换/回退/过渡/偏好；不装配业务端口"。
   - `LottoPresenterTest` 描述补"`state()` 缓存"。
   - `LottoMainCatalogTest` 描述补"`wireStyleCatalog` 接线：首帧/请求/重渲染"。
   - `MainWindowTest` 描述保持集成定位，补"目录先经 `wireStyleCatalog` 再注入窗口"。

- [ ] **Step 2: 更新头文件注释**

- `src/ui/lottoview.h`：把"交由宿主转发给输入侧适配器"改为"由组合根接线到输入侧适配器"。
- `src/adapters/lottopresenter.h`：类文档补一句 `state()` 缓存职责。
- `src/app/maincatalog.h`：文件文档补 `wireStyleCatalog` 的职责。
- `src/ui/mainwindow.h`：构造参数文档去掉 controller/presenter/initialState。

- [ ] **Step 3: 更新设计文档状态**

把 `docs/superpowers/specs/2026-11-21-view-wiring-composition-root-design.md`
的状态行改为 `状态: 已实施(2026-11-21)`。

- [ ] **Step 4: 提交**

```bash
git add CLAUDE.md src/ui/lottoview.h src/ui/lottoviewshell.h src/adapters/lottopresenter.h src/app/maincatalog.h src/ui/mainwindow.h docs/superpowers/specs/2026-11-21-view-wiring-composition-root-design.md
git commit -m "docs: 同步组合根接线后的架构说明"
```

---

### Task 5: 全量验证

**Files:**
- Modify: 无（只验证）。

**Interfaces:**
- Consumes: Task 1-4 的全部产出。
- Produces: 验证结论。

- [ ] **Step 1: 全量构建**

Run: `cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug`
Expected: 成功，无新增警告。

- [ ] **Step 2: 全量测试**

Run: `ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure`
Expected: 14/14 通过。

- [ ] **Step 3: 边界检查**

Run: `grep -rn "lottopresenter.h\|lottocontroller.h" src/ui/`
Expected: 无结果。

Run: `grep -rn "Quick\|Qml" build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/tests/CMakeFiles/LottoUiIsolationTest.dir/linkLibs.rsp 2>/dev/null || true`
Expected: 无结果（若文件路径不同，用构建目录中同名的 link 响应文件替代）。

- [ ] **Step 4: 冒烟运行**

Run: `timeout 5 ./build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/SuperLottoNumberGenerator.exe -platform offscreen`
Expected: 进程被 timeout 终止（退出码 124），期间无崩溃输出。

- [ ] **Step 5: 提交（若有验证产生的改动）**

```bash
git status --short
```

若无改动，不提交。
