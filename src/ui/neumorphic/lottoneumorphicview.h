#ifndef LOTTONEUMORPHICVIEW_H
#define LOTTONEUMORPHICVIEW_H

#include <QPointer>

#include "lottoview.h"

class LottoStyleHost;
class QQuickItem;
class QQuickWidget;

/*!
 * \brief LottoNeumorphicView 新拟物风格视图(QML + QQuickWidget)
 *
 * 组合而非继承 QQuickWidget(QQuickWidget 自身是 QWidget, 无法与
 * LottoView 多重继承): 内部承载 QML 场景, 把展示状态写为根对象属性,
 * 把 QML 交互信号回传为 LottoView 信号与风格切换请求。
 */
class LottoNeumorphicView : public LottoView
{
    Q_OBJECT
public:
    /*! 风格 id */
    static constexpr const char *STYLE_ID = "neumorphic";

    /*!
     * \brief 构造新拟物视图
     * \param host 宿主端口(不持有所有权)
     * \param parent 父控件
     */
    explicit LottoNeumorphicView(LottoStyleHost *host, QWidget *parent = nullptr);

    /*! 本视图风格 id */
    QString styleId() const override;
    /*! 缓存状态并在 QML 就绪后写入 */
    void renderState(const LottoViewState &state) override;
    /*! QML 加载中视为可用; 加载出错则不可用 */
    bool isReady() const override;
    /*! 更新 QML 侧当前风格选中态 */
    void styleChanged(const QString &currentStyleId) override;
    /*! 快照: 优先取 QQuickWidget 帧缓冲 */
    QPixmap snapshot() const override;

    /*! 内部 QQuickWidget(供测试) */
    QQuickWidget *quickWidget() const;
    /*! QML 根对象(未就绪时为空, 供测试) */
    QQuickItem *rootItem() const;

private slots:
    /*! QML 请求切换风格: 转交宿主端口 */
    void onStyleSwitchRequested(const QString &styleId);

private:
    void pushStyles();
    void pushStateToQml();
    void connectRootSignals();

    LottoStyleHost *m_host = nullptr;   /*!< 宿主端口(不持有所有权) */
    QQuickWidget *m_quick = nullptr;    /*!< QML 承载控件(本视图持有) */
    LottoViewState m_state;             /*!< 最近一次展示状态 */
    bool m_hasState = false;            /*!< 是否已收到过展示状态 */
};

#endif // LOTTONEUMORPHICVIEW_H
