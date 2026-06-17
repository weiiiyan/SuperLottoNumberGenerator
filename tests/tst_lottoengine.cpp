// 大乐透引擎单元测试
// 测试 LottoEngine::generate() 和 generateBatch() 的逻辑正确性
// 不依赖 UI，可在桌面端独立运行

#include <QTest>
#include <QSet>
#include <QVector>
#include <algorithm>

#include "lottoengine.h"

class TestLottoEngine : public QObject
{
    Q_OBJECT

private:
    LottoEngine *engine = nullptr;

    /// 验证单个 LottoResult 满足所有约束（数量、范围、排序、唯一性）
    static void verifySingleResult(const LottoResult &r)
    {
        // 数量
        QCOMPARE(r.front.size(), 5);
        QCOMPARE(r.back.size(), 2);

        // 范围
        QVERIFY(std::all_of(r.front.cbegin(), r.front.cend(), [](int n) { return n >= 1 && n <= 35; }));
        QVERIFY(std::all_of(r.back.cbegin(), r.back.cend(), [](int n) { return n >= 1 && n <= 12; }));

        // 排序
        QVERIFY(std::is_sorted(r.front.cbegin(), r.front.cend()));
        QVERIFY(std::is_sorted(r.back.cbegin(), r.back.cend()));

        // 唯一性
        QSet<int> fSet(r.front.cbegin(), r.front.cend());
        QCOMPARE(fSet.size(), 5);
        QSet<int> bSet(r.back.cbegin(), r.back.cend());
        QCOMPARE(bSet.size(), 2);
    }

private slots:
    // ── 测试生命周期 ──

    void initTestCase()
    {
        engine = new LottoEngine(this);
    }

    void cleanupTestCase()
    {
        // engine 作为 QObject 子对象自动析构，无需手动 delete
    }

    // ═══════════════════════════════════════════
    // generate() — 数量正确性
    // ═══════════════════════════════════════════

    void generate_shouldReturn5FrontNumbers()
    {
        LottoResult r = engine->generate();
        QCOMPARE(r.front.size(), 5);
    }

    void generate_shouldReturn2BackNumbers()
    {
        LottoResult r = engine->generate();
        QCOMPARE(r.back.size(), 2);
    }

    // ═══════════════════════════════════════════
    // generate() — 值域正确性
    // ═══════════════════════════════════════════

