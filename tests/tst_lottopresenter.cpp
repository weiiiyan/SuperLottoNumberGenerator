// LottoPresenter 输出侧适配器(展示器)单元测试
// 验证展示推导(号码格式化/占位符/按钮互斥状态/时间显示格式)
// 与输出端口桥接(present() → viewStateChanged 信号)
// 不依赖 GUI 与用例/引擎实现, 可独立运行

#include <QTest>
#include <QSignalSpy>

#include <QDebug>

#include "lottopresenter.h"
#include "lottoviewstate.h"

#include "testhelpers.h"

class TestLottoPresenter : public QObject
{
    Q_OBJECT

private:
    LottoPresenter *m_presenter = nullptr;
    QSignalSpy *m_spy = nullptr;

private slots:
    void init()
    {
        m_presenter = new LottoPresenter;
        m_spy = new QSignalSpy(m_presenter, &LottoPresenter::viewStateChanged);
    }

    void cleanup()
    {
        delete m_spy;
        delete m_presenter;
    }

    // 输出端口桥接: 用例经 present() 推送 → viewStateChanged 信号

    void present_shouldEmitViewState()
    {
        qDebug() << "验证作为输出端口被调用时重发 viewStateChanged";
        m_presenter->present(makeTicket(LottoTicket::GROUP_COUNT, false,
                                        QDateTime(QDate(2026, 1, 1), QTime(12, 0, 0))));

        QCOMPARE(m_spy->count(), 1);
        const LottoViewState state = qvariant_cast<LottoViewState>(m_spy->at(0).at(0));
        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        QVERIFY(state.lockButtonEnabled);          // 含号码后可锁定
        QCOMPARE(state.lockButtonText,
                 QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED));
        QCOMPARE(state.timeLabelText,
                 QString::fromUtf8(LottoViewText::TIME_LABEL));
        QVERIFY(!state.timeValueText.contains(
                    QString::fromUtf8(LottoViewText::TIME_PLACEHOLDER)));  // 有效时间
    }

    void presentLockedTicket_shouldEmitLockedViewState()
    {
        qDebug() << "验证 present(锁定票据) 重发锁定状态";
        m_presenter->present(makeTicket(LottoTicket::GROUP_COUNT, true, QDateTime()));

        QCOMPARE(m_spy->count(), 1);
        const LottoViewState state = qvariant_cast<LottoViewState>(m_spy->at(0).at(0));
        QVERIFY(state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED));
        QVERIFY(!state.generateEnabled);
    }
    // ── 空票据(初始状态) ──

    void emptyTicket_shouldUsePlaceholders()
    {
        qDebug() << "验证空票据全为占位符 + 按钮互斥初始状态";
        const LottoViewState state = LottoPresenter::buildViewState(LottoTicket());

        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        for (const QStringList &row : state.groups) {
            QCOMPARE(row.size(), LottoGroup::FRONT_COUNT + LottoGroup::BACK_COUNT);
            for (const QString &text : row)
                QCOMPARE(text, QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));
        }
        QCOMPARE(state.timeLabelText, QString::fromUtf8(LottoViewText::TIME_LABEL));
        QCOMPARE(state.timeValueText,
                 QString::fromUtf8(LottoViewText::TIME_PLACEHOLDER));
        QVERIFY(!state.lockButtonEnabled);
        QVERIFY(!state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED));
        QVERIFY(state.generateEnabled);
    }

    // ── 号码格式化 ──

    void numbers_shouldBeZeroPadded()
    {
        qDebug() << "验证号码零填充两位(如 3 → '03')";
        const LottoViewState state = LottoPresenter::buildViewState(makeTicket());

        // 组 0 前区为 1-5、后区为 1-2, 断言字面值(不用被测 formatNumber 作期望, 避免循环断言)
        const QStringList &row0 = state.groups.at(0);
        QCOMPARE(row0.at(0), QStringLiteral("01"));
        QCOMPARE(row0.at(1), QStringLiteral("02"));
        QCOMPARE(row0.at(2), QStringLiteral("03"));
        QCOMPARE(row0.at(3), QStringLiteral("04"));
        QCOMPARE(row0.at(4), QStringLiteral("05"));
        QCOMPARE(row0.at(LottoGroup::FRONT_COUNT + 0), QStringLiteral("01"));
        QCOMPARE(row0.at(LottoGroup::FRONT_COUNT + 1), QStringLiteral("02"));
    }

    void partialGroup_shouldFillMissingWithPlaceholder()
    {
        qDebug() << "验证缺项号码显示占位符(容错不完整票据)";
        QVector<LottoGroup> groups(1, LottoGroup());   // 1 组, 号码全空
        const LottoViewState state = LottoPresenter::buildViewState(
            LottoTicket(groups, QDateTime(), false));

        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);   // 始终 5 行
        for (const QString &text : state.groups.at(0)) {
            QCOMPARE(text, QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));
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
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED));
        QVERIFY(state.generateEnabled);
    }

    void locked_shouldDisableGenerateAndShowUnlockText()
    {
        qDebug() << "验证锁定票据: 锁定按钮选中, 生成禁用";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(1, true, QDateTime()));

        QVERIFY(state.lockButtonEnabled);
        QVERIFY(state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED));
        QVERIFY(!state.generateEnabled);
    }

    // ── 时间显示 ──

    void validTime_shouldFormatDisplayText()
    {
        qDebug() << "验证有效时间按固定格式显示";
        const QDateTime time =
            QDateTime::fromString("2026-01-01 12:00:00",
                                  QString::fromUtf8(LottoViewText::TIME_FORMAT));
        const LottoViewState state = LottoPresenter::buildViewState(makeTicket(1, false, time));

        QCOMPARE(state.timeLabelText, QString::fromUtf8(LottoViewText::TIME_LABEL));
        QCOMPARE(state.timeValueText, QString::fromLatin1("2026-01-01 12:00:00"));
    }

    // ── 文案契约: 无 emoji, 生成按钮文案随状态下发 ──

    void constantsLiveInTheStateHeader()
    {
        qDebug() << "验证展示常量与结构常量归属 lottoviewstate.h";
        QCOMPARE(QString::fromUtf8(LottoViewText::TIME_LABEL),
                 QString::fromUtf8("生成时间："));
        QCOMPARE(QString::fromUtf8(LottoViewText::SEPARATOR_TEXT),
                 QStringLiteral("+"));
        QCOMPARE(QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER),
                 QStringLiteral("?"));
        QCOMPARE(LottoViewShape::GROUP_COUNT, LottoTicket::GROUP_COUNT);
        QCOMPARE(LottoViewShape::FRONT_COUNT, LottoGroup::FRONT_COUNT);
        QCOMPARE(LottoViewShape::BACK_COUNT, LottoGroup::BACK_COUNT);
    }

    void constants_shouldNotContainEmoji()
    {
        const QVector<QString> texts = {
            QString::fromUtf8(LottoViewText::TIME_LABEL),
            QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED),
            QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED),
            QString::fromUtf8(LottoViewText::GENERATE_TEXT),
        };
        // 只拒绝真正的图标区段(杂项符号/装饰符/emoji), 中文全角标点不在其列
        auto isIconChar = [](uint ucs4) {
            return (ucs4 >= 0x1F000 && ucs4 <= 0x1FAFF)   // emoji / 符号图形
                || (ucs4 >= 0x2600 && ucs4 <= 0x27BF)     // 杂项符号 / 装饰符
                || (ucs4 >= 0x2B00 && ucs4 <= 0x2BFF)     // 杂项符号与箭头
                || ucs4 == 0xFE0F;                        // 变体选择符
        };
        for (const QString &text : texts) {
            const QVector<uint> cps = text.toUcs4();
            for (const uint ucs4 : cps)
                QVERIFY2(!isIconChar(ucs4),
                         QString("文案含图标字符: '%1'").arg(text).toUtf8());
        }
    }

    void state_shouldCarryGenerateButtonText()
    {
        const LottoViewState state = LottoPresenter::initialState();
        QCOMPARE(state.generateButtonText,
                 QString::fromUtf8(LottoViewText::GENERATE_TEXT));
    }

    void invalidTime_shouldRenderTimePlaceholder()
    {
        qDebug() << "验证无效时间显示占位文本";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(1, false, QDateTime()));

        QCOMPARE(state.timeLabelText, QString::fromUtf8(LottoViewText::TIME_LABEL));
        QCOMPARE(state.timeValueText,
                 QString::fromUtf8(LottoViewText::TIME_PLACEHOLDER));
    }

    // 票根: 票号/提示/注数/印章

    void emptyTicket_shouldShowSerialPlaceholder()
    {
        qDebug() << "验证空票据的票号为占位文本";
        const LottoViewState state = LottoPresenter::buildViewState(LottoTicket());
        QCOMPARE(state.serialText, QString::fromUtf8(LottoViewText::SERIAL_PLACEHOLDER));
    }

    void serial_shouldBeDeterministicFromGenerateTime()
    {
        qDebug() << "验证票号由生成时间确定性推导, 重绘不跳变";
        const QDateTime time(QDate(2026, 11, 20), QTime(20, 46, 7));
        const LottoTicket ticket = makeTicket(LottoTicket::GROUP_COUNT, false, time);

        const LottoViewState first = LottoPresenter::buildViewState(ticket);
        const LottoViewState second = LottoPresenter::buildViewState(ticket);
        QCOMPARE(first.serialText, second.serialText);

        QCOMPARE(first.serialText.left(11), QStringLiteral("NO. 261120 "));
        QCOMPARE(first.serialText.size(), 15);
        const QString tail = first.serialText.mid(11);
        QCOMPARE(tail.size(), 4);
        for (QChar c : tail)
            QVERIFY(c.isDigit());
    }

    void stubTexts_shouldFollowConstants()
    {
        qDebug() << "验证票根文案来自统一常量";
        const LottoViewState state =
            LottoPresenter::buildViewState(makeTicket(LottoTicket::GROUP_COUNT, false,
                                                      QDateTime(QDate(2026, 11, 20),
                                                                QTime(20, 46, 7))));
        QCOMPARE(state.appTitleText, QString::fromUtf8(LottoViewText::APP_TITLE));
        QCOMPARE(state.stubHintText, QString::fromUtf8(LottoViewText::STUB_HINT));
        QCOMPARE(state.stubCountText,
                 QString::fromUtf8(LottoViewText::STUB_COUNT_FORMAT)
                     .arg(LottoTicket::GROUP_COUNT));
        QCOMPARE(state.stubCountValueText, QString::number(LottoTicket::GROUP_COUNT));
        QCOMPARE(state.stampText, QString::fromUtf8(LottoViewText::STAMP_TEXT));
    }

    void headerMeta_shouldAppendLockSuffixWhenLocked()
    {
        qDebug() << "验证锁定后顶栏副标题追加状态后缀";
        const QDateTime time(QDate(2026, 11, 20), QTime(20, 46, 7));
        const QString suffix = QString::fromUtf8(LottoViewText::HEADER_META_LOCKED).trimmed();

        const LottoViewState unlocked =
            LottoPresenter::buildViewState(makeTicket(LottoTicket::GROUP_COUNT, false, time));
        const LottoViewState locked =
            LottoPresenter::buildViewState(makeTicket(LottoTicket::GROUP_COUNT, true, time));

        QVERIFY(!unlocked.headerMetaText.contains(suffix));
        QVERIFY(locked.headerMetaText.contains(suffix));
    }
};

QTEST_GUILESS_MAIN(TestLottoPresenter)

#include "tst_lottopresenter.moc"
