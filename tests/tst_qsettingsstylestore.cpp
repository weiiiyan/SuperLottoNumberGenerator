// QSettingsStyleStore 单元测试(实现位于 Main 组件)
// 覆盖: 未设置时为空、写入后可读回

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "qsettingsstylestore.h"

class TestQSettingsStyleStore : public QObject
{
    Q_OBJECT

private slots:
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

QTEST_MAIN(TestQSettingsStyleStore)
#include "tst_qsettingsstylestore.moc"
