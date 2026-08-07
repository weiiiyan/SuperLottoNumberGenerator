#include "qsettingsrepository.h"

#include <QRegularExpression>
#include <QSettings>

QSettingsTicketRepository::QSettingsTicketRepository(QSettings *settings)
    : m_settings(settings)
{
}

void QSettingsTicketRepository::save(const LottoTicket &ticket)
{
    if (ticket.isLocked()) {
        QVariantList frontList, backList;
        flatten(ticket.groups(), frontList, backList);
        m_settings->setValue("isLocked",     true);
        m_settings->setValue("frontNumbers", frontList);
        m_settings->setValue("backNumbers",  backList);
        m_settings->setValue("generateTime", formatGenerateTime(ticket.generateTime()));
    } else {
        // 未锁定时仅写标志, 磁盘残留旧号码键不参与恢复
        m_settings->setValue("isLocked", false);
    }
    m_settings->sync();
}

LottoTicket QSettingsTicketRepository::load()
{
    if (!m_settings->value("isLocked", false).toBool())
        return LottoTicket();

    const QVariantList frontNumbers = m_settings->value("frontNumbers").toList();
    const QVariantList backNumbers  = m_settings->value("backNumbers").toList();

    return LottoTicket(unflatten(frontNumbers, backNumbers),
                       parseGenerateTime(m_settings->value("generateTime").toString()),
                       true);
}

void QSettingsTicketRepository::clear()
{
    m_settings->clear();
    m_settings->sync();
}

void QSettingsTicketRepository::flatten(const QVector<LottoGroup> &groups,
                                        QVariantList &frontList,
                                        QVariantList &backList) const
{
    for (const LottoGroup &result : groups) {
        for (int number : result.front)
            frontList << number;
        for (int number : result.back)
            backList << number;
    }
}

QVector<LottoGroup> QSettingsTicketRepository::unflatten(const QVariantList &frontList,
                                                          const QVariantList &backList) const
{
    QVector<LottoGroup> groups;
    for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g) {
        LottoGroup result;
        for (int i = 0; i < LottoTicket::FRONT_COUNT; ++i) {
            const int idx = g * LottoTicket::FRONT_COUNT + i;
            if (idx < frontList.size())
                result.front << frontList[idx].toInt();
        }
        for (int i = 0; i < LottoTicket::BACK_COUNT; ++i) {
            const int idx = g * LottoTicket::BACK_COUNT + i;
            if (idx < backList.size())
                result.back << backList[idx].toInt();
        }
        groups.append(result);
    }
    return groups;
}

QDateTime QSettingsTicketRepository::parseGenerateTime(const QString &raw) const
{
    if (raw.isEmpty())
        return QDateTime();

    const QDateTime iso = QDateTime::fromString(raw, Qt::ISODate);
    if (iso.isValid())
        return iso;

    // 兼容旧版显示字符串: "🕐 生成时间：yyyy-MM-dd HH:mm:ss"(前缀可变)
    static const QRegularExpression timePattern("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})");
    const QRegularExpressionMatch match = timePattern.match(raw);
    if (match.hasMatch())
        return QDateTime::fromString(match.captured(1), "yyyy-MM-dd HH:mm:ss");

    return QDateTime();
}

QString QSettingsTicketRepository::formatGenerateTime(const QDateTime &time) const
{
    if (!time.isValid())
        return QString();
    return time.toString(Qt::ISODate);
}
