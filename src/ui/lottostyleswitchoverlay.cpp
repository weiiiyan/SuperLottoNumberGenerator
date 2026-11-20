#include "lottostyleswitchoverlay.h"

#include <QPainter>
#include <QPaintEvent>

LottoStyleSwitchOverlay::LottoStyleSwitchOverlay(const QPixmap &pixmap,
                                                 QWidget *parent)
    : QWidget(parent)
    , m_pixmap(pixmap)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);

    if (parent)
        setGeometry(parent->rect());
    raise();
}

void LottoStyleSwitchOverlay::setOpacity(qreal opacity)
{
    const qreal clamped = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(clamped, m_opacity))
        return;
    m_opacity = clamped;
    update();
}

void LottoStyleSwitchOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_pixmap.isNull())
        return;

    QPainter painter(this);
    painter.setOpacity(m_opacity);
    // 位图按当前控件尺寸缩放铺满(切换期间窗口尺寸可能变化)
    painter.drawPixmap(rect(), m_pixmap,
                       m_pixmap.rect().adjusted(0, 0, 0, 0));
}
