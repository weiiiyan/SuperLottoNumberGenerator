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
    // 用 shuffle + QRandomGenerator 而非 rand()，保证无重复且分布均匀、
    // ✅ 前区号码生成：从 1–35 中无重复随机选取 5 个数字
    //    - std::iota(frontPool.begin(), frontPool.end(), 1)：将 frontPool 填充为连续整数序列 [1, 2, ..., 35]
    //    - std::shuffle(..., *QRandomGenerator::global())：使用 Qt 提供的高质量、线程安全的全局随机引擎对序列进行随机重排
    //      （QRandomGenerator::global() 比 rand() 更均匀、更可靠，且无需手动 seed）
    //    - 取洗牌后前 5 个元素，升序排序以符合彩票标准显示格式
    std::vector<int> frontPool(35);
    std::iota(frontPool.begin(), frontPool.end(), 1);
    std::shuffle(frontPool.begin(), frontPool.end(), *QRandomGenerator::global());
    QVector<int> front(frontPool.begin(), frontPool.begin() + 5);
    std::sort(front.begin(), front.end());

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
    results.reserve(count);
    for (int i = 0; i < count; ++i)
        results.append(generate());
    return results;
}
