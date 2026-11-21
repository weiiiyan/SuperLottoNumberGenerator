#include "lottoticketview.h"

#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>
#include <QVBoxLayout>

#include "lottoballlabel.h"
#include "lottotickettheme.h"
#include "lottolayoutmetrics.h"
#include "lottostylehost.h"
#include "lottostyledrawer.h"
#include "lottoticketdecor.h"

namespace {

/*! 中文无衬线字族(与设计稿的 font-body 一致) */
void applyBodyFont(QWidget *widget, int pixelSize, QFont::Weight weight, qreal tracking)
{
    QFont f = widget->font();
    f.setFamilies({ QStringLiteral("PingFang SC"), QStringLiteral("Microsoft YaHei"),
                    QStringLiteral("Hiragino Sans GB"),
                    QStringLiteral("Source Han Sans SC") });
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    f.setLetterSpacing(QFont::AbsoluteSpacing, tracking);
    widget->setFont(f);
}

/*! 数字字族(与设计稿的 font-num 一致), 并开启等宽数字 */
void applyNumberFont(QWidget *widget, int pixelSize, QFont::Weight weight, qreal tracking)
{
    QFont f = widget->font();
    f.setFamilies({ QStringLiteral("Bahnschrift"), QStringLiteral("DIN Alternate"),
                    QStringLiteral("Arial Narrow"), QStringLiteral("Helvetica Neue"),
                    QStringLiteral("Arial") });
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    f.setLetterSpacing(QFont::AbsoluteSpacing, tracking);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    f.setFeature(QFont::Tag("tnum"), 1);
#endif
    widget->setFont(f);
}

/*! 风格按钮图标: 四角方块, 透明度按设计稿(1/.45/.45/.8) */
QIcon makeStyleSwitchIcon()
{
    QIcon icon;
    const QColor ink = LottoTicketTheme::Ink;

    // 1x / 2x / 3x 三档, 高 DPI 下仍保持锐利
    for (int scale = 1; scale <= 3; ++scale) {
        QPixmap pixmap(13 * scale, 13 * scale);
        pixmap.fill(Qt::transparent);
        pixmap.setDevicePixelRatio(scale);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);

        const QRectF cells[] = {
            QRectF(0.5, 0.5, 5.0, 5.0), QRectF(7.5, 0.5, 5.0, 5.0),
            QRectF(0.5, 7.5, 5.0, 5.0), QRectF(7.5, 7.5, 5.0, 5.0)
        };
        const qreal opacities[] = { 1.0, 0.45, 0.45, 0.8 };
        for (int i = 0; i < 4; ++i) {
            QColor color = ink;
            color.setAlphaF(opacities[i]);
            painter.setBrush(color);
            painter.drawRoundedRect(cells[i], 1.0, 1.0);
        }
        icon.addPixmap(pixmap);
    }

    return icon;
}

/*! 风格按钮: 1.5px 抗锯齿边框 + 四角图标(对齐设计稿) */
class StyleSwitchButton : public QPushButton
{
public:
    explicit StyleSwitchButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
    }

    QSize sizeHint() const override
    {
        const QFontMetricsF metrics(font());
        const qreal textWidth = metrics.horizontalAdvance(text());
        return QSize(qCeil(3.0 + 8.0 + 13.0 + 6.0 + textWidth + 10.0), 32);
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QColor ink = LottoTicketTheme::Ink;
        painter.setPen(Qt::NoPen);
        painter.setBrush(underMouse() ? QColor(255, 255, 255)
                                      : QColor(255, 255, 255, 115));
        painter.drawRoundedRect(QRectF(rect()), 8.0, 8.0);

        QColor border = ink;
        border.setAlphaF(0.55);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(border, 1.5));
        const QRectF borderRect(0.75, 0.75, width() - 1.5, height() - 1.5);
        painter.drawRoundedRect(borderRect, 8.0, 8.0);

        const qreal iconSize = 13.0;
        const qreal gap = 6.0;
        const qreal startX = 1.5 + 8.0;
        const int iconY = (height() - 13) / 2;
        icon().paint(&painter, QRect(qRound(startX), iconY, 13, 13));

        painter.setFont(font());
        painter.setPen(ink);
        painter.drawText(QRectF(startX + iconSize + gap, 0.0,
                                width() - startX - iconSize - gap, height()),
                         Qt::AlignLeft | Qt::AlignVCenter, text());
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
            update();
        return QPushButton::event(event);
    }
};

} // namespace

