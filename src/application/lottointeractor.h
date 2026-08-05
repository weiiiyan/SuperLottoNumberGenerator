#ifndef LOTTOINTERACTOR_H
#define LOTTOINTERACTOR_H

#include <QObject>

#include "lottoticket.h"

class TicketRepository;
class LottoEngine;

/*!
 * \brief LottoInteractor 用例层交互器(use case interactor), 持有票据状态并编排用例
 *
 * 不做输入/输出格式适配, 只执行应用特定业务规则(锁定时拒绝生成、
 * 状态变化守卫)与用例编排。视图只依赖本类(通过 ticketChanged 信号渲染),
 * 持久化只依赖 TicketRepository 抽象。本类不持有 repository/engine 所有权,
 * 生命周期由组合根保证。
 */
class LottoInteractor : public QObject
{
    Q_OBJECT
public:
    /*!
     * \brief 构造交互器
     * \param repository 票据仓储(不持有所有权)
     * \param engine 号码生成器(不持有所有权)
     * \param parent QObject 父对象
     */
    explicit LottoInteractor(TicketRepository *repository, LottoEngine *engine,
                             QObject *parent = nullptr);

    /*! 用例: 生成 5 组新号码(锁定时拒绝) */
    void generateNewTicket();
    /*! 用例: 切换锁定状态, 状态变化即保存 */
    void toggleLock();
    /*! 用例: 设置锁定状态, 状态未变化时不做任何事 */
    void setLocked(bool locked);
    /*! 用例: 异步恢复持久化的票据(singleShot(0), 不阻塞 Android 启动) */
    void load();

    /*! 返回当前票据(只读) */
    const LottoTicket &currentTicket() const;

signals:
    /*! 票据状态变化(生成/锁定/恢复)后发出, 视图唯一渲染入口 */
    void ticketChanged(const LottoTicket &ticket);

private:
    TicketRepository *m_repository = nullptr;  /*!< 票据仓储(不持有所有权) */
    LottoEngine      *m_engine     = nullptr;  /*!< 号码生成器(不持有所有权) */
    LottoTicket       m_ticket;                /*!< 交互器持有的唯一状态源 */
};

#endif // LOTTOINTERACTOR_H
