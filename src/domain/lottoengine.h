#ifndef LOTTOENGINE_H
#define LOTTOENGINE_H

#include <QObject>
#include <QVector>

#include "lottogroup.h"

/*!
 * \brief LottoEngine 使用 QRandomGenerator 生成随机大乐透号码
 */
class LottoEngine : public QObject
{
    Q_OBJECT
public:
    explicit LottoEngine(QObject *parent = nullptr);

    /*! 生成一组随机号码 */
    LottoGroup generate() const;
    /*! 批量生成 \a count 组随机号码 */
    QVector<LottoGroup> generateBatch(int count) const;
};

#endif // LOTTOENGINE_H