LottoTicketView::LottoTicketView(LottoStyleHost *host, QWidget *parent)
    : LottoView(parent)
    , m_host(host)
{
    // 本组件的 qrc 由自己负责初始化: 静态库资源不经外部入口也不会被丢弃
    Q_INIT_RESOURCE(resources);
    setObjectName(QStringLiteral("ticketRoot"));

    // 先按设计基准摆好初始几何, 避免构造期按默认窗口尺寸算出过大的
    // 布局最小宽度; 真实尺寸随后由宿主/窗口的 resize 覆盖
    resize(LottoLayout::CanvasWidth, LottoLayout::CanvasHeight);

    initUi();

    recomputeMetrics();
    applyMetricsToWidgets();
    positionOverlays();
}

QString LottoTicketView::styleId() const
{
    return QString::fromLatin1(STYLE_ID);
}

void LottoTicketView::initUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    // 设计规则: 窗口变小时压缩, 不出现滚动条 —— 不让布局最小尺寸锁死窗口
    m_mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    // ── 顶栏: 品牌 + 副标题 + 风格按钮 ──
    m_topBar = new QFrame(this);
    m_topBar->setObjectName(QStringLiteral("topBar"));

    m_brandMark = new QLabel(m_topBar);
    m_brandMark->setObjectName(QStringLiteral("brandMark"));
    m_brandMark->setFixedSize(11, 11);

    m_brandName = new QLabel(m_topBar);
    m_brandName->setObjectName(QStringLiteral("brandName"));
    applyBodyFont(m_brandName, 18, QFont::Bold, 0.54);

    m_brandMeta = new QLabel(m_topBar);
    m_brandMeta->setObjectName(QStringLiteral("brandMeta"));
    applyNumberFont(m_brandMeta, 11, QFont::Medium, 1.5);

    m_btnStyleSwitch = new StyleSwitchButton(m_topBar);
    m_btnStyleSwitch->setObjectName(QStringLiteral("btnStyleSwitch"));
    m_btnStyleSwitch->setFixedHeight(32);
    m_btnStyleSwitch->setCursor(Qt::PointingHandCursor);
    m_btnStyleSwitch->setFocusPolicy(Qt::NoFocus);
    applyBodyFont(m_btnStyleSwitch, 11, QFont::Medium, 0.7);
    m_btnStyleSwitch->setIcon(makeStyleSwitchIcon());
    m_btnStyleSwitch->setIconSize(QSize(13, 13));

    auto *topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(0, 0, 0, 22);
    topLayout->setSpacing(10);

    auto *markColumn = new QVBoxLayout();
    markColumn->setContentsMargins(0, 6, 0, 0);
    markColumn->addWidget(m_brandMark, 0, Qt::AlignTop);
    topLayout->addLayout(markColumn);

    auto *brandColumn = new QVBoxLayout();
    brandColumn->setSpacing(5);
    brandColumn->addWidget(m_brandName);
    brandColumn->addWidget(m_brandMeta);
    topLayout->addLayout(brandColumn);
    topLayout->addStretch(1);
    topLayout->addWidget(m_btnStyleSwitch, 0, Qt::AlignTop);

    // ── 时间条: 标签左 / 值右 ──
    m_metaStrip = new QFrame(this);
    m_metaStrip->setObjectName(QStringLiteral("metaStrip"));

    m_timeLabel = new QLabel(m_metaStrip);
    m_timeLabel->setObjectName(QStringLiteral("timeLabel"));
    applyBodyFont(m_timeLabel, 11, QFont::Medium, 1.7);

    m_timeValueLabel = new QLabel(m_metaStrip);
    m_timeValueLabel->setObjectName(QStringLiteral("timeValueLabel"));
    applyNumberFont(m_timeValueLabel, 13, QFont::Medium, 0.5);

    auto *metaLayout = new QHBoxLayout(m_metaStrip);
    metaLayout->setContentsMargins(0, 22, 0, 20);
    metaLayout->setSpacing(12);
    metaLayout->addWidget(m_timeLabel, 0, Qt::AlignBottom);
    metaLayout->addStretch(1);
    metaLayout->addWidget(m_timeValueLabel, 0, Qt::AlignBottom);

    // ── 号码带: 5 行, 行距 0, 虚线即行边界 ──
    m_groupsContainer = new QWidget(this);
    m_groupsContainer->setObjectName(QStringLiteral("groupsContainer"));
    m_groupsLayout = new QVBoxLayout(m_groupsContainer);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);
    m_groupsLayout->setSpacing(0);
    m_groupsLayout->addStretch(1);

    for (int g = 0; g < GROUP_COUNT; g++) {
        QFrame *row = new QFrame(m_groupsContainer);
        row->setObjectName(QStringLiteral("ticketRow_%1").arg(g));
        row->setProperty("last", g == GROUP_COUNT - 1);
        m_rowWidgets[g] = row;

        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        m_rowIndexLabels[g] = new QLabel(
            QStringLiteral("%1").arg(g + 1, 2, 10, QLatin1Char('0')), row);
        m_rowIndexLabels[g]->setObjectName(QStringLiteral("rowIndex_%1").arg(g));
        m_rowIndexLabels[g]->setFixedWidth(LottoLayout::RowIndexWidth);
        applyNumberFont(m_rowIndexLabels[g], 10, QFont::Medium, 1.2);
        rowLayout->addWidget(m_rowIndexLabels[g], 0, Qt::AlignVCenter);

        auto *line = new QHBoxLayout();
        line->setContentsMargins(0, 0, 0, 0);
        line->setSpacing(LottoLayout::BallGap);
        line->addStretch(1);

        for (int i = 0; i < FRONT_COUNT; i++) {
            auto *ball = new LottoBallLabel(LottoBallLabel::Front, this);
            ball->setObjectName(QStringLiteral("frontLabel_%1_%2").arg(g).arg(i));
            m_frontLabels[g][i] = ball;
            line->addWidget(ball, 0, Qt::AlignVCenter);
        }

        m_separators[g] = new QLabel(QString(), this);
        m_separators[g]->setObjectName(QStringLiteral("separator_%1").arg(g));
        m_separators[g]->setAlignment(Qt::AlignCenter);
        m_separators[g]->setFixedWidth(LottoLayout::SeparatorWidth);
        applyNumberFont(m_separators[g], 13, QFont::Normal, 0.0);
        line->addWidget(m_separators[g], 0, Qt::AlignVCenter);

        for (int i = 0; i < BACK_COUNT; i++) {
            auto *ball = new LottoBallLabel(LottoBallLabel::Back, this);
            ball->setObjectName(QStringLiteral("backLabel_%1_%2").arg(g).arg(i));
            m_backLabels[g][i] = ball;
            line->addWidget(ball, 0, Qt::AlignVCenter);
        }

        line->addStretch(1);
        rowLayout->addLayout(line, 1);

        m_groupsLayout->addWidget(row);
    }
    m_groupsLayout->addStretch(1);

    // ── 骑缝线占位 + 自绘骑缝线(票面全宽, 覆盖在占位之上)──
    m_perfHost = new QWidget(this);
    m_perfHost->setObjectName(QStringLiteral("perfHost"));
    m_perfHost->setFixedHeight(LottoLayout::PerfHeight);
    m_perfHost->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_perf = new LottoPerfLine(this);
    m_perf->setObjectName(QStringLiteral("perfLine"));

    // ── 票根: 上行票号 + 条形码, 下行提示 + 注数 / 印章 ──
    m_stub = new QFrame(this);
    m_stub->setObjectName(QStringLiteral("stub"));

    m_serialLabel = new QLabel(m_stub);
    m_serialLabel->setObjectName(QStringLiteral("serialLabel"));
    applyNumberFont(m_serialLabel, 11, QFont::Normal, 0.7);

    m_barcode = new LottoBarcode(m_stub);
    m_barcode->setObjectName(QStringLiteral("barcode"));

    m_stubHairline = new QFrame(m_stub);
    m_stubHairline->setObjectName(QStringLiteral("stubHairline"));
    m_stubHairline->setFixedHeight(1);

    m_stubHint = new QLabel(m_stub);
    m_stubHint->setObjectName(QStringLiteral("stubHint"));
    applyBodyFont(m_stubHint, 10, QFont::Medium, 0.6);

    m_stubCount = new QLabel(m_stub);
    m_stubCount->setObjectName(QStringLiteral("stubCount"));
    applyNumberFont(m_stubCount, 11, QFont::Medium, 1.5);
    m_stubCount->setTextFormat(Qt::RichText);
    // 注数只淡出、不退出布局, 锁定前后行高不变
    auto *countEffect = new QGraphicsOpacityEffect(m_stubCount);
    countEffect->setOpacity(1.0);
    m_stubCount->setGraphicsEffect(countEffect);

    m_stamp = new LottoStamp(m_stub);
    m_stamp->setObjectName(QStringLiteral("stampLabel"));
    // 印章不加入布局, 由 positionStamp() 绝对定位到票根右下角

    auto *stubLayout = new QVBoxLayout(m_stub);
    stubLayout->setContentsMargins(0, 18, 0, 16);
    stubLayout->setSpacing(0);

    auto *stubTopRow = new QHBoxLayout();
    stubTopRow->setSpacing(10);
    stubTopRow->addWidget(m_serialLabel, 0, Qt::AlignBottom);
    stubTopRow->addStretch(1);
    stubTopRow->addWidget(m_barcode, 0, Qt::AlignBottom);
    stubLayout->addLayout(stubTopRow);
    stubLayout->addSpacing(15);
    stubLayout->addWidget(m_stubHairline);
    stubLayout->addSpacing(9);

    auto *stubBottomRow = new QHBoxLayout();
    stubBottomRow->setSpacing(10);
    stubBottomRow->addWidget(m_stubHint, 0, Qt::AlignBottom);
    stubBottomRow->addStretch(1);
    stubBottomRow->addWidget(m_stubCount, 0, Qt::AlignBottom);
    stubLayout->addLayout(stubBottomRow);

    // ── 操作区: 左锁定 / 右生成 ──
    m_actions = new QFrame(this);
    m_actions->setObjectName(QStringLiteral("actions"));

    m_btnLock = new QPushButton(m_actions);
    m_btnLock->setObjectName(QStringLiteral("btnLock"));
    m_btnLock->setCheckable(true);
    m_btnLock->setCursor(Qt::PointingHandCursor);
    applyBodyFont(m_btnLock, 15, QFont::DemiBold, 0.9);
    m_btnLock->setFixedHeight(LottoLayout::ActionHeight);

    m_btnGenerate = new QPushButton(m_actions);
    m_btnGenerate->setObjectName(QStringLiteral("btnGenerate"));
    m_btnGenerate->setCursor(Qt::PointingHandCursor);
    applyBodyFont(m_btnGenerate, 15, QFont::DemiBold, 0.9);
    m_btnGenerate->setFixedHeight(LottoLayout::ActionHeight);

    auto *actionsLayout = new QHBoxLayout(m_actions);
    actionsLayout->setContentsMargins(0, 24, 0, 0);
    actionsLayout->setSpacing(10);
    actionsLayout->addWidget(m_btnLock, 100);
    actionsLayout->addWidget(m_btnGenerate, 132);

    // ── 风格抽屉 ──
    m_drawer = new LottoStyleDrawer(this);
    connect(m_drawer, &LottoStyleDrawer::styleChosen, this, [this](const QString &id) {
        if (m_host)
            m_host->requestSwitchTo(id);
    });

    buildLayout();

    // 统一样式表
    QFile qss(QStringLiteral(":/ticket/style.qss"));
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(qss.readAll());
    } else {
        qWarning() << "[LOTTO] 未能加载 :/ticket/style.qss, 样式表未应用"
                   << "(本组件的 Q_INIT_RESOURCE 未能注册资源)";
    }

    // 信号连接: 按钮点击只发请求信号, 由宿主转发给输入侧适配器
    connect(m_btnGenerate, &QPushButton::clicked,
            this, &LottoView::generateRequested);
    connect(m_btnLock, &QPushButton::clicked,
            this, &LottoView::lockRequested);
    connect(m_btnStyleSwitch, &QPushButton::clicked,
            this, &LottoTicketView::openStyleDrawer);

    updateStyleButtonText(m_host ? m_host->currentStyleId() : QString());

    // 布局防抖定时器(解决 Android resizeEvent 自激震荡)
    m_layoutDebounceTimer = new QTimer(this);
    m_layoutDebounceTimer->setSingleShot(true);
    connect(m_layoutDebounceTimer, &QTimer::timeout, this, [this]() {
        if (size() == m_lastAppliedSize)
            return;
        m_lastAppliedSize = size();
        recomputeMetrics();
        applyMetricsToWidgets();
        positionOverlays();
        update();
    });
}

