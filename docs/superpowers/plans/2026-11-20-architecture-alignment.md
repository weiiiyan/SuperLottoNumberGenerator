# 架构对齐：四个圆 + Main、风格可插拔 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把分层改回《架构整洁之道》的四个圆加 Main，并把每套 UI 风格拆成
可独立链接的组件——链接 UI 核心不被迫拖入另一套风格的依赖。

**Architecture:** `lotto_ui` 只保留端口、注册表类型、宿主、过渡与窗口壳；
`lotto_ui_classic` / `lotto_ui_neumorphic` 各自一个静态库；`lotto_main`
（圆外）承载 `QSettings` 存储与具体风格目录，运行期无人依赖它。展示状态
`LottoViewState` 与全部展示常量迁入 `lottoviewstate.h`，端口头不再包含
展示器。

**Tech Stack:** C++17, Qt 6.11.1 (Widgets + Qml + QuickWidgets +
QtQuick.Effects) / Qt 5.15 fallback, CMake 静态库, QTest + CTest。

**Spec:** [docs/superpowers/specs/2026-11-20-architecture-alignment-design.md](../specs/2026-11-20-architecture-alignment-design.md)

## Global Constraints

- 依赖只指向内层：`domain`(Entities) ← `application`(Use Cases) ←
  `adapters`(Interface Adapters) ← `ui`(Frameworks & Drivers)。
- `lotto_ui` **不得**链接 `lotto_ui_classic` 或 `lotto_ui_neumorphic`；
  两个风格库之间**不得**互相链接；风格库只链接 `lotto_ui`。
- 运行期任何目标**不得**依赖 `lotto_main`；只有可执行目标与 Main 级测试链接它。
- Main 之外的代码不得出现具体风格类名（`LottoClassicView` /
  `LottoNeumorphicView`）。
- 视图只消费 `LottoViewState`；不得包含 `lottopresenter.h`，不得包含
  `lottoticket.h` / `lottointeractor.h`。
- 展示文案与常量单一来源为 `src/adapters/lottoviewstate.h`；视图与测试
  不得重复魔法字符串，文案中不得出现 emoji。
- Qt 6 专属代码（QML 模块、`QQuickWidget`）必须在
  `if(QT_VERSION_MAJOR GREATER_EQUAL 6)` 内；Qt 5 下风格目录只有经典。
- 静态库的 Qt 依赖必须 `PUBLIC`。
- 注释中文，代码行 ≤100 字符，注释 ≤80 字符；类与公共函数用 `/*! ... */`。
- 构建目录 `build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug`，Qt 前缀
  `D:/Software/Qt6.11/6.11.1/mingw_64`。
- GUI 测试一律 `-platform offscreen`。
- 每个任务结束必须 `ctest` 全绿并提交。

## Review Focus

1. **Qt 5 配置构建**: 在 Qt 5.15 下 `cmake` 配置必须成功，且风格目录只含
   经典视图——任何无条件引用 Quick 的代码都会让老工具链构建失败。
2. **链接隔离**: 一个只链接 `lotto_ui` 的目标必须能编译并运行，且不引入
   `Qt6::Quick`/`Qt6::QuickWidgets` 依赖；这是"可插拔"的唯一硬证据。
3. **首帧与切换首帧**: `load()` 的 `singleShot(0)` 返回前，窗口与任何切换
   目标视图都必须显示占位符，而不是空白容器。
4. **Main 之外零具体类名**: 全仓搜索 `LottoClassicView` / `LottoNeumorphicView`
   只能命中 `lotto_main`、各风格库自身与测试。
5. **旧架构残留**: `lotto_app` 目标、`src/app/mainwindow.*`、五层叙事文档
   必须全部消失或改写，否则下次会话会被误导。

---

### Task 1: 抽出 `lottoviewstate.h`（DTO + 展示常量）

**Files:**
- Create: `src/adapters/lottoviewstate.h`
- Modify: `src/adapters/lottopresenter.h`, `src/adapters/lottopresenter.cpp`
- Modify: `src/adapters/CMakeLists.txt`
- Test: `tests/tst_lottopresenter.cpp`

