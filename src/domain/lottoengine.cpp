#include "lottoengine.h"
#include <QRandomGenerator>
#include <algorithm>
#include <vector>
#include <numeric>

LottoEngine::LottoEngine(QObject *parent)
    : QObject(parent)
{}

LottoGroup LottoEngine::generate() const
{
    // 前区：从 [FRONT_MIN, FRONT_MAX] 号码池中无重复随机选取 FRONT_COUNT 个, 升序排列
    // 使用 QRandomGenerator::global() 保证高质量随机性(无需手动 seed)
    std::vector<int> frontPool(LottoGroup::FRONT_MAX);
    std::iota(frontPool.begin(), frontPool.end(), LottoGroup::FRONT_MIN);
    std::shuffle(frontPool.begin(), frontPool.end(), *QRandomGenerator::global());
    QVector<int> front(frontPool.begin(), frontPool.begin() + LottoGroup::FRONT_COUNT);
    std::sort(front.begin(), front.end());

    // 后区：从 [BACK_MIN, BACK_MAX] 号码池中无重复随机选取 BACK_COUNT 个, 升序排列
    std::vector<int> backPool(LottoGroup::BACK_MAX);
    std::iota(backPool.begin(), backPool.end(), LottoGroup::BACK_MIN);
    std::shuffle(backPool.begin(), backPool.end(), *QRandomGenerator::global());
    QVector<int> back(backPool.begin(), backPool.begin() + LottoGroup::BACK_COUNT);
    std::sort(back.begin(), back.end());

    return LottoGroup(front, back);
}

QVector<LottoGroup> LottoEngine::generateBatch(int count) const
{
    QVector<LottoGroup> results;
    if (count <= 0)
        return results;
    results.reserve(count);
    for (int i = 0; i < count; ++i)
        results.append(generate());
    return results;
}
