#ifndef LOTTOBALLLABEL_H
#define LOTTOBALLLABEL_H

#include <QLabel>

class QPropertyAnimation;

/*!
 * \brief LottoBallLabel 自绘号码球
 *
 * QSS 无法表达多重 inset 阴影, 因此球面由 paintEvent 用 QRadialGradient
 * 双点绘制: 左上高光 + 底色 + 右下暗部, 再叠一圈 inset 白色描边。保留
 * QLabel 基类, 使既有测试仍可按类型与 objectName 定位控件。
 *
 * popScale 供"换号弹入"动效使用: 0.72 → 1.0, 由视图按球序错峰启动。
 */
class LottoBallLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(qreal popScale READ popScale WRITE setPopScale)

public:
    /*! 号码区归属, 决定球面色板 */
    enum Area { Front, Back };

    /*! 构造号码球 */
    explicit LottoBallLabel(Area area, QWidget *parent = nullptr);

    /*! 弹入缩放比例(1.0 为常态) */
    qreal popScale() const;
    /*! 设置弹入缩放比例并重绘 */
    void setPopScale(qreal scale);

    /*! 设置球径(同时固定控件尺寸) */
    void setBallDiameter(int diameter);
    /*! 按球序延迟启动弹入动效 */
    void playPopIn(int delayMs);
    /*! 关闭/开启动效(测试与减少动态效果场景) */
    void setAnimationEnabled(bool enabled);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Area  m_area = Front;
    qreal m_popScale = 1.0;
    bool  m_animationEnabled = true;
    int   m_popGeneration = 0;   /*!< 弹入代次, 作废旧的一次性定时器 */
    QPropertyAnimation *m_animation = nullptr;
};

#endif // LOTTOBALLLABEL_H
