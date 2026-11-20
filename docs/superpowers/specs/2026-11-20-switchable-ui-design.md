# 可切换 UI 组件 + 新拟物风格视图 设计文档

日期: 2026-11-20
状态: 待批准
范围: 新增可替换 UI 组件层; 经典视图与新增"新拟物"视图运行时可切换;
过渡动画; 风格偏好持久化

## 1. 意图与动机

现状 UI 与窗口、控制器、样式表耦合在 `src/app/MainWindow` 一个类里, 无法整体
替换。目标是:

- 视图成为可替换组件: 每套 UI 是一个独立 `LottoView` 实现, 视图之间互不认识。
- 新增"新拟物(Neumorphism)"风格视图, 用 QML 实现, 经 `QQuickWidget` 嵌入
  Widgets 应用。
- 运行时可切换, 带过渡动画, 选择持久化。
- 保持整洁架构: domain / application / adapters 三层零改动; 视图只消费
  `LottoViewState`, 不感知领域实体与用例。

## 2. 目标 / 非目标 / 反目标

目标:

- 两套视图在同一窗口内运行时切换, 切换后当前票据状态完整保留。
- 切换交互由视图自己提供(风格各异), 宿主只提供风格清单、当前风格与切换功能。
- 文案中去除 emoji; 图标由各视图自选。
- 新增风格只需: 添加一个视图类 + 在风格目录中注册一行。

非目标:

- 不做第二套 QML 视图(经典视图保持 Widgets, 不重写)。
- 不做跨设备主题同步、不做深色/浅色自动跟随。
- 不新增用例; 风格选择是呈现层偏好, 不进入 domain/application。
- 不引入 Qt Quick Controls 的样式引擎(Themes), 两套视图各自独立实现。

反目标(即使技术可用也算失败):

- 视图为了切换去感知另一套视图的具体类。
- 业务层为 UI 偏好新增依赖或状态。
- 切换导致票据状态丢失、重复保存或锁定状态错位。

## 3. 约束

- Qt 6.11.1, MinGW 64-bit 桌面 + Android arm64; 已安装
  `Qt6Quick` / `Qt6QuickWidgets` / `Qt6QuickEffects`(两套工具链均已确认)。
- 竖屏锁定; Android 无菜单栏。
- 现有 C++17、按层静态库、依赖只指向内层。
- 注释中文, 代码行宽 ≤100, 注释 ≤80。
- 现有 `MainWindowTest` 依赖 objectName 定位控件, 测试可改但须保留覆盖面。

## 4. 已定决策

| 决策 | 结论 | 依据 |
|------|------|------|
| 新拟物实现技术 | QML + `QQuickWidget` | 新拟物核心是圆角柔影与内陷; `RectangularShadow`(QtQuick.Effects) 在着色器内计算圆角矩形模糊, Qt 官方有 NeumorphicPanel 示例; Widgets 需自绘模糊位图 |
| 经典视图 | 保持 Widgets + QSS | 避免无收益重写与测试重做 |
| 文案归属 | 共用, 单来源留在 `LottoPresenter`; 去掉 emoji | 防止两套 UI 文案漂移; emoji 实为图片, 应归视图 |
| 图标归属 | 各视图自带 SVG/绘制, 自行决定有无与位置 | 风格决定装饰 |
| 切换入口 | 视图自己提供交互(N 选一) | 用户决定 |
| 切换机制 | 宿主懒创建 + 常驻 `QStackedWidget` + 状态重放 | 避免重复解析 QML 与状态丢失 |
| 过渡 | 快照叠加淡出(overlay cross-fade, 200ms) | 对 Widgets/QML 视图统一, 实现小 |
| 风格持久化 | `QSettings` 键 `ui/styleId` | 与现有 INI 同文件, 向后兼容 |
| 目录注入 | `LottoViewShell`/`MainWindow` 构造参数接受风格目录, 默认 `lottoStyleCatalog()` | 测试注入假视图, 与 QML 环境解耦 |
| 运行期视图创建失败 | 保留当前视图并告警; 启动时回退首个可用风格, 不覆盖已保存偏好 | 失败不丢恢复入口, 不改写用户选择 |

