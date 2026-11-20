#ifndef LOTTOCLASSICVIEW_H
#define LOTTOCLASSICVIEW_H

#include <QMenu>
#include <QToolButton>

#include "lottoview.h"

class LottoStyleHost;
class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;

/*!
 * \brief LottoClassicView 经典风格视图: 号码色块 + QSS 样式的 Widgets 实现
 *
 * 由原 MainWindow 的视图逻辑抽取而来, 保留既有控件 objectName 以便
 * 既有集成测试按名定位。只消费 LottoViewState 做机械写入; 按钮点击
 * 发出 generateRequested / lockRequested; 风格切换交互为右上角按钮
 * 弹出可选菜单。
 */
class LottoClassicView : public LottoView
{
    Q_OBJECT
public:
    /*! 风格 id */
    static constexpr const char *STYLE_ID = "classic";

    /*!
     * \brief 构造经典视图
     * \param host 宿主端口(不持有所有权, 用于枚举与请求风格切换)
     * \param parent 父控件
     */
    explicit LottoClassicView(LottoStyleHost *host, QWidget *parent = nullptr);

    /*! 本视图风格 id */
    QString styleId() const override;
    /*! 用展示状态渲染控件 */
    void renderState(const LottoViewState &state) override;
    /*! 更新风格菜单勾选态 */
    void styleChanged(const QString &currentStyleId) override;

protected:
    void resizeEvent(QResizeEvent *event) override;

public:
    // 彩票格式常量(别名展示器, 其别名领域层规则)
    static constexpr int GROUP_COUNT    = LottoPresenter::GROUP_COUNT;
    static constexpr int FRONT_COUNT    = LottoPresenter::FRONT_COUNT;
    static constexpr int BACK_COUNT     = LottoPresenter::BACK_COUNT;
    static constexpr int LABELS_PER_ROW = FRONT_COUNT + BACK_COUNT;

    // 竖屏宽度分档阈值(逻辑像素)
    static constexpr int WIDTH_TIER_XS = 280;
    static constexpr int WIDTH_TIER_SM = 350;
    static constexpr int WIDTH_TIER_MD = 440;

    // 标签尺寸约束
    static constexpr int LABEL_MAX_SIZE = 44;
    static constexpr int LABEL_MIN_SIZE_PORTRAIT = 22;
    static constexpr double LABEL_ROW_WIDTH_FRACTION = 0.94;

private:
    void initUi();
    void buildLayout();
    void rebuildGroupRows();
    void updateLabelSizes(int availableWidth);
    QHBoxLayout* createGroupRow(int group);
    void buildStyleMenu();
    void applyState(const LottoViewState &state);

    LottoStyleHost *m_host = nullptr;   /*!< 宿主端口(不持有所有权) */

    QLabel      *m_frontLabels[GROUP_COUNT][FRONT_COUNT] = {};
    QLabel      *m_backLabels[GROUP_COUNT][BACK_COUNT] = {};
    QLabel      *m_separators[GROUP_COUNT] = {};
    QLabel      *m_timeLabel = nullptr;
    QLabel      *m_timeValueLabel = nullptr;
    QPushButton *m_btnGenerate = nullptr;
    QPushButton *m_btnLock = nullptr;
    QToolButton *m_btnStyleSwitch = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
    QWidget     *m_groupsContainer = nullptr;
    QVBoxLayout *m_groupsLayout = nullptr;

    int m_labelWidth = 40;
    int m_spacing = 6;
    int m_separatorWidth = 18;

    QTimer *m_layoutDebounceTimer = nullptr;   /*!< 布局防抖定时器 */
    int m_lastAppliedWidth = -1;               /*!< 上次实际应用的可用宽度 */
};

#endif // LOTTOCLASSICVIEW_H
