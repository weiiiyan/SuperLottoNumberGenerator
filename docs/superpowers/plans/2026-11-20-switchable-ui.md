# 可切换 UI 组件层 + 新拟物视图 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把现有 UI 抽成可替换组件层 `src/ui/`, 新增 QML 实现的"新拟物"
视图, 两套视图可运行时切换并带过渡动画与偏好持久化。

**Architecture:** 新增 `lotto_ui` 静态库, 位于 `app → ui → adapters` 之间。
视图统一继承 `LottoView`(QWidget 子类), 只消费 `LottoViewState` 并发
`generateRequested`/`lockRequested` 信号; `LottoViewShell` 持有
`QStackedWidget`, 负责懒创建、状态重放、信号转发、切换过渡与偏好读写,
并实现视图使用的 `LottoStyleHost` 端口。`MainWindow` 退化为薄壳。

**Tech Stack:** C++17, Qt 6.11.1 (Widgets + QuickWidgets + Qml +
QtQuick.Effects), CMake, QTest/CTest, MinGW 64-bit 桌面 + Android arm64。

**Spec:** [docs/superpowers/specs/2026-11-20-switchable-ui-design.md](../specs/2026-11-20-switchable-ui-design.md)

## Global Constraints

- 依赖只指向内层: `app → ui → adapters → application → domain`;
  domain / application / adapters 三层本次零改动(除 §Task 2 的
  `LottoPresenter` 展示状态调整, 属 adapters 层的端口数据形状变化)。
- 视图不得包含 `LottoTicket` / `LottoInteractor` / `TicketRepository`,
  只依赖 `LottoViewState` 与 `LottoStyleHost`。
- 源码注释用中文, 代码行 ≤100 字符, 注释 ≤80 字符; 类与公共函数用
  `/*! ... */` QDoc 风格。
- 所有 UI 文案单一来源在 `LottoPresenter` 常量; 文案中不得出现 emoji。
- Qt 6 专属代码(QML 模块、`QQuickWidget`)必须包在
  `if(QT_VERSION_MAJOR GREATER_EQUAL 6)` 内; Qt 5 下风格目录只含经典视图。
- 静态库的 Qt 依赖必须 `PUBLIC`。
- 新增/修改 CMakeLists 后必须重新运行 cmake 配置以刷新
  `compile_commands.json`。
- 构建目录固定为 `build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug`,
  Qt 前缀 `D:/Software/Qt6.11/6.11.1/mingw_64`。
- 测试运行一律带 `-platform offscreen`(GUI 测试), 其余测试无此要求。
- 每个任务结束必须 `ctest` 全绿并在该任务内提交。

## Review Focus

1. **切换请求重入**: 用户在过渡动画进行中再次点击另一风格 — 预期旧动画
   立即结束、只保留最后一次目标, 不出现两个覆盖层或空白。
2. **空目录 / 视图工厂返回 null**: 预期宿主不崩溃、`currentStyleId()`
   为空串、后续切换请求被忽略。
3. **偏好值为空串或损坏**: INI 里 `ui/styleId` 存在但为空/未知 — 预期
   回退目录首项, 且不回写用户原值。
4. **切换发生在首次状态到达之前**: 预期新视图要么不收状态、要么收一次
   空状态, 均不崩溃, 之后正常接收。
5. **resize 过程中触发切换**: 快照尺寸与窗口不一致 — 预期过渡覆盖层
   铺满当前尺寸, 结束后释放, 不留残留子控件。

---

### Task 1: QML 构建集成冒烟

**Files:**

- Modify: `CMakeLists.txt`
- Create: `src/ui/CMakeLists.txt`
- Create: `src/ui/LottoUi/NeumorphicView.qml`
- Modify: `tests/CMakeLists.txt`
- Test: `tests/tst_lottoqmlsmoke.cpp`

**Interfaces:**

- Consumes: 无
- Produces: 静态库目标 `lotto_ui`; QML 模块 `LottoUi`, 资源位于
  `:/qt/qml/LottoUi/`, 根类型 `NeumorphicView`(占位实现, Task 7 替换)。

- [ ] **Step 1: 建最小 QML 模块**

`src/ui/LottoUi/NeumorphicView.qml`:

```qml
import QtQuick

Rectangle {
    color: "#E0E5EC"
    signal generateClicked()
    signal lockClicked()
    signal styleSwitchRequested(string styleId)
}
```

`src/ui/CMakeLists.txt`:

```cmake
add_library(lotto_ui STATIC)
target_include_directories(lotto_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lotto_ui PUBLIC lotto_adapters
    Qt${QT_VERSION_MAJOR}::Widgets)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Qml QuickWidgets)
    # 静态库 + 静态 QML 模块: 生成 qrc 与 qmldir, 不生成插件
    qt_add_qml_module(lotto_ui
        URI LottoUi
        VERSION 1.0
        NO_PLUGIN
        RESOURCE_PREFIX /qt/qml/
        QML_FILES
            LottoUi/NeumorphicView.qml
    )
    target_link_libraries(lotto_ui PUBLIC Qt6::Qml Qt6::QuickWidgets)
endif()
```

顶层 `CMakeLists.txt` 在 `add_subdirectory(src/adapters)` 之后加
`add_subdirectory(src/ui)`, 并把 `add_subdirectory(src/app)` 移到其后。

- [ ] **Step 2: 写冒烟测试**

`tests/tst_lottoqmlsmoke.cpp`:

```cpp
#include <QFile>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTest>

class TestQmlSmoke : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { qputenv("QT_QUICK_BACKEND", "software"); }

    void moduleShouldBeInResources()
    {
        QVERIFY2(QFile::exists(QStringLiteral(":/qt/qml/LottoUi/qmldir")),
                 "LottoUi qmldir 未进资源: 检查静态库资源初始化");
    }

    void quickWidgetShouldLoadModule()
    {
        QQuickWidget quick;
        quick.setResizeMode(QQuickWidget::SizeRootObjectToView);
        quick.loadFromModule("LottoUi", "NeumorphicView");
        QTRY_VERIFY_WITH_TIMEOUT(quick.status() != QQuickWidget::Loading, 5000);
        if (quick.status() != QQuickWidget::Ready)
            QSKIP("当前平台 Quick 未能初始化");
        QVERIFY(quick.rootObject() != nullptr);
    }
};

QTEST_MAIN(TestQmlSmoke)
#include "tst_lottoqmlsmoke.moc"
```

`tests/CMakeLists.txt` 在 GUI 测试区追加(仅 Qt6):

```cmake
if(TEST_QT_MAJOR GREATER_EQUAL 6)
    find_package(Qt${TEST_QT_MAJOR} REQUIRED COMPONENTS QuickWidgets)
    add_lotto_test(LottoQmlSmokeTest tst_lottoqmlsmoke.cpp)
    target_link_libraries(LottoQmlSmokeTest PRIVATE lotto_ui
        Qt${TEST_QT_MAJOR}::QuickWidgets)
    register_lotto_test(LottoQmlSmokeTest LottoQmlSmokeTest -platform offscreen)
endif()
```

