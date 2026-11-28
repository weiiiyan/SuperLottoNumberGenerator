// MainWindow GUI 集成测试
// 通过 QTest::mouseClick 模拟用户操作, 用 findChild 定位控件并验证属性
// 夹具为注入式: 每次重建窗口 = 全新 controller + 指向同一临时文件的仓储,
// 风格偏好存储实例跨重建保留(以验证持久化)
// 运行方式: MainWindowTest.exe -platform offscreen

#include <QDebug>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "ticket/lottoticketview.h"
#include "lottocontroller.h"
#include "lottoengine.h"
#include "lottogroup.h"
#include "lottointeractor.h"
#include "lottopresenter.h"
#include "lottostylecatalog.h"
#include "maincatalog.h"
#include "lottoviewshell.h"
#include "mainwindow.h"
#include "qsettingsrepository.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;                    /*!< 测试专属临时目录(自动清理) */
    QSettings *m_settings = nullptr;
    QSettingsTicketRepository *m_repo = nullptr;
    LottoEngine *m_engine = nullptr;
    LottoInteractor *m_interactor = nullptr;
    LottoController *m_controller = nullptr;
    LottoPresenter *m_presenter = nullptr;
    FakeLottoStyleStore *m_store = nullptr;  /*!< 跨窗口重建保留, 验证持久化 */
    MainWindow *m_window = nullptr;

    /// 默认混合目录: 真实票据视图 + 假视图(不依赖 QML, 供切换用例)
    static QVector<LottoStyleDefinition> makeTestCatalog()
    {
        QVector<LottoStyleDefinition> catalog;
        catalog.append({ {QStringLiteral("ticket"), QStringLiteral("票据")},
            [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
                return new LottoTicketView(host, parent);
            } });
        catalog.append({ {QStringLiteral("fake-b"), QStringLiteral("假风格")},
            [](LottoStyleHost *, QWidget *parent) -> LottoView * {
                return new FakeLottoView(nullptr, QStringLiteral("fake-b"), parent);
            } });
        return catalog;
    }

    /// 重建窗口: 全新 交互器 + 输入/输出适配器 + 指向同一临时文件的仓储
    /// 风格偏好存储 m_store 不重建(否则失去持久化语义)
    /// 与生产一致: load() 由组合根(此处为夹具)调用, 而非视图构造触发
    void rebuildWindow(const QVector<LottoStyleDefinition> &catalog)
    {
        delete m_window;
        delete m_presenter;
        delete m_controller;
        delete m_interactor;
        delete m_engine;
        delete m_repo;
        delete m_settings;

        m_settings = new QSettings(m_dir.filePath("test.ini"), QSettings::IniFormat);
        m_repo = new QSettingsTicketRepository(m_settings);
        m_engine = new LottoEngine;
        m_presenter = new LottoPresenter;
        m_interactor = new LottoInteractor(m_repo, m_engine, m_presenter);
        m_controller = new LottoController(m_interactor);
        const auto wired = wireStyleCatalog(catalog, m_controller, m_presenter);
        m_window = new MainWindow(m_store, wired);
        m_interactor->load();   // 用例启动: 排定异步恢复
        QTest::qWait(50);       // 等 interactor->load() 的 singleShot(0) 异步恢复
    }

    MainWindow* makeWindow() { rebuildWindow(makeTestCatalog()); return m_window; }

    // 控件快捷查找(减少 findChild 样板代码)

    QLabel* frontLabel(int g, int i) const {
        return m_window->findChild<QLabel*>(QString("frontLabel_%1_%2").arg(g).arg(i));
    }
    QLabel* backLabel(int g, int i) const {
        return m_window->findChild<QLabel*>(QString("backLabel_%1_%2").arg(g).arg(i));
    }
    QLabel* separatorLabel(int g) const {
        return m_window->findChild<QLabel*>(QString("separator_%1").arg(g));
    }
    QPushButton* btnGenerate() const {
        return m_window->findChild<QPushButton*>("btnGenerate");
    }
    QPushButton* btnLock() const {
        return m_window->findChild<QPushButton*>("btnLock");
    }
    QLabel* timeLabel() const {
        return m_window->findChild<QLabel*>("timeLabel");
    }
    QLabel* timeValueLabel() const {
        return m_window->findChild<QLabel*>("timeValueLabel");
    }

