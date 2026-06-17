// MainWindow GUI 集成测试
// 通过 QTest::mouseClick 模拟用户操作，用 findChild 定位控件并验证属性
// 运行方式: MainWindowTest.exe -platform offscreen

#include <QTest>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QSettings>
#include <QRegularExpression>

#include <QDebug>

#include "mainwindow.h"
#include "lottoengine.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private:
    MainWindow *m_window = nullptr;

    // 辅助方法

    static constexpr const char *SETTINGS_FILE = "SuperLottoNumberGenerator.ini";

    /// 清理 QSettings 残留数据
    static void clearSettings()
    {
        QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
        settings.clear();
        settings.sync();
    }

    // 控件快捷查找（减少 findChild 样板代码）

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

private slots:
    // 测试生命周期

    void initTestCase()
    {
        clearSettings();
        m_window = new MainWindow;
        // MainWindow 构造函数 → init() → restore()
        // 不在 offscreen 模式 show() 以避免平台差异
    }

    void init()
    {
        // 每个测试前重置状态：取消锁定、清空号码
        if (btnLock() && btnLock()->isChecked())
            QTest::mouseClick(btnLock(), Qt::LeftButton);  // 解锁
        clearSettings();
    }

    void cleanupTestCase()
    {
        delete m_window;
        m_window = nullptr;
        clearSettings();
    }

        // 场景 1：窗口初始化
    
    void shouldHaveCorrectWindowTitle()
    {
        qDebug() << "验证窗口标题正确";
        QCOMPARE(m_window->windowTitle(), QString("大乐透随机号码生成器"));
    }

    void allKeyWidgetsShouldExist()
    {
        qDebug() << "验证所有关键控件存在";
        QVERIFY(btnGenerate() != nullptr);
        QVERIFY(btnLock() != nullptr);
        QVERIFY(timeLabel() != nullptr);
        QVERIFY(m_window->findChild<QWidget*>("groupsContainer") != nullptr);
    }

    void allNumberLabelsAndSeparatorsShouldExist()
    {
        qDebug() << "验证所有号码标签和分隔符存在（5 组）";
        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            for (int i = 0; i < MainWindow::FRONT_COUNT; i++)
                QVERIFY(frontLabel(g, i) != nullptr);
            for (int i = 0; i < MainWindow::BACK_COUNT; i++)
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

        // 场景 2：点击"生成号码" — 号码格式验证
    
    void clickGenerate_shouldUpdateAllNumberLabels()
    {
        qDebug() << "验证点击生成后 35 个标签均为 2 位数字且在范围内";
        QVERIFY(btnGenerate() != nullptr);
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        // 验证 35 个号码标签都是 2 位数字格式且在正确范围内
        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            for (int i = 0; i < MainWindow::FRONT_COUNT; i++) {
                auto *label = frontLabel(g, i);
                QVERIFY(label != nullptr);
                bool ok;
                int num = label->text().toInt(&ok);
                QVERIFY2(ok, QString("前区标签 %1_%2 不是有效数字: '%3'")
                                 .arg(g).arg(i).arg(label->text()).toUtf8());
                QVERIFY2(num >= 1 && num <= 35,
                         QString("前区号码 %1 超出 [1,35]").arg(num).toUtf8());
                QCOMPARE(label->text().size(), 2);  // 零填充两位
            }
            for (int i = 0; i < MainWindow::BACK_COUNT; i++) {
                auto *label = backLabel(g, i);
                QVERIFY(label != nullptr);
                bool ok;
                int num = label->text().toInt(&ok);
                QVERIFY2(ok, QString("后区标签 %1_%2 不是有效数字: '%3'")
                                 .arg(g).arg(i).arg(label->text()).toUtf8());
                QVERIFY2(num >= 1 && num <= 12,
                         QString("后区号码 %1 超出 [1,12]").arg(num).toUtf8());
                QCOMPARE(label->text().size(), 2);
            }
        }
    }

    void clickGenerate_separatorsShouldDisplayPlus()
    {
        qDebug() << "验证分隔符显示 '+'";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            auto *sep = separatorLabel(g);
            QVERIFY(sep != nullptr);
            QCOMPARE(sep->text(), QString("+"));
        }
    }

        // 场景 3：生成后锁定按钮状态变化
    
    void clickGenerate_shouldEnableLockButton()
    {
        qDebug() << "验证生成号码后锁定按钮变为可用";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        QVERIFY(btnLock()->isEnabled());
        QVERIFY(!btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString("🔓 锁定号码"));
    }

        // 场景 4：锁定 / 解锁流程
    
    void lockButton_shouldToggleGenerateButtonState()
    {
        qDebug() << "验证锁定/解锁切换生成按钮状态和按钮文字";
        // 先生成号码
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);
        QVERIFY(btnLock()->isEnabled());

        // ── 锁定 ──
        QTest::mouseClick(btnLock(), Qt::LeftButton);
        QVERIFY(btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString("🔒 解锁生成"));
        QVERIFY(!btnGenerate()->isEnabled());

        // ── 解锁 ──
        QTest::mouseClick(btnLock(), Qt::LeftButton);
        QVERIFY(!btnLock()->isChecked());
        QCOMPARE(btnLock()->text(), QString("🔓 锁定号码"));
        QVERIFY(btnGenerate()->isEnabled());
    }

        // 场景 5：同组内前区号码 UI 展示应升序
    
    void clickGenerate_frontNumbersShouldDisplayAscending()
    {
        qDebug() << "验证 UI 上前区号码按升序显示（5 组）";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            QVector<int> numbers;
            for (int i = 0; i < MainWindow::FRONT_COUNT; i++)
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
        qDebug() << "验证 UI 上后区号码按升序显示（5 组）";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            QVector<int> numbers;
            for (int i = 0; i < MainWindow::BACK_COUNT; i++)
                numbers.append(backLabel(g, i)->text().toInt());
            for (int i = 1; i < numbers.size(); i++) {
                QVERIFY2(numbers[i] > numbers[i-1],
                         QString("组 %1: 后区号码未按升序排列 (%2 >= %3)")
                             .arg(g).arg(numbers[i-1]).arg(numbers[i]).toUtf8());
            }
        }
    }

        // 场景 6：时间标签更新
    
    void clickGenerate_shouldUpdateTimeLabel()
    {
        qDebug() << "验证生成后时间标签更新且格式正确";
        QTest::mouseClick(btnGenerate(), Qt::LeftButton);

        QString text = timeLabel()->text();
        QVERIFY2(text.contains("🕐 生成时间："),
                 QString("时间标签内容异常: '%1'").arg(text).toUtf8());

        // 验证时间格式: yyyy-MM-dd HH:mm:ss
        QRegularExpression re(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");
        QVERIFY2(re.match(text).hasMatch(),
                 QString("时间格式不正确: '%1'").arg(text).toUtf8());
    }

        // 场景 7：持久化 round-trip
    // 生成 → 锁定 → 销毁窗口 → 重建 → 验证号码恢复
    
    void saveAndRestore_roundTrip()
    {
        qDebug() << "验证持久化 round-trip: 生成→锁定→重建→恢复";
        // ─ Step 0: 验证 QSettings 读写通道正常 ─
        QSettings preSettings(SETTINGS_FILE, QSettings::IniFormat);
        preSettings.clear();

        QVariantList testFront, testBack;
        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            for (int i = 0; i < MainWindow::FRONT_COUNT; i++)
                testFront << (g * 10 + i + 1);   // 可识别的测试数据: 1,2,3,4,5,11,12,...
            for (int i = 0; i < MainWindow::BACK_COUNT; i++)
                testBack  << (g * 10 + i + 1);
        }
        preSettings.setValue("isLocked", true);
        preSettings.setValue("frontNumbers", testFront);
        preSettings.setValue("backNumbers", testBack);
        preSettings.setValue("generateTime", QString("🕐 测试时间：2024-01-01 00:00:00"));
        preSettings.sync();
        QVERIFY2(preSettings.value("isLocked").toBool(),
                 "QSettings isLocked 写入失败");
        QCOMPARE(preSettings.value("frontNumbers").toList().size(), 25);
        QCOMPARE(preSettings.value("backNumbers").toList().size(), 10);

        // ─ Step 1: 销毁旧窗口，用预设 QSettings 重建 → 触发 restore() ─
        delete m_window;
        m_window = new MainWindow;
        QTest::qWait(100);  // 等 QTimer::singleShot(0, restore) 执行

        // ─ Step 2: 验证锁定状态恢复 ─
        QVERIFY(btnLock() != nullptr);
        QVERIFY2(btnLock()->isChecked(), "重建窗口后锁定按钮应为选中状态");
        QVERIFY(!btnGenerate()->isEnabled());

        // ─ Step 3: 验证号码恢复为预设的测试数据 ─
        int fIdx = 0, bIdx = 0;
        for (int g = 0; g < MainWindow::GROUP_COUNT; g++) {
            for (int i = 0; i < MainWindow::FRONT_COUNT; i++)
                QCOMPARE(frontLabel(g, i)->text(), formatNumber(testFront[fIdx++].toInt()));
            for (int i = 0; i < MainWindow::BACK_COUNT; i++)
                QCOMPARE(backLabel(g, i)->text(), formatNumber(testBack[bIdx++].toInt()));
        }

        // ─ Step 4: 验证时间标签恢复 ─
        QCOMPARE(timeLabel()->text(), QString("🕐 测试时间：2024-01-01 00:00:00"));
    }

        // 场景 8：无锁定时不恢复
    
    void restore_whenNotLocked_shouldNotRestore()
    {
        qDebug() << "验证未锁定时不恢复任何状态";
        clearSettings();

        delete m_window;
        m_window = new MainWindow;
        QTest::qWait(50);

        QVERIFY(!btnLock()->isEnabled());
        QVERIFY(!btnLock()->isChecked());
    }

        // 场景 9：resize 布局自适应不崩溃
    
    void resizeWindow_shouldNotCrash()
    {
        qDebug() << "验证多种宽度 resize 不崩溃且标签尺寸正常";
        m_window->show();
        QTest::qWait(100);  // 等初始布局完成 + 防抖定时器

        // 多种宽度，验证布局重建不崩溃
        const QVector<int> widths = { 250, 320, 400, 500, 700 };
        for (int w : widths) {
            m_window->resize(w, 600);
            QTest::qWait(100);  // 等 50ms 防抖 + rebuildGroupRows

            // 验证标签仍然可访问
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