- [ ] **Step 3: 配置并运行, 确认失败或通过**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Software/Qt6.11/6.11.1/mingw_64
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoQmlSmokeTest --output-on-failure
```

预期: 两个用例通过。

- [ ] **Step 4: 资源被裁剪时修正**

若 `moduleShouldBeInResources` 失败, 说明静态库中的 qrc 初始化器未被链接。
在 `src/ui/lottoviewshell.cpp`(Task 4 建立)或任意 `lotto_ui` 的 .cpp 中调用
生成的资源初始化函数: 在构建目录搜索 `qInitResources_` 前缀的符号名(通常
为 `qInitResources_<模块名>_qmlcache` 或 `qInitResources_qml`), 以
`Q_INIT_RESOURCE(<名字>)` 引入。若无法确定符号, 去掉 `NO_PLUGIN` 让 CMake
生成插件 target(静态库下名称为 `lotto_uiplugin`), 在
`src/ui/CMakeLists.txt` 中显式
`target_link_libraries(lotto_ui PUBLIC lotto_uiplugin)` 并在同一 .cpp 中
`Q_IMPORT_QML_PLUGIN(LottoUiPlugin)`(`LottoUiPlugin` 是宏参数即插件类名,
与 target 名不同)。修改后重跑 Step 3 直至两个用例通过。

- [ ] **Step 5: Android 构建冒烟**

```bash
cmake -S . -B build/Qt_6_11_1_for_Android_arm64_v8a-Debug \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a
cmake --build build/Qt_6_11_1_for_Android_arm64_v8a-Debug
```

预期: 构建成功(测试在 Android 跳过)。真机 QML 显示留到 Task 10 验证。

- [ ] **Step 6: 提交**

```bash
git add CMakeLists.txt src/ui tests/CMakeLists.txt tests/tst_lottoqmlsmoke.cpp
git commit -m "build: 引入 lotto_ui 层与静态库 QML 模块冒烟测试"
```

---

### Task 2: 展示状态去 emoji、拆时间、补生成按钮文案

**Files:**

- Modify: `src/adapters/lottopresenter.h`
- Modify: `src/adapters/lottopresenter.cpp`
- Modify: `src/app/mainwindow.cpp`(临时适配旧视图, Task 6 删除)
- Test: `tests/tst_lottopresenter.cpp`, `tests/tst_mainwindow.cpp`

**Interfaces:**

- Produces: `LottoViewState` 字段 `groups`、`timeLabelText`、`timeValueText`、
  `generateButtonText`、`generateEnabled`、`lockButtonText`、
  `lockButtonEnabled`、`lockButtonChecked`;
  `LottoPresenter::TIME_LABEL` / `TIME_PLACEHOLDER` / `TIME_FORMAT` /
  `LOCK_TEXT_LOCKED` / `LOCK_TEXT_UNLOCKED` / `GENERATE_TEXT` /
  `NUMBER_PLACEHOLDER`。`TIME_PREFIX` 删除。

- [ ] **Step 1: 改测试(先红)**

`tests/tst_lottopresenter.cpp`: 把 `state.timeText` 断言改为
`state.timeLabelText` + `state.timeValueText`; 有效时间用例断言
`timeValueText == "2026-01-01 12:00:00"`, 无效时间断言
`timeValueText == TIME_PLACEHOLDER` 且 `timeLabelText == TIME_LABEL`。
新增用例:

```cpp
void constants_shouldNotContainEmoji()
{
    const QVector<QString> texts = {
        QString::fromUtf8(LottoPresenter::TIME_LABEL),
        QString::fromUtf8(LottoPresenter::LOCK_TEXT_LOCKED),
        QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED),
        QString::fromUtf8(LottoPresenter::GENERATE_TEXT),
    };
    for (const QString &text : texts)
        for (const QChar ch : text)
            QVERIFY2(ch.unicode() < 0x2600,
                     QString("文案含图标字符: '%1'").arg(text).toUtf8());
}

void state_shouldCarryGenerateButtonText()
{
    const LottoViewState state = LottoPresenter::initialState();
    QCOMPARE(state.generateButtonText,
             QString::fromUtf8(LottoPresenter::GENERATE_TEXT));
}
```

`tests/tst_mainwindow.cpp`: `timeLabel()` 断言暂时兼容(值标签尚未存在,
本任务让 `MainWindow` 把 `timeLabelText + timeValueText` 拼到一个标签)。

- [ ] **Step 2: 运行, 确认失败**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R "LottoPresenterTest|MainWindowTest" --output-on-failure
```

预期: 编译失败(字段不存在)。

- [ ] **Step 3: 改 `LottoViewState` 与常量**

`src/adapters/lottopresenter.h`:

```cpp
struct LottoViewState {
    QVector<QStringList> groups;
    QString timeLabelText;      /*!< 时间标签标题, 如 "生成时间：" */
    QString timeValueText;      /*!< 时间值或占位, 如 "2026-01-01 08:00:00" */
    QString generateButtonText;
    bool    generateEnabled = true;
    QString lockButtonText;
    bool    lockButtonEnabled = false;
    bool    lockButtonChecked = false;
};

static constexpr const char *TIME_LABEL        = "生成时间：";
static constexpr const char *TIME_PLACEHOLDER  = "年-月-日 时:分:秒";
static constexpr const char *TIME_FORMAT       = "yyyy-MM-dd HH:mm:ss";
static constexpr const char *LOCK_TEXT_LOCKED   = "解锁生成";
static constexpr const char *LOCK_TEXT_UNLOCKED = "锁定号码";
static constexpr const char *NUMBER_PLACEHOLDER = "?";
static constexpr const char *GENERATE_TEXT      = "生成号码";
```

`buildViewState()`: 有效时间 → `timeValueText = time.toString(TIME_FORMAT)`;
无效 → `TIME_PLACEHOLDER`; 两者都设 `timeLabelText = TIME_LABEL`;
设 `state.generateButtonText = GENERATE_TEXT`。

- [ ] **Step 4: 临时适配旧 `MainWindow`**

`mainwindow.cpp` 的 `applyState` 改为
`m_timeLabel->setText(state.timeLabelText + state.timeValueText);`
并把 `m_btnGenerate->setEnabled(state.generateEnabled)` 前加
`m_btnGenerate->setText(state.generateButtonText);`。删除 `init()` 中
`QString::fromUtf8(LottoPresenter::GENERATE_TEXT)` 的初始化。

- [ ] **Step 5: 运行, 确认通过**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期: 全部通过。

- [ ] **Step 6: 提交**

```bash
git add src/adapters/lottopresenter.h src/adapters/lottopresenter.cpp \
  src/app/mainwindow.cpp tests/tst_lottopresenter.cpp tests/tst_mainwindow.cpp
git commit -m "refactor: 展示状态去 emoji、拆时间标签、补生成按钮文案"
```

---

### Task 3: 视图端口、宿主端口、偏好端口与实现

**Files:**

- Create: `src/ui/lottoview.h`, `src/ui/lottoview.cpp`
- Create: `src/ui/lottostylehost.h`
- Create: `src/ui/lottostylestore.h`
- Create: `src/ui/qsettingsstylestore.h`, `src/ui/qsettingsstylestore.cpp`
- Modify: `src/ui/CMakeLists.txt`
- Test: `tests/tst_lottoui.cpp`, `tests/uitesthelpers.h`, `tests/CMakeLists.txt`

