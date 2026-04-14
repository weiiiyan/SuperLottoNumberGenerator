// F:/Qt Creator Code Learn/SuperLottoNumberGenerator/lottoengine.h
#ifndef LOTTOENGINE_H
#define LOTTOENGINE_H

#include <QObject>
#include <QVector>
#include <QMetaType>

struct LottoResult {
    QVector<int> front; // size == 5, sorted, [1,35]
    QVector<int> back;  // size == 2, sorted, [1,12]

public:
    LottoResult() = default;
    explicit LottoResult(const QVector<int> &f, const QVector<int> &b) : front(f), back(b) {}
    const QVector<int>& frontVec() const { return this->front; }
    const QVector<int>& backVec() const { return this->back; }
};

Q_DECLARE_METATYPE(LottoResult)

class LottoEngine : public QObject
{
    Q_OBJECT
public:
    explicit LottoEngine(QObject *parent = nullptr);

    // ✅ 主要接口：生成一组合法号码
    Q_INVOKABLE LottoResult generate() const;

    // ✅ 可选：批量生成（如“生成5注”）
    Q_INVOKABLE QVector<LottoResult> generateBatch(int count) const;

signals:

public slots:
};

#endif // LOTTOENGINE_H