// UI 层端口与偏好存储单元测试
// 覆盖: 视图基类默认行为、QSettings 风格偏好往返

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "qsettingsstylestore.h"
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

    void styleStore_shouldRoundTrip()
    {
        qDebug() << "验证风格偏好经 QSettings 往返";
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings settings(dir.filePath("s.ini"), QSettings::IniFormat);
        QSettingsStyleStore store(&settings);
        QCOMPARE(store.loadStyleId(), QString());
        store.saveStyleId(QStringLiteral("neumorphic"));
        QCOMPARE(store.loadStyleId(), QStringLiteral("neumorphic"));
    }
};

QTEST_MAIN(TestLottoUi)
#include "tst_lottoui.moc"