**Interfaces:**

- Consumes: `LottoViewState`(Task 2)。
- Produces:
  - `class LottoView : public QWidget` — `styleId()`、`renderState()`、
    `isReady()`、`styleChanged()`、`snapshot()`、信号 `generateRequested()` /
    `lockRequested()`
  - `struct LottoStyleInfo { QString id; QString displayName; };`
  - `class LottoStyleHost` — `availableStyles()`、`currentStyleId()`、
    `requestSwitchTo()`
  - `class LottoStyleStore` — `loadStyleId()`、`saveStyleId()`
  - `class QSettingsStyleStore : public LottoStyleStore`,
    构造 `explicit QSettingsStyleStore(QSettings *settings)`
  - 测试替身 `FakeLottoStyleStore`、`FakeStyleHost`、`FakeLottoView`
    (均无 `Q_OBJECT`, 只在 `tests/` 内使用)

- [ ] **Step 1: 写失败测试**

`tests/uitesthelpers.h`:

```cpp
#ifndef UITESTHELPERS_H
#define UITESTHELPERS_H

#include <QStringList>
#include <QVector>

#include "lottostylehost.h"
#include "lottostylestore.h"
#include "lottoview.h"

/*! 内存版风格偏好端口 */
class FakeLottoStyleStore : public LottoStyleStore
{
public:
    QString storedId;        /*!< 当前值, 空表示未设置 */
    int saveCount = 0;       /*!< saveStyleId() 调用次数 */
    QString loadStyleId() const override { return storedId; }
    void saveStyleId(const QString &id) override { storedId = id; ++saveCount; }
};

/*! 记录切换请求的宿主端口替身 */
class FakeStyleHost : public LottoStyleHost
{
public:
    QVector<LottoStyleInfo> styles;
    QString currentId;
    QStringList requested;   /*!< 历次 requestSwitchTo() 的 id */
    QVector<LottoStyleInfo> availableStyles() const override { return styles; }
    QString currentStyleId() const override { return currentId; }
    void requestSwitchTo(const QString &id) override { requested << id; }
};

/*! 无 Q_OBJECT 的视图替身: 记录渲染与通知, 可模拟就绪与点击 */
class FakeLottoView : public LottoView
{
public:
    FakeLottoView(LottoStyleHost *host, const QString &id, QWidget *parent = nullptr)
        : LottoView(parent), m_host(host), m_id(id) {}
    QString styleId() const override { return m_id; }
    void renderState(const LottoViewState &state) override
    { m_states.append(state); m_lastState = state; }
    bool isReady() const override { return m_ready; }
    void styleChanged(const QString &id) override
    { ++m_styleChangedCount; m_currentStyleId = id; }

    LottoStyleHost *host() const { return m_host; }
    int stateCount() const { return m_states.size(); }
    LottoViewState lastState() const { return m_lastState; }
    int styleChangedCount() const { return m_styleChangedCount; }
    QString currentStyleId() const { return m_currentStyleId; }
    void setReady(bool ready) { m_ready = ready; }
    void simulateGenerate() { emit generateRequested(); }
    void simulateLock() { emit lockRequested(); }

private:
    LottoStyleHost *m_host = nullptr;
    QString m_id;
    bool m_ready = true;
    QVector<LottoViewState> m_states;
    LottoViewState m_lastState;
    int m_styleChangedCount = 0;
    QString m_currentStyleId;
};

#endif // UITESTHELPERS_H
```

`tests/tst_lottoui.cpp`:

```cpp
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "qsettingsstylestore.h"
#include "uitesthelpers.h"

class TestLottoUi : public QObject
{
    Q_OBJECT
private slots:
    void viewDefaults_shouldBeReadyAndSnapshot()
    {
        FakeLottoView view(nullptr, QStringLiteral("x"));
        QVERIFY(view.isReady());
        view.renderState(LottoViewState());
        QCOMPARE(view.stateCount(), 1);
        view.snapshot();   // 不崩溃即可
    }

    void styleStore_shouldRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        QSettingsStyleStore store(&settings);
        QCOMPARE(store.loadStyleId(), QString());
        store.saveStyleId(QStringLiteral("neumorphic"));
        QCOMPARE(store.loadStyleId(), QStringLiteral("neumorphic"));
    }
};

QTEST_MAIN(TestLottoUi)
#include "tst_lottoui.moc"
```

视图替身是 `QWidget`, 必须用 `QTEST_MAIN`(创建 `QApplication`), 并在
CMake 中注册 `-platform offscreen`。

- [ ] **Step 2: 运行, 确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期: 编译失败(头文件不存在)。

- [ ] **Step 3: 实现端口**

`src/ui/lottoview.h` 按 Spec §6.2 定义; `lottoview.cpp` 提供
`isReady() { return true; }`、`styleChanged()` 空实现、
`snapshot() { return grab(); }`。

`src/ui/lottostylehost.h`:

```cpp
struct LottoStyleInfo {
    QString id;            /*!< 稳定 id, 用于持久化 */
    QString displayName;   /*!< 展示名 */
};

class LottoStyleHost
{
public:
    virtual ~LottoStyleHost() = default;
    virtual QVector<LottoStyleInfo> availableStyles() const = 0;
    virtual QString currentStyleId() const = 0;
    virtual void requestSwitchTo(const QString &styleId) = 0;
};
```

`src/ui/lottostylestore.h` 按 Spec §6.2。`qsettingsstylestore.cpp`:
键 `"ui/styleId"`, `loadStyleId()` 返回 `settings->value(KEY).toString()`,
`saveStyleId()` 写入后 `settings->sync()`。

- [ ] **Step 4: 接入 CMake 与测试目标**

`src/ui/CMakeLists.txt` 的 `add_library` 源列表加
`lottoview.h lottoview.cpp lottostylehost.h lottostylestore.h
qsettingsstylestore.h qsettingsstylestore.cpp`。

`tests/CMakeLists.txt` 追加:

```cmake
add_lotto_test(LottoUiTest tst_lottoui.cpp)
target_link_libraries(LottoUiTest PRIVATE lotto_ui)
register_lotto_test(LottoUiTest LottoUiTest -platform offscreen)
```