## 5. 备选方案

1. **外壳 + 视图端口 + 视图自供切换交互(选中)**。视图是 `QWidget` 子类,
   各自实现交互; 宿主只管清单/当前/创建/重放/持久化。切换交互风格自由。
2. 两套完整 `QMainWindow`, 启动时按配置择一。改动最小, 但运行时切换退化为
   重启生效, 与需求冲突。
3. 宿主提供统一切换入口(菜单/悬浮按钮), 视图被动。Android 无菜单栏, 悬浮
   按钮又与视图观感冲突; 且用户明确要求由视图提供交互。
4. 运行时切换的另一种载体: `QQuickView` + `createWindowContainer`。原生窗口
   嵌入带来层叠顺序与焦点问题, 文档推荐 `QQuickWidget`; 否决。

新拟物实现子方案: (a) QML + `QQuickWidget`(选中); (b) Widgets `QPainter`
自绘柔影缓存位图(可行但工作量大, 作为回退路径保留); (c) 仅换 QSS(做不到
双向柔影与内陷, 否决)。

## 6. 结构

### 6.1 分层与目录

```text
src/domain/       lotto_core          (不变)
src/application/  lotto_application   (不变)
src/adapters/     lotto_adapters      (不变)
src/ui/           lotto_ui            (新增: 视图组件 + 视图宿主 + 风格目录)
src/app/          lotto_app           (瘦身: MainWindow 窗口壳 + exe 组合根)
```

依赖: `app → ui → adapters → application → domain`。`lotto_ui` 依赖
`lotto_adapters`(取 `LottoViewState` / `LottoController` / `LottoPresenter`)
与 `Qt::Widgets`、`Qt::QuickWidgets`。

```text
src/ui/
  lottoview.h/.cpp                 视图抽象基类
  lottostylehost.h                 宿主端口(纯接口)
  lottostylecatalog.h/.cpp         风格目录(注册表: 唯一新增风格的点)
  lottoviewshell.h/.cpp            宿主: 栈、重放、过渡、持久化、信号转发
  lottostyleswitchoverlay.h/.cpp   过渡覆盖层(内部)
  lottostylestore.h                风格偏好端口
  qsettingsstylestore.h/.cpp       偏好端口实现
  classic/lottoclassicview.h/.cpp 经典视图(自 MainWindow 抽取)
  classic/style.qss, classic/resources.qrc, classic/icons/*
  neumorphic/lottoneumorphicview.h/.cpp QQuickWidget 子类
  LottoUi/*.qml                     QML 模块场景(URI LottoUi, 目录与
                                    target path 一致)
```

`src/app/` 保留 `mainwindow.h/.cpp`(QMainWindow 薄壳)、`mainwindow.ui`
(central widget / menu bar / status bar 骨架)与 `main.cpp`(组合根)。

### 6.2 三种端口

```cpp
// lottostylehost.h — 宿主端口: 后台提供"选项 + 功能", 视图自行调用
struct LottoStyleInfo {
    QString id;            // 稳定 id, 持久化用: "classic" / "neumorphic"
    QString displayName;   // 展示名: "经典" / "新拟物"
};

class LottoStyleHost {
public:
    virtual ~LottoStyleHost() = default;
    virtual QVector<LottoStyleInfo> availableStyles() const = 0;
    virtual QString currentStyleId() const = 0;
    virtual void requestSwitchTo(const QString &styleId) = 0;
};
```

`LottoStyleHost` 不继承 `QObject`: 宿主 `LottoViewShell` 同时是 `QWidget`,
避免 QObject 菱形继承。风格变化通过视图虚函数回调(见下)。

```cpp
// lottoview.h — 视图端口
class LottoView : public QWidget {
    Q_OBJECT
public:
    explicit LottoView(QWidget *parent = nullptr);
    virtual QString styleId() const = 0;
    virtual void renderState(const LottoViewState &state) = 0;
    virtual bool isReady() const { return true; }              // 默认恒真
    virtual void styleChanged(const QString &currentStyleId);  // 默认空实现
    virtual QPixmap snapshot() const;                          // 默认 grab()
signals:
    void generateRequested();
    void lockRequested();
};
```

