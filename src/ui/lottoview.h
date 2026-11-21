#ifndef LOTTOVIEW_H
#define LOTTOVIEW_H

#include <QMargins>
#include <QPixmap>
#include <QWidget>

#include "lottoviewstate.h"

/*!
 * \brief LottoView 视图端口: 所有风格的视图都实现本接口
 *
 * 视图只消费展示状态 LottoViewState, 并把用户操作翻译为信号; 信号由
 * 组合根(Main)接线到输入侧适配器。视图不感知 LottoTicket / LottoInteractor /
 * TicketRepository, 也不认识其它风格的视图。风格切换交互由视图自行
 * 实现(经 LottoStyleHost 端口), 因此各风格观感可独立演进。
 */
class LottoView : public QWidget
{
    Q_OBJECT
public:
    /*! 构造视图控件 */
    explicit LottoView(QWidget *parent = nullptr);

    /*! 本视图的风格 id(与风格目录条目一致) */
    virtual QString styleId() const = 0;
    /*! 用展示状态渲染控件(组合根创建视图时渲染首帧, 之后由展示器信号驱动) */
    virtual void renderState(const LottoViewState &state) = 0;
    /*! 本视图当前能否展示(异步初始化的视图覆写), 默认恒真 */
    virtual bool isReady() const;
    /*! 风格选中态变化通知(宿主在切换完成后调用), 默认空实现 */
    virtual void styleChanged(const QString &currentStyleId);
    /*! 当前画面快照, 供切换过渡使用(默认抓取本控件) */
    virtual QPixmap snapshot() const;

protected:
    /*!
     * \brief 顶层窗口的安全区(桌面/普通窗口为 QMargins())
     *
     * 全屏(edge-to-edge)时状态栏与手势条会覆盖窗口上下边缘, 返回的
     * 边距即内容需避让的最小距离; 视图据此放大自己的安全边距。
     */
    QMargins windowSafeArea() const;

signals:
    /*! 用户请求生成新号码 */
    void generateRequested();
    /*! 用户请求切换锁定状态 */
    void lockRequested();
};

#endif // LOTTOVIEW_H