void LottoTicketView::buildLayout()
{
    m_mainLayout->addWidget(m_topBar);
    m_mainLayout->addWidget(m_metaStrip);
    m_mainLayout->addWidget(m_groupsContainer, 1);
    m_mainLayout->addWidget(m_perfHost);
    m_mainLayout->addWidget(m_stub);
    m_mainLayout->addWidget(m_actions);
}

void LottoTicketView::updateCardRect()
{
    // 票面: 桌面宽窗居中成卡片, 窄屏满幅
    const int cardW = qMin(width(), static_cast<int>(LottoLayout::CanvasWidth));
    const int cardH = qMin(height(), static_cast<int>(LottoLayout::CanvasHeight));
    m_cardRect = QRect((width() - cardW) / 2, (height() - cardH) / 2, cardW, cardH);
}

void LottoTicketView::recomputeMetrics()
{
    updateCardRect();
    const int cardW = m_cardRect.width();
    const int cardH = m_cardRect.height();

    m_padTop = LottoLayout::PadTop;
    m_padBottom = LottoLayout::PadBottom;

    const int fixedHeight = LottoLayout::TopBarHeight + LottoLayout::MetaHeight
        + LottoLayout::PerfHeight + LottoLayout::StubHeight + LottoLayout::ActionsHeight;

    int rowsAvailable = cardH - m_padTop - m_padBottom - fixedHeight;

    // 行高先压到 68, 仍不够再压上下边距
    const int minRows = GROUP_COUNT * LottoLayout::RowHeightMin;
    if (rowsAvailable < minRows) {
        int deficit = minRows - rowsAvailable;
        const int cutTop = qMin(deficit / 2, m_padTop - 16);
        m_padTop -= cutTop;
        deficit -= cutTop;
        const int cutBottom = qMin(deficit, m_padBottom - 12);
        m_padBottom -= cutBottom;
        rowsAvailable = cardH - m_padTop - m_padBottom - fixedHeight;
    }

    m_rowHeight = qBound(24, rowsAvailable / GROUP_COUNT,
                         static_cast<int>(LottoLayout::RowHeight));

    const int contentWidth = cardW - 2 * LottoLayout::PadX;
    const int lineWidth = contentWidth - LottoLayout::RowIndexWidth - 8;
    const int fromWidth = (lineWidth - (LABELS_PER_ROW - 1) * LottoLayout::BallGap
                           - LottoLayout::SeparatorWidth) / LABELS_PER_ROW;
    m_ballSize = qBound(16, qMin(fromWidth, m_rowHeight - 28),
                        static_cast<int>(LottoLayout::BallSize));

    m_mainLayout->setContentsMargins(m_cardRect.left() + LottoLayout::PadX,
                                     m_cardRect.top() + m_padTop,
                                     width() - m_cardRect.right() - 1 + LottoLayout::PadX,
                                     height() - m_cardRect.bottom() - 1 + m_padBottom);
}

