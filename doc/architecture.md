# 架构设计

基于 Qt 6 Widgets 的大乐透随机号码生成器(桌面端 + Android),按整洁架构(Clean Architecture)分层,**依赖只指向内层**。

## 分层结构

```text
src/domain/   → lotto_core 库: 实体 + 用例 + 用例层接口(仅 Qt::Core)
src/adapters/ → lotto_adapters 库: 持久化实现(仅 Qt::Core)
src/app/      → lotto_app 库: 控制器 + 视图 + 组合根(Qt::Widgets)
```

依赖方向:`app → adapters → domain`,domain 不依赖任何外层。

## 各层职责

### 领域层(domain)

- `LottoResult` — 单组号码(前区 5 个 [1,35] + 后区 2 个 [1,12],已排序)
- `LottoTicket` — 领域实体:5 组号码 + 生成时间 + 锁定标志;游戏规则常量在此定义
- `LottoEngine` — 用例:随机生成号码(QRandomGenerator 洗牌,无状态)
- `TicketRepository` — 用例层持久化接口(纯抽象,DIP)

### 适配器层(adapters)

- `QSettingsTicketRepository` — TicketRepository 的 QSettings 实现。键名与旧版本兼容;时间存 ISO 字符串,读取时兼容旧版显示格式;锁定时写全量,未锁定仅写标志

### 应用层(app)

- `LottoController` — 应用编排器,持有唯一状态源 `LottoTicket`。提供生成(锁定时拒绝)、锁定切换(状态守卫防重复保存)、异步恢复;通过唯一信号 `ticketChanged` 推送状态
- `MainWindow` — 谦卑视图:只渲染 `ticketChanged` 推送的状态,按钮动作转发给控制器,不做业务判断。布局计算与 50ms 防抖(Android resizeEvent 自激震荡)保留在此
- `main.cpp` — 组合根:装配具体依赖(唯一接触 QSettings 具体类的点),栈分配,移交控制权

## 数据流

```text
用户点击(生成/锁定) → LottoController 用例 → 状态变化 → ticketChanged 信号
                                                          ↓
                                MainWindow 纯渲染(号码/时间/按钮互斥)
                                                          ↑
        QSettingsTicketRepository(仅锁定/解锁时读写, 恢复时异步加载)
```

跨层数据只传 `LottoTicket` 值类型,不传递 widget 或存储内部结构。

## 测试

四个测试 target 链接各层库,不手工编译被测源码:

| target | 覆盖 |
| --- | --- |
| LottoEngineTest | 引擎随机性/边界/分布 |
| LottoControllerTest | 状态机(注入内存 Fake 仓储) |
| QSettingsRepositoryTest | 持久化 round-trip 与旧 INI 兼容 |
| MainWindowTest | GUI 集成(注入式夹具,offscreen) |