**Interfaces:**
- Produces: `src/adapters/lottoviewstate.h` 中的
  - `struct LottoViewState`（字段与现有一致，见 Spec §7.2）
  - `Q_DECLARE_METATYPE(LottoViewState)`
  - `namespace LottoViewText`，含 `TIME_LABEL` / `TIME_PLACEHOLDER` /
    `TIME_FORMAT` / `LOCK_TEXT_LOCKED` / `LOCK_TEXT_UNLOCKED` /
    `GENERATE_TEXT` / `NUMBER_PLACEHOLDER` / `SEPARATOR_TEXT`（均为
    `constexpr const char *`，值与现有相同）
  - `namespace LottoViewShape`，含 `GROUP_COUNT` / `FRONT_COUNT` /
    `BACK_COUNT`（`= LottoGroup::*`）
- 删除：`LottoPresenter` 上的同名单例常量（不留别名）。

- [ ] **Step 1: 写失败测试**

在 `tests/tst_lottopresenter.cpp` 增加，并把原有对
`LottoPresenter::TIME_LABEL` 等的引用改为 `LottoViewText::TIME_LABEL`：

```cpp
void constantsLiveInTheStateHeader()
{
    QCOMPARE(QString::fromUtf8(LottoViewText::TIME_LABEL),
             QString::fromUtf8("生成时间："));
    QCOMPARE(QString::fromUtf8(LottoViewText::SEPARATOR_TEXT), QStringLiteral("+"));
    QCOMPARE(QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER),
             QStringLiteral("?"));
    QCOMPARE(LottoViewShape::GROUP_COUNT, LottoTicket::GROUP_COUNT);
    QCOMPARE(LottoViewShape::FRONT_COUNT, LottoGroup::FRONT_COUNT);
    QCOMPARE(LottoViewShape::BACK_COUNT, LottoGroup::BACK_COUNT);
}
```

- [ ] **Step 2: 运行确认失败**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期：`lottoviewstate.h: No such file or directory`。

- [ ] **Step 3: 建立新头并迁移**

`src/adapters/lottoviewstate.h` 内容为：把 `LottoViewState` 结构体、
`Q_DECLARE_METATYPE` 与全部常量从 `lottopresenter.h` 原样搬入，
`#include "lottogroup.h"`（取 `LottoGroup::*`）与 `<QString>` /
`<QStringList>` / `<QVector>` / `<QMetaType>`。常量放进
`namespace LottoViewText` 与 `namespace LottoViewShape`。

`lottopresenter.h` 删除结构体与常量，改为 `#include "lottoviewstate.h"`，
保留 `LottoPresenter` 类（成员与方法签名不变）。`lottopresenter.cpp` 内的
`QString::fromUtf8(TIME_LABEL)` 等改为 `LottoViewText::` 前缀（类内不再有
这些名字）。`src/adapters/CMakeLists.txt` 的源列表加入 `lottoviewstate.h`。

- [ ] **Step 4: 全仓改引用**

把 `LottoPresenter::TIME_LABEL` / `TIME_PLACEHOLDER` / `TIME_FORMAT` /
`LOCK_TEXT_LOCKED` / `LOCK_TEXT_UNLOCKED` / `GENERATE_TEXT` /
`NUMBER_PLACEHOLDER` / `SEPARATOR_TEXT` / `GROUP_COUNT` / `FRONT_COUNT` /
`BACK_COUNT` 的引用改为 `LottoViewText::` / `LottoViewShape::`，范围：
`src/ui/`、`src/app/`、`tests/`。`LottoPresenter::buildViewState` /
`initialState` / `formatNumber` 保持原名。

- [ ] **Step 5: 运行全量测试**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期：11/11 通过。

- [ ] **Step 6: 提交**

```bash
git add src/adapters tests
git commit -m "refactor(adapters): 展示状态与展示常量迁入 lottoviewstate.h"
```

---

### Task 2: 端口头只依赖 DTO；初始 ViewModel 改为注入