void LottoTicketView::applyMetricsToWidgets()
{
    m_topBar->setFixedHeight(LottoLayout::TopBarHeight);
    m_metaStrip->setFixedHeight(LottoLayout::MetaHeight);
    m_perfHost->setFixedHeight(LottoLayout::PerfHeight);
    m_stub->setFixedHeight(LottoLayout::StubHeight);
    m_actions->setFixedHeight(LottoLayout::ActionsHeight);

    for (int g = 0; g < GROUP_COUNT; g++) {
        m_rowWidgets[g]->setFixedHeight(m_rowHeight);
        m_separators[g]->setFixedSize(LottoLayout::SeparatorWidth, m_rowHeight);
        for (int i = 0; i < FRONT_COUNT; i++)
            m_frontLabels[g][i]->setBallDiameter(m_ballSize);
        for (int i = 0; i < BACK_COUNT; i++)
            m_backLabels[g][i]->setBallDiameter(m_ballSize);
    }
}

void LottoTicketView::positionOverlays()
{
    m_mainLayout->activate();

    const QPoint hostPos = m_perfHost->mapTo(this, QPoint(0, 0));
    m_perf->setGeometry(m_cardRect.left(), hostPos.y(),
                        m_cardRect.width(), LottoLayout::PerfHeight);
    m_perf->raise();
    positionStamp();

    m_drawer->reposition(m_cardRect);
}

