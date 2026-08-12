// 测试公共辅助: 内存版仓储/输出端口替身与固定票据构造器
// 供各单元测试共享, 避免逐文件复制导致的漂移风险

#ifndef TESTHELPERS_H
#define TESTHELPERS_H

#include <QDateTime>
#include <QVector>

#include "lottoengine.h"
#include "lottoinputboundary.h"
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

/*! 记录调用次数的输入边界替身: 供断言"外部输入 → 用例调用"的翻译 */
class FakeInputBoundary : public LottoInputBoundary
{
public:
    int generateCalls   = 0;   /*!< generateNewTicket() 调用次数 */
    int toggleLockCalls = 0;   /*!< toggleLock() 调用次数 */
    int setLockedCalls  = 0;   /*!< setLocked() 调用次数 */
    int loadCalls       = 0;   /*!< load() 调用次数 */

    void generateNewTicket() override { ++generateCalls; }
    void toggleLock() override { ++toggleLockCalls; }
    void setLocked(bool) override { ++setLockedCalls; }
    void load() override { ++loadCalls; }
};

/*! 返回固定号码的引擎替身: 供用例层确定性编排测试(不依赖真实随机性) */
class FakeLottoEngine : public LottoEngine
{
public:
    explicit FakeLottoEngine(const QVector<LottoGroup> &fixed = QVector<LottoGroup>())
        : m_fixed(fixed) {}

    /*! generate(): 返回第一组固定号码(无固定号码时返回空组) */
    LottoGroup generate() const override
    {
        return m_fixed.isEmpty() ? LottoGroup() : m_fixed.first();
    }

    /*! generateBatch(): 返回 \a count 组固定号码, 固定号码不足时循环复用 */
    QVector<LottoGroup> generateBatch(int count) const override
    {
        QVector<LottoGroup> out;
        for (int i = 0; i < count; ++i)
            out.append(m_fixed.isEmpty() ? LottoGroup() : m_fixed.at(i % m_fixed.size()));
        return out;
    }

private:
    QVector<LottoGroup> m_fixed;   /*!< 固定返回的号码 */
};

/*! 构造固定递增号码的票据(号码均在合法范围) */
inline LottoTicket makeTicket(int groupCount = LottoTicket::GROUP_COUNT,
                              bool locked = false,
                              const QDateTime &time = QDateTime())
{
    QVector<LottoGroup> groups;
    for (int g = 0; g < groupCount; ++g) {
        QVector<int> front, back;
        for (int i = 0; i < LottoTicket::FRONT_COUNT; ++i)
            front << g * LottoTicket::FRONT_COUNT + i + 1;
        for (int i = 0; i < LottoTicket::BACK_COUNT; ++i)
            back << g * LottoTicket::BACK_COUNT + i + 1;
        groups.append(LottoGroup(front, back));
    }
    return LottoTicket(groups, time, locked);
}

#endif // TESTHELPERS_H