- [ ] **Step 5: 运行, 确认通过**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoUiTest --output-on-failure
```

- [ ] **Step 6: 提交**

```bash
git add src/ui tests/uitesthelpers.h tests/tst_lottoui.cpp tests/CMakeLists.txt
git commit -m "feat(ui): 新增视图端口、风格宿主端口与偏好存储"
```

---

### Task 4: LottoViewShell 宿主

**Files:**

- Create: `src/ui/lottostylecatalog.h`(仅 `LottoStyleDefinition` 结构)
- Create: `src/ui/lottoviewshell.h`, `src/ui/lottoviewshell.cpp`
- Modify: `src/ui/CMakeLists.txt`
- Test: `tests/tst_lottoviewshell.cpp`, `tests/CMakeLists.txt`

**Interfaces:**

- Consumes: Task 3 的端口; `LottoController`、`LottoPresenter`。
- Produces:
  - `struct LottoStyleDefinition { LottoStyleInfo info;
    std::function<LottoView *(LottoStyleHost *, QWidget *)> create; };`
  - `LottoViewShell(LottoController *, LottoPresenter *, LottoStyleStore *,
    QVector<LottoStyleDefinition> catalog, int transitionMs = 200,
    QWidget *parent = nullptr)`
  - `LottoView *currentView() const;`、`int transitionMs() const;`
  - 信号 `styleSwitchFailed(const QString &styleId)`

- [ ] **Step 1: 写失败测试**

`tests/tst_lottoviewshell.cpp` 关键用例(全部注入假目录
`makeCatalog()` 返回两个 `FakeLottoView` 条目 `"a"`、`"b"`):

```cpp
void defaultsToFirstEntry()
{
    QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
    QCOMPARE(m_shell->currentView()->styleId(), QStringLiteral("a"));
}

void emptyCatalog_shouldNotCrash()
{
    LottoViewShell shell(m_controller, m_presenter, m_store, {}, 0);
    QCOMPARE(shell.currentStyleId(), QString());
    QVERIFY(shell.currentView() == nullptr);
    shell.requestSwitchTo(QStringLiteral("a"));
    QTest::qWait(10);
    QVERIFY(shell.currentView() == nullptr);
}

void restoresStoredStyle()
{
    m_store->storedId = QStringLiteral("b");
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(), 0);
    QCOMPARE(shell.currentStyleId(), QStringLiteral("b"));
}

void emptyStoredStyle_fallsBackToFirst()
{
    m_store->storedId = QString();
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(), 0);
    QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
    QCOMPARE(m_store->storedId, QString());   // 不回写
}

void switchReplaysLastStateAndNotifies()
{
    m_shell->requestSwitchTo(QStringLiteral("b"));
    QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
    QCOMPARE(m_shell->currentView()->styleId(), QStringLiteral("b"));
    auto *b = static_cast<FakeLottoView *>(m_shell->currentView());
    QCOMPARE(b->stateCount(), 1);
    QCOMPARE(b->lastState().generateButtonText, m_lastButtonText);
    QCOMPARE(b->styleChangedCount(), 1);
    QCOMPARE(m_store->storedId, QStringLiteral("b"));
}

void unknownStyle_shouldFailWithoutPersisting()
{
    QSignalSpy spy(m_shell, &LottoViewShell::styleSwitchFailed);
    m_shell->requestSwitchTo(QStringLiteral("nope"));
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
    QCOMPARE(m_store->saveCount, 0);
}

void notReadyView_shouldFailWithoutPersisting()
{
    // 目录中 "b" 的工厂返回 isReady()==false 的视图
    ...
    QTRY_COMPARE(failSpy.count(), 1);
    QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
    QCOMPARE(m_store->saveCount, 0);
}

void nullViewFromFactory_shouldFail()
{
    // 工厂返回 nullptr
    QTRY_COMPARE(failSpy.count(), 1);
    QVERIFY(m_shell->currentView() != nullptr);
}

void generateSignalForwardsToController()
{
    auto *view = static_cast<FakeLottoView *>(m_shell->currentView());
    view->simulateGenerate();
    QCOMPARE(m_input->generateCalls, 1);   // FakeInputBoundary
}

void switchBeforeAnyState_shouldNotCrash()
{
    // 不调用 presenter.present 直接切换
    m_shell->requestSwitchTo(QStringLiteral("b"));
    QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
    QCOMPARE(static_cast<FakeLottoView *>(m_shell->currentView())->stateCount(), 0);
}

void transitionDisabledByDefaultInTests()
{
    QCOMPARE(m_shell->transitionMs(), 0);
}
```

夹具:`m_input = new FakeInputBoundary`、`m_controller = new LottoController(m_input)`、
`m_presenter = new LottoPresenter`、`m_store = new FakeLottoStyleStore`、
`m_shell = new LottoViewShell(m_controller, m_presenter, m_store,
makeCatalog(), 0)`; `init()` 里先 `m_presenter->present(makeTicket())`
并记录 `m_lastButtonText = LottoPresenter::GENERATE_TEXT`。

- [ ] **Step 2: 运行, 确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期: 编译失败(`LottoViewShell` 不存在)。

- [ ] **Step 3: 实现宿主**

`lottoviewshell.h` 按 Spec §6.3; 私有成员:
`QStackedWidget *m_stack`、`QVector<LottoStyleDefinition> m_catalog`、
`QString m_currentStyleId`、`LottoViewState m_lastState`、
`bool m_hasState = false`、`QPointer<LottoView> m_currentView`、
过渡动画与覆盖层指针(本任务留空, Task 9 填充)。

`requestSwitchTo()` 检查 id 相同则直接返回, 否则
`QMetaObject::invokeMethod(this, [this, id]{ applySwitch(id); },
Qt::QueuedConnection)`。`applySwitch()`:

1. 在 `m_catalog` 中查找 id, 找不到 → `emit styleSwitchFailed(id)` 返回。
2. 若视图尚未创建: 调 `create(this, this)`; 返回 `nullptr` 或
   `!isReady()` → `qWarning` + `emit styleSwitchFailed(id)`, 删除视图, 返回。
3. `stack->addWidget(view)`(若尚未加入), 接
   `generateRequested` → `m_controller->onGenerateRequested`,
   `lockRequested` → `m_controller->onLockRequested`。
4. `m_hasState` 为真 → `view->renderState(m_lastState)`。
5. `stack->setCurrentWidget(view)`; `m_currentStyleId = id`;
   调用新旧视图 `styleChanged(id)`; `m_store->saveStyleId(id)`。
6. 本任务不播放过渡。

构造函数: 读 `m_store->loadStyleId()`, 命中目录则创建该视图, 否则用
目录首项; 目录为空则不创建任何视图。连接
`m_presenter->viewStateChanged` → 缓存到 `m_lastState` /
`m_hasState=true` → 若当前视图存在则 `renderState(m_lastState)`。

- [ ] **Step 4: 接入 CMake 与测试目标**

`src/ui/CMakeLists.txt` 源列表加 `lottostylecatalog.h lottoviewshell.h
lottoviewshell.cpp`; `tests/CMakeLists.txt` 追加 `LottoViewShellTest`
(链接 `lotto_ui lotto_application lotto_core`, 注册 `-platform offscreen`)。

- [ ] **Step 5: 运行, 确认通过**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoViewShellTest --output-on-failure
```

- [ ] **Step 6: 提交**

```bash
git add src/ui tests/tst_lottoviewshell.cpp tests/CMakeLists.txt
git commit -m "feat(ui): 实现视图宿主(栈、重放、信号转发、失败回退)"
```

---

### Task 5: 经典视图抽取与风格目录

**Files:**

- Create: `src/ui/classic/lottoclassicview.h`, `classic/lottoclassicview.cpp`
- Move: `src/app/style.qss` → `src/ui/classic/style.qss`
- Move: `src/app/resources.qrc` → `src/ui/classic/resources.qrc`
- Create: `src/ui/lottostylecatalog.cpp`
- Modify: `src/ui/CMakeLists.txt`, `src/app/CMakeLists.txt`
- Test: `tests/tst_lottoclassicview.cpp`, `tests/CMakeLists.txt`

