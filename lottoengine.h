#ifndef LOTTOENGINE_H
#define LOTTOENGINE_H

#include <QObject>
#include <QVector>
#include <QMetaType>

struct LottoResult {
    QVector<int> front; // size == 5, sorted, [1,35]
    QVector<int> back;  // size == 2, sorted, [1,12]

    LottoResult() = default;
    explicit LottoResult(const QVector<int> &f, const QVector<int> &b) : front(f), back(b) {}
    const QVector<int>& frontVec() const { return front; }
    const QVector<int>& backVec() const { return back; }

    bool operator==(const LottoResult &other) const {
        return front == other.front && back == other.back;
    }
    bool operator!=(const LottoResult &other) const {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(LottoResult)

class LottoEngine : public QObject
{
    Q_OBJECT
public:
    explicit LottoEngine(QObject *parent = nullptr);

    Q_INVOKABLE LottoResult generate() const;
    Q_INVOKABLE QVector<LottoResult> generateBatch(int count) const;
};

#endif // LOTTOENGINE_H