**Files:**
- Modify: `src/ui/lottoview.h`
- Modify: `src/ui/lottoviewshell.h`, `src/ui/lottoviewshell.cpp`
- Modify: `src/ui/classic/lottoclassicview.h`, `classic/lottoclassicview.cpp`
- Modify: `src/ui/neumorphic/lottoneumorphicview.h`
- Modify: `src/app/mainwindow.h`, `mainwindow.cpp`, `main.cpp`
- Test: `tests/tst_lottoviewshell.cpp`, `tests/tst_lottoclassicview.cpp`

**Interfaces:**
- Consumes: Task 1 的 `lottoviewstate.h`。
- Produces:
  - `LottoView` 头只 `#include "lottoviewstate.h"`
  - `LottoViewShell(LottoController *, LottoPresenter *, LottoStyleStore *,
    QVector<LottoStyleDefinition>, const LottoViewState &initialState,
    int transitionMs = 200, QWidget *parent = nullptr)` —— `initialState`
    为必选参数；构造时置 `m_lastState = initialState; m_hasState = true;`
  - `MainWindow(LottoController *, LottoPresenter *, LottoStyleStore *,
    QVector<LottoStyleDefinition>, const LottoViewState &initialState,
    QWidget *parent = nullptr)`
- 视图构造不再调用 `LottoPresenter::initialState()`。

- [ ] **Step 1: 写失败测试**

`tests/tst_lottoviewshell.cpp`：

- 所有 `LottoViewShell shell(...)` / `new LottoViewShell(...)` 增加
  `LottoPresenter::initialState()` 作为 initialState 实参。
- 改写现有用例：

```cpp
void switchBeforeAnyState_shouldRenderInjectedSeed()
{
    qDebug() << "验证首次状态到达前切换, 新视图显示注入的初始状态";
    auto seed = LottoPresenter::initialState();
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                         seed, 0);
    shell.requestSwitchTo(QStringLiteral("b"));
    QTRY_COMPARE(shell.currentStyleId(), QStringLiteral("b"));
    auto *b = static_cast<FakeLottoView *>(shell.currentView());
    QCOMPARE(b->stateCount(), 1);
    QCOMPARE(b->lastState().generateButtonText, seed.generateButtonText);
}
```

- 新增：

```cpp
void initialViewReceivesSeedImmediately()
{
    qDebug() << "验证构造后首个视图立即收到初始状态";
    auto seed = LottoPresenter::initialState();
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                         seed, 0);
    auto *view = static_cast<FakeLottoView *>(shell.currentView());
    QCOMPARE(view->stateCount(), 1);
    QCOMPARE(view->lastState().numberPlaceholder, seed.numberPlaceholder);
}
```

`tests/tst_lottoclassicview.cpp`：`rendersStateIntoWidgets` 之前增加

```cpp
void firstFrameShowsPlaceholdersWithoutAnyRenderCall()
{
    qDebug() << "验证经典视图未收到状态时不自我渲染(首帧由宿主注入)";
    FakeStyleHost host;
    LottoClassicView view(&host);
    // 构造期不再自渲染: 号码标签为空, 不存在展示器推导
    QCOMPARE(view.findChild<QLabel *>("frontLabel_0_0")->text(), QString());
}
```

并删除该文件其余对 `LottoPresenter::initialState()` 的调用，改为直接构造
`LottoViewState()` 或 `buildViewState(makeTicket())`。

- [ ] **Step 2: 运行确认失败**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期：编译失败（构造函数参数不匹配）。

- [ ] **Step 3: 实现注入**

`lottoview.h` 把 `#include "lottopresenter.h"` 改为
`#include "lottoviewstate.h"`。

`LottoViewShell` 构造函数增加 `const LottoViewState &initialState` 参数
（放在 catalog 之后、transitionMs 之前），函数体内在创建首个视图之前执行
`m_lastState = initialState; m_hasState = true;`，并删除构造末尾
`if (m_hasState) view->renderState(...)` 的判空依赖——现在恒定成立。

`LottoClassicView::initUi()` 删除 `applyState(LottoPresenter::initialState())`
一行；`lottoclassicview.h` 的常量别名改为
`LottoViewShape::GROUP_COUNT` 等。`lottoneumorphicview.h` 同理（若引用了
展示器常量）。

