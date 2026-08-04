#ifndef QSETTINGSREPOSITORY_H
#define QSETTINGSREPOSITORY_H

#include <QVector>

#include "ticketrepository.h"

class QSettings;

/*!
 * \brief QSettingsTicketRepository 基于 QSettings(INI) 的票据仓储实现
 *
 * 适配器层组件: 实现用例层定义的 TicketRepository 接口,
 * 键名与旧版本完全兼容(可读取旧 INI 数据)。
 * 不持有 QSettings 所有权, 生命周期由组合根/测试保证。
 */
class QSettingsTicketRepository : public TicketRepository
{
public:
    /*! \param settings 目标 QSettings(不持有所有权) */
    explicit QSettingsTicketRepository(QSettings *settings);

    /*! 保存: 锁定时写全量 + sync; 未锁定时仅写 isLocked=false */
    void save(const LottoTicket &ticket) override;
    /*! 加载: 未锁定时返回默认票据(空号码, 无效时间) */
    LottoTicket load() override;
    /*! 清空全部键 + sync */
    void clear() override;

private:
    /*! 将 5 组号码展平为前区/后区两个扁平 int 列表 */
    void flatten(const QVector<LottoResult> &groups,
                 QVariantList &frontList, QVariantList &backList) const;
    /*! 将扁平列表还原为 5 组号码, 缺项位置留空(容错旧数据) */
    QVector<LottoResult> unflatten(const QVariantList &frontList,
                                   const QVariantList &backList) const;
    /*!
     * \brief 解析 generateTime 字符串
     *
     * ISO 格式优先, 失败则用正则提取旧版显示字符串中的
     * "yyyy-MM-dd HH:mm:ss", 均失败返回无效时间。
     */
    QDateTime parseGenerateTime(const QString &raw) const;
    /*! 生成时间转为 ISO 字符串(本地时间) */
    QString formatGenerateTime(const QDateTime &time) const;

    QSettings *m_settings = nullptr;  /*!< 目标设置(不持有所有权) */
};

#endif // QSETTINGSREPOSITORY_H