    void generate_frontNumbersShouldBeInRange1to35()
    {
        // 多次验证以增加置信度
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            for (int n : r.front) {
                QVERIFY2(n >= 1 && n <= 35, QString("前区号码 %1 超出 [1,35] 范围").arg(n).toUtf8());
            }
        }
    }

    void generate_backNumbersShouldBeInRange1to12()
    {
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            for (int n : r.back) {
                QVERIFY2(n >= 1 && n <= 12, QString("后区号码 %1 超出 [1,12] 范围").arg(n).toUtf8());
            }
        }
    }

    // ═══════════════════════════════════════════
    // generate() — 排序正确性
    // ═══════════════════════════════════════════

    void generate_frontNumbersShouldBeSortedAscending()
    {
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            QVERIFY2(std::is_sorted(r.front.begin(), r.front.end()), "前区号码未按升序排列");
        }
    }

    void generate_backNumbersShouldBeSortedAscending()
    {
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            QVERIFY2(std::is_sorted(r.back.begin(), r.back.end()), "后区号码未按升序排列");
        }
    }

    // ═══════════════════════════════════════════
    // generate() — 唯一性（无重复）
    // ═══════════════════════════════════════════

    void generate_frontNumbersShouldBeUnique()
    {
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            QSet<int> seen;
            for (int n : r.front) {
                QVERIFY2(!seen.contains(n), QString("前区号码 %1 重复出现").arg(n).toUtf8());
                seen.insert(n);
            }
        }
    }

    void generate_backNumbersShouldBeUnique()
    {
        for (int iter = 0; iter < 100; ++iter) {
            LottoResult r = engine->generate();
            QSet<int> seen;
            for (int n : r.back) {
                QVERIFY2(!seen.contains(n), QString("后区号码 %1 重复出现").arg(n).toUtf8());
                seen.insert(n);
            }
        }
    }

    // ═══════════════════════════════════════════
    // generate() — 一次调用综合验证
    // ═══════════════════════════════════════════

    void generate_allPropertiesCombined()
    {
        verifySingleResult(engine->generate());
    }

    // ═══════════════════════════════════════════
    // generateBatch() — 边界与数量
    // ═══════════════════════════════════════════

    void generateBatch_countZero_shouldReturnEmpty()
    {
        QVector<LottoResult> results = engine->generateBatch(0);
        QCOMPARE(results.size(), 0);
    }

    void generateBatch_countNegative_shouldReturnEmpty()
    {
        // 负数应被视为无效输入，返回空结果
        QVector<LottoResult> results = engine->generateBatch(-1);
        QCOMPARE(results.size(), 0);

        results = engine->generateBatch(-100);
        QCOMPARE(results.size(), 0);
    }

    void generateBatch_countOne_shouldReturnOneResult()
    {
        QVector<LottoResult> results = engine->generateBatch(1);
        QCOMPARE(results.size(), 1);
        verifySingleResult(results[0]);
    }

    void generateBatch_countMultiple_shouldReturnCorrectCount()
    {
        constexpr int expected = 10;
        QVector<LottoResult> results = engine->generateBatch(expected);
        QCOMPARE(results.size(), expected);
    }

    void generateBatch_allResultsShouldBeValid()
    {
        constexpr int count = 50;
        QVector<LottoResult> results = engine->generateBatch(count);
        QCOMPARE(results.size(), count);

        for (int i = 0; i < count; ++i)
            verifySingleResult(results[i]);
    }

    void generateBatch_largeBatch_shouldNotCrash()
    {
        // 大批量测试：确保性能和内存合理
        constexpr int count = 1000;
        QVector<LottoResult> results = engine->generateBatch(count);
        QCOMPARE(results.size(), count);
    }

    // ═══════════════════════════════════════════
    // 随机性测试（概率性，但失败概率极低）
    // ═══════════════════════════════════════════

    void generate_consecutiveCallsShouldNotBeIdentical()
    {
        // 连续 100 次调用，应该至少产生 2 种不同的结果
        constexpr int rounds = 100;
        QVector<LottoResult> results = engine->generateBatch(rounds);

        bool hasDifference = false;
        for (int i = 1; i < results.size() && !hasDifference; ++i) {
            if (results[i] != results[0]) {
                hasDifference = true;
            }
        }
        QVERIFY2(hasDifference, "连续 100 次生成全部相同，随机性可能存在严重问题");
    }

    void generate_numberDistributionIsReasonable()
    {
        // 生成大量号码，验证每个号码至少出现过一次
        // 样本量足够大使得 "某个号码从未出现" 的概率极低（<< 10^-300）
        constexpr int sampleSize = 5000;
        QVector<LottoResult> results = engine->generateBatch(sampleSize);

        // 前区统计 (1-35)
        QVector<int> frontCount(36, 0);  // 索引 0 不用
        for (const auto &r : results) {
            for (int n : r.front) {
                frontCount[n]++;
            }
        }
        for (int i = 1; i <= 35; ++i) {
            QVERIFY2(frontCount[i] > 0,
                     QString("前区号码 %1 在 %2 次生成中从未出现")
                         .arg(i).arg(sampleSize).toUtf8());
        }

        // 后区统计 (1-12)
        QVector<int> backCount(13, 0);
        for (const auto &r : results) {
            for (int n : r.back) {
                backCount[n]++;
            }
        }
        for (int i = 1; i <= 12; ++i) {
            QVERIFY2(backCount[i] > 0,
                     QString("后区号码 %1 在 %2 次生成中从未出现")
                         .arg(i).arg(sampleSize).toUtf8());
        }
    }

    void generate_runsWithoutInitTestCase()
    {
        // 验证 LottoEngine 可以在没有 init 准备的情况下工作
        //（测试引擎的无状态特性）
        LottoEngine standaloneEngine;
        LottoResult r = standaloneEngine.generate();
        QCOMPARE(r.front.size(), 5);
        QCOMPARE(r.back.size(), 2);
    }
};

QTEST_GUILESS_MAIN(TestLottoEngine)
#include "tst_lottoengine.moc"