**Interfaces:**

- Consumes: Task 3 端口、Task 4 的 `LottoStyleDefinition`。
- Produces:
  - `class LottoClassicView : public LottoView`,
    `static constexpr const char *STYLE_ID = "classic"`,
    构造 `LottoClassicView(LottoStyleHost *host, QWidget *parent = nullptr)`,
    根控件 objectName `classicRoot`, 保留既有全部 objectName, 新增
    `btnStyleSwitch`
  - `QVector<LottoStyleDefinition> lottoStyleCatalog();`
    (本任务只含 `classic`; Task 8 追加 `neumorphic`)

- [ ] **Step 1: 写失败测试**

`tests/tst_lottoclassicview.cpp`:

```cpp
void rendersStateIntoWidgets()
{
    FakeStyleHost host;
    LottoClassicView view(&host);
    LottoTicket ticket = makeTicket();          // testhelpers.h
    view.renderState(LottoPresenter::buildViewState(ticket));
    QCOMPARE(view.findChild<QLabel *>("frontLabel_0_0")->text(),
             QStringLiteral("01"));
    QCOMPARE(view.findChild<QLabel *>("timeValueLabel")->text().isEmpty(), false);
    QCOMPARE(view.findChild<QLabel *>("timeLabel")->text(),
             QString::fromUtf8(LottoPresenter::TIME_LABEL));
}

void buttonsEmitRequestSignals()
{
    FakeStyleHost host;
    LottoClassicView view(&host);
    QSignalSpy gen(&view, &LottoView::generateRequested);
    QSignalSpy lock(&view, &LottoView::lockRequested);
    QTest::mouseClick(view.findChild<QPushButton *>("btnGenerate"), Qt::LeftButton);
    QTest::mouseClick(view.findChild<QPushButton *>("btnLock"), Qt::LeftButton);
    QCOMPARE(gen.count(), 1);
    QCOMPARE(lock.count(), 1);
}

void styleMenuListsHostStylesAndRequests()
{
    FakeStyleHost host;
    host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                    {QStringLiteral("neo"), QStringLiteral("新拟物")} };
    host.currentId = QStringLiteral("classic");
    LottoClassicView view(&host);
    auto *btn = view.findChild<QToolButton *>("btnStyleSwitch");
    QVERIFY(btn != nullptr);
    auto *menu = btn->menu();
    QCOMPARE(menu->actions().size(), 2);
    QVERIFY(menu->actions().at(0)->isChecked());
    menu->actions().at(1)->trigger();
    QCOMPARE(host.requested, QStringList{ QStringLiteral("neo") });
}

void styleChangedUpdatesCheckedAction()
{
    FakeStyleHost host;
    host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                    {QStringLiteral("neo"), QStringLiteral("新拟物")} };
    LottoClassicView view(&host);
    view.styleChanged(QStringLiteral("neo"));
    auto *menu = view.findChild<QToolButton *>("btnStyleSwitch")->menu();
    QVERIFY(menu->actions().at(1)->isChecked());
    QVERIFY(!menu->actions().at(0)->isChecked());
}

void catalogShouldContainClassic()
{
    const auto catalog = lottoStyleCatalog();
    QVERIFY(!catalog.isEmpty());
    QCOMPARE(catalog.first().info.id, QStringLiteral("classic"));
}
```

- [ ] **Step 2: 运行, 确认失败**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

预期: 编译失败。

- [ ] **Step 3: 迁移实现**

把 `MainWindow` 的 `init()`/`applyState()`/`buildLayout()`/
`rebuildGroupRows()`/`updateLabelSizes()`/`createGroupRow()`/
`spacingForWidth()`/`clearLayout()` 与常量整体搬到 `LottoClassicView`,
改动:

- 根控件构造时 `setObjectName(QStringLiteral("classicRoot"))`;
  `init()` 改名 `initUi()` 在构造函数内调用; 不再使用 `Ui::MainWindow`
  (`ui->centralwidget` → `this`)。
- `m_timeLabel` 拆为 `m_timeLabel`(objectName `timeLabel`)与
  `m_timeValueLabel`(objectName `timeValueLabel`), 分别取
  `state.timeLabelText` / `state.timeValueText`。
- 按钮 `clicked` 连接改为 `emit generateRequested()` / `emit lockRequested()`。
- 新增 `btnStyleSwitch`(`QToolButton`, `PopupMode::InstantPopup`,
  objectName `btnStyleSwitch`), 菜单由 `host->availableStyles()` 构建
  (可勾选 `QActionGroup`), `triggered` → `host->requestSwitchTo(id)`。
  `styleChanged()` 更新勾选态。
- 样式表加载 `:/style.qss` 改为 `:/classic/style.qss`;
  `resources.qrc` 移入 `src/ui/classic/` 并改 `<file>style.qss</file>` →
  `<file>classic/style.qss</file>` + `<qresource prefix="/classic">`,
  保持文件基名 `resources.qrc` 以维持 `Q_INIT_RESOURCE(resources)`。
- `src/app/CMakeLists.txt` 从 `lotto_app` 源列表移除
  `mainwindow.ui` 之外的 `resources.qrc`,`lotto_app` 链接 `lotto_ui`。
- `lottostylecatalog.cpp`:

```cpp
QVector<LottoStyleDefinition> lottoStyleCatalog()
{
    QVector<LottoStyleDefinition> catalog;
    catalog.append({ {QStringLiteral("classic"), QStringLiteral("经典")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoClassicView(host, parent);
        } });
    return catalog;
}
```

