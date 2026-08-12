#include "lottointeractor.h"

#include <QDateTime>
#include <QTimer>

#include "lottoengine.h"
#include "lottopresenterport.h"
#include "ticketrepository.h"

LottoInteractor::LottoInteractor(TicketRepository *repository, LottoEngine *engine,
                                 LottoPresenterPort *presenter, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_engine(engine)
    , m_presenter(presenter)
{
}

void LottoInteractor::generateNewTicket()
{
    // 锁定状态下拒绝生成(视图禁用按钮只是辅助, 此处判定是权威)
    if (m_ticket.isLocked())
        return;

    m_ticket.setGroups(m_engine->generateBatch(LottoTicket::GROUP_COUNT));
    m_ticket.setGenerateTime(QDateTime::currentDateTimeUtc().toLocalTime());
    m_ticket.setLocked(false);
    m_presenter->present(m_ticket);
}

void LottoInteractor::toggleLock()
{
    setLocked(!m_ticket.isLocked());
}

void LottoInteractor::setLocked(bool locked)
{
    // 状态变化守卫: 相同状态不重复保存
    if (m_ticket.isLocked() == locked)
        return;

    // 含号码才可锁定(视图禁用按钮只是辅助, 此处判定权威):
    // 防止 "locked + 空号码" 进入无可恢复的视图死锁状态
    if (locked && !m_ticket.hasNumbers())
        return;

    m_ticket.setLocked(locked);
    m_repository->save(m_ticket);
    m_presenter->present(m_ticket);
}

void LottoInteractor::load()
{
    // 异步恢复, 避免阻塞 Android 启动
    QTimer::singleShot(0, this, [this] {
        m_ticket = m_repository->load();
        // 恢复 "锁定但空号码" 的损坏数据时解除锁定, 避免视图死锁
        if (m_ticket.isLocked() && !m_ticket.hasNumbers())
            m_ticket.setLocked(false);
        m_presenter->present(m_ticket);
    });
}

const LottoTicket &LottoInteractor::currentTicket() const
{
    return m_ticket;
}