- 视图**不再**持有 `LottoController`; 只发 `generateRequested`/`lockRequested`,
  由宿主接到控制器。视图不感知交互器、票据、仓储。
- 切换交互: 视图构造时注入 `LottoStyleHost *`, 自行渲染清单
  (`availableStyles()`)与当前选中(`currentStyleId()`), 用户操作时调用
  `requestSwitchTo(id)`。经典视图用角落按钮 + 弹出菜单; 新拟物视图用其风格的
  胶囊分段控件。
- `styleChanged()` 由宿主在切换完成后对旧视图与新视图各调用一次, 用于刷新
  选中态。

```cpp
// lottostylestore.h — 风格偏好端口(呈现层偏好, 不属用例)
class LottoStyleStore {
public:
    virtual ~LottoStyleStore() = default;
    virtual QString loadStyleId() const = 0;   // 空表示未设置
    virtual void saveStyleId(const QString &id) = 0;
};
```

实现 `QSettingsStyleStore` 放 `src/ui/`, 键 `ui/styleId`; 组合根构造并注入,
`main.cpp` 仍是唯一选择具体实现的点(复用现有 `QSettings` 实例)。

```cpp
// lottostylecatalog.h — 注册表
struct LottoStyleDefinition {
    LottoStyleInfo info;
    std::function<LottoView *(LottoStyleHost *, QWidget *)> create;
};
QVector<LottoStyleDefinition> lottoStyleCatalog();   // 应用真实目录, 顺序即展示顺序
```

首个条目为默认风格。新增风格 = 新增视图 + 在此表追加一行。目录是**注入点**:
`LottoViewShell` 与 `MainWindow` 的构造参数接受一个目录(默认
`lottoStyleCatalog()`), 测试注入假视图目录, 与真实 QML 环境解耦。

### 6.3 宿主: LottoViewShell

职责(全部可单测):

1. 持有 `QStackedWidget`; 首个视图按 `LottoStyleStore` 恢复, 非法 id 回退
   目录首个条目。
2. 实现 `LottoStyleHost`: 返回目录清单、当前 id、接收切换请求。
3. 连接 `LottoPresenter::viewStateChanged` → 缓存 `m_lastState` → 转发当前视图。
4. 视图懒创建; 创建时注入 `LottoStyleHost*`, 接
   `generateRequested`/`lockRequested` 到 `LottoController`, 立即用
   `m_lastState` 调 `renderState()`。
5. 切换: 校验 id → `QMetaObject::invokeMethod(Qt::QueuedConnection)` 队列化执行
   (避免在旧视图自身的事件处理中隐藏它) → 取快照 → 创建/显示新视图 → 重放
   → 播放过渡 → 通知两视图 `styleChanged` → 保存偏好。
6. 过渡时长可注入(默认 200ms, 测试传 0 即瞬时切换)。
7. 失败路径: 目标视图创建失败或 `isReady()` 为假(如 QML `status()==Error`)时,
   丢弃新视图、保持当前视图可见、`qWarning` 并发出 `styleSwitchFailed(id)`,
   **不写入偏好**。启动时若持久化 id 不可用, 回退到目录首个视图, 但保留原
   偏好值不覆盖, 便于修复后恢复用户选择。

```cpp
class LottoViewShell : public QWidget, public LottoStyleHost {
    Q_OBJECT
public:
    // catalog 默认 lottoStyleCatalog(); 测试注入假视图目录
    LottoViewShell(LottoController *controller, LottoPresenter *presenter,
                   LottoStyleStore *store,
                   QVector<LottoStyleDefinition> catalog = lottoStyleCatalog(),
                   int transitionMs = 200, QWidget *parent = nullptr);

    QVector<LottoStyleInfo> availableStyles() const override;
    QString currentStyleId() const override;
    void requestSwitchTo(const QString &styleId) override;

    LottoView *currentView() const;      // 供测试/调试
    int transitionMs() const;            // 供测试

signals:
    /*! 目标视图创建或就绪失败: 保持当前视图, 不写入偏好 */
    void styleSwitchFailed(const QString &styleId);
};
```

