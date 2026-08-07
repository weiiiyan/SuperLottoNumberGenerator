#ifndef LOTTOGROUP_H
#define LOTTOGROUP_H

#include <QVector>
#include <QMetaType>

/*!
 * \brief LottoGroup 保存一组大乐透号码(前区 5 个 + 后区 2 个)
 *
 * 号码均为升序排列,前区范围 [1, 35],后区范围 [1, 12]。
 * 组级游戏规则常量(每注个数/号码范围)在此定义,是全项目唯一来源。
 * 通过 Q_DECLARE_METATYPE 注册为 Qt 元类型。
 */
struct LottoGroup {
    // 游戏规则常量(组级): 每注号码个数与号码范围
    static constexpr int FRONT_COUNT = 5;  /*!< 前区每注号码个数 */
    static constexpr int BACK_COUNT  = 2;  /*!< 后区每注号码个数 */
    static constexpr int FRONT_MIN   = 1;  /*!< 前区号码最小值 */
    static constexpr int FRONT_MAX   = 35; /*!< 前区号码最大值 */
    static constexpr int BACK_MIN    = 1;  /*!< 后区号码最小值 */
    static constexpr int BACK_MAX    = 12; /*!< 后区号码最大值 */

    QVector<int> front; /*!< 前区号码,FRONT_COUNT 个,升序,范围 [FRONT_MIN, FRONT_MAX] */
    QVector<int> back;  /*!< 后区号码,BACK_COUNT 个,升序,范围 [BACK_MIN, BACK_MAX] */

    LottoGroup() = default;
    explicit LottoGroup(const QVector<int> &f, const QVector<int> &b) : front(f), back(b) {}

    bool operator==(const LottoGroup &other) const {
        return front == other.front && back == other.back;
    }
    bool operator!=(const LottoGroup &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(LottoGroup)

#endif // LOTTOGROUP_H
