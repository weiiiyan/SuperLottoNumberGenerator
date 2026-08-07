#include "lottoticket.h"

LottoTicket::LottoTicket()
{
}

LottoTicket::LottoTicket(const QVector<LottoGroup> &groups,
                         const QDateTime &generateTime, bool isLocked)
    : m_groups(groups)
    , m_generateTime(generateTime)
    , m_isLocked(isLocked)
{
}

const QVector<LottoGroup> &LottoTicket::groups() const
{
    return m_groups;
}

void LottoTicket::setGroups(const QVector<LottoGroup> &groups)
{
    m_groups = groups;
}

LottoGroup LottoTicket::groupAt(int index) const
{
    return (index >= 0 && index < m_groups.size()) ? m_groups[index] : LottoGroup();
}

QDateTime LottoTicket::generateTime() const
{
    return m_generateTime;
}

void LottoTicket::setGenerateTime(const QDateTime &time)
{
    m_generateTime = time;
}

bool LottoTicket::isLocked() const
{
    return m_isLocked;
}

void LottoTicket::setLocked(bool locked)
{
    m_isLocked = locked;
}

bool LottoTicket::hasNumbers() const
{
    for (const LottoGroup &result : m_groups) {
        if (!result.front.isEmpty() || !result.back.isEmpty())
            return true;
    }
    return false;
}

bool LottoTicket::isValid() const
{
    if (m_groups.size() != GROUP_COUNT)
        return false;

    for (const LottoGroup &result : m_groups) {
        if (result.front.size() != FRONT_COUNT || result.back.size() != BACK_COUNT)
            return false;
        for (int i = 0; i < FRONT_COUNT; ++i) {
            if (result.front[i] < LottoGroup::FRONT_MIN || result.front[i] > LottoGroup::FRONT_MAX)
                return false;
            if (i > 0 && result.front[i] <= result.front[i - 1])
                return false;
        }
        for (int i = 0; i < BACK_COUNT; ++i) {
            if (result.back[i] < LottoGroup::BACK_MIN || result.back[i] > LottoGroup::BACK_MAX)
                return false;
            if (i > 0 && result.back[i] <= result.back[i - 1])
                return false;
        }
    }
    return true;
}

bool LottoTicket::operator==(const LottoTicket &other) const
{
    return m_groups == other.m_groups
        && m_generateTime == other.m_generateTime
        && m_isLocked == other.m_isLocked;
}
