#ifndef LOTTOSTYLEDRAWER_H
#define LOTTOSTYLEDRAWER_H

#include <QVector>
#include <QWidget>

#include "lottostylecatalog.h"

class QLabel;
class QPropertyAnimation;
class QVBoxLayout;

/*!
 * \brief LottoStyleDrawer 风格切换底部抽屉
 *
 * 覆盖整个视图的半透明遮罩 + 贴票面底部的抽屉面板; 选项来自宿主的
 * 风格清单。设计稿要求两套主题共用同一套入口(右上角按钮 + 底部抽屉),
 * 因此抽屉是视图内部控件, 不进入核心库。
 *
 * 切换请求只发信号, 由视图转交 LottoStyleHost, 抽屉自身不感知宿主。
 */
class LottoStyleDrawer : public QWidget
{
    Q_OBJECT
public:
    /*! 构造抽屉(初始隐藏) */
    explicit LottoStyleDrawer(QWidget *parent = nullptr);

    /*! 写入可选风格与当前选中项, 重建选项按钮 */
    void setStyles(const QVector<LottoStyleInfo> &styles, const QString &currentId);
    /*! 在指定票面矩形上展开抽屉 */
    void openFor(const QRect &cardRect);
    /*! 收起抽屉(带下滑动画) */
    void closeDrawer();
    /*! 票面矩形变化时静默重排(不重播上滑动画) */
    void reposition(const QRect &cardRect);
    /*! 抽屉是否展开 */
    bool isOpen() const;

signals:
    /*! 用户选择了某个风格 */
    void styleChosen(const QString &id);
    /*! 抽屉被遮罩或返回键关闭 */
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void rebuildOptions();
    int sheetHeight() const;

    QWidget      *m_sheet = nullptr;        /*!< 抽屉面板 */
    QVBoxLayout  *m_sheetLayout = nullptr;  /*!< 面板内布局 */
    QLabel       *m_title = nullptr;        /*!< 面板标题 */
    QVector<LottoStyleInfo> m_styles;       /*!< 风格清单 */
    QString       m_currentId;              /*!< 当前风格 id */
    QRect         m_cardRect;               /*!< 票面矩形(面板贴底) */
    bool          m_open = false;           /*!< 展开状态 */
    QPropertyAnimation *m_slide = nullptr;  /*!< 上滑/下滑动画 */
};

#endif // LOTTOSTYLEDRAWER_H
