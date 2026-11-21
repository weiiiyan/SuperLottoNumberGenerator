#include "lottoballlabel.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QRadialGradient>
#include <QTimer>

namespace {

/*! 前区球面色板(高光 / 主色 / 暗部) */
const QColor kFrontLight(0xE8, 0x43, 0x4F);
const QColor kFrontCore(0xC1, 0x12, 0x1F);
const QColor kFrontDeep(0x8C, 0x0C, 0x16);

/*! 后区球面色板(高光 / 主色 / 暗部) */
const QColor kBackLight(0x2E, 0x96, 0xCC);
const QColor kBackCore(0x0B, 0x6F, 0xA4);
const QColor kBackDeep(0x06, 0x4E, 0x75);

/*! 号码文字颜色 */
const QColor kBallText(0xFF, 0xF8, 0xF4);

} // namespace

LottoBallLabel::LottoBallLabel(Area area, QWidget *parent)
    : QLabel(parent)
    , m_area(area)
{
    setAlignment(Qt::AlignCenter);
    setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_Hover, false);
    setFixedSize(40, 40);
}

qreal LottoBallLabel::popScale() const
{
    return m_popScale;
}

void LottoBallLabel::setPopScale(qreal scale)
{
    if (qFuzzyCompare(m_popScale, scale))
        return;
    m_popScale = scale;
    update();
}

void LottoBallLabel::setBallDiameter(int diameter)
{
    setFixedSize(diameter, diameter);
}

void LottoBallLabel::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
    if (!enabled && m_animation)
        m_animation->stop();
}

void LottoBallLabel::playPopIn(int delayMs)
{
    if (!m_animationEnabled) {
        setPopScale(1.0);
        return;
    }

    if (m_animation)
        m_animation->stop();
    setPopScale(0.72);

    const int generation = ++m_popGeneration;
    QTimer::singleShot(delayMs, this, [this, generation]() {
        if (generation != m_popGeneration)
            return;   // 期间又换了号, 这次动效作废
        if (!m_animation) {
            m_animation = new QPropertyAnimation(this, "popScale", this);
            m_animation->setDuration(340);
            QEasingCurve curve(QEasingCurve::OutBack);
            curve.setAmplitude(0.62);
            m_animation->setEasingCurve(curve);
        }
        m_animation->stop();
        m_animation->setStartValue(qreal(0.72));
        m_animation->setEndValue(qreal(1.0));
        m_animation->start();
    });
}

QSize LottoBallLabel::sizeHint() const
{
    return QSize(40, 40);
}

void LottoBallLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // 以控件中心为基准缩放, 弹入时球与数字一起放大
    const QPointF center(width() / 2.0, height() / 2.0);
    painter.translate(center);
    painter.scale(m_popScale, m_popScale);
    painter.translate(-center);

    const qreal d = qMin(width(), height());
    const QRectF circle(0.5, 0.5, d - 1.0, d - 1.0);

    QRadialGradient ballGradient(QPointF(circle.left() + circle.width() * 0.34,
                                         circle.top() + circle.height() * 0.22),
                                 circle.width() * 1.25);
    if (m_area == Front) {
        ballGradient.setColorAt(0.0, kFrontLight);
        ballGradient.setColorAt(0.52, kFrontCore);
        ballGradient.setColorAt(1.0, kFrontDeep);
    } else {
        ballGradient.setColorAt(0.0, kBackLight);
        ballGradient.setColorAt(0.52, kBackCore);
        ballGradient.setColorAt(1.0, kBackDeep);
    }

    QPainterPath path;
    path.addEllipse(circle);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ballGradient);
    painter.drawPath(path);

    // 下侧暗部: 近似 CSS 的 inset 0 -7px 11px
    painter.save();
    painter.setClipPath(path);
    QLinearGradient shade(circle.topLeft(), circle.bottomLeft());
    const QColor shadeColor = (m_area == Front) ? QColor(70, 0, 6, 96)
                                                : QColor(0, 20, 35, 96);
    shade.setColorAt(0.55, QColor(0, 0, 0, 0));
    shade.setColorAt(1.0, shadeColor);
    painter.fillRect(circle, shade);
    painter.restore();

    // inset 白色描边
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255, 66), 1.0));
    painter.drawEllipse(circle.adjusted(0.5, 0.5, -0.5, -0.5));

    // 号码: 两位数字, 白色, 17px 粗体
    QFont f = font();
    f.setPixelSize(qMax(10, qRound(d * 0.425)));
    f.setWeight(QFont::Bold);
    f.setFamilies({ QStringLiteral("Bahnschrift"), QStringLiteral("DIN Alternate"),
                    QStringLiteral("Arial Narrow"), QStringLiteral("Arial") });
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    f.setFeature(QFont::Tag("tnum"), 1);   // 等宽数字
#endif
    painter.setFont(f);
    painter.setPen(kBallText);
    painter.drawText(rect(), Qt::AlignCenter, text());
}
