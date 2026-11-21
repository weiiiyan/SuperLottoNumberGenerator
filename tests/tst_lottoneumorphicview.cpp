// LottoNeumorphicView 新拟物视图测试
// 需要 QQuickWidget; 平台 Quick 不可用时跳过
// 运行方式: LottoNeumorphicViewTest.exe -platform offscreen

#include <QCoreApplication>
#include <QMouseEvent>
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
        QCOMPARE(root->property("stubCountValueText").toString(),
                 QString::number(LottoTicket::GROUP_COUNT));
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
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
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

        view.styleChanged(QStringLiteral("ticket"));
        QCOMPARE(view.rootItem()->property("currentStyleId").toString(),
                 QStringLiteral("ticket"));
    }

    void styleSwitchRequestForwardsToHost()
    {
        qDebug() << "验证 QML 风格切换请求转交宿主端口";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")} };
        host.currentId = QStringLiteral("neumorphic");
        LottoNeumorphicView view(&host);
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");

        QVERIFY(QMetaObject::invokeMethod(view.rootItem(), "requestStyleSwitch",
                                          Q_ARG(QVariant, QStringLiteral("ticket"))));
        QCOMPARE(host.requested, QStringList{ QStringLiteral("ticket") });
    }

    void styleButtonOpensOverlayAndRequestsSwitch()
    {
        qDebug() << "验证右上角风格按钮展开抽屉, 选择选项后发出切换请求";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
                        {QStringLiteral("neumorphic"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("neumorphic");
        LottoNeumorphicView view(&host);
        view.resize(390, 844);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QTest::qWait(150);

        // 直接向 QQuickWidget 投递鼠标事件: 离屏平台下窗口级投递不可靠
        auto clickAt = [&view](const QPointF &scenePos) {
            const QPoint pos = scenePos.toPoint();
            const QPoint global = view.quickWidget()->mapToGlobal(pos);
            QMouseEvent press(QEvent::MouseButtonPress, pos, global,
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(view.quickWidget(), &press);
            QMouseEvent release(QEvent::MouseButtonRelease, pos, global,
                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(view.quickWidget(), &release);
        };

        const QList<QQuickItem *> items = view.rootItem()->findChildren<QQuickItem *>();
        auto findByObjectName = [&items](const QString &name) -> QQuickItem * {
            for (QQuickItem *item : items) {
                if (item->objectName() == name)
                    return item;
            }
            return nullptr;
        };

        QQuickItem *button = findByObjectName(QStringLiteral("styleButton"));
        QVERIFY2(button != nullptr, "未找到风格按钮");

        QQuickItem *icon = findByObjectName(QStringLiteral("styleIcon"));
        QVERIFY2(icon != nullptr, "未找到风格图标");
        QCOMPARE(icon->property("kind").toString(), QStringLiteral("style"));
        QCOMPARE(icon->property("color").value<QColor>(), QColor(0x4A, 0x55, 0x68));

        clickAt(button->mapToScene(QPointF(button->width() / 2, button->height() / 2)));

        QQuickItem *overlay = findByObjectName(QStringLiteral("styleOverlay"));
        QVERIFY2(overlay != nullptr, "未找到风格抽屉");
        QTRY_VERIFY(overlay->property("opened").toBool());

        // 抽屉展开 == 设计稿 [aria-expanded=true], 按钮应切到内陷态
        QQuickItem *face = findByObjectName(QStringLiteral("styleButtonFace"));
        QVERIFY2(face != nullptr, "未找到风格按钮面板");
        QTRY_COMPARE(face->property("color").value<QColor>(),
                     QColor(0xD6, 0xDC, 0xE5));

        QTest::qWait(300);   // 等上滑动画结束

        // 委托不是可枚举的 QObject 子对象, 经 Repeater::itemAt 取回
        QQuickItem *option = nullptr;
        const QList<QQuickItem *> allItems = view.rootItem()->findChildren<QQuickItem *>();
        for (QQuickItem *item : allItems) {
            if (!QString::fromLatin1(item->metaObject()->className())
                     .contains(QStringLiteral("Repeater")))
                continue;
            if (item->property("count").toInt() < 2)
                continue;
            QQuickItem *candidate = nullptr;
            QMetaObject::invokeMethod(item, "itemAt", Q_RETURN_ARG(QQuickItem *, candidate),
                                      Q_ARG(int, 0));
            if (candidate) {
                option = candidate;
                break;
            }
        }
        QVERIFY2(option != nullptr, "未找到风格选项");
        clickAt(option->mapToScene(QPointF(option->width() / 2, option->height() / 2)));
        QTRY_COMPARE(host.requested, QStringList{ QStringLiteral("ticket") });
        QTRY_COMPARE(face->property("color").value<QColor>(),
                     QColor(0xE0, 0xE5, 0xEC));
    }

    void lockedStampDoesNotResizeStubRow()
    {
        qDebug() << "验证锁定印章不改变票根下行高度";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        view.resize(390, 844);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QTest::qWait(150);

        QQuickItem *row = nullptr;
        QQuickItem *stamp = nullptr;
        const QList<QQuickItem *> items = view.rootItem()->findChildren<QQuickItem *>();
        for (QQuickItem *item : items) {
            if (item->objectName() == QStringLiteral("stubBottomRow"))
                row = item;
            else if (item->objectName() == QStringLiteral("stubStamp"))
                stamp = item;
        }
        QVERIFY2(row != nullptr, "未找到票根下行");
        QVERIFY2(stamp != nullptr, "未找到锁定印章");

        view.renderState(LottoPresenter::buildViewState(makeTicket()));
        QTest::qWait(50);
        const qreal heightBefore = row->height();
        const qreal yBefore = row->y();

        view.renderState(LottoPresenter::buildViewState(
            makeTicket(LottoTicket::GROUP_COUNT, true)));
        QTest::qWait(320);   // 等印章/注数动画结束

        QCOMPARE(row->height(), heightBefore);
        QCOMPARE(row->y(), yBefore);
        QVERIFY2(stamp->x() > row->width() / 2, "印章应贴票根下行右端");
        QVERIFY2(stamp->isVisible(), "锁定时印章应显示");
    }

    void styleButtonWidthFollowsLabel()
    {
        qDebug() << "验证风格按钮宽度随切换后的文案立即更新";
        FakeStyleHost host;
        host.styles = { {QStringLiteral("ticket"), QStringLiteral("票据")},
                        {QStringLiteral("neumorphic"), QStringLiteral("新拟物")} };
        host.currentId = QStringLiteral("ticket");
        LottoNeumorphicView view(&host);
        view.resize(390, 844);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        QTest::qWait(150);
        view.renderState(LottoPresenter::buildViewState(makeTicket()));

        QQuickItem *button = nullptr;
        for (QQuickItem *item : view.rootItem()->findChildren<QQuickItem *>()) {
            if (item->objectName() == QStringLiteral("styleButton")) {
                button = item;
                break;
            }
        }
        QVERIFY2(button != nullptr, "未找到风格按钮");
        const qreal ticketWidth = button->width();
        const qreal ticketX = button->x();

        view.styleChanged(QStringLiteral("neumorphic"));
        QTest::qWait(50);

        QVERIFY2(button->width() > ticketWidth, "更长文案应使风格按钮增宽");
        QVERIFY2(button->x() < ticketX, "右缘固定, 增宽后左缘应左移");
        const qreal neoWidth = button->width();
        const qreal neoX = button->x();

        view.renderState(LottoPresenter::buildViewState(
            makeTicket(LottoTicket::GROUP_COUNT, true)));
        QTest::qWait(50);
        QCOMPARE(button->width(), neoWidth);
        QCOMPARE(button->x(), neoX);
    }

    void lockedLockButtonTextUsesFrontColor()
    {
        qDebug() << "验证锁定后'解锁生成'使用 front 色";
        FakeStyleHost host;
        LottoNeumorphicView view(&host);
        view.resize(390, 844);
        view.show();
        if (!waitReady(view))
            QSKIP("当前平台 Quick 未能初始化");
        view.renderState(LottoPresenter::buildViewState(
            makeTicket(LottoTicket::GROUP_COUNT, true)));
        QTest::qWait(100);

        QQuickItem *lockButton = nullptr;
        for (QQuickItem *item : view.rootItem()->findChildren<QQuickItem *>()) {
            if (item->objectName() == QStringLiteral("lockButton")) {
                lockButton = item;
                break;
            }
        }
        QVERIFY2(lockButton != nullptr, "未找到锁定按钮");
        QVERIFY2(lockButton->property("checked").toBool(), "锁定按钮应处于选中态");
        QQuickItem *label = lockButton->findChild<QQuickItem *>(
            QStringLiteral("buttonText"));
        QVERIFY2(label != nullptr, "未找到按钮文字");
        QCOMPARE(label->property("color").value<QColor>(), QColor(0xC8, 0x6A, 0x6A));
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
