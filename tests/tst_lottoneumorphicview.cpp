// LottoNeumorphicView 新拟物视图测试
// 需要 QQuickWidget; 平台 Quick 不可用时跳过
// 运行方式: LottoNeumorphicViewTest.exe -platform offscreen

#include <QQuickItem>
#include <QQuickWidget>
#include <QSignalSpy>
#include <QTest>

#include "lottopresenter.h"
#include "neumorphic/lottoneumorphicview.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestLottoNeumorphicView : public QObject
{
    Q_OBJECT

private:
    static bool waitReady(LottoNeumorphicView &view)
    {
        if (!QTest::qWaitFor([&view]() { return view.isReady(); }, 8000))
            return false;
        // 等根对象就绪
        return QTest::qWaitFor([&view]() { return view.rootItem() != nullptr; }, 8000);
    }

private slots:
    void initTestCase() { qputenv("QT_QUICK_BACKEND", "software"); }

    void becomesReady()
    {
        qDebug() << "验证 QML 场景加载完成";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QVERIFY(view.rootItem() != nullptr);
        QCOMPARE(view.styleId(), QStringLiteral("neumorphic"));
    }

    void renderStatePushesProperties()
    {
        qDebug() << "验证展示状态写入 QML 根对象属性";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        view.renderState(LottoPresenter::buildViewState(makeTicket()));

        QQuickItem *root = view.rootItem();
        QTRY_COMPARE(root->property("generateButtonText").toString(),
                     QString::fromUtf8(LottoViewText::GENERATE_TEXT));
        QCOMPARE(root->property("timeLabelText").toString(),
                 QString::fromUtf8(LottoViewText::TIME_LABEL));
        const QVariantList groups = root->property("groups").toList();
        QCOMPARE(groups.size(), LottoTicket::GROUP_COUNT);
        QCOMPARE(groups.at(0).toStringList().size(),
                 LottoGroup::FRONT_COUNT + LottoGroup::BACK_COUNT);
        QCOMPARE(groups.at(0).toStringList().at(0), QStringLiteral("01"));
    }

    void qmlSignalsForwardToView()
    {
        qDebug() << "验证 QML 交互信号回传为视图信号";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        QSignalSpy gen(&view, &LottoView::generateRequested);
        QSignalSpy lock(&view, &LottoView::lockRequested);
        QVERIFY(QMetaObject::invokeMethod(view.rootItem(), "generateClicked"));
        QVERIFY(QMetaObject::invokeMethod(view.rootItem(), "lockClicked"));
        QCOMPARE(gen.count(), 1);
        QCOMPARE(lock.count(), 1);
    }

    void stylesArePushed()
    {
        qDebug() << "验证风格清单与当前风格写入 QML";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                        {QStringLiteral("neumorphic"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("neumorphic");
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        QQuickItem *root = view.rootItem();
        QTRY_COMPARE(root->property("styles").toList().size(), 2);
        QCOMPARE(root->property("currentStyleId").toString(),
                 QStringLiteral("neumorphic"));
    }

    void styleChangedUpdatesCurrentId()
    {
        qDebug() << "验证 styleChanged 更新 QML 当前风格";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        view.styleChanged(QStringLiteral("classic"));
        QCOMPARE(view.rootItem()->property("currentStyleId").toString(),
                 QStringLiteral("classic"));
    }

    void styleSwitchRequestForwardsToHost()
    {
        qDebug() << "验证 QML 风格切换请求转交宿主端口";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")} };
        host.currentId = QStringLiteral("neumorphic");
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        QVERIFY(QMetaObject::invokeMethod(view.rootItem(), "requestStyleSwitch",
                                          Q_ARG(QVariant, QStringLiteral("classic"))));
        QCOMPARE(host.requested, QStringList{ QStringLiteral("classic") });
    }

    void pillClickRequestsSwitch()
    {
        qDebug() << "验证真实点击风格胶囊触发切换请求(点击 MouseArea, 不走函数入口)";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("classic"), QStringLiteral("经典")},
                        {QStringLiteral("neumorphic"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("neumorphic");
        LottoNeumorphicView view(&host);
        view.resize(480, 720);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QTest::qWait(150);   // 等布局完成, 胶囊才有几何尺寸

        // 通过底部 Repeater 取回胶囊实例(委托尚未成为可枚举子对象时也可用)
        QQuickItem *pill = nullptr;
        const QList<QQuickItem *> items = view.rootItem()->findChildren<QQuickItem *>();
        for (QQuickItem *item : items) {
            if (!QString::fromLatin1(item->metaObject()->className())
                     .contains(QStringLiteral("Repeater")))
                continue;
            if (item->property("count").toInt() < 2)
                continue;

            QQuickItem *candidate = nullptr;
            QMetaObject::invokeMethod(item, "itemAt", Q_RETURN_ARG(QQuickItem *, candidate),
                                      Q_ARG(int, 0));
            if (candidate) {
                pill = candidate;
                break;
            }
        }
        QVERIFY2(pill != nullptr, "未找到风格胶囊委托");

        const QPointF center = pill->mapToScene(QPointF(pill->width() / 2,
                                                        pill->height() / 2));
        QTest::mouseClick(view.quickWidget(), Qt::LeftButton, Qt::NoModifier,
                          center.toPoint());
        QTRY_COMPARE(host.requested, QStringList{ QStringLiteral("classic") });
    }

    void snapshotReturnsImage()
    {
        qDebug() << "验证快照返回非空图像";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        view.resize(400, 600);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QTest::qWait(100);

        const QPixmap shot = view.snapshot();
        QVERIFY(!shot.isNull());
    }
};

QTEST_MAIN(TestLottoNeumorphicView)
#include "tst_lottoneumorphicview.moc"