`MainWindow` 构造增加 `const LottoViewState &initialState` 并透传给宿主；
`main.cpp` 用 `LottoPresenter presenter; ... LottoViewState seed =
LottoPresenter::initialState();` 传入 `MainWindow`。

- [ ] **Step 4: 运行全量测试**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

- [ ] **Step 5: 用探针验证端口不再泄漏领域实体**

```bash
export PATH="/d/Software/Qt6.11/Tools/mingw1310_64/bin:$PATH"
cat > /tmp/probe.cpp <<'EOF'
#include "lottoview.h"
void probe(LottoTicket *);   // 期望: 编译错误 (LottoTicket 不在作用域)
EOF
g++ -fsyntax-only -Isrc/ui -Isrc/adapters -Isrc/application -Isrc/domain \
  -ID:/Software/Qt6.11/6.11.1/mingw_64/include \
  -ID:/Software/Qt6.11/6.11.1/mingw_64/include/QtCore \
  -ID:/Software/Qt6.11/6.11.1/mingw_64/include/QtWidgets \
  -ID:/Software/Qt6.11/6.11.1/mingw_64/include/QtGui -std=c++17 /tmp/probe.cpp
```

预期：报 `'LottoTicket' has not been declared`（Task 2 之前该探针编译通过）。

- [ ] **Step 6: 提交**

```bash
git add src/ui src/app tests
git commit -m "refactor(ui): 端口只依赖展示状态, 初始视图模型由 Main 注入"
```

---

### Task 3: 拆出风格组件，核心库去掉具体风格

**Files:**
- Create: `src/ui/classic/CMakeLists.txt`
- Create: `src/ui/neumorphic/CMakeLists.txt`
- Modify: `src/ui/CMakeLists.txt`
- Modify: `src/ui/lottostylecatalog.h`（仅保留类型，删除函数声明）
- Delete: `src/ui/lottostylecatalog.cpp`
- Modify: `src/ui/classic/lottoclassicview.cpp`（自带 qrc 初始化）
- Test: `tests/CMakeLists.txt`, `tests/tst_lottoclassicview.cpp`,
  `tests/tst_lottoneumorphicview.cpp`, `tests/tst_lottoui.cpp`

**Interfaces:**
- Produces:
  - `lotto_ui_classic`（STATIC）：`LottoClassicView` + `style.qss` + qrc
  - `lotto_ui_neumorphic`（STATIC，仅 Qt6）：`LottoNeumorphicView` + QML 模块
  - `lotto_ui` 不再含任何 `classic/` 或 `neumorphic/` 源，且不再链接它们
  - `LottoStyleCatalog` 函数从 `lottostylecatalog.h` 移除

- [ ] **Step 1: 写可链接性失败测试**

新建 `tests/tst_uiisolation.cpp`（新测试目标，只链接 `lotto_ui`）：

```cpp
#include <QTest>
#include <QWidget>

#include "lottoview.h"          // 端口可用
#include "lottoviewshell.h"     // 宿主可用
#include "lottostylecatalog.h"  // 注册表类型可用

class TestUiIsolation : public QObject
{
    Q_OBJECT
private slots:
    void coreLinksWithoutAnyStyle()
    {
        qDebug() << "验证只链接 lotto_ui 时端口与宿主类型可用";
        LottoStyleDefinition definition;   // 注册表类型可用
        QVERIFY(!definition.create);       // 默认构造无工厂
    }
};

QTEST_MAIN(TestUiIsolation)
#include "tst_uiisolation.moc"
```

`tests/CMakeLists.txt` 追加：

```cmake
add_lotto_test(LottoUiIsolationTest tst_uiisolation.cpp)
target_link_libraries(LottoUiIsolationTest PRIVATE lotto_ui)
register_lotto_test(LottoUiIsolationTest LottoUiIsolationTest -platform offscreen)
```

同时在既有测试里加入"必须是具体风格"的断言，防止测试走错库：
`tst_lottoclassicview.cpp` 的 `initTestCase` 增加
`QCOMPARE(QString::fromLatin1(LottoClassicView::STYLE_ID), QStringLiteral("classic"));`。