- [ ] **Step 4: 运行, 确认通过**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R "LottoClassicViewTest|LottoQmlSmokeTest|LottoUiTest" --output-on-failure
```

预期: 通过。此时 `MainWindow` 仍保留旧代码(`src/app`)尚未接线,
`MainWindowTest` 可能因资源前缀变更失败 — 若失败, 本任务内同步修正
`mainwindow.cpp` 的 `:/style.qss` → `:/classic/style.qss`。

- [ ] **Step 5: 提交**

```bash
git add src/ui src/app tests/tst_lottoclassicview.cpp tests/CMakeLists.txt
git commit -m "feat(ui): 抽取经典视图为可替换组件并新增风格目录"
```

---

### Task 6: MainWindow 薄壳接线与集成测试适配

**Files:**

- Modify: `src/app/mainwindow.h`, `src/app/mainwindow.cpp`
- Modify: `src/app/main.cpp`
- Modify: `tests/tst_mainwindow.cpp`
- Test: 同上

**Interfaces:**

- Consumes: `LottoViewShell`、`lottoStyleCatalog()`。
- Produces:
  - `MainWindow(LottoController *controller, LottoPresenter *presenter,
    LottoStyleStore *store,
    QVector<LottoStyleDefinition> catalog = lottoStyleCatalog(),
    QWidget *parent = nullptr)`
  - `LottoViewShell *viewShell() const;`

- [ ] **Step 1: 改写测试夹具与断言**

`tests/tst_mainwindow.cpp`:

- 夹具改为两个入口: 默认 `makeWindow()` 注入**混合目录**
  `makeTestCatalog()` —— 条目 0 是真实经典视图(`LottoClassicView`),
  条目 1 是 `FakeLottoView`(`"fake-b"`), 这样既有控件断言与切换用例都能
  工作, 且不依赖 QML; 另一个 `makeWindowWithCatalog(const
  QVector<LottoStyleDefinition> &catalog)` 供持久化/真实目录用例使用。
  两者共用同一个 `FakeLottoStyleStore` 实例 `m_store`, 且只重建窗口 +
  交互器(`rebuildWindow(catalog)`), 不重建 `m_store`。
- `m_window = new MainWindow(m_controller, m_presenter, m_store, catalog);`
- 时间断言:`timeLabel()` 断言等于 `TIME_LABEL`; 新增 `timeValueLabel()`
  断言匹配 `\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}`。
- 控件查找 `m_window->findChild<...>(name)` 不变(经典视图在窗口内),
  另加 `classicRoot()` 断言存在。
- 新增用例(默认混合目录):

```cpp
void switchStyle_keepsTicketState()
{
    QTest::mouseClick(btnGenerate(), Qt::LeftButton);
    const QString before = frontLabel(0, 0)->text();

    auto *shell = m_window->viewShell();
    QVERIFY(shell != nullptr);
    shell->requestSwitchTo(QStringLiteral("fake-b"));
    QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("fake-b"));

    auto *b = static_cast<FakeLottoView *>(shell->currentView());
    QCOMPARE(b->lastState().groups.at(0).at(0), before);

    // 切回经典视图, 断言控件仍显示同一号码
    shell->requestSwitchTo(QStringLiteral("classic"));
    QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("classic"));
    QCOMPARE(frontLabel(0, 0)->text(), before);
}
```

- 新增真实目录冒烟用例(可跳过):

```cpp
void realCatalog_canCreateNeumorphicView()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    rebuildWindow(lottoStyleCatalog());
    auto *shell = m_window->viewShell();
    shell->requestSwitchTo(QStringLiteral("neumorphic"));
    QTRY_VERIFY_WITH_TIMEOUT(
        shell->currentStyleId() == QStringLiteral("neumorphic"), 8000);
    QVERIFY(shell->currentView()->isReady());
#else
    QSKIP("Qt 5 构建无新拟物视图");
#endif
}
```

若目标视图未就绪, `LottoViewShell` 不会切换 `currentStyleId`, 该用例
`QTRY_VERIFY_WITH_TIMEOUT` 超时即失败; 在 DOM 环境 Quick 不可用的平台上,
先检查一次 `QQuickWidget` 能否 `Ready`, 不能则 `QSKIP`。

- [ ] **Step 2: 运行, 确认失败**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R MainWindowTest --output-on-failure
```

预期: 编译失败(构造签名不符)。

- [ ] **Step 3: 精简 MainWindow**

`mainwindow.h`: 只保留 `Q_OBJECT`、构造、`viewShell()`、`Ui::MainWindow *ui`、
`LottoViewShell *m_shell`; 删除全部布局/常量/标签成员。

`mainwindow.cpp`: `init()` 只剩 `ui->setupUi(this)`、`setWindowTitle(...)`、
`m_shell = new LottoViewShell(controller, presenter, store, catalog, 200, this)`、
`setCentralWidget(m_shell)`; 删除对 `lottocontroller.h`/`QFile`/`QTimer` 等
不再需要的包含。

`main.cpp` 组合根按 Spec §6.8: 新增
`QSettingsStyleStore styleStore(&settings);` 与传入 `MainWindow`, 保留
`Q_INIT_RESOURCE(resources)`。

- [ ] **Step 4: 运行, 确认通过**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

- [ ] **Step 5: 手工跑一次桌面程序**

```bash
./build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/SuperLottoNumberGenerator.exe
```

预期: 界面与原版一致, 右上角出现风格按钮(仅"经典"一项)。

- [ ] **Step 6: 提交**

```bash
git add src/app tests/tst_mainwindow.cpp
git commit -m "refactor(app): MainWindow 退化为薄壳, 视图由 LottoViewShell 承载"
```

---

### Task 7: 新拟物视图与 QML 场景

**Files:**

- Create: `src/ui/neumorphic/lottoneumorphicview.h/.cpp`
- Create: `src/ui/LottoUi/NeumoTheme.qml`, `NeumoBall.qml`,
  `NeumoButton.qml`, `NeumoTicketRow.qml`, `NeumoValueRow.qml`
- Modify: `src/ui/LottoUi/NeumorphicView.qml`(占位 → 完整场景)
- Modify: `src/ui/CMakeLists.txt`, `src/ui/lottostylecatalog.cpp`
- Test: `tests/tst_lottoneumorphicview.cpp`, `tests/CMakeLists.txt`

**Interfaces:**

- Consumes: `LottoView`、`LottoStyleHost`、`LottoViewState`。
- Produces:
  - `class LottoNeumorphicView : public LottoView`,
    `static constexpr const char *STYLE_ID = "neumorphic"`,
    构造 `LottoNeumorphicView(LottoStyleHost *host, QWidget *parent = nullptr)`,
    `QQuickWidget *quickWidget() const`、`QQuickItem *rootItem() const`,
    覆写 `isReady()` / `snapshot()`
  - 目录追加 `neumorphic` 条目

- [ ] **Step 1: 写失败测试**

`tests/tst_lottoneumorphicview.cpp`:

```cpp
void initTestCase() { qputenv("QT_QUICK_BACKEND", "software"); }

void becomesReady()
{
    FakeStyleHost host;
    LottoNeumorphicView view(&host);
    QTRY_VERIFY_WITH_TIMEOUT(view.isReady(), 5000);
    QVERIFY(view.rootItem() != nullptr);
}

void renderStatePushesProperties()
{
    FakeStyleHost host;
    LottoNeumorphicView view(&host);
    QTRY_VERIFY_WITH_TIMEOUT(view.isReady(), 5000);
    view.renderState(LottoPresenter::buildViewState(makeTicket()));
    auto *root = view.rootItem();
    QTRY_COMPARE(root->property("generateButtonText").toString(),
                 QString::fromUtf8(LottoPresenter::GENERATE_TEXT));
    QTRY_COMPARE(root->property("timeLabelText").toString(),
                 QString::fromUtf8(LottoPresenter::TIME_LABEL));
    const QVariantList groups = root->property("groups").toList();
    QCOMPARE(groups.size(), LottoTicket::GROUP_COUNT);
    QCOMPARE(groups.at(0).toStringList().size(),
             LottoGroup::FRONT_COUNT + LottoGroup::BACK_COUNT);
}

void qmlSignalsForwardToView()
{
    FakeStyleHost host;
    LottoNeumorphicView view(&host);
    QTRY_VERIFY_WITH_TIMEOUT(view.isReady(), 5000);
    QSignalSpy gen(&view, &LottoView::generateRequested);
    QMetaObject::invokeMethod(view.rootItem(), "generateClicked");
    QCOMPARE(gen.count(), 1);
}

void stylesArePushed()
{
    FakeStyleHost host;
    host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")} };
    host.currentId = QStringLiteral("neumorphic");
    LottoNeumorphicView view(&host);
    QTRY_VERIFY_WITH_TIMEOUT(view.isReady(), 5000);
    const QVariantList styles = view.rootItem()->property("styles").toList();
    QCOMPARE(styles.size(), 1);
    QCOMPARE(view.rootItem()->property("currentStyleId").toString(),
             QStringLiteral("neumorphic"));
}
```

