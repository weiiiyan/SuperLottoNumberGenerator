// 测试公共辅助: 内存版仓储/输出端口替身与固定票据构造器
// 供各单元测试共享, 避免逐文件复制导致的漂移风险

#ifndef TESTHELPERS_H
#define TESTHELPERS_H

#include <QDateTime>
#include <QVector>

#include "lottopresenterport.h"
#include "lottoticket.h"
#include "ticketrepository.h"

/*! 内存版票据仓储: 记录每次保存的票据, 可预设加载结果 */
class FakeTicketRepository : public TicketRepository
{
public:
    LottoTicket loadResult;              /*!< 预设的 load 返回值 */
    QVector<LottoTicket> savedTickets;   /*!< 历次 save 的票据 */

    int saveCount() const { return savedTickets.size(); }

    void save(const LottoTicket &ticket) override { savedTickets.append(ticket); }
    LottoTicket load() override { return loadResult; }
    void clear() override { savedTickets.clear(); }
};

/*! 内存版输出边界端口: 记录每次 present() 的票据, 供断言"状态已推送" */
class FakePresenterPort : public LottoPresenterPort
{
public:
    QVector<LottoTicket> presented;   /*!< 历次 present() 的票据 */

    int presentCount() const { return presented.size(); }

    void present(const LottoTicket &ticket) override { presented.append(ticket); }
    void clear() { presented.clear(); }
};

/*! 构造固定递增号码的票据(号码均在合法范围) */
inline LottoTicket makeTicket(int groupCount = LottoTicket::GROUP_COUNT,
                              bool locked = false,
                              const QDateTime &time = QDateTime())
{
    QVector<LottoResult> groups;
    for (int g = 0; g < groupCount; ++g) {
        QVector<int> front, back;
        for (int i = 0; i < LottoTicket::FRONT_COUNT; ++i)
            front << g * LottoTicket::FRONT_COUNT + i + 1;
        for (int i = 0; i < LottoTicket::BACK_COUNT; ++i)
            back << g * LottoTicket::BACK_COUNT + i + 1;
        groups.append(LottoResult(front, back));
    }
    return LottoTicket(groups, time, locked);
}

#endif // TESTHELPERS_H
