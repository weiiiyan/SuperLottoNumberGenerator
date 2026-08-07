// LottoPresenter 展示器单元测试
// 验证展示推导(号码格式化/占位符/按钮互斥状态/时间显示格式)
// 不依赖 GUI, 可在桌面端独立运行

#include <QTest>

#include <QDebug>

#include "lottopresenter.h"

class TestLottoPresenter : public QObject
{
    Q_OBJECT

private:
    /// 构造指定组数的票据(每组号码依次递增, 供确定性断言)
    static LottoTicket makeTicket(int groupCount = LottoTicket::GROUP_COUNT,
                                  bool locked = false,
                                  const QDateTime &time = QDateTime())
    {
        QVector<LottoGroup> groups;
        for (int g = 0; g < groupCount; ++g) {
            QVector<int> front, back;
            for (int i = 0; i < LottoGroup::FRONT_COUNT; ++i)
                front << g * LottoGroup::FRONT_COUNT + i + 1;
            for (int i = 0; i < LottoGroup::BACK_COUNT; ++i)
                back << g * LottoGroup::BACK_COUNT + i + 1;
            groups.append(LottoGroup(front, back));
        }
        return LottoTicket(groups, time, locked);
    }

private slots:
    // ── 空票据(初始状态) ──

    void emptyTicket_shouldUsePlaceholders()
    {
        qDebug() << "验证空票据全为占位符 + 按钮互斥初始状态";
        const LottoViewState state = LottoPresenter::present(LottoTicket());

        QCOMPARE(state.groups.size(), LottoTicket::GROUP_COUNT);
        for (const QStringList &row : state.groups) {
            QCOMPARE(row.size(), LottoGroup::FRONT_COUNT + LottoGroup::BACK_COUNT);
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
        const LottoViewState state = LottoPresenter::present(makeTicket());

        // 组 0 前区: 1-5; 后区: 1-2 → 均应零填充
        const QStringList &row0 = state.groups.at(0);
        for (int i = 0; i < LottoGroup::FRONT_COUNT; ++i)
            QCOMPARE(row0.at(i), QString::number(i + 1).rightJustified(2, '0'));
        for (int i = 0; i < LottoGroup::BACK_COUNT; ++i)
            QCOMPARE(row0.at(LottoGroup::FRONT_COUNT + i),
                     QString::number(i + 1).rightJustified(2, '0'));
    }

    void partialGroup_shouldFillMissingWithPlaceholder()
    {
        qDebug() << "验证缺项号码显示占位符(容错不完整票据)";
        QVector<LottoGroup> groups(1, LottoGroup());   // 1 组, 号码全空
        const LottoViewState state = LottoPresenter::present(
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
        const LottoViewState state = LottoPresenter::present(makeTicket(1, false, QDateTime()));

        QVERIFY(state.lockButtonEnabled);
        QVERIFY(!state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_UNLOCKED));
        QVERIFY(state.generateEnabled);
    }

    void locked_shouldDisableGenerateAndShowUnlockText()
    {
        qDebug() << "验证锁定票据: 锁定按钮选中, 生成禁用";
        const LottoViewState state = LottoPresenter::present(makeTicket(1, true, QDateTime()));

        QVERIFY(state.lockButtonEnabled);
        QVERIFY(state.lockButtonChecked);
        QCOMPARE(state.lockButtonText, QString::fromUtf8(LottoPresenter::LOCK_TEXT_LOCKED));
        QVERIFY(!state.generateEnabled);
    }

    // ── 时间显示 ──

    void validTime_shouldFormatDisplayText()
    {
        qDebug() << "验证有效时间按固定格式显示";
        const QDateTime time = QDateTime::fromString("2026-01-01 12:00:00",
                                                     QString::fromUtf8(LottoPresenter::TIME_FORMAT));
        const LottoViewState state = LottoPresenter::present(makeTicket(1, false, time));

        QCOMPARE(state.timeText, QString::fromUtf8(LottoPresenter::TIME_PREFIX)
                                     + QString::fromLatin1("2026-01-01 12:00:00"));
    }

    void invalidTime_shouldRenderTimePlaceholder()
    {
        qDebug() << "验证无效时间显示占位文本";
        const LottoViewState state = LottoPresenter::present(makeTicket(1, false, QDateTime()));

        QCOMPARE(state.timeText, QString::fromUtf8(LottoPresenter::TIME_PREFIX)
                                     + QString::fromUtf8(LottoPresenter::TIME_PLACEHOLDER));
    }
};

QTEST_GUILESS_MAIN(TestLottoPresenter)

#include "tst_lottopresenter.moc"
