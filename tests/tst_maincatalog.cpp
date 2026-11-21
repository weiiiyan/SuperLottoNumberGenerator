// Main 组件: 具体风格目录测试
// 目录是唯一允许出现具体风格类名的地方(ch26),
// 因此本目标必须链接两个风格库。

#include <QTest>

#include "maincatalog.h"

class TestMainCatalog : public QObject
{
    Q_OBJECT

private slots:
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