void LottoTicketView::positionStamp()
{
    if (!m_stamp || !m_stub || !m_stubCount || !m_stubHint)
        return;

    // 票根下行由布局决定, 先强制激活以取得实际几何
    if (m_stub->layout())
        m_stub->layout()->activate();

    // 设计稿: .stamp { position:absolute; right:-2px; bottom:-4px }
    const int rowRight = m_stubCount->geometry().right() + 1;
    const int rowBottom = m_stubHint->geometry().bottom() + 1;
    m_stamp->move(rowRight + 2 - m_stamp->width(),
                  rowBottom + 4 - m_stamp->height());
    m_stamp->raise();
}

void LottoTicketView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 展示底: 暖灰径向渐变
    QRadialGradient stageGradient(QPointF(width() * 0.5, -height() * 0.18),
                                  qMax(width(), height()) * 1.1);
    stageGradient.setColorAt(0.0, QColor(0xEF, 0xEA, 0xDF));
    stageGradient.setColorAt(0.58, LottoTicketTheme::Stage);
    stageGradient.setColorAt(1.0, QColor(0xDC, 0xD5, 0xC6));
    painter.fillRect(rect(), stageGradient);

    const bool fullBleed = (m_cardRect.size() == size());
    const qreal radius = fullBleed ? 0.0 : 36.0;

    if (!fullBleed) {
        // 卡片投影: 多层半透明圆角矩形近似柔影
        painter.setPen(Qt::NoPen);
        for (int i = 10; i >= 1; --i) {
            const QRectF shadowRect = QRectF(m_cardRect)
                .adjusted(-i * 1.5, -i * 1.5 + 8, i * 1.5, i * 2.0 + 10);
            painter.setBrush(QColor(60, 50, 30, 4));
            painter.drawRoundedRect(shadowRect, radius + i * 1.5, radius + i * 1.5);
        }
        painter.setBrush(QColor(28, 27, 25, 20));
        painter.setPen(QPen(QColor(28, 27, 25, 20), 1));
        painter.drawRoundedRect(m_cardRect, radius, radius);
    }

    // 票面: 上浅下深的纸色渐变
    QLinearGradient paper(m_cardRect.topLeft(), m_cardRect.bottomLeft());
    paper.setColorAt(0.0, LottoTicketTheme::PaperTop);
    paper.setColorAt(0.42, LottoTicketTheme::Paper);
    paper.setColorAt(1.0, LottoTicketTheme::PaperDeep);
    painter.setPen(Qt::NoPen);
    painter.setBrush(paper);
    painter.drawRoundedRect(m_cardRect, radius, radius);

    // 纸纹: 每 3px 一条 1px 横线, 对应设计稿 repeating-linear-gradient
    painter.save();
    QPainterPath cardPath;
    cardPath.addRoundedRect(m_cardRect, radius, radius);
    painter.setClipPath(cardPath);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(LottoTicketTheme::PaperGrain, 1.0));
    for (int y = m_cardRect.top(); y <= m_cardRect.bottom(); y += 3)
        painter.drawLine(m_cardRect.left(), y, m_cardRect.right(), y);
    painter.restore();

    // 顶部柔光
    painter.save();
    painter.setClipRect(m_cardRect);
    QRadialGradient sheen(QPointF(m_cardRect.center().x(), m_cardRect.top()),
                          m_cardRect.width() * 0.75);
    sheen.setColorAt(0.0, QColor(255, 255, 255, 140));
    sheen.setColorAt(0.7, QColor(255, 255, 255, 0));
    painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
    painter.fillRect(m_cardRect, sheen);
    painter.restore();
}