- [ ] **Step 2: 运行, 确认为红**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

- [ ] **Step 3: 实现 C++ 视图**

`LottoNeumorphicView` 组合 `QQuickWidget`(不继承): 构造时
`m_quick = new QQuickWidget(this)`, `setResizeMode(SizeRootObjectToView)`,
`loadFromModule(QStringLiteral("LottoUi"), QStringLiteral("NeumorphicView"))`,
`m_quick` 用布局铺满本视图。构造后立即推送 `styles` / `currentStyleId`。
`renderState()` 保存 `m_state` 并 `pushStateToQml()`(根对象为空时留待
`statusChanged`)。`pushStateToQml()` 用 `QMetaObject::invokeMethod` 或
`root->setProperty` 写 `groups`(转 `QVariantList`)、各文本与三个布尔。
QML 信号用 `QObject::connect(root, SIGNAL(generateClicked()), this,
SLOT(onGenerateClicked()))` 形式接三个私有槽, 槽内 `emit` 基类信号或
`host->requestSwitchTo(id)`。`isReady()` 返回
`m_quick->status() != QQuickWidget::Error && m_quick->rootObject() != nullptr`。
`snapshot()` 返回 `m_quick->grabFramebuffer()`(空则回退 `grab()`)。
`statusChanged` 到 `Error` 时 `qWarning() << m_quick->errors()`。

- [ ] **Step 4: 实现 QML 场景**

`NeumoTheme.qml`(`pragma Singleton`, CMake 中设
`QT_QML_SINGLETON_TYPE TRUE`): 底色 `#E0E5EC`、亮影 `#FFFFFF`、暗影
`#A3B1C6`、圆角 16、前区色 `#C86A6A`、后区色 `#6A8FC8`、文字色 `#4A5568`。

`NeumoBall.qml`:`property string text`、`property color ballColor`;
圆角矩形 + 两枚 `RectangularShadow`(`import QtQuick.Effects`), 左上偏移
`(-6,-6)` 白色、右下 `(6,6)` 暗色, `blur: 12`, `cached: true`。

`NeumoButton.qml`:`property string text`、`property bool enabled`、
`property bool checked`、信号 `clicked()`; 同上双向阴影, `checked` 时改为
内陷观感(阴影偏移反转 + 底色加深)。

`NeumoTicketRow.qml`:`property var numbers`(7 个字符串);
`RowLayout` 中前 5 个 `NeumoBall`(`ballColor` 前区色)、`Text "+"`、
后 2 个 `NeumoBall`(后区色); `Layout.fillWidth` 自适应。

`NeumoValueRow.qml`:`property string label`、`property string value`;
水平排列两个 `Text`。

`NeumorphicView.qml`: 根 `Item`, 声明 Spec §6.6 的十个属性与三个信号;
`ColumnLayout`: `NeumoValueRow`(时间)、5 × `NeumoTicketRow`、
`NeumoValueRow`(占位留白)、按钮行(`NeumoButton` 锁定 + 生成, 生成按钮
宽于锁定); 底部风格胶囊行 `Repeater model: styles` +
`NeumoButton`(`text: modelData.displayName`), 点击
`styleSwitchRequested(modelData.id)`; `enabled` 直接用属性绑定。

- [ ] **Step 5: CMake 与目录接线**

`src/ui/CMakeLists.txt`:`QML_FILES` 追加全部新 QML 文件; 在
`qt_add_qml_module` 前
`set_source_files_properties(LottoUi/NeumoTheme.qml PROPERTIES
QT_QML_SINGLETON_TYPE TRUE)`; Qt6 分支内把 `neumorphic/*.cpp` 加入
`target_sources`。

`lottostylecatalog.cpp` 在 Qt6 分支追加 `neumorphic` 条目。

`tests/CMakeLists.txt` 追加 `LottoNeumorphicViewTest`(链接
`lotto_ui lotto_adapters lotto_application lotto_core`, 注册
`-platform offscreen`)。

- [ ] **Step 6: 运行, 确认通过**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R "LottoNeumorphicViewTest|LottoViewShellTest" --output-on-failure
```

- [ ] **Step 7: 提交**

```bash
git add src/ui tests/tst_lottoneumorphicview.cpp tests/CMakeLists.txt
git commit -m "feat(ui): 新增 QML 新拟物视图与场景组件"
```

---

### Task 8: 新拟物风格胶囊、图标与切换交互

**Files:**

- Create: `src/ui/LottoUi/NeumoPill.qml`, `src/ui/LottoUi/NeumoIcon.qml`
- Modify: `src/ui/LottoUi/NeumorphicView.qml`
- Modify: `tests/tst_lottoneumorphicview.cpp`

**Interfaces:**

- Consumes: Task 7 的场景与属性。
- Produces: QML 根对象信号 `styleSwitchRequested(string styleId)` 被
  胶囊触发; `NeumoIcon.qml` 支持 `kind: "generate" | "lock" | "style"`。

- [ ] **Step 1: 写失败测试**

```cpp
void pillClickRequestsSwitch()
{
    FakeStyleHost host;
    host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                    {QStringLiteral("neumorphic"), QStringLiteral("新拟物")} };
    host.currentId = QStringLiteral("neumorphic");
    LottoNeumorphicView view(&host);
    QTRY_VERIFY_WITH_TIMEOUT(view.isReady(), 5000);

    QVERIFY(QMetaObject::invokeMethod(view.rootItem(), "requestStyleSwitch",
                                      Q_ARG(QVariant, QStringLiteral("classic"))));
    QCOMPARE(host.requested, QStringList{ QStringLiteral("classic") });
}
```

`NeumorphicView.qml` 增加 `function requestStyleSwitch(id)` 供胶囊调用
(同时便于测试直接触发)。

- [ ] **Step 2: 运行, 确认为红**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoNeumorphicViewTest --output-on-failure
```

- [ ] **Step 3: 实现**

`NeumoPill.qml`:`property string text`、`property bool current`;
当前项内陷(双向阴影偏移反转), 点击发 `clicked()`。

`NeumoIcon.qml`:`property string kind`; 用 `Canvas` 绘制三种几何图标
(生成 = 圆点方阵、锁定 = 矩形 + 弧、风格 = 四角方块), 不引入
`Qt6::Svg` 依赖, 避免额外部署模块。

`NeumorphicView.qml`: 底部 `Repeater` 改用 `NeumoPill`; 生成/锁定按钮加
`NeumoIcon`; 增加 `requestStyleSwitch(id)` 函数内
`signal styleSwitchRequested` 的发射。

