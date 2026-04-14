// F:/Qt Creator Code Learn/SuperLottoNumberGenerator/lottoengine.cpp
#include "lottoengine.h"
#include <QRandomGenerator>
#include <algorithm>
#include <vector>
#include <numeric>

LottoEngine::LottoEngine(QObject *parent)
    : QObject(parent)
{

}

LottoResult LottoEngine::generate() const
{
    // ✅ 前区号码生成：从 1–35 中无重复随机选取 5 个数字
    //    - std::iota(frontPool.begin(), frontPool.end(), 1)：将 frontPool 填充为连续整数序列 [1, 2, ..., 35]
    //    - std::shuffle(..., *QRandomGenerator::global())：使用 Qt 提供的高质量、线程安全的全局随机引擎对序列进行随机重排
    //      （QRandomGenerator::global() 比 rand() 更均匀、更可靠，且无需手动 seed）
    //    - 取洗牌后前 5 个元素，升序排序以符合彩票标准显示格式
    std::vector<int> frontPool(35);
    std::iota(frontPool.begin(), frontPool.end(), 1);
    std::shuffle(frontPool.begin(), frontPool.end(), *QRandomGenerator::global());
    QVector<int> front;
    front.reserve(5);
    for (int i = 0; i < 5; ++i)
        front.append(frontPool[i]);
    std::sort(front.begin(), front.end());

    // ✅ 后区号码生成：从 1–12 中无重复随机选取 2 个数字
    //    - std::iota(backPool.begin(), backPool.end(), 1)：构建初始序列 [1, 2, ..., 12]
    //    - std::shuffle(..., *QRandomGenerator::global())：再次使用同一高质量随机引擎独立洗牌，确保前后区互不影响
    //    - 取前 2 个并升序排列
    std::vector<int> backPool(12);
    std::iota(backPool.begin(), backPool.end(), 1);
    std::shuffle(backPool.begin(), backPool.end(), *QRandomGenerator::global());
    QVector<int> back;
    back.reserve(2);
    for (int i = 0; i < 2; ++i)
        back.append(backPool[i]);
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