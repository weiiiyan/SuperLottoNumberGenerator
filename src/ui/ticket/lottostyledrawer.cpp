#include "lottostyledrawer.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>

#include "lottotickettheme.h"

namespace {
constexpr int kSheetPad = 18;      /*!< 面板内边距 */
constexpr int kOptionHeight = 48;  /*!< 单个选项高度(设计稿 14px 内边距 + 文案) */
constexpr int kOptionGap = 8;      /*!< 选项间距 */
constexpr int kSlideMs = 240;      /*!< 上滑时长 */
/*! 风格选项: 自绘 1.5px 边框, 文案左对齐、状态右对齐(设计稿 space-between) */
class StyleOptionButton : public QPushButton
{
public:
    explicit StyleOptionButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
    }

    void setStateText(const QString &text)
    {
        m_stateText = text;
        update();
    }

    void setCurrent(bool current)
    {
        m_current = current;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255));
        painter.drawRoundedRect(QRectF(rect()), 10.0, 10.0);

        QColor border(28, 27, 25);
        if (m_current)
            border = QColor(0xC1, 0x12, 0x1F);
        else
            border.setAlphaF(underMouse() ? 0.40 : 0.14);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(border, 1.5));
        const QRectF borderRect(0.75, 0.75, width() - 1.5, height() - 1.5);
        painter.drawRoundedRect(borderRect, 10.0, 10.0);

        if (m_current) {
            painter.setPen(QPen(QColor(0xC1, 0x12, 0x1F), 1.0));
            painter.drawRoundedRect(borderRect.adjusted(1.0, 1.0, -1.0, -1.0),
                                    9.0, 9.0);
        }

        // 设计稿 box-sizing:border-box, 内边距从边框内侧起算
        const qreal pad = 1.5 + 14.0;
        QFont labelFont = font();
        labelFont.setPixelSize(14);
        painter.setFont(labelFont);
        painter.setPen(QColor(0x1C, 0x1B, 0x19));
        painter.drawText(QRectF(pad, 0.0, width() - 2.0 * pad, height()),
                         Qt::AlignLeft | Qt::AlignVCenter, text());

        QFont stateFont = font();
        stateFont.setPixelSize(11);
        stateFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.88);
        stateFont.setWeight(m_current ? QFont::DemiBold : QFont::Normal);
        painter.setFont(stateFont);
        painter.setPen(m_current ? QColor(0xC1, 0x12, 0x1F)
                                 : QColor(0x6A, 0x65, 0x5B));
        painter.drawText(QRectF(pad, 0.0, width() - 2.0 * pad, height()),
                         Qt::AlignRight | Qt::AlignVCenter, m_stateText);
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
            update();
        return QPushButton::event(event);
    }

private:
    QString m_stateText;   /*!< 右侧状态文案 */
    bool m_current = false;/*!< 是否为当前风格 */
};

} // namespace

LottoStyleDrawer::LottoStyleDrawer(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("styleDrawer"));
    setAttribute(Qt::WA_NoSystemBackground, true);
    hide();

    m_sheet = new QWidget(this);
    m_sheet->setObjectName(QStringLiteral("styleDrawerSheet"));
    m_sheet->setAttribute(Qt::WA_StyledBackground, true);

    // 面板上沿投影(设计稿: 0 -18px 40px rgba(40,30,15,.55))
    auto *sheetShadow = new QGraphicsDropShadowEffect(m_sheet);
    sheetShadow->setBlurRadius(40);
    sheetShadow->setOffset(0, -18);
    sheetShadow->setColor(QColor(40, 30, 15, 140));
    m_sheet->setGraphicsEffect(sheetShadow);

    m_sheetLayout = new QVBoxLayout(m_sheet);
    m_sheetLayout->setContentsMargins(kSheetPad, kSheetPad, kSheetPad, kSheetPad);
    m_sheetLayout->setSpacing(kOptionGap);

    m_title = new QLabel(QStringLiteral("界面风格"), m_sheet);
    m_title->setObjectName(QStringLiteral("styleDrawerTitle"));
    m_sheetLayout->addWidget(m_title);
    m_sheetLayout->addSpacing(4);

    m_slide = new QPropertyAnimation(m_sheet, "pos", this);
    m_slide->setDuration(kSlideMs);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);
}

