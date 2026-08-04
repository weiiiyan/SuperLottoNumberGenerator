// QSettingsTicketRepository 持久化测试
// 使用 QTemporaryDir 真文件隔离, 覆盖读写 round-trip 与旧 INI 格式兼容

#include <QTest>
#include <QSettings>
#include <QTemporaryDir>

#include <QDebug>

#include "qsettingsrepository.h"

class TestQSettingsRepository : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;                  /*!< 每个测试实例独立的临时目录 */
    QSettings *m_settings = nullptr;
    QSettingsTicketRepository *m_repo = nullptr;

    /// 构造固定票据: 5 组合法号码 + 已知时间 + 锁定
    static LottoTicket makeFixedTicket()
    {
        QVector<LottoResult> groups;
        for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g) {
            QVector<int> front, back;
            for (int i = 0; i < LottoTicket::FRONT_COUNT; ++i)
                front << g * LottoTicket::FRONT_COUNT + i + 1;
            for (int i = 0; i < LottoTicket::BACK_COUNT; ++i)
                back << g * LottoTicket::BACK_COUNT + i + 1;
            groups.append(LottoResult(front, back));
        }
        return LottoTicket(groups, QDateTime(QDate(2026, 1, 1), QTime(12, 30, 45)), true);
    }

    /// 写入旧版格式数据(键与现版本相同, 时间字段为显示字符串)
    void writeLegacyData(const QString &timeText, const QVariantList &frontList,
                         const QVariantList &backList)
    {
        m_settings->setValue("isLocked", true);
        m_settings->setValue("frontNumbers", frontList);
        m_settings->setValue("backNumbers", backList);
        m_settings->setValue("generateTime", timeText);
        m_settings->sync();
    }

private slots:
    void init()
    {
        m_settings = new QSettings(m_dir.filePath("test.ini"), QSettings::IniFormat);
        m_repo = new QSettingsTicketRepository(m_settings);
    }

    void cleanup()
    {
        delete m_repo;
        delete m_settings;
    }

    // ── 读写 round-trip ──

    void saveLocked_roundTrip_shouldBeEqual()
    {
        const LottoTicket ticket = makeFixedTicket();
        m_repo->save(ticket);

        const LottoTicket loaded = m_repo->load();
        QVERIFY(loaded == ticket);   // 号码/时间(秒级)/锁定标志完全一致
        QVERIFY(loaded.isValid());
    }

    void saveUnlocked_shouldLoadEmptyTicket()
    {
        LottoTicket ticket = makeFixedTicket();
        ticket.setLocked(false);
        m_repo->save(ticket);

        const LottoTicket loaded = m_repo->load();
        QVERIFY(loaded.groups().isEmpty());
        QVERIFY(!loaded.isLocked());
    }

    void clear_shouldRemoveAllKeys()
    {
        m_repo->save(makeFixedTicket());
        m_repo->clear();

        QVERIFY(m_settings->allKeys().isEmpty());
        QVERIFY(!m_repo->load().isLocked());
    }

    // ── 旧 INI 格式兼容 ──

    void legacyIni_shouldBeReadable()
    {
        QVariantList frontList, backList;
        for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g) {
            for (int i = 0; i < LottoTicket::FRONT_COUNT; ++i)
                frontList << g * LottoTicket::FRONT_COUNT + i + 1;
            for (int i = 0; i < LottoTicket::BACK_COUNT; ++i)
                backList << g * LottoTicket::BACK_COUNT + i + 1;
        }
        // 旧版时间字段为显示字符串(含 emoji 前缀)
        writeLegacyData("🕐 测试时间：2024-01-01 00:00:00", frontList, backList);

        const LottoTicket ticket = m_repo->load();
        QVERIFY(ticket.isLocked());
        QCOMPARE(ticket.groups().size(), LottoTicket::GROUP_COUNT);
        QCOMPARE(ticket.groupAt(0).front, QVector<int>({1, 2, 3, 4, 5}));
        QCOMPARE(ticket.groupAt(4).back, QVector<int>({9, 10}));
        QCOMPARE(ticket.generateTime(), QDateTime(QDate(2024, 1, 1), QTime(0, 0, 0)));
    }

    void legacyIni_isoGenerateTime_shouldBeReadable()
    {
        QVariantList frontList, backList;
        for (int i = 0; i < 10; ++i) {
            frontList << i + 1;
            backList << i + 1;
        }
        // 新版 ISO 字符串(本地时间无时区后缀)
        writeLegacyData("2024-01-01T00:00:00", frontList, backList);

        const LottoTicket ticket = m_repo->load();
        QVERIFY(ticket.isLocked());
        QCOMPARE(ticket.generateTime(), QDateTime(QDate(2024, 1, 1), QTime(0, 0, 0)));
    }

    void legacyIni_unparseableTime_shouldNotCrash()
    {
        QVariantList frontList, backList;
        for (int i = 0; i < 10; ++i) {
            frontList << i + 1;
            backList << i + 1;
        }
        writeLegacyData("not a time at all", frontList, backList);

        const LottoTicket ticket = m_repo->load();
        QVERIFY(ticket.isLocked());
        QVERIFY(!ticket.generateTime().isValid());   // 无效时间兜底, 不崩溃
        QCOMPARE(ticket.groupAt(1).front.size(), LottoTicket::FRONT_COUNT);
    }

    void legacyIni_partialNumbers_shouldTolerate()
    {
        QVariantList frontList, backList;
        for (int i = 0; i < 10; ++i) {   // 仅两组数量(缺 3 组)
            frontList << i + 1;
            backList << i + 1;
        }
        writeLegacyData("🕐 测试时间：2024-01-01 00:00:00", frontList, backList);

        const LottoTicket ticket = m_repo->load();
        QCOMPARE(ticket.groups().size(), LottoTicket::GROUP_COUNT);
        QCOMPARE(ticket.groupAt(0).front.size(), LottoTicket::FRONT_COUNT);  // 完整组
        QCOMPARE(ticket.groupAt(2).front.size(), 0);                          // 缺项留空
        QVERIFY(ticket.isLocked());
    }
};

QTEST_GUILESS_MAIN(TestQSettingsRepository)

#include "tst_qsettingsrepository.moc"
