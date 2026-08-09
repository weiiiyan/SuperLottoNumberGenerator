// LottoController 输入侧适配器单元测试
// 验证"外部输入(点击) → 用例调用"的翻译
// 注入记录调用的 FakeInputBoundary 替身, 与真实用例/引擎解耦,
// 使测试只依赖输入边界契约, 不依赖交互器的实现行为
// (锁定时拒绝生成等应用规则由 LottoInteractorTest 覆盖, 此处不重复)

#include <QTest>

#include "lottocontroller.h"

#include "testhelpers.h"

class TestLottoController : public QObject
{
    Q_OBJECT

private:
    FakeInputBoundary *m_boundary = nullptr;
    LottoController *m_controller = nullptr;

private slots:
    void init()
    {
        m_boundary = new FakeInputBoundary;
        m_controller = new LottoController(m_boundary);
    }

    void cleanup()
    {
        delete m_controller;
        delete m_boundary;
    }

    void onGenerateRequested_shouldInvokeGenerateUseCase()
    {
        m_controller->onGenerateRequested();

        QCOMPARE(m_boundary->generateCalls, 1);
        QCOMPARE(m_boundary->toggleLockCalls, 0);   // 仅生成被调用
    }

    void onLockRequested_shouldInvokeToggleLockUseCase()
    {
        m_controller->onLockRequested();

        QCOMPARE(m_boundary->toggleLockCalls, 1);
        QCOMPARE(m_boundary->generateCalls, 0);     // 仅切换被调用
    }
};

QTEST_GUILESS_MAIN(TestLottoController)

#include "tst_lottocontroller.moc"
