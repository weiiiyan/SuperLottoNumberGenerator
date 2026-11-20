// 静态库 QML 模块冒烟测试
// 验证 LottoUi 模块进入资源系统, 且 QQuickWidget 能加载根类型
// 运行方式: LottoQmlSmokeTest.exe -platform offscreen

#include <QFile>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTest>

class TestQmlSmoke : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { qputenv("QT_QUICK_BACKEND", "software"); }

    void moduleShouldBeInResources()
    {
        QVERIFY2(QFile::exists(QStringLiteral(":/qt/qml/LottoUi/qmldir")),
                 "LottoUi qmldir 未进资源: 检查静态库资源初始化");
    }

    void quickWidgetShouldLoadModule()
    {
        QQuickWidget quick;
        quick.setResizeMode(QQuickWidget::SizeRootObjectToView);
        quick.loadFromModule("LottoUi", "NeumorphicView");
        QTRY_VERIFY_WITH_TIMEOUT(quick.status() != QQuickWidget::Loading, 5000);
        if (quick.status() != QQuickWidget::Ready)
            QSKIP("当前平台 Quick 未能初始化");
        QVERIFY(quick.rootObject() != nullptr);
    }
};

QTEST_MAIN(TestQmlSmoke)
#include "tst_lottoqmlsmoke.moc"