void LottoStyleDrawer::setStyles(const QVector<LottoStyleInfo> &styles,
                                 const QString &currentId)
{
    m_styles = styles;
    m_currentId = currentId;
    rebuildOptions();
}

void LottoStyleDrawer::rebuildOptions()
{
    // 清掉旧选项(保留标题项)
    while (m_sheetLayout->count() > 2) {
        QLayoutItem *item = m_sheetLayout->takeAt(2);
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < m_styles.size(); ++i) {
        const LottoStyleInfo &info = m_styles.at(i);
        const bool current = (info.id == m_currentId);

        auto *option = new StyleOptionButton(m_sheet);
        option->setObjectName(QStringLiteral("styleOption_%1").arg(i));
        option->setProperty("styleId", info.id);
        option->setProperty("current", current);
        QFont optionFont;
        optionFont.setFamilies({ QStringLiteral("PingFang SC"),
                                 QStringLiteral("Microsoft YaHei"),
                                 QStringLiteral("Hiragino Sans GB"),
                                 QStringLiteral("Source Han Sans SC") });
        option->setFont(optionFont);
        option->setText(info.displayName);
        option->setStateText(current ? QStringLiteral("当前")
                                     : QStringLiteral("切换"));
        option->setCurrent(current);
        option->setFixedHeight(kOptionHeight);
        option->setCursor(Qt::PointingHandCursor);
        option->setFocusPolicy(Qt::NoFocus);

        const QString id = info.id;
        connect(option, &QPushButton::clicked, this, [this, id]() {
            // 立即收起: 紧接着就要切换视图, 下滑动画看不见, 且不能把
            // 可见的抽屉留在控件树里(切回本视图时会重新露出来)
            m_open = false;
            hide();
            emit styleChosen(id);
        });

        m_sheetLayout->addWidget(option);
    }
}

int LottoStyleDrawer::sheetHeight() const
{
    const int count = qMax(1, m_styles.size());
    return kSheetPad * 2 + 18 + 4 + count * kOptionHeight + (count - 1) * kOptionGap;
}

bool LottoStyleDrawer::isOpen() const
{
    return m_open;
}

void LottoStyleDrawer::openFor(const QRect &cardRect)
{
    m_cardRect = cardRect;
    setGeometry(parentWidget() ? parentWidget()->rect() : QRect());

    const int height = sheetHeight();
    const int x = cardRect.left();
    const int w = cardRect.width();
    m_sheet->setGeometry(x, cardRect.bottom() + 1, w, height);
    m_sheet->raise();

    show();
    raise();
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    m_open = true;
    m_slide->stop();
    m_slide->setStartValue(QPoint(x, cardRect.bottom() + 1));
    m_slide->setEndValue(QPoint(x, cardRect.bottom() + 1 - height));
    m_slide->start();

    update();
}

void LottoStyleDrawer::reposition(const QRect &cardRect)
{
    m_cardRect = cardRect;
    setGeometry(parentWidget() ? parentWidget()->rect() : QRect());
    if (!m_open)
        return;
    m_sheet->setGeometry(cardRect.left(), cardRect.bottom() + 1 - sheetHeight(),
                         cardRect.width(), sheetHeight());
    update();
}

void LottoStyleDrawer::closeDrawer()
{
    if (!m_open) {
        // 已由选项点击置为非展开, 但控件可能仍可见
        if (!isHidden())
            hide();
        return;
    }
    m_open = false;

    m_slide->stop();
    m_slide->setStartValue(m_sheet->pos());
    m_slide->setEndValue(QPoint(m_cardRect.left(), m_cardRect.bottom() + 1));
    connect(m_slide, &QPropertyAnimation::finished, this, [this]() {
        if (!m_open)
            hide();
    }, Qt::SingleShotConnection);
    m_slide->start();

    update();
}

void LottoStyleDrawer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), QColor(28, 27, 25, 87));
}

void LottoStyleDrawer::mousePressEvent(QMouseEvent *event)
{
    if (!m_sheet->geometry().contains(event->pos())) {
        closeDrawer();
        emit dismissed();
    }
}

void LottoStyleDrawer::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        closeDrawer();
        emit dismissed();
    } else {
        QWidget::keyPressEvent(event);
    }
}
