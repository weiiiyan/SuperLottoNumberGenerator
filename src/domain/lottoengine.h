#ifndef LOTTOENGINE_H
#define LOTTOENGINE_H

#include <QObject>
#include <QVector>

#include "lottoresult.h"

/*!
 * \brief LottoEngine 使用 QRandomGenerator 生成随机大乐透号码
 *
 * 所有方法标记为 Q_INVOKABLE，可被 QML 调用。
 */
class LottoEngine : public QObject
{
    Q_OBJECT
public:
    explicit LottoEngine(QObject *parent = nullptr);

    /*! 生成一组随机号码 */
    Q_INVOKABLE LottoResult generate() const;
    /*! 批量生成 \a count 组随机号码 */
    Q_INVOKABLE QVector<LottoResult> generateBatch(int count) const;
};

#endif // LOTTOENGINE_H