- [ ] **Step 2: 运行确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期：`LottoUiIsolationTest` 链接失败（`lotto_ui` 仍把经典与 QML 一起编入，
且此时目标不存在导致配置失败亦可）。

- [ ] **Step 3: 拆分 CMake 与源**

`src/ui/classic/CMakeLists.txt`：

```cmake
add_library(lotto_ui_classic STATIC
    lottoclassicview.h lottoclassicview.cpp
    resources.qrc
)
target_include_directories(lotto_ui_classic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_ui_classic PUBLIC lotto_ui
    Qt${QT_VERSION_MAJOR}::Widgets)
```

`src/ui/neumorphic/CMakeLists.txt`（内容仅在 Qt6 下求值）：

```cmake
add_library(lotto_ui_neumorphic STATIC
    lottoneumorphicview.h lottoneumorphicview.cpp
)
target_include_directories(lotto_ui_neumorphic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_ui_neumorphic PUBLIC lotto_ui
    Qt6::Qml Qt6::QuickWidgets)
qt_add_qml_module(lotto_ui_neumorphic
    URI LottoUi
    VERSION 1.0
    NO_PLUGIN
    RESOURCE_PREFIX /qt/qml/
    QML_FILES ...   # 从原 src/ui/CMakeLists.txt 整体搬迁
)
```

`src/ui/CMakeLists.txt`：从 `add_library(lotto_ui ...)` 移除
`classic/...` 与 `classic/resources.qrc`；删除 `qt_add_qml_module` 段与
`target_sources(... neumorphic ...)`；删除对 `Qt6::Qml` / `Qt6::QuickWidgets`
的链接（核心不再需要）。在文件末尾：

```cmake
add_subdirectory(classic)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    add_subdirectory(neumorphic)
endif()
```

`lottostylecatalog.h` 删除 `lottoStyleCatalog()` 声明，保留
`LottoStyleDefinition`；删除 `lottostylecatalog.cpp`。

`lottoclassicview.cpp` 中把资源初始化收进本组件：在构造函数体开头调用
`Q_INIT_RESOURCE(resources);`（幂等），并删除
`main.cpp` / 测试里对应的初始化调用。

- [ ] **Step 4: 修正测试链接**

`tests/CMakeLists.txt`：

- `LottoClassicViewTest` → 链接 `lotto_ui_classic`
- `LottoNeumorphicViewTest` → 链接 `lotto_ui_neumorphic`
- `LottoQmlSmokeTest` → 链接 `lotto_ui_neumorphic`
- `LottoViewShellTest` / `LottoUiTest` / `LottoUiIsolationTest` → 保持只链
  `lotto_ui`
- `MainWindowTest` → 暂保持链接 `lotto_app`（Task 5 处理）

`tst_lottoclassicview.cpp` 的 `catalogShouldContainClassic` 改为在本文件内
构造目录断言（Task 5 会改为调用 `buildStyleCatalog()`）：

```cpp
void classicViewCanBeRegisteredInACatalog()
{
    LottoStyleDefinition definition{
        {QStringLiteral("classic"), QStringLiteral("经典")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoClassicView(host, parent);
        }};
    QCOMPARE(definition.info.id, QStringLiteral("classic"));
    QVERIFY(static_cast<bool>(definition.create));
}
```

- [ ] **Step 5: 运行全量测试**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期：12/12 通过（新增 `LottoUiIsolationTest`）。

- [ ] **Step 6: 证明核心库不拖入 Quick**

```bash
grep -n "Quick\|Qml" build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/CMakeFiles/LottoUiIsolationTest.dir/linkLibs.rsp 2>/dev/null || \
  grep -rn "libQt6Quick" build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/tests/CMakeFiles/LottoUiIsolationTest.dir/ 2>/dev/null
```

预期：无 `Quick` 相关条目。

- [ ] **Step 7: 提交**

```bash
git add src/ui tests
git commit -m "refactor(ui): 风格拆为 lotto_ui_classic / lotto_ui_neumorphic 两个组件"
```

---

### Task 4: `MainWindow` 并入 UI 核心库，`lotto_app` 改为 `lotto_main`

