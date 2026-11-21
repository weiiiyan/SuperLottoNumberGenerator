#ifndef LOTTOTICKETDECOR_H
#define LOTTOTICKETDECOR_H

#include <QWidget>

/*!
 * \brief LottoPerfLine 骑缝线: 横向虚线 + 两端半圆缺口
 *
 * 宽度应等于票面全宽, 缺口圆心落在左右边界上, 一半被控件边界裁掉,
 * 与设计稿的 overflow:hidden 表现一致。
 */
class LottoPerfLine : public QWidget
{
    Q_OBJECT
public:
    /*! 构造骑缝线控件 */
    explicit LottoPerfLine(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

/*!
 * \brief LottoBarcode 票根条形码(纯装饰, 不含信息)
 */
class LottoBarcode : public QWidget
{
    Q_OBJECT
public:
    /*! 构造条形码控件 */
    explicit LottoBarcode(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

/*!
 * \brief LottoStamp 锁定印章: 斜置双框 + 文字
 *
 * 常态隐藏, 锁定时由视图 show() 并播放淡入/微放大动效。
 */
class LottoStamp : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal stampScale READ stampScale WRITE setStampScale)

public:
    /*! 构造印章控件 */
    explicit LottoStamp(QWidget *parent = nullptr);

    /*! 印章文字(默认取自展示状态) */
    void setStampText(const QString &text);
    /*! 印章缩放(用于淡入时微放大) */
    qreal stampScale() const;
    /*! 设置印章缩放并重绘 */
    void setStampScale(qreal scale);
    /*! 播放淡入 + 微放大动效 */
    void playIn();

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;            /*!< 印章文字 */
    qreal   m_scale = 1.0;     /*!< 淡入时的微放大比例 */
};

#endif // LOTTOTICKETDECOR_H
