#ifndef LOTTOSTYLEHOST_H
#define LOTTOSTYLEHOST_H

#include <QString>
#include <QVector>

/*!
 * \brief LottoStyleInfo 一种可用风格的标识与展示名
 *
 * id 为稳定标识(用于持久化与切换), displayName 供视图直接展示。
 */
struct LottoStyleInfo {
    QString id;            /*!< 稳定 id, 如 "ticket" / "neumorphic" */
    QString displayName;   /*!< 展示名, 如 "票据" / "新拟物" */
};

/*!
 * \brief LottoStyleHost 宿主端口: 向视图提供风格清单与切换能力
 *
 * 后台(视图宿主)提供"可选项 + 切换功能", 由视图自行决定切换交互的
 * 外观与位置。本接口不继承 QObject: 宿主同时是 QWidget, 避免 QObject
 * 菱形继承; 风格变化通过 LottoView::styleChanged() 回调视图。
 */
class LottoStyleHost
{
public:
    virtual ~LottoStyleHost() = default;

    /*! 全部可用风格(顺序即展示顺序) */
    virtual QVector<LottoStyleInfo> availableStyles() const = 0;
    /*! 当前风格 id */
    virtual QString currentStyleId() const = 0;
    /*! 请求切换到指定风格(非法 id 由宿主忽略并上报失败) */
    virtual void requestSwitchTo(const QString &styleId) = 0;
};

#endif // LOTTOSTYLEHOST_H
