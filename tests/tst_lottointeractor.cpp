// LottoInteractor 应用层逻辑单元测试
// 通过注入内存 FakeTicketRepository / FakePresenterPort 与固定号码引擎
// FakeLottoEngine, 无需 GUI/真实随机性即可确定性验证状态机与
// "经输出端口推送状态"的编排

#include <QTest>

#include <QDebug>

#include "lottointeractor.h"

#include "testhelpers.h"

class TestLottoInteractor : public QObject
{
    Q_OBJECT

private:
    FakeTicketRepository *m_repo = nullptr;
    FakePresenterPort *m_presenter = nullptr;
    FakeLottoEngine *m_engine = nullptr;
    LottoInteractor *m_controller = nullptr;

    /// 构造确定性固定号码(5 组, 组 g 前区 5g+1..5g+5、后区 2g+1..2g+2)
    static QVector<LottoGroup> fixedGroups()
    {
        return makeTicket(LottoTicket::GROUP_COUNT).groups();
    }

    /// 构造一组固定票据, 供恢复断言使用(号码/时间/锁定已知)
    static LottoTicket makeFixedTicket()
    {
        return makeTicket(LottoTicket::GROUP_COUNT, true,
                          QDateTime::fromString("2026-01-01 12:00:00", "yyyy-MM-dd HH:mm:ss"));
    }

private slots:
    void init()
    {
        m_repo = new FakeTicketRepository;
        m_presenter = new FakePresenterPort;
        m_engine = new FakeLottoEngine(fixedGroups());   // 固定号码, 断言精确化
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

    void generateNewTicket_shouldWriteEngineOutputToTicket()
    {
        m_controller->generateNewTicket();

        const LottoTicket ticket = m_controller->currentTicket();
        QCOMPARE(ticket.groups(), fixedGroups());           // 引擎固定输出原样写入票据
        QVERIFY(ticket.generateTime().isValid());
        QVERIFY(!ticket.isLocked());
        QCOMPARE(m_presenter->presentCount(), 1);           // 经输出端口推送一次
        QCOMPARE(m_presenter->presented.last(), ticket);    // 推送的即当前票据
    }

    void generateNewTicket_shouldNotSave()
    {
        m_controller->generateNewTicket();
        QCOMPARE(m_repo->saveCount(), 0);   // 生成本身不落盘, 仅锁定/解锁时保存
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
        QVector<LottoGroup> groups;
        for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g)
            groups.append(LottoGroup());
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