private slots:
    // 测试生命周期

    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        // 资源初始化已内聚到 lotto_ui_ticket; 测试不再手工初始化,
        // 这样票据视图的自初始化若失效, 本测试会真实暴露
        m_store = new FakeLottoStyleStore;
        makeWindow();
    }

    void init()
    {
        // 每个测试前重置: 清空风格偏好并按默认混合目录重建窗口,
        // 避免上一个用例注入的目录/偏好泄漏到本用例
        m_store->storedId.clear();
        rebuildWindow(makeTestCatalog());
    }

    void cleanupTestCase()
    {
        delete m_window;
        delete m_presenter;
        delete m_controller;
        delete m_interactor;
        delete m_engine;
        delete m_repo;
        delete m_settings;
        delete m_store;
    }

    // ── 场景 1: 窗口初始化 ──

    void shouldHaveCorrectWindowTitle()
    {
        qDebug() << "验证窗口标题正确";
        QCOMPARE(m_window->windowTitle(), QString("大乐透随机号码生成器"));
    }

    void windowShellOwnsNoNumberWidgetsItself()
    {
        qDebug() << "验证窗口壳自身不直接创建号码控件(全部来自视图)";
        QVERIFY(m_window->viewShell() != nullptr);
        auto *label = frontLabel(0, 0);
        QVERIFY(label != nullptr);
        QVERIFY(m_window->viewShell()->currentView() != nullptr);
        QVERIFY(label->isAncestorOf(m_window->viewShell()->currentView())
                || m_window->viewShell()->currentView()->isAncestorOf(label));
    }

    void viewShellShouldExist()
    {
        qDebug() << "验证窗口承载视图宿主, 且当前为票据视图";
        QVERIFY(m_window->viewShell() != nullptr);
        QCOMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("ticket"));
        QVERIFY(m_window->findChild<QWidget*>("ticketRoot") != nullptr);
    }

    void allKeyWidgetsShouldExist()
    {
        qDebug() << "验证所有关键控件存在";
        QVERIFY(btnGenerate() != nullptr);
        QVERIFY(btnLock() != nullptr);
        QVERIFY(timeLabel() != nullptr);
        QVERIFY(timeValueLabel() != nullptr);
        QVERIFY(m_window->findChild<QWidget*>("groupsContainer") != nullptr);
        QVERIFY(m_window->findChild<QPushButton*>("btnStyleSwitch") != nullptr);
    }

    void allNumberLabelsAndSeparatorsShouldExist()
    {
        qDebug() << "验证所有号码标签和分隔符存在(5 组)";
        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            for (int i = 0; i < LottoViewShape::FRONT_COUNT; i++)
                QVERIFY(frontLabel(g, i) != nullptr);
            for (int i = 0; i < LottoViewShape::BACK_COUNT; i++)
                QVERIFY(backLabel(g, i) != nullptr);
            QVERIFY(separatorLabel(g) != nullptr);
        }
    }

    void btnLockShouldBeInitiallyDisabled()
    {
        qDebug() << "验证初始化时锁定按钮禁用且未选中";
        auto *btn = btnLock();
        QVERIFY(btn != nullptr);
        QVERIFY(!btn->isEnabled());
        QVERIFY(!btn->isChecked());
    }

    void btnGenerateShouldBeInitiallyEnabled()
    {
        qDebug() << "验证初始化时生成按钮可用";
        auto *btn = btnGenerate();
        QVERIFY(btn != nullptr);
        QVERIFY(btn->isEnabled());
    }

    // ── 场景 2: 点击"生成号码" — 号码格式验证 ──

    void clickGenerate_shouldUpdateAllNumberLabels()
    {
        qDebug() << "验证点击生成后 35 个标签均为 2 位数字且在范围内";
        QVERIFY(btnGenerate() != nullptr);
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            for (int i = 0; i < LottoViewShape::FRONT_COUNT; i++) {
                auto *label = frontLabel(g, i);
                QVERIFY(label != nullptr);
                bool ok;
                int num = label->text().toInt(&ok);
                QVERIFY2(ok, QString("前区标签 %1_%2 不是有效数字: '%3'")
                                 .arg(g).arg(i).arg(label->text()).toUtf8());
                QVERIFY2(num >= LottoGroup::FRONT_MIN && num <= LottoGroup::FRONT_MAX,
                         QString("前区号码 %1 超出范围").arg(num).toUtf8());
                QCOMPARE(label->text().size(), 2);  // 零填充两位
            }
            for (int i = 0; i < LottoViewShape::BACK_COUNT; i++) {
                auto *label = backLabel(g, i);
                QVERIFY(label != nullptr);
                bool ok;
                int num = label->text().toInt(&ok);
                QVERIFY2(ok, QString("后区标签 %1_%2 不是有效数字: '%3'")
                                 .arg(g).arg(i).arg(label->text()).toUtf8());
                QVERIFY2(num >= LottoGroup::BACK_MIN && num <= LottoGroup::BACK_MAX,
                         QString("后区号码 %1 超出范围").arg(num).toUtf8());
                QCOMPARE(label->text().size(), 2);
            }
        }
    }

    void clickGenerate_separatorsShouldDisplayPlus()
    {
        qDebug() << "验证分隔符显示 '+'";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            auto *sep = separatorLabel(g);
            QVERIFY(sep != nullptr);
            QCOMPARE(sep->text(), QString("+"));
        }
    }

    // ── 场景 3: 生成后锁定按钮状态变化 ──

    void clickGenerate_shouldEnableLockButton()
    {
        qDebug() << "验证生成号码后锁定按钮变为可用";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        QVERIFY(btnLock()->isEnabled());
        QVERIFY(!btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED));
        QCOMPARE(btnGenerate()->text(), QString::fromUtf8(LottoViewText::GENERATE_TEXT));
    }

    // ── 场景 4: 锁定 / 解锁流程 ──

    void lockButton_shouldToggleGenerateButtonState()
    {
        qDebug() << "验证锁定/解锁切换生成按钮状态和按钮文字";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);
        QVERIFY(btnLock()->isEnabled());

        // ── 锁定 ──
        QTest::mouseClick(btnLock(), Qt::LeftButton);
        QVERIFY(btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED));
        QVERIFY(!btnGenerate()->isEnabled());

        // ── 解锁 ──
        QTest::mouseClick(btnLock(), Qt::LeftButton);
        QVERIFY(!btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED));
        QVERIFY(btnGenerate()->isEnabled());
    }

    // ── 场景 5: 同组内号码 UI 展示应升序 ──

    void clickGenerate_frontNumbersShouldDisplayAscending()
    {
        qDebug() << "验证 UI 上前区号码按升序显示(5 组)";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            QVector<int> numbers;
            for (int i = 0; i < LottoViewShape::FRONT_COUNT; i++)
                numbers.append(frontLabel(g, i)->text().toInt());
            for (int i = 1; i < numbers.size(); i++) {
                QVERIFY2(numbers[i] > numbers[i-1],
                         QString("组 %1: 前区号码未按升序排列 (%2 >= %3)")
                             .arg(g).arg(numbers[i-1]).arg(numbers[i]).toUtf8());
            }
        }
    }

    void clickGenerate_backNumbersShouldDisplayAscending()
    {
        qDebug() << "验证 UI 上后区号码按升序显示(5 组)";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            QVector<int> numbers;
            for (int i = 0; i < LottoViewShape::BACK_COUNT; i++)
                numbers.append(backLabel(g, i)->text().toInt());
            for (int i = 1; i < numbers.size(); i++) {
                QVERIFY2(numbers[i] > numbers[i-1],
                         QString("组 %1: 后区号码未按升序排列 (%2 >= %3)")
                             .arg(g).arg(numbers[i-1]).arg(numbers[i]).toUtf8());
            }
        }
    }

    // ── 场景 6: 时间标签更新(标题与值分两个控件)──

    void clickGenerate_shouldUpdateTimeLabel()
    {
        qDebug() << "验证生成后时间标题与时间值分别更新";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        QCOMPARE(timeLabel()->text(), QString::fromUtf8(LottoViewText::TIME_LABEL));

        const QString value = timeValueLabel()->text();
        QRegularExpression re(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");
        QVERIFY2(re.match(value).hasMatch(),
                 QString("时间值格式不正确: '%1'").arg(value).toUtf8());
    }

    // ── 场景 6b: 风格切换保留票据状态 ──

    void switchStyle_keepsTicketState()
    {
        qDebug() << "验证切换风格后号码状态保留, 切回后控件仍显示同一号码";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);
        const QString before = frontLabel(0, 0)->text();

        auto *shell = m_window->viewShell();
        QVERIFY(shell != nullptr);
        shell->requestSwitchTo(QStringLiteral("fake-b"));
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("fake-b"));

        auto *b = static_cast<FakeLottoView *>(shell->currentView());
        QVERIFY(b != nullptr);
        QCOMPARE(b->lastState().groups.at(0).at(0), before);

        shell->requestSwitchTo(QStringLiteral("ticket"));
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("ticket"));
        QCOMPARE(frontLabel(0, 0)->text(), before);
    }

    void hiddenView_receivesStateUpdatesWhileInactive()
    {
        qDebug() << "验证隐藏视图在非当前状态时仍收到状态更新, 切回即最新";
        auto *shell = m_window->viewShell();
        shell->requestSwitchTo(QStringLiteral("fake-b"));
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("fake-b"));

        auto *fakeB = static_cast<FakeLottoView *>(shell->currentView());
        QVERIFY(fakeB != nullptr);
        const int before = fakeB->stateCount();

        // 票据视图此时不可见, 但仍在栈中; 经控制器触发用例
        m_controller->onGenerateRequested();
        QTRY_VERIFY(frontLabel(0, 0)->text()
                    != QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));

        // 两个视图都收到同一状态
        QCOMPARE(fakeB->stateCount(), before + 1);
        QCOMPARE(fakeB->lastState().groups.at(0).at(0), frontLabel(0, 0)->text());
    }

    void wiredNotReadyView_isDiscardedOnSwitch()
    {
        qDebug() << "验证已接线但未就绪的视图被丢弃, 不影响当前视图";
        QPointer<FakeLottoView> created;
        bool factoryCalled = false;
        auto catalog = makeTestCatalog();
        catalog.append({ {QStringLiteral("not-ready"), QStringLiteral("未就绪")},
            [&created, &factoryCalled](LottoStyleHost *, QWidget *parent) -> LottoView * {
                auto *view = new FakeLottoView(nullptr, QStringLiteral("not-ready"), parent);
                view->setReady(false);
                created = view;
                factoryCalled = true;
                return view;
            } });

        rebuildWindow(catalog);
        QVERIFY(!factoryCalled);   // 懒创建: 切换前不构造

        auto *shell = m_window->viewShell();
        const int savesBefore = m_store->saveCount;
        QSignalSpy spy(shell, &LottoViewShell::styleSwitchFailed);
        shell->requestSwitchTo(QStringLiteral("not-ready"));
        QTRY_COMPARE(spy.count(), 1);

        QVERIFY(factoryCalled);        // 视图已被创建并接线
        QCOMPARE(shell->currentStyleId(), QStringLiteral("ticket"));
        QCOMPARE(m_store->saveCount, savesBefore);   // 失败不写偏好
        QTRY_VERIFY(created.isNull());               // 已接线视图被 deleteLater 丢弃
    }

    void styleDrawer_doesNotReappearAfterSwitchBack()
    {
        qDebug() << "验证从抽屉切换风格后, 切回票据视图时抽屉不残留";
        auto *shell = m_window->viewShell();
        auto *ticket = qobject_cast<LottoTicketView *>(shell->currentView());
        QVERIFY(ticket != nullptr);

        auto *btn = ticket->findChild<QPushButton *>("btnStyleSwitch");
        QVERIFY(btn != nullptr);
        QTest::mouseClick(btn, Qt::LeftButton);

        auto *drawer = ticket->findChild<QWidget *>("styleDrawer");
        QVERIFY(drawer != nullptr);
        QVERIFY(!drawer->isHidden());

        auto *option = drawer->findChild<QPushButton *>("styleOption_1");
        QVERIFY(option != nullptr);
        QTest::mouseClick(option, Qt::LeftButton);   // 切到 fake-b
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("fake-b"));
        QVERIFY2(drawer->isHidden(), "选择风格后抽屉应立即收起");

        shell->requestSwitchTo(QStringLiteral("ticket"));
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("ticket"));
        QVERIFY2(drawer->isHidden(), "切回票据后抽屉不得残留");
    }

    void realCatalog_neuomorphicEntryCanBeActivated()
    {
        qDebug() << "验证真实目录可切换到新拟物视图(未接入或 Quick 不可用时跳过)";
        const auto catalog = buildStyleCatalog();
        bool hasNeumorphic = false;
        for (const LottoStyleDefinition &definition : catalog) {
            if (definition.info.id == QStringLiteral("neumorphic"))
                hasNeumorphic = true;
        }
        if (!hasNeumorphic)
            QSKIP("新拟物视图尚未接入风格目录");

        rebuildWindow(catalog);
        auto *shell = m_window->viewShell();
        shell->requestSwitchTo(QStringLiteral("neumorphic"));
        QTRY_VERIFY_WITH_TIMEOUT(
            shell->currentStyleId() == QStringLiteral("neumorphic"), 8000);
        QVERIFY(shell->currentView()->isReady());

        // 复位到票据视图, 避免影响后续用例
        shell->requestSwitchTo(QStringLiteral("ticket"));
        QTRY_COMPARE(shell->currentStyleId(), QStringLiteral("ticket"));
    }

    // ── 场景 6c: 风格偏好持久化 ──

    void styleChoice_persistsAcrossWindowRebuild()
    {
        qDebug() << "验证风格选择写入偏好并在窗口重建后恢复";
        m_window->viewShell()->requestSwitchTo(QStringLiteral("fake-b"));
        QTRY_COMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("fake-b"));
        QCOMPARE(m_store->storedId, QStringLiteral("fake-b"));

        rebuildWindow(makeTestCatalog());   // 同一 m_store, 新窗口
        QCOMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("fake-b"));
    }

    void unknownStoredStyle_fallsBackAndKeepsValue()
    {
        qDebug() << "验证未知偏好回退首项且保留原值";
        m_store->storedId = QStringLiteral("ghost");
        rebuildWindow(makeTestCatalog());
        QCOMPARE(m_window->viewShell()->currentStyleId(), QStringLiteral("ticket"));
        QCOMPARE(m_store->storedId, QStringLiteral("ghost"));
        m_store->storedId.clear();   // 复位, 避免影响后续用例
    }

    // ── 场景 7: 持久化 round-trip(数据层已由仓储测试覆盖, 此处验证视图管线)──

    void saveAndRestore_roundTrip()
    {
        qDebug() << "验证持久化 round-trip: 生成→锁定→重建→恢复";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);
        QVERIFY(btnLock()->isEnabled());
        QTest::mouseClick(btnLock(), Qt::LeftButton);   // 锁定 → controller 保存

        QStringList expectedFront, expectedBack;
        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            for (int i = 0; i < LottoViewShape::FRONT_COUNT; i++)
                expectedFront << frontLabel(g, i)->text();
            for (int i = 0; i < LottoViewShape::BACK_COUNT; i++)
                expectedBack << backLabel(g, i)->text();
        }

        makeWindow();

        QVERIFY2(btnLock()->isChecked(), "重建窗口后锁定按钮应为选中状态");
        QCOMPARE(btnLock()->text(), QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED));
        QVERIFY(!btnGenerate()->isEnabled());

        int fIdx = 0, bIdx = 0;
        for (int g = 0; g < LottoViewShape::GROUP_COUNT; g++) {
            for (int i = 0; i < LottoViewShape::FRONT_COUNT; i++)
                QCOMPARE(frontLabel(g, i)->text(), expectedFront[fIdx++]);
            for (int i = 0; i < LottoViewShape::BACK_COUNT; i++)
                QCOMPARE(backLabel(g, i)->text(), expectedBack[bIdx++]);
        }

        QCOMPARE(timeLabel()->text(), QString::fromUtf8(LottoViewText::TIME_LABEL));
        QRegularExpression re(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");
        QVERIFY2(re.match(timeValueLabel()->text()).hasMatch(),
                 QString("时间值格式不正确: '%1'").arg(timeValueLabel()->text()).toUtf8());
    }

    // ── 场景 8: 无锁定时不恢复 ──

    void restore_whenNotLocked_shouldNotRestore()
    {
        qDebug() << "验证未锁定时不恢复任何状态";
        m_repo->clear();   // 清空持久化, 等价于从未锁定过

        makeWindow();

        QVERIFY(!btnLock()->isEnabled());
        QVERIFY(!btnLock()->isChecked());
        QCOMPARE(frontLabel(0, 0)->text(),
                 QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));
    }

    // ── 场景 9: 票据视图最小尺寸不随当前宽度增长 ──

    void ticketMinimumWidthDoesNotBlockWindowShrink()
    {
        qDebug() << "验证票据视图按宽窗布局后, 窗口仍可缩回窄屏"
                    "(最小宽度不得随当前宽度增长, 否则窄屏右侧内容会被裁)";
        m_window->resize(1200, 900);
        m_window->show();
        QTest::qWait(150);   // 等首轮布局与 50ms 防抖

        m_window->resize(420, 700);
        QTest::qWait(150);

        QVERIFY2(m_window->width() <= 460,
                 qPrintable(QStringLiteral("窗口宽度仍为 %1, 被票据页最小宽度锁死")
                                .arg(m_window->width())));

        auto *ticket = m_window->findChild<LottoTicketView *>();
        QVERIFY(ticket != nullptr);
        QVERIFY2(ticket->width() <= m_window->viewShell()->width(),
                 "票据页宽度不应超过视图宿主");
    }

    // ── 场景 10: resize 布局自适应不崩溃 ──

    void resizeWindow_shouldNotCrash()
    {
        qDebug() << "验证多种宽度 resize 不崩溃且标签尺寸正常";
        m_window->show();
        QTest::qWait(100);  // 等初始布局完成 + 防抖定时器

        const QVector<int> widths = { 250, 320, 400, 500, 700 };
        for (int w : widths) {
            m_window->resize(w, 600);
            QTest::qWait(100);  // 等 50ms 防抖 + rebuildGroupRows

            auto *front00 = frontLabel(0, 0);
            QVERIFY(front00 != nullptr);
            QVERIFY2(front00->width() > 0 && front00->height() > 0,
                     QString("宽度 %1 时标签尺寸异常: %2x%3")
                         .arg(w).arg(front00->width()).arg(front00->height()).toUtf8());
        }
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
