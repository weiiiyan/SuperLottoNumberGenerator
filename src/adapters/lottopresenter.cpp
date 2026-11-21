#include "lottopresenter.h"

#include <QMetaType>

// 视图结构常量与领域规则的编译期绑定: 两处定义不得漂移
static_assert(LottoViewShape::GROUP_COUNT == LottoTicket::GROUP_COUNT,
              "LottoViewShape::GROUP_COUNT 必须与 LottoTicket::GROUP_COUNT 一致");
static_assert(LottoViewShape::FRONT_COUNT == LottoTicket::FRONT_COUNT,
              "LottoViewShape::FRONT_COUNT 必须与 LottoTicket::FRONT_COUNT 一致");
static_assert(LottoViewShape::BACK_COUNT == LottoTicket::BACK_COUNT,
              "LottoViewShape::BACK_COUNT 必须与 LottoTicket::BACK_COUNT 一致");

LottoPresenter::LottoPresenter(QObject *parent)
    : QObject(parent)
{
    // Qt 5 下 QSignalSpy/排队连接需运行时注册自定义类型
    qRegisterMetaType<LottoViewState>();
    m_lastState = initialState();
}

void LottoPresenter::present(const LottoTicket &ticket)
{
    m_lastState = buildViewState(ticket);
    emit viewStateChanged(m_lastState);
}

const LottoViewState &LottoPresenter::state() const
{
    return m_lastState;
}

LottoViewState LottoPresenter::buildViewState(const LottoTicket &ticket)
{
    LottoViewState state;

    // 每组: 前区 FRONT_COUNT 个 + 后区 BACK_COUNT 个展示文本, 缺项填占位符
    for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g) {
        const LottoGroup result = ticket.groupAt(g);
        QStringList row;
        for (int i = 0; i < LottoGroup::FRONT_COUNT; ++i) {
            row << (i < result.front.size() ? formatNumber(result.front.at(i))
                                            : QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));
        }
        for (int i = 0; i < LottoGroup::BACK_COUNT; ++i) {
            row << (i < result.back.size() ? formatNumber(result.back.at(i))
                                           : QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER));
        }
        state.groups.append(row);
    }

    // 时间: 标题与值分开下发, 由视图决定排版(有效时间按固定格式, 无效显示占位)
    const QDateTime time = ticket.generateTime();
    state.timeLabelText = QString::fromUtf8(LottoViewText::TIME_LABEL);
    state.timeValueText = time.isValid()
        ? time.toString(QString::fromUtf8(LottoViewText::TIME_FORMAT))
        : QString::fromUtf8(LottoViewText::TIME_PLACEHOLDER);

    // 按钮互斥状态与文字: 含号码才可锁定, 锁定时禁用生成
    state.lockButtonText = ticket.isLocked() ? QString::fromUtf8(LottoViewText::LOCK_TEXT_LOCKED)
                                             : QString::fromUtf8(LottoViewText::LOCK_TEXT_UNLOCKED);
    state.lockButtonEnabled = ticket.hasNumbers();
    state.lockButtonChecked = ticket.isLocked();
    state.generateEnabled   = !ticket.isLocked();
    state.generateButtonText = QString::fromUtf8(LottoViewText::GENERATE_TEXT);
    state.numberPlaceholder  = QString::fromUtf8(LottoViewText::NUMBER_PLACEHOLDER);
    state.separatorText      = QString::fromUtf8(LottoViewText::SEPARATOR_TEXT);

    // 顶栏: 应用名与副标题(注数固定, 锁定后追加状态后缀)
    state.appTitleText = QString::fromUtf8(LottoViewText::APP_TITLE);
    state.headerMetaText = QString::fromUtf8(LottoViewText::HEADER_META_FORMAT)
                               .arg(LottoTicket::GROUP_COUNT);
    if (ticket.isLocked())
        state.headerMetaText += QString::fromUtf8(LottoViewText::HEADER_META_LOCKED);

    // 票根: 票号/提示/注数/印章
    state.serialText = buildSerial(time, ticket.hasNumbers());
    state.stubHintText = QString::fromUtf8(LottoViewText::STUB_HINT);
    state.stubCountText = QString::fromUtf8(LottoViewText::STUB_COUNT_FORMAT)
                              .arg(LottoTicket::GROUP_COUNT);
    state.stubCountValueText = QString::number(LottoTicket::GROUP_COUNT);
    state.stampText = QString::fromUtf8(LottoViewText::STAMP_TEXT);

    return state;
}

/*! rief 票号推导: 日期取生成时间, 后四位取时间戳尾数(确定性, 重绘不跳变) */
QString LottoPresenter::buildSerial(const QDateTime &time, bool hasNumbers)
{
    if (!hasNumbers || !time.isValid())
        return QString::fromUtf8(LottoViewText::SERIAL_PLACEHOLDER);

    const QString date = time.toString(QStringLiteral("yyMMdd"));
    const qint64 tail = time.toSecsSinceEpoch() % 10000;
    const QString suffix = QString::number(tail).rightJustified(4, QLatin1Char('0'));

    return QString::fromUtf8(LottoViewText::SERIAL_PREFIX)
        + date + QLatin1Char(' ') + suffix;
}

LottoViewState LottoPresenter::initialState()
{
    return buildViewState(LottoTicket());
}
