#ifndef LOTTOINTERACTOR_H
#define LOTTOINTERACTOR_H

#include <QObject>

#include "lottoinputboundary.h"
#include "lottoticket.h"

class TicketRepository;
class LottoEngine;
class LottoPresenterPort;

/*!
 * \brief LottoInteractor 用例层交互器(use case interactor), 持有票据状态并编排用例
 *
 * 不做输入/输出格式适配, 只执行应用特定业务规则(锁定时拒绝生成、
 * 状态变化守卫)与用例编排。实现输入边界 LottoInputBoundary(供 Controller
 * 调用), 通过输出边界 LottoPresenterPort 把状态推送给出输出适配器。
 * 持久化依赖 TicketRepository 抽象。本类不持有 repository/engine/presenter
 * 所有权, 生命周期由组合根保证, 三者均须非空。装配错误将直接崩溃暴露,
 * 而非静默失效。
 */
class LottoInteractor : public QObject, public LottoInputBoundary
{
    Q_OBJECT
public:
    /*!
     * \brief 构造交互器
     * \param repository 票据仓储(不持有所有权)
     * \param engine 号码生成器(不持有所有权)
     * \param presenter 输出边界端口(不持有所有权, 由展示器实现)
     * \param parent QObject 父对象
     */
    explicit LottoInteractor(TicketRepository *repository, LottoEngine *engine,
                             LottoPresenterPort *presenter,
                             QObject *parent = nullptr);

    /*! 用例: 生成 5 组新号码(锁定时拒绝) */
    void generateNewTicket() override;
    /*! 用例: 切换锁定状态, 状态变化即保存 */
    void toggleLock() override;
    /*! 用例: 设置锁定状态, 状态未变化时不做任何事 */
    void setLocked(bool locked) override;
    /*! 用例: 异步恢复持久化的票据(singleShot(0), 不阻塞 Android 启动) */
    void load() override;

    /*! 返回当前票据(只读) */
    const LottoTicket &currentTicket() const;

private:
    TicketRepository  *m_repository = nullptr;  /*!< 票据仓储(不持有所有权) */
    LottoEngine       *m_engine     = nullptr;  /*!< 号码生成器(不持有所有权) */
    LottoPresenterPort *m_presenter = nullptr;  /*!< 输出边界端口(不持有所有权) */
    LottoTicket        m_ticket;                /*!< 交互器持有的唯一状态源 */
};

#endif // LOTTOINTERACTOR_H
