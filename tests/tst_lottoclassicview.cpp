// LottoClassicView 经典视图单元测试
// 覆盖: 状态渲染到控件、按钮发请求信号、风格菜单列出宿主风格并请求切换

#include <QLabel>
#include <QResource>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QToolButton>

#include "classic/lottoclassicview.h"
#include "lottostylecatalog.h"
#include "lottopresenter.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestLottoClassicView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // 与生产入口一致: 静态库 qrc 资源需显式初始化
        Q_INIT_RESOURCE(resources);
    }

    void rendersStateIntoWidgets()
    {
        qDebug() << "验证展示状态写入号码/时间控件";
        FakeStyleHost host;
        LottoClassicView view(&host);
        view.renderState(LottoPresenter::buildViewState(makeTicket()));

        auto *front = view.findChild<QLabel *>("frontLabel_0_0");
        QVERIFY(front != nullptr);
        QCOMPARE(front->text(), QStringLiteral("01"));

        auto *timeValue = view.findChild<QLabel *>("timeValueLabel");
        QVERIFY(timeValue != nullptr);
        QVERIFY(!timeValue->text().isEmpty());

        auto *timeLabel = view.findChild<QLabel *>("timeLabel");
        QVERIFY(timeLabel != nullptr);
        QCOMPARE(timeLabel->text(), QString::fromUtf8(LottoPresenter::TIME_LABEL));
    }

    void buttonsEmitRequestSignals()
    {
        qDebug() << "验证按钮点击发出请求信号";
        FakeStyleHost host;
        LottoClassicView view(&host);
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

    void styleMenuListsHostStylesAndRequests()
    {
        qDebug() << "验证风格菜单列出宿主全部风格并发出切换请求";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                        {QStringLiteral("neo"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("classic");
        LottoClassicView view(&host);
        view.styleChanged(QStringLiteral("classic"));

        auto *btn = view.findChild<QToolButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QVERIFY(btn->menu() != nullptr);
        QCOMPARE(btn->menu()->actions().size(), 2);
        QVERIFY(btn->menu()->actions().at(0)->isChecked());

        btn->menu()->actions().at(1)->trigger();
        QCOMPARE(host.requested, QStringList{ QStringLiteral("neo") });
    }

    void styleChangedUpdatesCheckedAction()
    {
        qDebug() << "验证 styleChanged 更新菜单勾选态";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                        {QStringLiteral("neo"), QStringLiteral("新拟物")} };
        LottoClassicView view(&host);
        view.renderState(LottoPresenter::initialState());
        view.styleChanged(QStringLiteral("neo"));

        auto *menu = view.findChild<QToolButton *>("btnStyleSwitch")->menu();
        QVERIFY(menu->actions().at(1)->isChecked());
        QVERIFY(!menu->actions().at(0)->isChecked());
    }

    void catalogShouldContainClassic()
    {
        qDebug() << "验证风格目录首项为经典视图";
        const auto catalog = lottoStyleCatalog();
        QVERIFY(!catalog.isEmpty());
        QCOMPARE(catalog.first().info.id, QStringLiteral("classic"));
    }
};

QTEST_MAIN(TestLottoClassicView)
#include "tst_lottoclassicview.moc"