void LottoTicketView::renderState(const LottoViewState &state)
{
    applyState(state);
}

void LottoTicketView::resizeEvent(QResizeEvent *event)
{
    LottoView::resizeEvent(event);

    // 票面矩形只影响绘制, 立即更新以免首帧画在旧位置; 布局改动仍走防抖
    updateCardRect();
    update();

    // 防抖: 每次 resize 重启定时器, 50ms 内无新事件才执行布局更新
    // 解决 Android 端宽度在相近值间反复跳动导致的自激震荡
    if (m_layoutDebounceTimer)
        m_layoutDebounceTimer->start(50);
}

void LottoTicketView::applyState(const LottoViewState &state)
{
    // 防御: 对异常展示状态做边界保护, 避免越界访问
    if (state.groups.size() < GROUP_COUNT)
        return;

    if (m_brandName)
        m_brandName->setText(state.appTitleText);
    if (m_brandMeta)
        m_brandMeta->setText(state.headerMetaText);
    if (m_timeLabel)
        m_timeLabel->setText(state.timeLabelText);
    if (m_timeValueLabel)
        m_timeValueLabel->setText(state.timeValueText);
    if (m_serialLabel)
        m_serialLabel->setText(state.serialText);
    if (m_stubHint)
        m_stubHint->setText(state.stubHintText);
    if (m_stubCount) {
        if (state.stubCountValueText.isEmpty()) {
            m_stubCount->setText(state.stubCountText);
        } else {
            const QString value = QStringLiteral(
                "<span style=\"font-size:13px; font-weight:700; "
                "letter-spacing:0.5px; color:%1;\">%2</span>")
                .arg(LottoTicketTheme::Ink.name(), state.stubCountValueText);
            m_stubCount->setText(QString::fromUtf8(LottoViewText::STUB_COUNT_FORMAT)
                                     .arg(value));
        }
    }
    if (m_stamp)
        m_stamp->setStampText(state.stampText);

    for (int g = 0; g < GROUP_COUNT; g++) {
        const QStringList &row = state.groups.at(g);
        for (int i = 0; i < FRONT_COUNT; i++)
            if (i < row.size() && m_frontLabels[g][i])
                m_frontLabels[g][i]->setText(row.at(i));
        for (int i = 0; i < BACK_COUNT; i++)
            if (FRONT_COUNT + i < row.size() && m_backLabels[g][i])
                m_backLabels[g][i]->setText(row.at(FRONT_COUNT + i));
        if (m_separators[g])
            m_separators[g]->setText(state.separatorText);
    }

    // 换号动效: 号码确实变化且不是占位符时, 按球序错峰弹入
    const bool hasNumbers = !state.groups.isEmpty() && !state.groups.first().isEmpty()
        && state.groups.first().first() != state.numberPlaceholder;
    const bool numbersChanged = (state.groups != m_lastGroups);
    if (numbersChanged && hasNumbers) {
        for (int g = 0; g < GROUP_COUNT; g++) {
            for (int i = 0; i < FRONT_COUNT; i++)
                m_frontLabels[g][i]->playPopIn((g * LABELS_PER_ROW + i) * 16);
            for (int i = 0; i < BACK_COUNT; i++)
                m_backLabels[g][i]->playPopIn((g * LABELS_PER_ROW + FRONT_COUNT + i) * 16);
        }
    }
    m_lastGroups = state.groups;

    if (m_btnLock) {
        m_btnLock->setEnabled(state.lockButtonEnabled);
        m_btnLock->setChecked(state.lockButtonChecked);
        m_btnLock->setText(state.lockButtonText);
    }
    if (m_btnGenerate) {
        m_btnGenerate->setText(state.generateButtonText);
        m_btnGenerate->setEnabled(state.generateEnabled);
    }

    // 锁定反馈: 印章替掉注数(注数淡出但保留布局, 保持行高)
    const bool locked = state.lockButtonChecked;
    if (m_stubCount && locked != m_lastLocked) {
        auto *effect = qobject_cast<QGraphicsOpacityEffect *>(
            m_stubCount->graphicsEffect());
        if (effect) {
            auto *fade = new QPropertyAnimation(effect, "opacity", this);
            fade->setDuration(180);
            fade->setStartValue(effect->opacity());
            fade->setEndValue(locked ? 0.0 : 1.0);
            fade->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    if (m_stamp) {
        if (locked && !m_lastLocked)
            m_stamp->playIn();
        else if (!locked)
            m_stamp->hide();
    }
    positionStamp();
    m_lastLocked = locked;
}

void LottoTicketView::updateStyleButtonText(const QString &currentStyleId)
{
    QString label = QStringLiteral("风格");
    if (m_host) {
        const QVector<LottoStyleInfo> styles = m_host->availableStyles();
        for (const LottoStyleInfo &info : styles) {
            if (info.id == currentStyleId) {
                label = info.displayName;
                break;
            }
        }
    }
    if (m_btnStyleSwitch)
        m_btnStyleSwitch->setText(label);
}

void LottoTicketView::openStyleDrawer()
{
    if (!m_host || !m_drawer)
        return;
    m_drawer->setStyles(m_host->availableStyles(), m_host->currentStyleId());
    m_drawer->openFor(m_cardRect);
}

void LottoTicketView::styleChanged(const QString &currentStyleId)
{
    updateStyleButtonText(currentStyleId);
    if (m_host && m_drawer)
        m_drawer->setStyles(m_host->availableStyles(), currentStyleId);
    // 无条件收起: 抽屉可能因视图切换而残留为可见状态
    if (m_drawer)
        m_drawer->closeDrawer();
}