**Files:**
- Move: `src/app/mainwindow.h` → `src/ui/mainwindow.h`
- Move: `src/app/mainwindow.cpp` → `src/ui/mainwindow.cpp`
- Move: `src/app/mainwindow.ui` → `src/ui/mainwindow.ui`
- Modify: `src/ui/CMakeLists.txt`
- Modify: `src/app/CMakeLists.txt`
- Modify: `tests/tst_mainwindow.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 3 的三个目标。
- Produces: `lotto_ui` 内含 `MainWindow`；`lotto_app` 不再是层库（Task 5 改造）。

- [ ] **Step 1: 写失败测试**

`tests/tst_mainwindow.cpp` 增加一条断言，验证窗口壳把视图创建完全交给宿主：

```cpp
void windowShellOwnsNoNumberWidgetsItself()
{
    qDebug() << "验证窗口壳自身不直接创建号码控件(全部来自视图)";
    QVERIFY(m_window->viewShell() != nullptr);
    // 号码标签的父链必须落在当前视图内, 而不是窗口壳的直接子控件
    auto *label = frontLabel(0, 0);
    QVERIFY(label != nullptr);
    QVERIFY(m_window->viewShell()->currentView() != nullptr);
    QVERIFY(label->isAncestorOf(m_window->viewShell()->currentView())
            || m_window->viewShell()->currentView()->isAncestorOf(label));
}
```

真正的结构性证据在 Step 2 的编译失败：移动文件后 `lotto_app` 不再提供
`mainwindow.h`。

- [ ] **Step 2: 运行确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期：`MainWindowTest` / 可执行目标找不到 `mainwindow.h`（若只移动文件不
改 CMake）。

- [ ] **Step 3: 移动并登记**

用 `git mv` 搬三个文件到 `src/ui/`。

`src/ui/CMakeLists.txt` 的 `add_library(lotto_ui STATIC ...)` 源列表加入
`mainwindow.h mainwindow.cpp mainwindow.ui`。

`src/app/CMakeLists.txt`：把 `add_library(lotto_app STATIC ...)` 直接改名为
`lotto_main`，本任务先只放存储与占位目录：

```cmake
add_library(lotto_main STATIC
    qsettingsstylestore.h qsettingsstylestore.cpp
    maincatalog.h maincatalog.cpp
)
target_include_directories(lotto_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_main PUBLIC
    lotto_ui lotto_ui_classic lotto_adapters lotto_core
    Qt${QT_VERSION_MAJOR}::Widgets)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_link_libraries(lotto_main PUBLIC lotto_ui_neumorphic)
endif()
```

（`qsettingsstylestore.*` 自 `src/ui/` 迁入、`maincatalog.*` 新建；两者在
Task 5 完成内容，本任务先保证目标存在且可链接，避免中间态无法构建。）

可执行目标 `target_link_libraries` 改为 `lotto_main`。`mainwindow.ui` 的
AUTOUIC 由 `lotto_ui` 目标处理。`MainWindowTest` 同步改链 `lotto_main`。

- [ ] **Step 4: 运行全量测试**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期：11/11（`LottoUiIsolationTest` 仍在），`MainWindowTest` 通过
`lotto_ui` 解析到 `mainwindow.h`。

- [ ] **Step 5: 提交**

```bash
git add -A src tests
git commit -m "refactor(ui): MainWindow 并入 UI 核心库, 删除 lotto_app 层库"
```

---

### Task 5: `lotto_main`（圆外的 Main 组件）与具体目录

**Files:**
- Create: `src/app/maincatalog.h`, `src/app/maincatalog.cpp`
- Move: `src/ui/qsettingsstylestore.h/.cpp` → `src/app/`
- Modify: `src/app/CMakeLists.txt`（建立 `lotto_main`）
- Modify: `src/app/main.cpp`
- Test: `tests/tst_lottoui.cpp`（拆出存储测试）,
  `tests/tst_lottoclassicview.cpp`, `tests/tst_mainwindow.cpp`,
  `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 3/4 的目标与入口。
