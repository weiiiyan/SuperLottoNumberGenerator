// LottoInteractor 应用层逻辑单元测试
// 通过注入内存 FakeTicketRepository / FakePresenterPort, 无需 GUI 与真实文件
// 即可验证状态机与"经输出端口推送状态"的编排

#include <QTest>

#include <QDebug>

#include "lottoengine.h"
#include "lottointeractor.h"

#include "testhelpers.h"

class TestLottoInteractor : public QObject
{
    Q_OBJECT

private:
    FakeTicketRepository *m_repo = nullptr;
    FakePresenterPort *m_presenter = nullptr;
    LottoEngine *m_engine = nullptr;
    LottoInteractor *m_controller = nullptr;

    /// 构造一组固定号码, 供确定性断言使用(号码均合法: 前区 1-25, 后区 1-10)
    static LottoTicket makeFixedTicket()
    {
        return makeTicket(LottoTicket::GROUP_COUNT, true,
                          QDateTime::fromString("2026-01-01 12:00:00", "yyyy-MM-dd HH:mm:ss"));
    }

    /// 校验一组号码是否合法(数量/范围/升序)
    static bool isValidGroup(const LottoResult &result)
    {
        if (result.front.size() != LottoResult::FRONT_COUNT
            || result.back.size() != LottoResult::BACK_COUNT)
            return false;
        for (int i = 0; i < result.front.size(); ++i) {
            if (result.front[i] < LottoResult::FRONT_MIN || result.front[i] > LottoResult::FRONT_MAX)
                return false;
            if (i > 0 && result.front[i] <= result.front[i - 1])
                return false;
        }
        for (int i = 0; i < result.back.size(); ++i) {
            if (result.back[i] < LottoResult::BACK_MIN || result.back[i] > LottoResult::BACK_MAX)
                return false;
            if (i > 0 && result.back[i] <= result.back[i - 1])
                return false;
        }
        return true;
    }

private slots:
    void init()
    {
        m_repo = new FakeTicketRepository;
        m_presenter = new FakePresenterPort;
        m_engine = new LottoEngine;
        m_controller = new LottoInteractor(m_repo, m_engine, m_presenter);
    }

    void cleanup()
    {
        delete m_controller;
        delete m_engine;
        delete m_presenter;
        delete m_repo;
    }

    // ── 生成 ──

    void generateNewTicket_shouldProduceValidTicket()
    {
        m_controller->generateNewTicket();

        const LottoTicket ticket = m_controller->currentTicket();
        QCOMPARE(ticket.groups().size(), LottoTicket::GROUP_COUNT);
        for (const LottoResult &result : ticket.groups())
            QVERIFY2(isValidGroup(result), "生成的号码应合法(数量/范围/升序)");
        QVERIFY(ticket.generateTime().isValid());
        QVERIFY(!ticket.isLocked());
        QCOMPARE(m_presenter->presentCount(), 1);   // 经输出端口推送一次
    }

    void generateNewTicket_shouldNotSave()
    {
        m_controller->generateNewTicket();
        QCOMPARE(m_repo->saveCount(), 0);   // 生成本身不落盘, 仅锁定/解锁时保存
    }

    void generateNewTicket_shouldClearPreviousLock()
    {
        m_controller->setLocked(true);
        m_controller->setLocked(false);
        m_controller->generateNewTicket();

        QVERIFY(!m_controller->currentTicket().isLocked());
        QVERIFY(m_controller->currentTicket().generateTime().isValid());
    }

    void generateNewTicket_whenLocked_shouldBeNoop()
    {
        m_controller->generateNewTicket();
        m_controller->setLocked(true);
        const LottoTicket before = m_controller->currentTicket();  // 锁定后快照
        m_presenter->clear();

        m_controller->generateNewTicket();

        QCOMPARE(m_controller->currentTicket(), before);  // 票据未被替换
        QCOMPARE(m_presenter->presentCount(), 0);         // 且未向输出端口推送
    }

