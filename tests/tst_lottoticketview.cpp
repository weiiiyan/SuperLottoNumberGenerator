// LottoTicketView 票据视图单元测试
// 覆盖: 状态渲染到控件、按钮发请求信号、风格菜单列出宿主风格并请求切换

#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "lottolayoutmetrics.h"
#include "lottostylecatalog.h"
#include "lottoticketdecor.h"
#include "ticket/lottoticketview.h"
#include "lottopresenter.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestLottoTicketView : public QObject
{
    Q_OBJECT

private slots:
    void firstFrameShowsPlaceholdersWithoutAnyRenderCall()
    {
        qDebug() << "验证票据视图未收到状态时不自我渲染(首帧由宿主注入)";
        FakeStyleHost host;
        LottoTicketView view(&host);
        // 构造期不再自渲染: 号码标签为空, 不存在展示器推导
        QCOMPARE(view.findChild<QLabel *>("frontLabel_0_0")->text(), QString());
    }

    void rendersStateIntoWidgets()
    {
        qDebug() << "验证展示状态写入号码/时间控件";
        FakeStyleHost host;
        LottoTicketView view(&host);
        view.renderState(LottoPresenter::buildViewState(makeTicket()));

        auto *front = view.findChild<QLabel *>("frontLabel_0_0");
        QVERIFY(front != nullptr);
        QCOMPARE(front->text(), QStringLiteral("01"));

        auto *timeValue = view.findChild<QLabel *>("timeValueLabel");
        QVERIFY(timeValue != nullptr);
        QVERIFY(!timeValue->text().isEmpty());

        auto *timeLabel = view.findChild<QLabel *>("timeLabel");
        QVERIFY(timeLabel != nullptr);
        QCOMPARE(timeLabel->text(), QString::fromUtf8(LottoViewText::TIME_LABEL));
    }

    void buttonsEmitRequestSignals()
    {
        qDebug() << "验证按钮点击发出请求信号";
        FakeStyleHost host;
        LottoTicketView view(&host);
        // 空票据时锁定按钮禁用, 先渲染含号码的状态使其可用
        view.renderState(LottoPresenter::buildViewState(makeTicket()));
        QSignalSpy gen(&view, &LottoView::generateRequested);
        QSignalSpy lock(&view, &LottoView::lockRequested);

        auto *generate = view.findChild<QPushButton *>("btnGenerate");
        auto *lockBtn = view.findChild<QPushButton *>("btnLock");
        QVERIFY(generate != nullptr);
        QVERIFY(lockBtn != nullptr);
        QTest::mouseClick(generate, Qt::LeftButton);
        QTest::mouseClick(lockBtn, Qt::LeftButton);

        QCOMPARE(gen.count(), 1);
        QCOMPARE(lock.count(), 1);
    }

    void styleButtonOpensDrawerAndRequests()
    {
        qDebug() << "验证右上角按钮展开抽屉, 选择选项后发出切换请求";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
                        {QStringLiteral("neo"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("ticket");
        LottoTicketView view(&host);
        view.resize(390, 844);

        auto *btn = view.findChild<QPushButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QCOMPARE(btn->text(), QStringLiteral("票据"));

        QTest::mouseClick(btn, Qt::LeftButton);

        auto *drawer = view.findChild<QWidget *>("styleDrawer");
        QVERIFY(drawer != nullptr);
        QVERIFY2(!drawer->isHidden(), "抽屉应已展开");

        auto *option = drawer->findChild<QPushButton *>("styleOption_1");
        QVERIFY(option != nullptr);
        QTest::mouseClick(option, Qt::LeftButton);
        QCOMPARE(host.requested, QStringList{ QStringLiteral("neo") });
    }

    void choosingStyleHidesDrawerImmediately()
    {
        qDebug() << "验证选择风格后抽屉立即收起(切回本视图时不残留)";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
                        {QStringLiteral("neo"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("ticket");
        LottoTicketView view(&host);
        view.resize(390, 844);

        auto *btn = view.findChild<QPushButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QTest::mouseClick(btn, Qt::LeftButton);

        auto *drawer = view.findChild<QWidget *>("styleDrawer");
        QVERIFY(drawer != nullptr);
        QVERIFY(!drawer->isHidden());

        auto *option = drawer->findChild<QPushButton *>("styleOption_1");
        QVERIFY(option != nullptr);
        QTest::mouseClick(option, Qt::LeftButton);

        QVERIFY2(drawer->isHidden(), "选择风格后抽屉应立即隐藏");
        QCOMPARE(host.requested, QStringList{ QStringLiteral("neo") });
    }

    void styleChangedUpdatesButtonText()
    {
        qDebug() << "验证 styleChanged 更新风格按钮文案";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
                        {QStringLiteral("neo"), QStringLiteral("新拟物")} };
        LottoTicketView view(&host);
        view.renderState(LottoViewState());
        view.styleChanged(QStringLiteral("neo"));

        auto *btn = view.findChild<QPushButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QCOMPARE(btn->text(), QStringLiteral("新拟物"));
    }

    void rendersSerialAndStubTexts()
    {
        qDebug() << "验证票号/注数/印章文案写入票根控件";
        FakeStyleHost host;
        LottoTicketView view(&host);
        view.renderState(LottoPresenter::buildViewState(makeTicket()));

        auto *serial = view.findChild<QLabel *>("serialLabel");
        QVERIFY(serial != nullptr);
        QVERIFY(serial->text().startsWith(QString::fromUtf8(LottoViewText::SERIAL_PREFIX)));

        auto *hint = view.findChild<QLabel *>("stubHint");
        QVERIFY(hint != nullptr);
        QCOMPARE(hint->text(), QString::fromUtf8(LottoViewText::STUB_HINT));

        auto *count = view.findChild<QLabel *>("stubCount");
        QVERIFY(count != nullptr);
        QCOMPARE(count->textFormat(), Qt::RichText);
        QVERIFY2(count->text().contains(QStringLiteral(">5<")),
                 "注数数字应被强调");
    }

    void actionButtonsMatchActionHeight()
    {
        qDebug() << "验证操作区按钮高度与设计令牌一致(56)";
        FakeStyleHost host;
        LottoTicketView view(&host);
        view.resize(390, 844);
        QTest::qWait(80);   // 等 50ms 布局防抖

        auto *lockBtn = view.findChild<QPushButton *>("btnLock");
        auto *generate = view.findChild<QPushButton *>("btnGenerate");
        QVERIFY(lockBtn != nullptr);
        QVERIFY(generate != nullptr);
        QCOMPARE(lockBtn->height(), static_cast<int>(LottoLayout::ActionHeight));
        QCOMPARE(generate->height(), static_cast<int>(LottoLayout::ActionHeight));
    }

    void styleButtonShowsFourSquareGlyph()
    {
        qDebug() << "验证风格按钮带四矩形自绘图标";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")} };
        host.currentId = QStringLiteral("ticket");
        LottoTicketView view(&host);

        auto *btn = view.findChild<QPushButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QVERIFY2(!btn->icon().isNull(), "风格按钮缺少四矩形图标");
        QCOMPARE(btn->iconSize(), QSize(13, 13));

        const QImage icon = btn->icon().pixmap(QSize(13, 13)).toImage();
        const qreal dpr = icon.devicePixelRatio();
        auto alphaAt = [&icon, dpr](qreal x, qreal y) {
            return icon.pixelColor(qRound(x * dpr), qRound(y * dpr)).alpha();
        };
        // 四块中心不透明, 中间十字缝隙透明
        QVERIFY(alphaAt(3, 3) > 0);
        QVERIFY(alphaAt(10, 3) > 0);
        QVERIFY(alphaAt(3, 10) > 0);
        QVERIFY(alphaAt(10, 10) > 0);
        QCOMPARE(alphaAt(6, 6), 0);
    }

    void perfNotchesHaveInsetShadow()
    {
        qDebug() << "验证骑缝线两端半圆缺口带内阴影";
        LottoPerfLine perf;
        perf.resize(200, 26);

        QImage image(perf.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        perf.render(&painter);
        painter.end();

        const QColor top = image.pixelColor(2, 6);
        const QColor bottom = image.pixelColor(2, 20);
        QVERIFY(top.alpha() > 0);
        QVERIFY(bottom.alpha() > 0);
        QVERIFY2(qGray(top.rgb()) < qGray(bottom.rgb()),
                 "缺口上缘应比下缘更暗(inset 阴影)");
    }

    void stubRowGeometryStableAfterLock()
    {
        qDebug() << "验证锁定后票根行几何不变(印章绝对定位)";
        FakeStyleHost host;
        LottoTicketView view(&host);
        view.resize(390, 844);
        view.show();
        QTest::qWait(80);

        auto *hint = view.findChild<QLabel *>("stubHint");
        auto *serial = view.findChild<QLabel *>("serialLabel");
        auto *count = view.findChild<QLabel *>("stubCount");
        auto *stamp = view.findChild<QWidget *>("stampLabel");
        auto *stub = view.findChild<QWidget *>("stub");
        QVERIFY(hint != nullptr);
        QVERIFY(serial != nullptr);
        QVERIFY(count != nullptr);
        QVERIFY(stamp != nullptr);
        QVERIFY(stub != nullptr);
        QLayout *stubLayout = stub->layout();
        QVERIFY(stubLayout != nullptr);
        QLayoutItem *bottomRow = stubLayout->itemAt(stubLayout->count() - 1);
        QVERIFY(bottomRow != nullptr);

        view.renderState(LottoPresenter::buildViewState(makeTicket()));
        QTest::qWait(20);
        const QRect hintBefore = hint->geometry();
        const QRect serialBefore = serial->geometry();
        const QRect countBefore = count->geometry();
        const QRect rowBefore = bottomRow->geometry();

        view.renderState(LottoPresenter::buildViewState(
            makeTicket(LottoTicket::GROUP_COUNT, true)));
        QTest::qWait(20);

        QCOMPARE(hint->geometry(), hintBefore);
        QCOMPARE(serial->geometry(), serialBefore);
        QCOMPARE(count->geometry(), countBefore);
        QCOMPARE(bottomRow->geometry(), rowBefore);
        QVERIFY2(!stamp->isHidden(), "锁定时印章应显示");
    }

    void ticketViewCanBeRegisteredInACatalog()
    {
        qDebug() << "验证票据视图可注册进风格目录";
        LottoStyleDefinition definition{
            {QStringLiteral("ticket"), QStringLiteral("票据")},
            [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
                return new LottoTicketView(host, parent);
            }};
        QCOMPARE(definition.info.id, QStringLiteral("ticket"));
        QVERIFY(static_cast<bool>(definition.create));
    }
};

QTEST_MAIN(TestLottoTicketView)
#include "tst_lottoticketview.moc"
