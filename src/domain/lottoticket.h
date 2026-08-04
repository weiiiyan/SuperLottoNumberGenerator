#ifndef LOTTOTICKET_H
#define LOTTOTICKET_H

#include <QDateTime>
#include <QMetaType>
#include <QVector>

#include "lottoresult.h"

/*!
 * \brief LottoTicket 保存一期完整的大乐透票据(5 组号码 + 生成时间 + 锁定标志)
 *
 * 纯数据值类型(非 QObject),可复制、可存入 QVariant,
 * 是跨层边界传输的唯一数据载体。
 */
class LottoTicket
{
public:
    // 游戏规则常量: 注数 / 前区每注个数 / 后区每注个数
    static constexpr int GROUP_COUNT = 5;
    static constexpr int FRONT_COUNT = 5;
    static constexpr int BACK_COUNT  = 2;

    LottoTicket();   /*!< 默认构造: 空号码, 无效时间, 未锁定 */
    /*!
     * \brief 构造完整票据
     * \param groups 5 组号码
     * \param generateTime 生成时间(本地时间)
     * \param isLocked 是否已锁定
     */
    LottoTicket(const QVector<LottoResult> &groups,
                const QDateTime &generateTime = QDateTime(),
                bool isLocked = false);

    /*! 返回全部组(只读) */
    const QVector<LottoResult> &groups() const;
    /*! 设置全部组 */
    void setGroups(const QVector<LottoResult> &groups);
    /*! 返回第 \a index 组号码 */
    LottoResult groupAt(int index) const;

    /*! 返回生成时间戳(本地时间) */
    QDateTime generateTime() const;
    /*! 设置生成时间戳 */
    void setGenerateTime(const QDateTime &time);

    /*! 返回锁定标志 */
    bool isLocked() const;
    /*! 设置锁定标志 */
    void setLocked(bool locked);

    /*!
     * \brief 校验票据是否合法
     *
     * 组数 = GROUP_COUNT,每组前区 FRONT_COUNT 个、后区 BACK_COUNT 个,
     * 号码在规则范围内且升序。
     */
    bool isValid() const;

    /*! 相等比较: 号码、时间、锁定标志全部相同 */
    bool operator==(const LottoTicket &other) const;

private:
    QVector<LottoResult> m_groups;  /*!< 5 组号码 */
    QDateTime m_generateTime;       /*!< 生成时间戳 */
    bool m_isLocked = false;        /*!< 锁定标志 */
};

Q_DECLARE_METATYPE(LottoTicket)

#endif // LOTTOTICKET_H
