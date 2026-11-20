#ifndef LOTTOSTYLESWITCHOVERLAY_H
#define LOTTOSTYLESWITCHOVERLAY_H

#include <QPixmap>
#include <QWidget>

/*!
 * \brief LottoStyleSwitchOverlay 风格切换过渡覆盖层
 *
 * 持有切换前的视图快照位图, 由宿主用动画驱动其不透明度从 1 降到 0,
 * 形成"旧画面淡出、新画面显现"的过渡。不接收鼠标事件。
 */
class LottoStyleSwitchOverlay : public QWidget
{
    Q_OBJECT
public:
    /*!
     * \brief 构造覆盖层
     * \param pixmap 切换前的视图快照
     * \param parent 父控件(宿主, 覆盖层铺满父控件)
     */
    explicit LottoStyleSwitchOverlay(const QPixmap &pixmap, QWidget *parent);

    /*! 设置绘制不透明度(1 为完全不透明) */
    void setOpacity(qreal opacity);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_pixmap;      /*!< 切换前快照 */
    qreal m_opacity = 1.0; /*!< 当前不透明度 */
};

#endif // LOTTOSTYLESWITCHOVERLAY_H