- [ ] **Step 4: 运行, 确认通过**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoNeumorphicViewTest --output-on-failure
```

- [ ] **Step 5: 提交**

```bash
git add src/ui/LottoUi tests/tst_lottoneumorphicview.cpp
git commit -m "feat(ui): 新拟物风格胶囊与自绘图标"
```

---

### Task 9: 切换过渡覆盖层

**Files:**

- Create: `src/ui/lottostyleswitchoverlay.h/.cpp`
- Modify: `src/ui/lottoviewshell.h/.cpp`
- Modify: `src/ui/CMakeLists.txt`
- Test: `tests/tst_lottoviewshell.cpp`

**Interfaces:**

- Consumes: `LottoView::snapshot()`。
- Produces:
  - `class LottoStyleSwitchOverlay : public QWidget`,
    构造 `(const QPixmap &pixmap, QWidget *parent)`,
    `void setOpacity(qreal opacity)`
  - `LottoViewShell` 内部播放 200ms 淡出; `transitionMs <= 0` 时完全不创建
    覆盖层

- [ ] **Step 1: 写失败测试**

```cpp
void zeroTransition_createsNoOverlay()
{
    m_shell->requestSwitchTo(QStringLiteral("b"));
    QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
    QVERIFY(m_shell->findChild<LottoStyleSwitchOverlay *>() == nullptr);
}

void fadeTransition_removesOverlayAfterwards()
{
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(), 60);
    shell.resize(400, 600);
    shell.show();
    shell.requestSwitchTo(QStringLiteral("b"));
    QTRY_COMPARE(shell.currentStyleId(), QStringLiteral("b"));
    QVERIFY(shell.findChild<LottoStyleSwitchOverlay *>() != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(
        shell.findChild<LottoStyleSwitchOverlay *>() == nullptr, 2000);
}

void rapidSwitches_leaveSingleOverlay()
{
    LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(), 200);
    shell.resize(400, 600);
    shell.show();
    shell.requestSwitchTo(QStringLiteral("b"));
    QTest::qWait(20);
    shell.requestSwitchTo(QStringLiteral("a"));
    QTest::qWait(20);
    shell.requestSwitchTo(QStringLiteral("b"));
    QTRY_VERIFY_WITH_TIMEOUT(
        shell.findChild<LottoStyleSwitchOverlay *>() == nullptr, 3000);
    QCOMPARE(shell.currentStyleId(), QStringLiteral("b"));
}
```

- [ ] **Step 2: 运行, 确认为红**

```bash
cmake -S . -B build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
```

- [ ] **Step 3: 实现**

`LottoStyleSwitchOverlay`: 存 `QPixmap m_pixmap`、`qreal m_opacity = 1.0`;
`setOpacity()` 更新后 `update()`; `paintEvent` 用
`QPainter::setOpacity(m_opacity)` 绘制缩放铺满的位图; 构造时设
`setAttribute(Qt::WA_TransparentForMouseEvents)`。

`LottoViewShell::applySwitch()` 在 `m_transitionMs > 0` 且当前视图存在时:
切换前 `QPixmap shot = m_currentView->snapshot()`; 切换后创建覆盖层
`new LottoStyleSwitchOverlay(shot, this)` 并 `raise()`, 用
`QVariantAnimation`(0 → 1, `InOutQuad`)驱动其透明度从 1 降到 0, `finished`
时 `deleteLater()`。新的切换开始时先删除尚未结束的覆盖层与其动画。

- [ ] **Step 4: 运行, 确认通过**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug \
  -R LottoViewShellTest --output-on-failure
```

- [ ] **Step 5: 提交**

```bash
git add src/ui tests/tst_lottoviewshell.cpp
git commit -m "feat(ui): 切换过渡覆盖层与重入保护"
```

---

### Task 10: 持久化集成验证、Android 与文档

**Files:**

- Modify: `tests/tst_mainwindow.cpp`
- Modify: `CLAUDE.md`
- Modify: `docs/superpowers/specs/2026-11-20-switchable-ui-design.md`
  (状态改为已实施, 若实现有偏差同步更正)

**Interfaces:**

- Consumes: 全部前序任务。
- Produces: 无新接口。

- [ ] **Step 1: 写持久化集成测试**

```cpp
void styleChoice_persistsAcrossWindowRebuild()
{
    m_window->viewShell()->requestSwitchTo(QStringLiteral("fake-b"));
    QTRY_COMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("fake-b"));
    QCOMPARE(m_store->storedId, QStringLiteral("fake-b"));

    rebuildWindow(makeTestCatalog());   // 同一 m_store, 新窗口
    QCOMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("fake-b"));
}

void unknownStoredStyle_fallsBackAndKeepsValue()
{
    m_store->storedId = QStringLiteral("ghost");
    rebuildWindow(makeTestCatalog());
    QCOMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("classic"));
    QCOMPARE(m_store->storedId, QStringLiteral("ghost"));
}
```

`rebuildWindow()` 只重建窗口与交互器, 保持 `m_store` 实例不变
(否则失去持久化语义)。

- [ ] **Step 2: 运行全量测试**

```bash
cmake --build build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug
ctest --test-dir build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug --output-on-failure
```

预期: 9 个测试目标全绿。

- [ ] **Step 3: Android 构建与真机验证**

```bash
cmake --build build/Qt_6_11_1_for_Android_arm64_v8a-Debug
adb install -r build/Qt_6_11_1_for_Android_arm64_v8a-Debug/android-build-SuperLottoNumberGenerator/build/outputs/apk/debug/android-build-SuperLottoNumberGenerator-debug.apk
```

手工验证: 启动为上次选择的风格; 切换到新拟物后有阴影与内陷观感;
生成、锁定、重启恢复均正常; 来回切换无空白帧。

- [ ] **Step 4: 更新 CLAUDE.md**

新增/修订: `src/ui/` 层说明(`lotto_ui` 位置与依赖方向)、三类端口、
`LottoViewShell` 职责、视图自供切换交互、`LottoViewState` 去 emoji 与
时间拆分的约定、QML 模块构建约束(`qt_add_qml_module` + 静态库 +
`RESOURCE_PREFIX /qt/qml/`)、新增风格的步骤、测试目标数量(6 → 9)。

- [ ] **Step 5: 提交**

```bash
git add CLAUDE.md docs tests/tst_mainwindow.cpp
git commit -m "docs: 更新架构说明并补充风格持久化集成测试"
```

---

## 自检记录

- 规格覆盖: §6.1→T1/T3/T5, §6.2→T3, §6.3→T4/T9, §6.4→T2,
  §6.5→T5, §6.6→T7/T8, §6.7→T4/T6, §6.8→T6, §6.9→T5/T7 CMake 分支,
  §7→T1, §8→T3/T4/T5/T6/T7/T9/T10, §9 风险→T1/T4/T9, §10 实现自由度→
  T7/T8/T9。
- 类型一致性: `styleId`/`renderState`/`isReady`/`styleChanged`/`snapshot`
  在 T3 定义并在 T4–T7 一致使用; `LottoStyleDefinition` 在 T4 定义、
  T5/T7 目录追加; `transitionMs` 在 T4 定义、T9 使用。
- 卡点: T5 引入资源前缀变更可能暂时影响旧 `MainWindow`, 已在 T5 Step 4
  内给出修正要求。
