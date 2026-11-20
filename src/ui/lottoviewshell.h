#ifndef LOTTOVIEWSHELL_H
#define LOTTOVIEWSHELL_H

#include <QPointer>
#include <QWidget>

#include "lottopresenter.h"
#include "lottostylecatalog.h"
#include "lottostylehost.h"
#include "lottostylestore.h"

class LottoController;
class LottoView;
class QStackedWidget;

/*!
 * \brief LottoViewShell 视图宿主: 承载可切换的风格视图
 *
 * 职责: 按风格目录懒创建视图, 缓存并重放最近一次展示状态, 把视图的
 * 用户请求转发给输入侧适配器, 接收视图的切换请求并在切换后持久化偏好。
 * 视图之间互不认识, 只通过本类提供的 LottoStyleHost 端口感知"有哪些
 * 风格、当前是哪个、如何切换"。
 */
class LottoViewShell : public QWidget, public LottoStyleHost
{
    Q_OBJECT
public:
    /*!
     * \brief 构造宿主
     * \param controller 输入侧适配器(不持有所有权)
     * \param presenter 输出侧适配器(不持有所有权)
     * \param store 风格偏好端口(不持有所有权)
     * \param catalog 风格目录, 顺序即展示顺序, 首项为默认风格
     * \param transitionMs 切换过渡时长(毫秒), <=0 表示瞬时切换
     * \param parent 父控件
     */
    LottoViewShell(LottoController *controller, LottoPresenter *presenter,
                   LottoStyleStore *store,
                   QVector<LottoStyleDefinition> catalog,
                   int transitionMs = 200, QWidget *parent = nullptr);

    /*! 全部可用风格(顺序即展示顺序) */
    QVector<LottoStyleInfo> availableStyles() const override;
    /*! 当前风格 id, 目录为空时为空串 */
    QString currentStyleId() const override;
    /*! 请求切换风格: 校验 id 后排队执行, 失败经 styleSwitchFailed 上报 */
    void requestSwitchTo(const QString &styleId) override;

    /*! 当前显示的视图(供测试与调试), 目录为空时为空 */
    LottoView *currentView() const;
    /*! 构造时注入的过渡时长 */
    int transitionMs() const;

signals:
    /*! 目标视图缺失、工厂返回空或视图未就绪: 保持当前视图且不写偏好 */
    void styleSwitchFailed(const QString &styleId);

private:
    void applySwitch(const QString &styleId);
    LottoView *createView(const LottoStyleDefinition &definition);
    LottoView *findExistingView(const QString &styleId) const;

    LottoController *m_controller = nullptr;   /*!< 输入侧适配器(不持有所有权) */
    LottoPresenter *m_presenter = nullptr;     /*!< 输出侧适配器(不持有所有权) */
    LottoStyleStore *m_store = nullptr;        /*!< 风格偏好端口(不持有所有权) */
    QVector<LottoStyleDefinition> m_catalog;   /*!< 风格目录 */
    QStackedWidget *m_stack = nullptr;         /*!< 视图容器 */
    QPointer<LottoView> m_currentView;         /*!< 当前视图 */
    QString m_currentStyleId;                  /*!< 当前风格 id */
    LottoViewState m_lastState;                /*!< 最近一次展示状态 */
    bool m_hasState = false;                   /*!< 是否已收到过展示状态 */
    int m_transitionMs = 200;                  /*!< 过渡时长 */
};

#endif // LOTTOVIEWSHELL_H
