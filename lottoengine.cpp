#include "lottoengine.h"
#include <QRandomGenerator>
#include <algorithm>
#include <vector>
#include <numeric>

LottoEngine::LottoEngine(QObject *parent)
    : QObject(parent)
{}

LottoResult LottoEngine::generate() const
{
    // 前区：从 1–35 号码池中无重复随机选取 5 个，升序排列
    // 使用 QRandomGenerator::global() 保证高质量随机性（无需手动 seed）
    std::vector<int> frontPool(35);
    std::iota(frontPool.begin(), frontPool.end(), 1);
    std::shuffle(frontPool.begin(), frontPool.end(), *QRandomGenerator::global());
    QVector<int> front(frontPool.begin(), frontPool.begin() + 5);
    std::sort(front.begin(), front.end());

    // 后区：从 1–12 号码池中无重复随机选取 2 个，升序排列
    std::vector<int> backPool(12);
    std::iota(backPool.begin(), backPool.end(), 1);
    std::shuffle(backPool.begin(), backPool.end(), *QRandomGenerator::global());
    QVector<int> back(backPool.begin(), backPool.begin() + 2);
    std::sort(back.begin(), back.end());

    return LottoResult(front, back);
}

QVector<LottoResult> LottoEngine::generateBatch(int count) const
{
    QVector<LottoResult> results;
    if (count <= 0)
        return results;
    results.reserve(count);
    for (int i = 0; i < count; ++i)
        results.append(generate());
    return results;
}