过渡实现(`LottoStyleSwitchOverlay`): 切换前取旧视图 `snapshot()` 位图, 在栈顶
放一个 `WA_TransparentForMouseEvents` 覆盖层, 用 `QVariantAnimation`
(200ms, InOutQuad) 把不透明度从 1 降到 0, 结束后删除覆盖层。
`LottoNeumorphicView` 覆写 `snapshot()` 用 `grabFramebuffer()` 取图。

### 6.4 展示状态调整: LottoViewState

去 emoji、拆时间、补按钮文案, 使视图零字面量:

```cpp
struct LottoViewState {
    QVector<QStringList> groups;   // 前区+后区展示文本, 缺项为占位符
    QString timeLabelText;         // "生成时间："
    QString timeValueText;         // "2026-11-20 10:00:00" / 占位
    QString generateButtonText;    // "生成号码"
    bool    generateEnabled;
    QString lockButtonText;        // "锁定号码" / "解锁生成"
    bool    lockButtonEnabled;
    bool    lockButtonChecked;
};
```

`LottoPresenter` 常量去掉 emoji: `TIME_LABEL="生成时间："`,
`LOCK_TEXT_LOCKED="解锁生成"`, `LOCK_TEXT_UNLOCKED="锁定号码"`,
`GENERATE_TEXT="生成号码"`; `TIME_PLACEHOLDER` / `NUMBER_PLACEHOLDER` 保留
(无值占位, 两套视图共用)。`buildViewState()` 把时间拆成标签与值两段;
`initialState()` 语义不变。

### 6.5 经典视图(LottoClassicView)

自 `MainWindow` 原样迁移以下逻辑, 保留全部 objectName:
`init()`/`buildLayout()`/`rebuildGroupRows()`/`updateLabelSizes()`/
`resizeEvent()` 50ms 防抖/`spacingForWidth()`。改动点:

- 继承 `LottoView`; 根控件 `setObjectName("classicRoot")`。
- 时间标签拆成 `timeLabel`(标题)与 `timeValueLabel`(值), QSS 同步调整。
- 按钮 `clicked` → 发 `generateRequested`/`lockRequested`, 不再直连控制器。
- 新增角落切换按钮 `btnStyleSwitch`(QToolButton, 弹出 N 项可勾选菜单)。
- 样式表与其 qrc 移入 `src/ui/classic/`, 由本视图加载; `main.cpp` 的
  `Q_INIT_RESOURCE(resources)` 保留(资源名不变)。
- 图标: 本视图默认不加图标(纯文字), 保持既有观感。

### 6.6 新拟物视图(LottoNeumorphicView)

- **组合而非继承**: `LottoNeumorphicView : LottoView` 内部持有一个
  `QQuickWidget`(QQuickWidget 本身是 QWidget, 无法与 LottoView 多重继承),
  `isReady()` 返回 `m_quick->status() != QQuickWidget::Error`(加载中视为可用,
  就绪后自动补渲染); `renderState()` 在未就绪时暂存, `statusChanged` 后重放;
  `status()==Error` 时打印 `errors()`。
- 资源与导入: `QQuickWidget` 的引擎在加载前 `addImportPath(":/")`, 模块
  `LottoUi` 由 `qt_add_qml_module` 放在 `:/LottoUi/` 下, 用
  `loadFromModule("LottoUi", "NeumorphicView")` 加载。
- 根 QML 暴露属性: `groups`、`timeLabelText`、`timeValueText`、
  `generateButtonText`、`generateEnabled`、`lockButtonText`、
  `lockButtonEnabled`、`lockButtonChecked`、`styles`、`currentStyleId`;
  QML 侧信号 `generateClicked`/`lockClicked`/`styleSwitchRequested(id)`,
  C++ 侧通过根对象 `QObject::connect` 接收。