- Produces:
  - `QVector<LottoStyleDefinition> buildStyleCatalog();`（`src/app/maincatalog.h`）
  - `LottoStyleStore` 实现 `QSettingsStyleStore` 位于 `src/app/`
  - `lotto_main`（STATIC）：`qsettingsstylestore.*` + `maincatalog.*`

- [ ] **Step 1: 写失败测试**

新建 `tests/tst_maincatalog.cpp`：

```cpp
#include <QTest>

#include "lottoclassicview.h"
#include "maincatalog.h"

class TestMainCatalog : public QObject
{
    Q_OBJECT
private slots:
    void firstEntryIsClassic()
    {
        qDebug() << "验证 Main 目录首项为经典视图";
        const auto catalog = buildStyleCatalog();
        QVERIFY(!catalog.isEmpty());
        QCOMPARE(catalog.first().info.id, QStringLiteral("classic"));
    }

    void qt6CatalogContainsNeumorphic()
    {
        qDebug() << "验证 Qt6 下目录含新拟物条目";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        bool found = false;
        for (const auto &definition : buildStyleCatalog())
            if (definition.info.id == QStringLiteral("neumorphic"))
                found = true;
        QVERIFY(found);
#else
        QSKIP("Qt5 构建无新拟物风格");
#endif
    }

    void everyEntryHasAFactoryAndUniqueId()
    {
        qDebug() << "验证目录条目均有工厂且 id 唯一";
        QStringList ids;
        for (const auto &definition : buildStyleCatalog()) {
            QVERIFY(static_cast<bool>(definition.create));
            QVERIFY(!ids.contains(definition.info.id));
            ids << definition.info.id;
        }
    }
};

QTEST_MAIN(TestMainCatalog)
#include "tst_maincatalog.moc"
```

`tests/tst_lottoui.cpp` 删除 `styleStore_shouldRoundTrip`，其内容迁入新
`tests/tst_qsettingsstylestore.cpp`（同断言，链接 `lotto_main`）。

`tests/CMakeLists.txt` 追加：

```cmake
add_lotto_test(LottoMainCatalogTest tst_maincatalog.cpp)
target_link_libraries(LottoMainCatalogTest PRIVATE
    lotto_main lotto_ui_classic lotto_ui lotto_adapters lotto_application lotto_core)
register_lotto_test(LottoMainCatalogTest LottoMainCatalogTest -platform offscreen)

add_lotto_test(QSettingsStyleStoreTest tst_qsettingsstylestore.cpp)
target_link_libraries(QSettingsStyleStoreTest PRIVATE lotto_main)
target_include_directories(QSettingsStyleStoreTest PRIVATE ${CMAKE_SOURCE_DIR}/src/app)
register_lotto_test(QSettingsStyleStoreTest QSettingsStyleStoreTest -platform offscreen)
```

- [ ] **Step 2: 运行确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期：`maincatalog.h: No such file or directory` / 目标不存在。

- [ ] **Step 3: 实现 Main 组件**

`src/app/maincatalog.h`：

```cpp
QVector<LottoStyleDefinition> buildStyleCatalog();
```

`src/app/maincatalog.cpp`：Qt6 下追加 `neumorphic` 条目、Qt5 下只注册
`classic`（内容来自原 `lottostylecatalog.cpp`，改为 `classic` 条目固定第一）。
两个具体视图头在此文件内 include——这是全仓唯一允许出现两个具体风格类名
的地方。

`lotto_main` 目标已在 Task 4 建立，本步骤只填充两个源文件的内容。注意
`lotto_main` 只被 exe 与 Main 级测试链接（Global Constraints）。

- [ ] **Step 4: 接线并删除旧目录函数引用**

`src/app/main.cpp`：`MainWindow window(&controller, &presenter, &styleStore,
buildStyleCatalog(), LottoPresenter::initialState());`，并删除
`Q_INIT_RESOURCE(resources)`（已移入经典组件）。

`tests/tst_lottoclassicview.cpp` 删除
`classicViewCanBeRegisteredInACatalog`（覆盖已由 `LottoMainCatalogTest` 承担）。
`tests/tst_mainwindow.cpp` 的 `realCatalog_neuomorphicEntryCanBeActivated`
改为 `rebuildWindow(buildStyleCatalog())`；其默认混合目录改为
`buildStyleCatalog()` 首项 + `FakeLottoView`（保持不依赖 QML 的确定性）。

