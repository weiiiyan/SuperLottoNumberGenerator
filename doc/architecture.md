# 架构设计

基于 Qt 6 Widgets 的大乐透随机号码生成器(桌面端 + Android),按整洁架构(Clean Architecture)分层,**依赖只指向内层**。

## 图表与文档

| 图/文档 | PlantUML 源文件 | 渲染结果 |
| --- | --- | --- |
| 模块依赖图(分层/目标/测试) | [module-dependency.puml](module-dependency.puml) | [module-dependency.svg](module-dependency.svg) |
| 控制流图(恢复/生成/锁定) | [control-flow.puml](control-flow.puml) | [control-flow.svg](control-flow.svg) |
| Web 化迁移分析(边界划分/替换清单/关键决策) | — | [web-migration.md](web-migration.md) |

## 分层结构

```text
src/domain/      → lotto_core 库: 实体 + 领域服务(仅 Qt::Core)
src/application/ → lotto_application 库: 用例交互器 + 用例层持久化接口(仅 Qt::Core)
src/adapters/    → lotto_adapters 库: 持久化实现(仅 Qt::Core)
src/app/         → lotto_app 库: 谦卑视图(Qt::Widgets)
src/app/main.cpp → 组合根: 装配具体依赖(唯一接触 QSettings 具体类的点)
```

依赖方向:`app → adapters → application → domain`,内层不依赖任何外层。

与《架构整洁之道》洋葱图的映射:

| 洋葱图层 | 目录 | 内容 |
| --- | --- | --- |
| 业务实体(Entities) | src/domain/ | LottoResult/LottoTicket/LottoEngine |
| 用例(Use Cases) | src/application/ | LottoInteractor + TicketRepository 端口 |
| 接口适配器(Interface Adapters) | src/adapters/(存储侧) + app 内展示器(展示侧) | QSettingsTicketRepository、LottoPresenter |
| 框架与驱动(Frameworks & Drivers) | src/app/ | MainWindow(谦卑视图)/QSS/qrc |
| Main 组件(洋葱外) | src/app/main.cpp | 装配具体依赖 |

**展示器为何在 app/ 而非 adapters/ 或 application/(有意权衡)**:按洋葱图展示器属接口适配器层,与仓储同层对称(用例层展示/存储两个输出端口),不放 application/(用例层不做 I/O 格式适配)。物理放 app/ 是因为与谦卑视图配对、共享展示词汇(按钮文字/时间前缀),同库内聚;真正约束(展示器不依赖 Widget、依赖只指向内层)已守住,目录归属只是组织方式——不完全边界(ch24)。多视图/展示器膨胀时应独立为 adapters/presenter/ 或单独组件。

领域层使用 Qt::Core 类型(QDateTime/QVector/QObject)是有意权衡:本项目以 Qt 为应用语言,不追求与框架无关的纯领域,但绝不依赖任何 UI/存储框架。

## 各层职责

### 领域层(domain)

- `LottoResult` — 单组号码(前区 5 个 [1,35] + 后区 2 个 [1,12],已排序);组级游戏规则常量 `FRONT_COUNT`/`BACK_COUNT`/`FRONT_MIN`/`FRONT_MAX`/`BACK_MIN`/`BACK_MAX` 定义于此(全项目唯一来源)
- `LottoTicket` — 领域实体:5 组号码 + 生成时间 + 锁定标志;票据级常量 `GROUP_COUNT` 定义于此(`FRONT_COUNT`/`BACK_COUNT` 为 `LottoResult::*` 别名);`hasNumbers()`/`isValid()` 校验在此定义
- `LottoEngine` — 领域服务:随机生成号码(QRandomGenerator 洗牌,无状态)

### 用例层(application)

- `LottoInteractor` — 用例交互器(use case interactor),而非 MVC 控制器:不做输入/输出格式适配,只执行应用特定业务规则与编排。持有唯一状态源 `LottoTicket`;提供生成(锁定时拒绝)、锁定切换(状态守卫防重复保存)、异步恢复(`load()` 由组合根在窗口构造后调用,视图不驱动用例启动);通过唯一信号 `ticketChanged` 推送状态。仅依赖 Qt::Core,不接触任何 UI/存储具体类
  - **书中 Controller 角色说明(有意权衡)**:书图 22.2 中 Controller(接口适配器层)与 UseCaseInteractor(用例层)是两个角色,前者把外部输入打包成 InputData。本项目输入是按钮 click,转发仅一行 lambda 无翻译逻辑,**输入侧适配器被谦卑视图 MainWindow 吸收**,未独立建类;输出侧才有可测试逻辑,单独提取为 LottoPresenter。判定层归属的标准是职责与依赖方向而非名称。未来若出现需解析的表单输入,应引入真正的输入侧适配器(Controller)
- `TicketRepository` — 用例层持久化接口(纯抽象,DIP 端口):端口归用例层所有,由适配器层实现

### 适配器层(adapters)

- `QSettingsTicketRepository` — TicketRepository 的 QSettings 实现。键名与旧版本兼容;时间存 ISO 字符串,读取时兼容旧版显示格式;锁定时写全量,未锁定仅写标志

### 视图层(app)

- `LottoPresenter` — 展示器(谦卑对象模式):所有可测试的展示推导集中于此,`present(LottoTicket) → LottoViewState`(号码格式化/占位符/按钮互斥状态/时间显示格式),不依赖任何 Widget。UI 展示字符串常量单来源
- `MainWindow` — 谦卑视图:只消费 `LottoViewState` 做机械控件写入,按钮动作转发给控制器,不做业务判断与展示推导。布局计算与 50ms 防抖(Android resizeEvent 自激震荡)保留在此
- `main.cpp` — 组合根:装配具体依赖(唯一接触 QSettings 具体类的点),启动用例(`controller.load()`),栈分配,移交控制权

## 数据流

```text
用户点击(生成/锁定) → LottoInteractor 用例 → 状态变化 → ticketChanged 信号
                                                          ↓
                                MainWindow 纯渲染(号码/时间/按钮互斥)
                                                          ↑
        QSettingsTicketRepository(仅锁定/解锁时读写, 恢复时异步加载)
```

跨层数据只传 `LottoTicket` 值类型,不传递 widget 或存储内部结构。

## 测试

五个测试 target 链接各层库,不手工编译被测源码:

| target | 覆盖 |
| --- | --- |
| LottoEngineTest | 引擎随机性/边界/分布 |
| LottoInteractorTest | 状态机(注入内存 Fake 仓储,无 GUI 依赖) |
| QSettingsRepositoryTest | 持久化 round-trip 与旧 INI 兼容 |
| LottoPresenterTest | 展示推导(格式化/占位符/按钮互斥/时间格式,无 GUI 依赖) |
| MainWindowTest | GUI 集成(注入式夹具,offscreen) |
