// UI 层端口与偏好存储单元测试
// 覆盖: 视图基类默认行为(偏好存储测试见 QSettingsStyleStoreTest)

#include <QTest>

#include "uitesthelpers.h"

class TestLottoUi : public QObject
{
    Q_OBJECT

private slots:
    void viewDefaults_shouldBeReadyAndSnapshot()
    {
        qDebug() << "验证视图基类默认就绪且可渲染/快照";
        FakeLottoView view(nullptr, QStringLiteral("x"));
        QVERIFY(view.isReady());
        view.renderState(LottoViewState());
        QCOMPARE(view.stateCount(), 1);
        view.snapshot();   // 不崩溃即可
    }
};

QTEST_MAIN(TestLottoUi)
#include "tst_lottoui.moc"