`tests/CMakeLists.txt`：`MainWindowTest` 改链 `lotto_main`（并保留
`lotto_ui lotto_adapters ...`）。

- [ ] **Step 5: 运行全量测试**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期：13/13 通过。

- [ ] **Step 6: 验证"Main 之外零具体类名"**

```bash
grep -rn "LottoClassicView\|LottoNeumorphicView" src --include=*.h --include=*.cpp \
  | grep -v "^src/ui/classic/" | grep -v "^src/ui/neumorphic/" | grep -v "^src/app/"
```

预期：无输出。（`src/app/` 内仅 `maincatalog.cpp` 命中，属允许范围。）

- [ ] **Step 7: 提交**

```bash
git add -A src tests
git commit -m "refactor(app): Main 组件承载存储与具体风格目录"
```

---

### Task 6: 文档与架构图对齐

**Files:**
- Modify: `CLAUDE.md`
- Modify: `doc/architecture.md`
- Modify: `doc/module-dependency.puml`, `doc/control-flow.puml`
- Modify: `docs/superpowers/specs/2026-11-20-architecture-alignment-design.md`
  （状态改为已实施）

**Interfaces:**
- Consumes: 全部前序任务的最终结构。
- Produces: 无代码接口。

- [ ] **Step 1: 更新 CLAUDE.md 分层叙事**

把"五层 `app → ui → adapters`"改为四个圆 + Main：

- `src/domain/` Entities、`src/application/` Use Cases、
  `src/adapters/` Interface Adapters（含 `lottoviewstate.h`）、
  `src/ui/` Frameworks & Drivers（端口/宿主/过渡/窗口壳）、
  `src/ui/classic/`、`src/ui/neumorphic/` 为可插拔风格组件、
  `src/app/` 为圆外的 Main（`main.cpp` + `lotto_main`）。
- 明确写出两条硬约束：核心库不链接风格库；运行期无人依赖 `lotto_main`。
- 测试目标数量更新（11 → 13），并说明 `LottoUiIsolationTest` 是链接隔离
  的守门测试。

- [ ] **Step 2: 更新 doc/ 架构文档与 puml**

`doc/architecture.md` 的分层表、依赖方向、洋葱图映射改为新版；
`doc/module-dependency.puml` 与 `doc/control-flow.puml` 更新节点与箭头。

- [ ] **Step 3: 重新渲染 SVG（若本机可用）**

```bash
which plantuml || echo "本机无 PlantUML: 用 VS Code PlantUML 插件或 kroki.io 渲染"
```

若不可用，在 `doc/architecture.md` 顶部注明 `.svg` 待重新渲染，并把
`.puml` 视为权威源。

- [ ] **Step 4: 运行全量测试并提交**

```bash
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
git add CLAUDE.md doc docs
git commit -m "docs: 分层叙事改为四个圆 + Main, 更新架构图"
```

---

## 自检记录

- 规格覆盖：§5 结构→T3/T4/T5；§6 决策→T1/T2/T3/T5；§7 组件职责→T3/T4/T5；
  §7.1 初始 VM→T2；§7.2 常量新家→T1；§7.3 Main 可测性→T5；
  §8 行为清单→T2/T3/T5 的测试；§9 风险→T3 Step 6、T5 Step 6；
  §10 留给实施者→T6 Step 3；§11 顺序→任务编号。
- 类型一致性：`LottoViewText` / `LottoViewShape` 在 T1 定义、T2–T6 一致使用；
  `buildStyleCatalog()` 在 T5 定义并被 T5/T6 引用；
  `LottoViewShell` 的 `initialState` 参数在 T2 定义、T5 透传。
- Review Focus 的五条各落在：Qt5 构建→T3/T4 的 CMake 条件；
  链接隔离→T3 Step 6；首帧→T2 的两条新测试；
  Main 外零类名→T5 Step 6；旧架构残留→T4 删除 `lotto_app` + T6 文档。
