#ifndef LOTTORESULT_H
#define LOTTORESULT_H

#include <QVector>
#include <QMetaType>

/*!
 * \brief LottoResult 保存一组大乐透开奖号码(前区 5 个 + 后区 2 个)
 *
 * 号码均为升序排列,前区范围 [1, 35],后区范围 [1, 12]。
 * 通过 Q_DECLARE_METATYPE 注册为 Qt 元类型。
 */
struct LottoResult {
    QVector<int> front; /*!< 前区号码,5 个,升序,范围 [1, 35] */
    QVector<int> back;  /*!< 后区号码,2 个,升序,范围 [1, 12] */

    LottoResult() = default;
    explicit LottoResult(const QVector<int> &f, const QVector<int> &b) : front(f), back(b) {}
    /*! 返回前区号码(只读) */
    const QVector<int>& frontVec() const { return front; }
    /*! 返回后区号码(只读) */
    const QVector<int>& backVec() const { return back; }

    bool operator==(const LottoResult &other) const {
        return front == other.front && back == other.back;
    }
    bool operator!=(const LottoResult &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(LottoResult)

#endif // LOTTORESULT_H
