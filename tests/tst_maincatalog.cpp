// Main 组件: 具体风格目录测试
// 目录是唯一允许出现具体风格类名的地方(ch26),
// 因此本目标必须链接两个风格库。

#include <QTest>

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottoview.h"
#include "maincatalog.h"
#include "testhelpers.h"
#include "uitesthelpers.h"

class TestMainCatalog : public QObject
{
    Q_OBJECT

private slots:
    void wireStyleCatalog_shouldRenderCurrentStateAndForwardIntents()
    {
        qDebug() << "验证接线: 首帧取展示器缓存, 请求信号到达输入边界, present 后重渲染";

        FakeInputBoundary input;
        LottoController controller(&input);
        LottoPresenter presenter;
        FakeStyleHost host;

        // 先推送一次, 确保视图创建时拿到的是最新缓存而不是初始种子
        presenter.present(makeTicket());

        QVector<LottoStyleDefinition> raw;
        raw.append({ {QStringLiteral("fake"), QStringLiteral("假风格")},
            [](LottoStyleHost *, QWidget *parent) -> LottoView * {
                return new FakeLottoView(nullptr, QStringLiteral("fake"), parent);
            } });

        const auto wired = wireStyleCatalog(raw, &controller, &presenter);
        QCOMPARE(wired.size(), 1);

        LottoView *view = wired.first().create(&host, nullptr);
        auto *fake = static_cast<FakeLottoView *>(view);
        QVERIFY(fake != nullptr);

        // 首帧: 展示器当前状态(含号码, 不是空种子)
        QCOMPARE(fake->stateCount(), 1);
        QCOMPARE(fake->lastState().groups, presenter.state().groups);
        QCOMPARE(fake->lastState().lockButtonEnabled, true);

        // 点击 → 输入边界
        fake->simulateGenerate();
        fake->simulateLock();
        QCOMPARE(input.generateCalls, 1);
        QCOMPARE(input.toggleLockCalls, 1);

        // present → 视图重渲染
        presenter.present(makeTicket());
        QCOMPARE(fake->stateCount(), 2);

        delete view;
    }

    void firstEntryIsTicket()
    {
        qDebug() << "验证 Main 目录首项为票据视图";
        const auto catalog = buildStyleCatalog();
        QVERIFY(!catalog.isEmpty());
        QCOMPARE(catalog.first().info.id, QStringLiteral("ticket"));
    }

    void qt6CatalogContainsNeumorphic()
    {
        qDebug() << "验证 Qt6 下目录含新拟物条目";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        bool found = false;
        const auto catalog = buildStyleCatalog();
        for (const LottoStyleDefinition &definition : catalog)
            if (definition.info.id == QStringLiteral("neumorphic"))
                found = true;
        QVERIFY(found);
#else
        QSKIP("Qt5 构建无新拟物风格");
#endif
    }

    void everyEntryHasAFactoryAndUniqueId()
    {
        qDebug() << "验证目录条目均有工厂且 id 唯一";
        QStringList ids;
        const auto catalog = buildStyleCatalog();
        for (const LottoStyleDefinition &definition : catalog) {
            QVERIFY(static_cast<bool>(definition.create));
            QVERIFY(!ids.contains(definition.info.id));
            ids << definition.info.id;
        }
    }
};

QTEST_MAIN(TestMainCatalog)
#include "tst_maincatalog.moc"
