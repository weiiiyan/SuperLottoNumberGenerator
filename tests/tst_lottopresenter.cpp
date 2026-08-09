// LottoPresenter 输出侧适配器(展示器)单元测试
// 验证展示推导(号码格式化/占位符/按钮互斥状态/时间显示格式)
// 与输出端口桥接(用例经 LottoPresenterPort.present() 推送 → viewStateChanged 信号)
// 不依赖 GUI, 可在桌面端独立运行

#include <QTest>
#include <QSignalSpy>

#include <QDebug>

#include "lottopresenter.h"
#include "lottoengine.h"
#include "lottointeractor.h"

#include "testhelpers.h"

class TestLottoPresenter : public QObject
{
    Q_OBJECT

private:
    FakeTicketRepository *m_repo = nullptr;
    LottoEngine *m_engine = nullptr;
    LottoInteractor *m_interactor = nullptr;
    LottoPresenter *m_presenter = nullptr;
    QSignalSpy *m_spy = nullptr;

private slots:
    // 桥接测试夹具: 真实交互器注入输出端口 = 展示器
    void init()
    {
        m_repo = new FakeTicketRepository;
        m_engine = new LottoEngine;
        m_presenter = new LottoPresenter;
        m_interactor = new LottoInteractor(m_repo, m_engine, m_presenter);
        m_spy = new QSignalSpy(m_presenter, &LottoPresenter::viewStateChanged);
    }

    void cleanup()
    {
        delete m_spy;
        delete m_interactor;
        delete m_presenter;
        delete m_engine;
        delete m_repo;
    }

    // 输出端口桥接: 用例经 present() 推送 → viewStateChanged 信号

    void present_shouldEmitViewState()
    {
        qDebug() << "验证作为输出端口被调用时重发 viewStateChanged";
        m_presenter->present(makeTicket());

        QCOMPARE(m_spy->count(), 1);
        const LottoViewState state = qvariant_cast<LottoViewState>(m_spy->at(0).at(0));
        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        QVERIFY(state.lockButtonEnabled);          // 含号码后可锁定
        QCOMPARE(state.lockButtonText,
                 QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED));
    }

    void generate_shouldEmitViewState()
    {
        qDebug() << "验证用例生成后经输出端口推送 viewStateChanged";
        m_interactor->generateNewTicket();

        QCOMPARE(m_spy->count(), 1);
        const LottoViewState state = qvariant_cast<LottoViewState>(m_spy->at(0).at(0));
        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        QVERIFY(state.lockButtonEnabled);          // 含号码后可锁定
        QCOMPARE(state.lockButtonText,
                 QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED));
        QVERIFY(!state.timeText.contains(
                    QString::fromUtf8(LottoPresenter::TIME_PLACEHOLDER)));  // 有效时间
    }

    void setLocked_shouldEmitLockedViewState()
    {
        qDebug() << "验证锁定后展示器重发锁定状态";
        m_interactor->generateNewTicket();
        m_spy->clear();

        m_interactor->setLocked(true);

        QCOMPARE(m_spy->count(), 1);
        const LottoViewState state = qvariant_cast<LottoViewState>(m_spy->at(0).at(0));
        QVERIFY(state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_LOCKED));
        QVERIFY(!state.generateEnabled);
    }
    // ── 空票据(初始状态) ──

    void emptyTicket_shouldUsePlaceholders()
    {
        qDebug() << "验证空票据全为占位符 + 按钮互斥初始状态";
        const LottoViewState state = LottoPresenter::buildViewState(LottoTicket());

        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        for (const QStringList &row : state.groups) {
            QCOMPARE(row.size(), LottoResult::FRONT_COUNT + LottoResult::BACK_COUNT);
            for (const QString &text : row)
                QCOMPARE(text, QString::fromUtf8(LottoPresenter::NUMBER_PLACEHOLDER));
        }
        QCOMPARE(state.timeText, QString::fromUtf8(LottoPresenter::TIME_PREFIX)
                                     + QString::fromUtf8(LottoPresenter::TIME_PLACEHOLDER));
        QVERIFY(!state.lockButtonEnabled);
        QVERIFY(!state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED));
        QVERIFY(state.generateEnabled);
    }

    // ── 号码格式化 ──

    void numbers_shouldBeZeroPadded()
    {
        qDebug() << "验证号码零填充两位(如 3 → '03')";
        const LottoViewState state = LottoPresenter::buildViewState(makeTicket());

        // 组 0 前区: 1-5; 后区: 1-2 → 均应零填充
        const QStringList &row0 = state.groups.at(0);
        for (int i = 0; i < LottoResult::FRONT_COUNT; ++i)
            QCOMPARE(row0.at(i), LottoPresenter::formatNumber(i + 1));
        for (int i = 0; i < LottoResult::BACK_COUNT; ++i)
            QCOMPARE(row0.at(LottoResult::FRONT_COUNT + i), LottoPresenter::formatNumber(i + 1));
    }

    void partialGroup_shouldFillMissingWithPlaceholder()
    {
        qDebug() << "验证缺项号码显示占位符(容错不完整票据)";
        QVector<LottoResult> groups(1, LottoResult());   // 1 组, 号码全空
        const LottoViewState state = LottoPresenter::buildViewState(
            LottoTicket(groups, QDateTime(), false));

        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);   // 始终 5 行
        for (const QString &text : state.groups.at(0)) {
            QCOMPARE(text, QString::fromUtf8(LottoPresenter::NUMBER_PLACEHOLDER));
        }
    }

    // ── 按钮互斥状态与文字 ──

    void unlocked_shouldEnableGenerateAndAllowLock()
    {
        qDebug() << "验证未锁定票据: 锁定按钮可用, 生成可用";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(1, false, QDateTime()));

        QVERIFY(state.lockButtonEnabled);
        QVERIFY(!state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED));
        QVERIFY(state.generateEnabled);
    }

    void locked_shouldDisableGenerateAndShowUnlockText()
    {
        qDebug() << "验证锁定票据: 锁定按钮选中, 生成禁用";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(1, true, QDateTime()));

        QVERIFY(state.lockButtonEnabled);
        QVERIFY(state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_LOCKED));
        QVERIFY(!state.generateEnabled);
    }

    // ── 时间显示 ──

    void validTime_shouldFormatDisplayText()
    {
        qDebug() << "验证有效时间按固定格式显示";
        const QDateTime time =
            QDateTime::fromString("2026-01-01 12:00:00",
                                  QString::fromUtf8(LottoPresenter::TIME_FORMAT));
        const LottoViewState state = LottoPresenter::buildViewState(makeTicket(1, false, time));

        QCOMPARE(state.timeText, QString::fromUtf8(LottoPresenter::TIME_PREFIX)
                                     + QString::fromLatin1("2026-01-01 12:00:00"));
    }

    void invalidTime_shouldRenderTimePlaceholder()
    {
        qDebug() << "验证无效时间显示占位文本";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(1, false, QDateTime()));

        QCOMPARE(state.timeText, QString::fromUtf8(LottoPresenter::TIME_PREFIX)
                                     + QString::fromUtf8(LottoPresenter::TIME_PLACEHOLDER));
    }
};

QTEST_GUILESS_MAIN(TestLottoPresenter)

#include "tst_lottopresenter.moc"