- 视觉: 底色 `#E0E5EC` 一类的低对比灰; 每个面板/按钮/号码球 = 圆角
  `Rectangle` + 两枚 `RectangularShadow`(左上亮色、右下暗色); 按下/选中态
  改为反向偏移与更深底色形成内陷感; 号码球尺寸随 `Layout` 自适应; 前区/后区
  沿用红/蓝色系但取新拟物的低饱和变体。
- 自带 SVG 图标(生成/锁定/风格切换)与字号档位, 不引用经典视图资源。
- 风格切换: 底部一组风格胶囊, 当前项内陷; 点击调用
  `host->requestSwitchTo(id)`。
- 无 resize 防抖需求(QML 布局无自激)。

### 6.7 数据流

```text
按钮点击(任一视图) → LottoView::generateRequested
  → LottoViewShell → LottoController::onGenerateRequested
  → LottoInteractor → LottoPresenter::present(ticket)
  → viewStateChanged(LottoViewState) → Shell 缓存并转发当前视图
  → 当前视图 renderState()

风格胶囊/菜单点击 → host->requestSwitchTo(id)
  → Shell: 创建/显示目标视图 → renderState(缓存状态) → 过渡 → 保存 id
```

### 6.8 组合根

```cpp
QSettings settings("SuperLottoNumberGenerator.ini", QSettings::IniFormat);
QSettingsTicketRepository repository(&settings);
QSettingsStyleStore styleStore(&settings);
LottoEngine engine;
LottoPresenter presenter;
LottoInteractor interactor(&repository, &engine, &presenter);
LottoController controller(&interactor);
MainWindow window(&controller, &presenter, &styleStore);   // 目录默认 lottoStyleCatalog()
interactor.load();
```

`MainWindow` 构造: `setupUi` → 标题 → `setCentralWidget(new LottoViewShell(...))`。
`MainWindow` 同样接受可选的目录参数并透传给 `LottoViewShell`, 供集成测试
注入假视图, 使切换/持久化用例不依赖 QML 运行环境。

### 6.9 Qt 版本范围

`lotto_ui` 的新拟物视图依赖 Qt 6(`qt_add_qml_module` / `QQuickWidget` /
`QtQuick.Effects`)。处理方式: CMake 中 `if(QT_VERSION_MAJOR GREATER_EQUAL 6)`
才把 `neumorphic/` 源与 QML 模块纳入编译; 风格目录在 Qt 5 下只注册经典视图。
这样顶层保留的 Qt 5 回退路径仍可构建, 不需要为 Qt 5 实现新拟物。

## 7. 构建集成(QML 模块 + 静态库)

- `lotto_ui` 用 `qt_add_qml_module(lotto_ui URI LottoUi VERSION 1.0
  QML_FILES ... RESOURCES <svg>)`; QML 文件放 `src/ui/neumorphic/qml/`, 通过
  `QT_RESOURCE_ALIAS` 归位到模块根, 避免目录结构告警。
- 静态库下生成的 QML 插件可能被链接器丢弃。做法: 链接生成的插件 target, 并在
  `lotto_ui` 内放一个被视图构造函数引用的 TU 声明
  `Q_IMPORT_QML_PLUGIN(LottoUiPlugin)`。
- **实现第一步必须先验证**此集成在桌面与 Android 上均能加载 QML; 若失败,
  回退 `Q_INIT_RESOURCE` + `setSource(QUrl("qrc:/qt/qml/LottoUi/..."))`。
- Android 部署: `androiddeployqt` 需把 QtQuick / QtQuick.Effects 打进包; CMake
  的 qml import scan 默认处理, 需在设备上实测。

## 8. 测试策略

