# Web 化迁移分析

将本应用从桌面 Qt Widgets 迁移为「浏览器前端 + 后台 API」时的边界划分。

依据:《架构整洁之道》ch17(边界画线)/ch18(边界剖析)/ch22(整洁架构)/ch31(Web 是实现细节)。

## 总原则

划线的位置**不在"显示"那里,而在用例边界**。Web 只是换了一组 I/O 插件:

- **永不动(策略核心)**: 领域层 + 用例层 + TicketRepository 端口
- **必须换(展示通道)**: 桌面 视图+Controller/Presenter → 前端 + JSON 序列化 + Web Controller
- **可选换(存储通道)**: QSettings 仓储 → 数据库仓储(取决于部署形态,与 Web 无关)

## 两条正交的 I/O 通道

```
┌────────────────────────────────────────────────────────────┐
│  展示通道(换不换取决于显示方式)                                │
│  桌面: MainWindow + LottoController + LottoPresenter           │
│  Web:  前端 HTML/JS + JSON Presenter + Web Controller          │
└────────────────────────────────────────────────────────────┘
                LottoInteractor(用例) / TicketRepository(端口)
┌────────────────────────────────────────────────────────────┐
│  存储通道(换不换取决于部署形态, 与显示方式无关)                 │
│  本地: QSettingsTicketRepository(INI 文件)                   │
│  服务端: 数据库仓储                                            │
└────────────────────────────────────────────────────────────┘
```

存储通道的替换**不是 Web 化的必然动作**:Web 化 → 部署变服务端 → 存储需求变
(多用户/并发/可靠性)→ 才换仓储实现。单用户个人服务可原样保留 QSettings 实现,
端口( TicketRepository )的意义正在于此——换实现不碰用例层。

## 迁移清单

| 组件 | 现状 | Web 化后 | 动作 |
| --- | --- | --- | --- |
| src/domain/(LottoGroup/Ticket/Engine) | 领域层 | 后台, 原样保留 | 不动 |
| LottoInteractor(用例) | 用例层 | 后台, 原样保留 | 不动 |
| TicketRepository(端口) | 用例层 | 后台, 原样保留 | 不动 |
| MainWindow + QSS + qrc | 桌面谦卑视图 | 前端 HTML/JS(新的谦卑视图, 只渲染 JSON/转发点击) | 替换 |
| LottoPresenter | 输出侧适配器 | JSON Presenter(序列化 LottoTicket → JSON) | 替换 |
| LottoController | 输入侧适配器 | Web Controller(解析 HTTP 请求 → 用例调用) | 替换 |
| QSettingsTicketRepository | 存储适配器 | 数据库仓储(多用户/并发时才需要) | 可选换 |
| main.cpp(组合根) | 桌面装配 | HTTP 服务器装配 + DB 连接 | 替换 |

## 输入侧: Web Controller 替换桌面 LottoController

桌面版已引入输入侧适配器 `LottoController`(按钮点击 → 用例调用),把输入翻译从
谦卑视图剥离(见 architecture.md 用例层说明)。Web 化后 HTTP 请求需要解析(路径/
方法/JSON 体),翻译逻辑升级为 Web Controller,用例端口不变:

```
桌面: MainWindow 按钮 clicked ──► LottoController ──► generateNewTicket()/toggleLock()
Web:  POST /api/tickets/generate ──解析请求──► Web Controller ──► generateNewTicket()
      POST /api/tickets/lock     ──解析请求──► Web Controller ──► toggleLock()
```

## 输出侧: Presenter 换实现而非复用

`LottoViewState` 的按钮文字("🔒 解锁生成")/时间前缀是**桌面 UI 词汇**,Web 前端
文案自己写,不复用。真正跨端复用的是用例层输出的 `LottoTicket` 值类型:

```
桌面: LottoPresenter ──► LottoViewState(按钮文字/时间前缀/emoji)
Web:  JSON Presenter ──► { front:[...], back:[...], locked:true, time:"ISO 字符串" }
```

## 两个关键决策点

### 1. 输出侧信号链在 Web 端换形态

桌面版用例状态经输出边界 `LottoPresenterPort::present(LottoTicket)` 由输出侧
适配器 LottoPresenter 接收,重发 `viewStateChanged(LottoViewState)` 供视图渲染。
跨网络后该"推送"通道需换形态,三种替代:

- **请求-响应**(简单路径):POST generate → 响应直接返回新票据 JSON;状态由服务端
  持有,每会话一个 interactor 实例
- **WebSocket/SSE**(多客户端路径):锁定状态变化需要广播
- **状态移到客户端**(激进路径):浏览器持有票据,API 无状态,interactor 退化为操作函数

### 2. 领域层用 Qt::Core 的权衡成本

服务端若继续用 Qt(C++ + QHttpServer),domain/application 原样保留;若换语言
(Node/Python/Go),整个核心层需要移植。这是「以 Qt 为应用语言」权衡的代价,好在
核心层干净(无 UI/存储依赖),移植面可控。

## 边界形式升级(ch18)

桌面版是**源码边界**(同进程函数调用 + 信号);Web 化后同一位置升级为**网络边界**
(进程/服务)。线的位置不变:

```
[浏览器] ←HTTP/JSON→ [Web Controller ⇄ LottoInteractor ⇄ TicketRepository 端口 ⇄ DB]
                         └── 线内不动(领域 + 用例 + 端口), 线外按需替换 ──┘
```

## 一句话总结

Web 化划线的答案:`LottoInteractor`(用例)与 `TicketRepository`(端口)构成的边界
就是那条线——线内(领域 + 用例 + 端口)归后台永不动,线外全部按需替换(输入解析、
输出序列化、存储实现、前端渲染)。
