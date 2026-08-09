// LottoController 输入侧适配器单元测试
// 验证"外部输入(点击) → 用例调用"的翻译, 以及业务守卫仍由用例层承担
// 注入内存 FakeTicketRepository / FakePresenterPort, 无需 GUI 与真实文件

#include <QTest>

#include <QDebug>

#include "lottocontroller.h"
#include "lottointeractor.h"
#include "lottoengine.h"
#include "lottopresenterport.h"
#include "ticketrepository.h"

/*! 内存版票据仓储: 记录每次保存的票据, 可预设加载结果 */
class FakeTicketRepository : public TicketRepository
{
public:
    LottoTicket loadResult;              /*!< 预设的 load 返回值 */
    QVector<LottoTicket> savedTickets;   /*!< 历次 save 的票据 */

    int saveCount() const { return savedTickets.size(); }

    void save(const LottoTicket &ticket) override { savedTickets.append(ticket); }
    LottoTicket load() override { return loadResult; }
    void clear() override { savedTickets.clear(); }
};

/*! 内存版输出边界端口: 记录每次 present() 的票据 */
class FakePresenterPort : public LottoPresenterPort
{
public:
    QVector<LottoTicket> presented;   /*!< 历次 present() 的票据 */

    int presentCount() const { return presented.size(); }

    void present(const LottoTicket &ticket) override { presented.append(ticket); }
    void clear() { presented.clear(); }
};

class TestLottoController : public QObject
{
    Q_OBJECT

private:
    FakeTicketRepository *m_repo = nullptr;
    FakePresenterPort *m_presenter = nullptr;
    LottoEngine *m_engine = nullptr;
    LottoInteractor *m_interactor = nullptr;
    LottoController *m_controller = nullptr;

private slots:
    void init()
    {
        m_repo = new FakeTicketRepository;
        m_presenter = new FakePresenterPort;
        m_engine = new LottoEngine;
        m_interactor = new LottoInteractor(m_repo, m_engine, m_presenter);
        m_controller = new LottoController(m_interactor);
    }

    void cleanup()
    {
        delete m_controller;
        delete m_interactor;
        delete m_engine;
        delete m_presenter;
        delete m_repo;
    }

    // ── 生成 ──

    void onGenerateRequested_shouldInvokeGenerateUseCase()
    {
        m_controller->onGenerateRequested();

        QCOMPARE(m_interactor->currentTicket().groups().size(), LottoTicket::GROUP_COUNT);
        QVERIFY(m_interactor->currentTicket().generateTime().isValid());
        QCOMPARE(m_presenter->presentCount(), 1);   // 用例经输出端口推送
    }

    void onGenerateRequested_whenLocked_shouldBeNoop()
    {
        // 锁定时拒绝生成的判定在用例层(控制器不重复业务规则)
        m_interactor->generateNewTicket();
        m_interactor->setLocked(true);
        const LottoTicket before = m_interactor->currentTicket();
        m_presenter->clear();

        m_controller->onGenerateRequested();

        QCOMPARE(m_interactor->currentTicket(), before);  // 票据未被替换
        QCOMPARE(m_presenter->presentCount(), 0);         // 且未向输出端口推送
    }

    // ── 锁定 ──

    void onLockRequested_shouldToggleAndSave()
    {
        m_interactor->generateNewTicket();
        m_presenter->clear();

        m_controller->onLockRequested();
        QVERIFY(m_interactor->currentTicket().isLocked());
        QCOMPARE(m_repo->saveCount(), 1);

        m_controller->onLockRequested();
        QVERIFY(!m_interactor->currentTicket().isLocked());
        QCOMPARE(m_repo->saveCount(), 2);
    }
};

QTEST_GUILESS_MAIN(TestLottoController)

#include "tst_lottocontroller.moc"
