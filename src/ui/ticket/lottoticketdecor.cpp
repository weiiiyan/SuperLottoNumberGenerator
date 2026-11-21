#include "lottoticketdecor.h"

#include <QGraphicsOpacityEffect>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

#include "lottotickettheme.h"

// ── 骑缝线 ──

LottoPerfLine::LottoPerfLine(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFixedHeight(26);
}

void LottoPerfLine::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal centerY = height() / 2.0;

    // 横向虚线: 2px 画笔, 4px 实 / 6px 空
    QPen pen(LottoTicketTheme::Rule, 2.0, Qt::CustomDashLine);
    pen.setDashPattern({ 2.0, 3.0 });
    painter.setPen(pen);
    painter.drawLine(QPointF(0, centerY), QPointF(width(), centerY));

    // 两端半圆缺口: 圆心落在左右边界上, 内阴影按设计稿
    // inset 0 1px 2px rgba(60,50,30,.25)
    const qreal radius = 8.5;
    const QColor shadow(60, 50, 30, 64);
    auto drawNotch = [&](qreal cx) {
        QPainterPath circle;
        circle.addEllipse(QPointF(cx, centerY), radius, radius);

        painter.save();
        painter.setClipPath(circle);
        painter.fillPath(circle, LottoTicketTheme::Stage);

        QLinearGradient gradient(0, centerY - radius, 0, centerY - radius + 4.0);
        gradient.setColorAt(0.0, shadow);
        gradient.setColorAt(1.0, QColor(60, 50, 30, 0));
        painter.fillRect(circle.boundingRect(), gradient);
        painter.restore();
    };
    drawNotch(0);
    drawNotch(width());
}

// ── 条形码 ──

LottoBarcode::LottoBarcode(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFixedSize(92, 20);
}

void LottoBarcode::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setOpacity(0.45);
    painter.setPen(Qt::NoPen);
    painter.setBrush(LottoTicketTheme::Ink);

    // 12px 周期的条纹组, 与设计稿 repeating-linear-gradient 一致:
    // 实 0–1 / 空 1–3 / 实 3–5 / 空 5–7 / 实 7–8 / 空 8–12
    for (int x = 0; x < width(); x += 12) {
        painter.drawRect(x + 0, 0, 1, height());
        painter.drawRect(x + 3, 0, 2, height());
        painter.drawRect(x + 7, 0, 1, height());
    }
}

// ── 锁定印章 ──

LottoStamp::LottoStamp(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFixedSize(86, 30);

    auto *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(0.0);
    setGraphicsEffect(effect);

    hide();
}

void LottoStamp::setStampText(const QString &text)
{
    m_text = text;
    update();
}

qreal LottoStamp::stampScale() const
{
    return m_scale;
}

void LottoStamp::setStampScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale))
        return;
    m_scale = scale;
    update();
}

void LottoStamp::playIn()
{
    show();
    raise();

    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(graphicsEffect());
    if (effect) {
        auto *fade = new QPropertyAnimation(effect, "opacity", this);
        fade->setDuration(220);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    }

    auto *grow = new QPropertyAnimation(this, "stampScale", this);
    grow->setDuration(220);
    grow->setEasingCurve(QEasingCurve::OutCubic);
    grow->setStartValue(0.94);
    grow->setEndValue(1.0);
    grow->start(QAbstractAnimation::DeleteWhenStopped);
}

QSize LottoStamp::sizeHint() const
{
    return QSize(86, 30);
}

void LottoStamp::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QPointF center(width() / 2.0, height() / 2.0);
    painter.translate(center);
    painter.rotate(-7.0);
    painter.scale(m_scale, m_scale);
    painter.translate(-center);

    const QRectF box(1.5, 1.5, width() - 3.0, height() - 3.0);
    const QColor stampColor(0xC1, 0x12, 0x1F);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xF7, 0xF2, 0xE6, 128));
    painter.drawRoundedRect(box, 5, 5);

    painter.setBrush(Qt::NoBrush);
    QColor border = stampColor;
    border.setAlphaF(0.50);
    painter.setPen(QPen(border, 2.0));
    painter.drawRoundedRect(box, 5, 5);

    QColor inner = stampColor;
    inner.setAlphaF(0.28);
    painter.setPen(QPen(inner, 1.0));
    painter.drawRoundedRect(box.adjusted(-3, -3, 3, 3), 7, 7);

    QFont f = font();
    f.setPixelSize(12);
    f.setWeight(QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    painter.setFont(f);
    QColor text = stampColor;
    text.setAlphaF(0.80);
    painter.setPen(text);
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}
