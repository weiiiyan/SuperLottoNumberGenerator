#ifndef LOTTOTICKETVIEW_H
#define LOTTOTICKETVIEW_H

#include <QPointer>
#include <QPixmap>
#include <QRect>
#include <QStringList>
#include <QVector>
#include <QWindow>

#include "lottoview.h"

class LottoBallLabel;
class LottoStyleHost;
class LottoBarcode;
class LottoPerfLine;
class LottoStamp;
class LottoStyleDrawer;
class QFrame;
class QLabel;
class QPainter;
class QPushButton;
class QShowEvent;
class QTimer;
class QVBoxLayout;

/*!
 * \brief LottoTicketView 票据风格视图
 *
 * Widgets + QSS 实现, 按 390 × 844 竖屏骨架排布: 顶栏 → 时间条 → 号码带
 * → 骑缝线 → 两行票根 → 操作区。号码球、骑缝线、条形码与锁定印章为
 * 自绘控件(QSS 无法表达多重 inset 阴影与条纹)。桌面宽窗口下票面居中成
 * 卡片, 窄屏(Android)下满幅铺开。
 *
 * 只消费 LottoViewState 做机械写入; 按钮点击发出 generateRequested /
 * lockRequested; 风格切换为右上角按钮 + 底部抽屉。
 */
class LottoTicketView : public LottoView
{
    Q_OBJECT
public:
    /*! 风格 id */
    static constexpr const char *STYLE_ID = "ticket";

    /*!
     * \brief 构造票据视图
     * \param host 宿主端口(不持有所有权, 用于枚举与请求风格切换)
     * \param parent 父控件
     */
    explicit LottoTicketView(LottoStyleHost *host, QWidget *parent = nullptr);

    /*! 本视图风格 id */
    QString styleId() const override;
    /*! 用展示状态渲染控件 */
    void renderState(const LottoViewState &state) override;
    /*! 更新风格按钮文案并关闭抽屉 */
    void styleChanged(const QString &currentStyleId) override;

public:
    // 彩票格式常量(别名展示器, 其别名领域层规则)
    static constexpr int GROUP_COUNT    = LottoViewShape::GROUP_COUNT;
    static constexpr int FRONT_COUNT    = LottoViewShape::FRONT_COUNT;
    static constexpr int BACK_COUNT     = LottoViewShape::BACK_COUNT;
    static constexpr int LABELS_PER_ROW = FRONT_COUNT + BACK_COUNT;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initUi();
    void buildLayout();
    void applyState(const LottoViewState &state);
    void updateCardRect();
    void paintBackdrop(QPainter &painter);
    void attachSafeAreaListener();
    void recomputeMetrics();
    void applyMetricsToWidgets();
    void positionOverlays();
    void positionStamp();
    void updateStyleButtonText(const QString &currentStyleId);
    void openStyleDrawer();

    LottoStyleHost *m_host = nullptr;   /*!< 宿主端口(不持有所有权) */

    QFrame      *m_topBar = nullptr;         /*!< 顶栏 */
    QLabel      *m_brandMark = nullptr;      /*!< 品牌圆点 */
    QLabel      *m_brandName = nullptr;      /*!< 应用名 */
    QLabel      *m_brandMeta = nullptr;      /*!< 顶栏副标题 */
    QPushButton *m_btnStyleSwitch = nullptr; /*!< 右上角风格按钮 */

    QFrame      *m_metaStrip = nullptr;      /*!< 时间条 */
    QLabel      *m_timeLabel = nullptr;      /*!< 时间标签标题 */
    QLabel      *m_timeValueLabel = nullptr; /*!< 时间值 */

    QWidget     *m_groupsContainer = nullptr;             /*!< 号码带容器 */
    QVBoxLayout *m_groupsLayout = nullptr;                /*!< 号码带纵向布局 */
    QFrame      *m_rowWidgets[GROUP_COUNT] = {};          /*!< 单行容器 */
    QLabel      *m_rowIndexLabels[GROUP_COUNT] = {};      /*!< 行号 01–05 */
    LottoBallLabel *m_frontLabels[GROUP_COUNT][FRONT_COUNT] = {};
    LottoBallLabel *m_backLabels[GROUP_COUNT][BACK_COUNT] = {};
    QLabel      *m_separators[GROUP_COUNT] = {};

    QWidget     *m_perfHost = nullptr;   /*!< 骑缝线占位(布局占 26px) */
    LottoPerfLine *m_perf = nullptr;     /*!< 骑缝线自绘控件(票面全宽) */

    QWidget     *m_stub = nullptr;       /*!< 两行票根容器 */
    QLabel      *m_serialLabel = nullptr;/*!< 票号 */
    LottoBarcode *m_barcode = nullptr;   /*!< 条形码 */
    QFrame      *m_stubHairline = nullptr;/*!< 票根两行之间的 hairline */
    QLabel      *m_stubHint = nullptr;   /*!< 提示文案 */
    QLabel      *m_stubCount = nullptr;  /*!< 注数 */
    LottoStamp  *m_stamp = nullptr;      /*!< 锁定印章 */

    QWidget     *m_actions = nullptr;    /*!< 操作区容器 */
    QPushButton *m_btnLock = nullptr;    /*!< 锁定按钮 */
    QPushButton *m_btnGenerate = nullptr;/*!< 生成按钮 */

    LottoStyleDrawer *m_drawer = nullptr;/*!< 风格切换抽屉 */
    QVBoxLayout *m_mainLayout = nullptr; /*!< 主布局 */

    QTimer *m_layoutDebounceTimer = nullptr; /*!< 布局防抖定时器 */
    QSize   m_lastAppliedSize;               /*!< 上次实际应用的尺寸 */
    QPointer<QWindow> m_safeAreaWindow;      /*!< 已监听安全区的顶层窗口 */

    QRect   m_cardRect;      /*!< 票面矩形(桌面居中卡片 / 窄屏满幅) */
    QPixmap m_backdrop;      /*!< 票面背景缓存(仅随尺寸失效) */
    int     m_ballSize = 40; /*!< 当前球径 */
    int     m_rowHeight = 76;/*!< 当前行高 */
    int     m_padTop = 40;   /*!< 当前上边距(窗口变矮时压缩) */
    int     m_padBottom = 34;/*!< 当前下边距 */

    QVector<QStringList> m_lastGroups; /*!< 上次号码, 用于弹入动效判定 */
    bool    m_lastLocked = false;      /*!< 上次锁定状态 */
};

#endif // LOTTOTICKETVIEW_H