    void generateAfterUnlock_shouldWork()
    {
        m_controller->generateNewTicket();
        m_controller->setLocked(true);
        m_controller->setLocked(false);
        m_presenter->clear();

        m_controller->generateNewTicket();

        QCOMPARE(m_presenter->presentCount(), 1);
        QVERIFY(m_controller->currentTicket().generateTime().isValid());
    }

    // ── 锁定 ──

    void setLocked_true_shouldSaveAndPresent()
    {
        m_controller->generateNewTicket();
        m_presenter->clear();

        m_controller->setLocked(true);

        QCOMPARE(m_repo->saveCount(), 1);
        QVERIFY(m_repo->savedTickets.last().isLocked());
        QVERIFY(m_controller->currentTicket().isLocked());
        QCOMPARE(m_presenter->presentCount(), 1);
        QVERIFY(m_presenter->presented.last().isLocked());
    }

    void setLocked_false_shouldSaveUnlocked()
    {
        m_controller->generateNewTicket();
        m_controller->setLocked(true);
        m_presenter->clear();

        m_controller->setLocked(false);

        QCOMPARE(m_repo->saveCount(), 2);
        QVERIFY(!m_repo->savedTickets.last().isLocked());
        QVERIFY(!m_controller->currentTicket().isLocked());
    }

    void setLocked_twiceSameState_shouldSaveOnce()
    {
        m_controller->generateNewTicket();
        m_presenter->clear();

        m_controller->setLocked(true);
        m_controller->setLocked(true);   // 重复同状态: 应被守卫拦截

        QCOMPARE(m_repo->saveCount(), 1);
        QCOMPARE(m_presenter->presentCount(), 1);
    }

    void toggleLock_shouldFlipState()
    {
        m_controller->generateNewTicket();

        m_controller->toggleLock();
        QVERIFY(m_controller->currentTicket().isLocked());

        m_controller->toggleLock();
        QVERIFY(!m_controller->currentTicket().isLocked());

        QCOMPARE(m_repo->saveCount(), 2);
    }

    // ── 恢复 ──

    void load_shouldRestoreFromRepository()
    {
        m_repo->loadResult = makeFixedTicket();
        m_presenter->clear();

        m_controller->load();
        QTest::qWait(10);   // 等待 singleShot(0) 异步触发

        QCOMPARE(m_controller->currentTicket(), makeFixedTicket());
        QCOMPARE(m_presenter->presentCount(), 1);
        QCOMPARE(m_presenter->presented.last(), makeFixedTicket());
    }

    void load_whenRepositoryReturnsUnlocked_shouldGiveEmptyTicket()
    {
        m_repo->loadResult = LottoTicket();   // 默认票据: 空号码
        m_presenter->clear();

        m_controller->load();
        QTest::qWait(10);

        QVERIFY(m_controller->currentTicket().groups().isEmpty());
        QVERIFY(!m_controller->currentTicket().isLocked());
        QCOMPARE(m_presenter->presentCount(), 1);
    }

    // 锁定守卫: 空票据不允许锁定

    void setLocked_whenEmptyTicket_shouldBeNoop()
    {
        // 空票据(无号码)不允许锁定: 用例层守卫, 防止锁定空号码
        m_controller->setLocked(true);

        QVERIFY(!m_controller->currentTicket().isLocked());
        QCOMPARE(m_repo->saveCount(), 0);
        QCOMPARE(m_presenter->presentCount(), 0);
    }

    void load_whenLockedButEmpty_shouldRecoverToUnlocked()
    {
        // 损坏数据(locked + 空号码)在恢复时解除锁定, 避免 UI 死锁
        QVector<LottoResult> groups;
        for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g)
            groups.append(LottoResult());
        m_repo->loadResult = LottoTicket(groups, QDateTime(), true);
        m_presenter->clear();

        m_controller->load();
        QTest::qWait(10);

        QVERIFY(!m_controller->currentTicket().isLocked());
        QCOMPARE(m_presenter->presented.last().isLocked(), false);
    }
};

QTEST_GUILESS_MAIN(TestLottoInteractor)

#include "tst_lottointeractor.moc"