| 测试 | 变更 |
|------|------|
| `LottoPresenterTest` | 适配新字段(时间拆分、生成按钮文案、无 emoji) |
| `LottoEngineTest` / `LottoInteractorTest` / `LottoControllerTest` / `QSettingsRepositoryTest` | 不变 |
| `MainWindowTest` | 改作用域查找(先按 `classicRoot` 找根再 `findChild`); 时间拆两个标签; 注入假目录验证"生成→切换→状态保留→切回"与"切风格后重建窗口恢复"; 另加一条真实目录用例, QML 视图未就绪时 `QSKIP` |
| 新增 `LottoUiTest` | 视图基类默认行为(`isReady`/`styleChanged`/`snapshot`)与 `QSettingsStyleStore` 往返 |
| 新增 `LottoClassicViewTest` | 经典视图控件齐全、点击发信号、`renderState` 写入、风格菜单列出全部风格并请求切换 |
| 新增 `LottoViewShellTest` | 注入 `transitionMs=0`; 断言默认风格、非法 id 回退、切换重放、`styleChanged` 通知、偏好写入、创建失败不写偏好 |
| 新增 `LottoNeumorphicViewTest` | `-platform offscreen`; 等 `Ready`; `renderState` 后读根对象属性; 点击胶囊触发切换请求; Quick 初始化失败时 `QSKIP` |

`LottoView` 信号与宿主转发用 `QSignalSpy` 覆盖, 无需真实渲染。集成测试通过
注入假目录获得确定性覆盖, 真实 QML 视图的覆盖放在单独的
`LottoNeumorphicViewTest`(可 `QSKIP`), 两者互不牵连。Android 构建继续跳过
测试。

## 9. 风险与回退

| 风险 | 影响 | 回退 |
|------|------|------|
| 静态库 QML 模块注册被裁剪 | QML 空白 | 按 §7 验证; 回退 qrc + setSource |
| 运行期 QML 加载失败 | 目标视图空白 | Shell 保持当前视图 + `styleSwitchFailed`; 启动回退首个可用风格 |
| Qt 5 构建 | 新拟物不可用 | 目录在 Qt 5 下只含经典视图(§6.9) |
| `QQuickWidget` 低端 Android 性能/包体 | 卡顿或包体增大 | 新拟物视图改 Widgets 自绘(端口以上零改动) |
| `-platform offscreen` 下 Quick 不可用 | QML 测试无法运行 | `QSKIP` + 真机手测 |
| `snapshot()` 抓 QQuickWidget 慢 | 切换首帧延迟 | 用 `grabFramebuffer()`; 首次切换前预热视图 |
| 两套视图 objectName 重名 | 测试取错控件 | 测试一律先从视图根 objectName 进入 |
| 快速连续切换 | 动画叠加/状态错乱 | 切换时先结束现有动画; 队列化执行 |
| emoji 去除影响旧数据 | 无(时间串仅展示) | 无 |

## 10. 留给实现者的决定

- 新拟物配色、圆角、阴影参数的具体数值; 图标造型。
- 过渡时长与缓动曲线的最终取值(默认 200ms / InOutQuad)。
- 是否在首次显示后预热新拟物视图(建议: 是, `QTimer::singleShot` 延迟创建
  隐藏实例)。
- 经典视图切换按钮的具体样式与位置(角落)。
- QML 到 C++ 的桥接方式(根对象 connect 或 C++ invokable 桥接对象)。

## 11. 落地顺序建议(供编写实施计划)

1. 构建集成冒烟: 最小 QML 视图在桌面/Android 加载成功(验证 §7)。
2. 展示状态去 emoji + 时间拆分, 适配 `LottoPresenterTest`。
3. 抽出 `LottoView`/`LottoStyleHost`/目录/偏好端口 + 单元测试。
4. 经典视图自 `MainWindow` 抽为 `LottoClassicView` + 视图单测。
5. 宿主 `LottoViewShell`: 栈、重放、信号转发、(先瞬时)切换 + 单测。
6. `MainWindow` 薄壳 + 组合根接线; 适配 `MainWindowTest` 为注入假目录。
7. 新拟物 QML 视图: 静态场景 → 状态绑定 → 切换胶囊 → 图标。
8. 过渡覆盖层 + 快照; 测试注入 `transitionMs=0` 保证稳定。
9. 持久化与集成测试(切换 round-trip、重建窗口恢复)。
10. 更新 CLAUDE.md(新增 ui 层、端口、切换机制、去 emoji 约定、QML 构建约束)。
