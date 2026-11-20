// LottoViewShell 宿主单元测试
// 全部使用假视图目录, 不依赖 QML 运行环境
// 覆盖: 默认/恢复风格、非法 id、空目录、失败不写偏好、状态重放、信号转发

#include <QSignalSpy>
#include <QTest>

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottostyleswitchoverlay.h"
#include "lottoviewshell.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestLottoViewShell : public QObject
{
    Q_OBJECT

private:
    FakeInputBoundary *m_input = nullptr;
    LottoController *m_controller = nullptr;
    LottoPresenter *m_presenter = nullptr;
    FakeLottoStyleStore *m_store = nullptr;
    LottoViewShell *m_shell = nullptr;
    LottoViewState m_seed;          /*!< 注入宿主与窗口的初始展示状态 */
    QString m_lastButtonText;

    static QVector<LottoStyleDefinition> makeCatalog()
    {
        return {
            { {QStringLiteral("a"), QStringLiteral("甲")},
              [](LottoStyleHost *, QWidget *parent) -> LottoView * {
                  return new FakeLottoView(nullptr, QStringLiteral("a"), parent);
              } },
            { {QStringLiteral("b"), QStringLiteral("乙")},
              [](LottoStyleHost *, QWidget *parent) -> LottoView * {
                  return new FakeLottoView(nullptr, QStringLiteral("b"), parent);
              } },
        };
    }

private slots:
    void init()
    {
        m_input = new FakeInputBoundary;
        m_controller = new LottoController(m_input);
        m_presenter = new LottoPresenter;
        m_store = new FakeLottoStyleStore;
        m_lastButtonText = QString::fromUtf8(LottoViewText::GENERATE_TEXT);
        m_seed = LottoPresenter::initialState();
        m_shell = new LottoViewShell(m_controller, m_presenter, m_store,
                                     makeCatalog(), m_seed, 0);
        m_presenter->present(makeTicket());
    }

    void cleanup()
    {
        delete m_shell;
        delete m_store;
        delete m_presenter;
        delete m_controller;
        delete m_input;
    }

    void defaultsToFirstEntry()
    {
        qDebug() << "验证默认使用目录首项";
        QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
        QVERIFY(m_shell->currentView() != nullptr);
        QCOMPARE(m_shell->currentView()->styleId(), QStringLiteral("a"));
    }

    void emptyCatalog_shouldNotCrash()
    {
        qDebug() << "验证空目录不崩溃且忽略切换请求";
        LottoViewShell shell(m_controller, m_presenter, m_store, {}, m_seed, 0);
        QCOMPARE(shell.currentStyleId(), QString());
        QVERIFY(shell.currentView() == nullptr);
        shell.requestSwitchTo(QStringLiteral("a"));
        QTest::qWait(10);
        QVERIFY(shell.currentView() == nullptr);
    }

    void restoresStoredStyle()
    {
        qDebug() << "验证按偏好恢复风格";
        m_store->storedId = QStringLiteral("b");
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 0);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("b"));
    }

    void emptyStoredStyle_fallsBackToFirst()
    {
        qDebug() << "验证偏好为空时回退首项且不回写";
        m_store->storedId = QString();
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 0);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
        QCOMPARE(m_store->storedId, QString());
    }

    void unknownStoredStyle_fallsBackAndKeepsValue()
    {
        qDebug() << "验证偏好为未知 id 时回退首项且保留原值";
        m_store->storedId = QStringLiteral("ghost");
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 0);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
        QCOMPARE(m_store->storedId, QStringLiteral("ghost"));
    }

    void switchReplaysLastStateAndNotifies()
    {
        qDebug() << "验证切换重放最近状态, 通知两视图并保存偏好";
        m_shell->requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
        QCOMPARE(m_shell->currentView()->styleId(), QStringLiteral("b"));

        auto *b = static_cast<FakeLottoView *>(m_shell->currentView());
        QCOMPARE(b->stateCount(), 1);
        QCOMPARE(b->lastState().generateButtonText, m_lastButtonText);
        QCOMPARE(b->styleChangedCount(), 1);
        QCOMPARE(m_store->storedId, QStringLiteral("b"));
    }

    void unknownStyle_shouldFailWithoutPersisting()
    {
        qDebug() << "验证切换到未知风格时上报失败且不写偏好";
        QSignalSpy spy(m_shell, &LottoViewShell::styleSwitchFailed);
        m_shell->requestSwitchTo(QStringLiteral("nope"));
        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("nope"));
        QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
        QCOMPARE(m_store->saveCount, 0);
    }

    void notReadyView_shouldFailWithoutPersisting()
    {
        qDebug() << "验证目标视图未就绪时保持当前视图且不写偏好";
        auto catalog = makeCatalog();
        catalog[1].create = [](LottoStyleHost *, QWidget *parent) -> LottoView * {
            auto *view = new FakeLottoView(nullptr, QStringLiteral("b"), parent);
            view->setReady(false);
            return view;
        };
        LottoViewShell shell(m_controller, m_presenter, m_store, catalog, m_seed, 0);
        QSignalSpy spy(&shell, &LottoViewShell::styleSwitchFailed);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
        QCOMPARE(m_store->saveCount, 0);
    }

    void nullViewFromFactory_shouldFail()
    {
        qDebug() << "验证工厂返回 null 时保持当前视图";
        auto catalog = makeCatalog();
        catalog[1].create = [](LottoStyleHost *, QWidget *) -> LottoView * {
            return nullptr;
        };
        LottoViewShell shell(m_controller, m_presenter, m_store, catalog, m_seed, 0);
        QSignalSpy spy(&shell, &LottoViewShell::styleSwitchFailed);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
        QVERIFY(shell.currentView() != nullptr);
    }

    void generateSignalForwardsToController()
    {
        qDebug() << "验证视图信号经宿主转发到输入侧适配器";
        auto *view = static_cast<FakeLottoView *>(m_shell->currentView());
        view->simulateGenerate();
        view->simulateLock();
        QCOMPARE(m_input->generateCalls, 1);
        QCOMPARE(m_input->toggleLockCalls, 1);
    }

    void switchBeforeAnyState_shouldRenderInjectedSeed()
    {
        qDebug() << "验证首次状态到达前切换, 新视图显示注入的初始状态";
        delete m_shell;
        m_shell = new LottoViewShell(m_controller, m_presenter, m_store,
                                     makeCatalog(), m_seed, 0);
        m_shell->requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
        auto *b = static_cast<FakeLottoView *>(m_shell->currentView());
        QCOMPARE(b->stateCount(), 1);
        QCOMPARE(b->lastState().generateButtonText, m_seed.generateButtonText);
    }

    void initialViewReceivesSeedImmediately()
    {
        qDebug() << "验证构造后首个视图立即收到初始状态";
        auto *view = static_cast<FakeLottoView *>(m_shell->currentView());
        QVERIFY(view != nullptr);
        QVERIFY(view->stateCount() >= 1);
        QCOMPARE(view->lastState().numberPlaceholder, m_seed.numberPlaceholder);
    }

    void zeroTransition_createsNoOverlay()
    {
        qDebug() << "验证 transitionMs=0 时不创建过渡覆盖层";
        m_shell->requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("b"));
        QVERIFY(m_shell->findChild<LottoStyleSwitchOverlay *>() == nullptr);
    }

    void fadeTransition_removesOverlayAfterwards()
    {
        qDebug() << "验证过渡覆盖层播放后自动销毁";
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 400);
        shell.resize(400, 600);
        shell.show();
        QTest::qWait(10);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(shell.currentStyleId(), QStringLiteral("b"));
        // 过渡期间必须有覆盖层; 否则"之后消失"的断言是自证的
        QVERIFY(shell.findChild<LottoStyleSwitchOverlay *>() != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(
            shell.findChild<LottoStyleSwitchOverlay *>() == nullptr, 3000);
    }

    void rapidSwitches_leaveSingleOverlay()
    {
        qDebug() << "验证快速连续切换后不残留覆盖层且停在最后目标";
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 200);
        shell.resize(400, 600);
        shell.show();
        QTest::qWait(10);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTest::qWait(20);
        shell.requestSwitchTo(QStringLiteral("a"));
        QTest::qWait(20);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTRY_VERIFY_WITH_TIMEOUT(
            shell.findChild<LottoStyleSwitchOverlay *>() == nullptr, 4000);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("b"));
    }

    void startupNotReadyView_shouldFallBackToFirst()
    {
        qDebug() << "验证启动时恢复的视图未就绪则回退首项且不覆盖偏好";
        m_store->storedId = QStringLiteral("b");
        auto catalog = makeCatalog();
        catalog[1].create = [](LottoStyleHost *, QWidget *parent) -> LottoView * {
            auto *view = new FakeLottoView(nullptr, QStringLiteral("b"), parent);
            view->setReady(false);
            return view;
        };
        LottoViewShell shell(m_controller, m_presenter, m_store, catalog, m_seed, 0);
        QCOMPARE(shell.currentStyleId(), QStringLiteral("a"));
        QVERIFY(shell.currentView() != nullptr);
        QCOMPARE(shell.currentView()->styleId(), QStringLiteral("a"));
        QCOMPARE(m_store->storedId, QStringLiteral("b"));   // 原值保留
    }

    void requestWhileSwitchPending_keepsLastIntent()
    {
        qDebug() << "验证切换排队期间再次请求, 以最后一次请求为准";
        m_shell->requestSwitchTo(QStringLiteral("b"));
        m_shell->requestSwitchTo(QStringLiteral("a"));   // 同一事件轮次内回切
        QTRY_COMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
        QTest::qWait(20);
        QCOMPARE(m_shell->currentStyleId(), QStringLiteral("a"));
    }

    void resizeDuringTransition_keepsOverlayCoveringAndDisposes()
    {
        qDebug() << "验证过渡期间 resize: 覆盖层跟随尺寸并在结束后释放";
        LottoViewShell shell(m_controller, m_presenter, m_store, makeCatalog(),
                             m_seed, 400);
        shell.resize(400, 600);
        shell.show();
        QTest::qWait(10);
        shell.requestSwitchTo(QStringLiteral("b"));
        QTRY_COMPARE(shell.currentStyleId(), QStringLiteral("b"));

        auto *overlay = shell.findChild<LottoStyleSwitchOverlay *>();
        QVERIFY(overlay != nullptr);
        QCOMPARE(overlay->size(), shell.size());

        shell.resize(320, 480);
        QTRY_COMPARE(overlay->size(), shell.size());

        QTRY_VERIFY_WITH_TIMEOUT(
            shell.findChild<LottoStyleSwitchOverlay *>() == nullptr, 3000);
        QCOMPARE(shell.findChildren<LottoStyleSwitchOverlay *>().size(), 0);
    }

    void sameStyleRequestIsIgnored()
    {
        qDebug() << "验证请求当前风格时不做任何事";
        m_shell->requestSwitchTo(QStringLiteral("a"));
        QTest::qWait(10);
        QCOMPARE(m_store->saveCount, 0);
    }
};

QTEST_MAIN(TestLottoViewShell)
#include "tst_lottoviewshell.moc"
