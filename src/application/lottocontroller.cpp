#include "lottocontroller.h"

#include <QDateTime>
#include <QTimer>

#include "lottoengine.h"
#include "ticketrepository.h"

LottoController::LottoController(TicketRepository *repository, LottoEngine *engine,
                                 QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_engine(engine)
{
}

void LottoController::generateNewTicket()
{
    // 锁定状态下拒绝生成(视图禁用按钮只是辅助, 此处判定是权威)
    if (m_ticket.isLocked())
        return;

    m_ticket.setGroups(m_engine->generateBatch(LottoTicket::GROUP_COUNT));
    m_ticket.setGenerateTime(QDateTime::currentDateTimeUtc().toLocalTime());
    m_ticket.setLocked(false);
    emit ticketChanged(m_ticket);
}

void LottoController::toggleLock()
{
    setLocked(!m_ticket.isLocked());
}

void LottoController::setLocked(bool locked)
{
    // 状态变化守卫: 相同状态不重复保存
    if (m_ticket.isLocked() == locked)
        return;

    m_ticket.setLocked(locked);
    m_repository->save(m_ticket);
    emit ticketChanged(m_ticket);
}

void LottoController::load()
{
    // 异步恢复, 避免阻塞 Android 启动
    QTimer::singleShot(0, this, [this] {
        m_ticket = m_repository->load();
        emit ticketChanged(m_ticket);
    });
}

const LottoTicket &LottoController::currentTicket() const
{
    return m_ticket;
}
