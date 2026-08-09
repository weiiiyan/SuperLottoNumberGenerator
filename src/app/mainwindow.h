#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "lottopresenter.h"

class LottoController;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;

/*!
 * \brief MainWindow 谦卑视图: 只消费展示状态渲染控件, 按钮点击转发给控制器
 *
 * 所有业务状态(号码/锁定/时间)由 LottoInteractor 持有, 展示推导由
 * LottoPresenter(输出侧适配器)承担, 本类只消费 viewStateChanged
 * (LottoViewState) 信号做机械的控件写入, 不感知领域实体 LottoTicket,
 * 不做任何业务判断与展示推导。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief 构造主窗口
     * \param controller 输入侧适配器(不持有所有权, 组合根保证生命周期)
     * \param presenter 输出侧适配器(不持有所有权, 组合根保证生命周期)
     * \param parent 父窗口
     */
    MainWindow(LottoController *controller, LottoPresenter *presenter,
               QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /*! 渲染展示器推送的展示状态(视图唯一渲染入口) */
    void onViewStateChanged(const LottoViewState &state);

public:
    // 彩票格式常量(别名, 实际定义于展示器 LottoPresenter, 其别名领域层 LottoTicket)
    static constexpr int GROUP_COUNT   = LottoPresenter::GROUP_COUNT;  // 注数(5 组)
    static constexpr int FRONT_COUNT   = LottoPresenter::FRONT_COUNT;  // 前区每注号码个数(1-35)
    static constexpr int BACK_COUNT    = LottoPresenter::BACK_COUNT;   // 后区每注号码个数(1-12)
    static constexpr int LABELS_PER_ROW = FRONT_COUNT + BACK_COUNT;  // 每行标签总数 = 7

    // 竖屏宽度分档阈值(逻辑像素)
    static constexpr int WIDTH_TIER_XS = 280;  // ~iPhone SE / 小屏 Android
    static constexpr int WIDTH_TIER_SM = 350;  // ~iPhone 6/7/8
    static constexpr int WIDTH_TIER_MD = 440;  // ~iPhone Plus / 中屏 Android

    // 标签尺寸约束
    static constexpr int LABEL_MAX_SIZE = 44;           // 标签尺寸上限
    static constexpr int LABEL_MIN_SIZE_PORTRAIT = 22;  // 标签尺寸下限
    // 宽度安全系数(消化 QLabel 内边距和系统圆角)
    static constexpr double LABEL_ROW_WIDTH_FRACTION = 0.94;

private:
    void init();
    void buildLayout();                                // 构建固定竖屏布局(仅在 init 中调用一次)
    void rebuildGroupRows();                           // 间距档位变化时重建组行
    void updateLabelSizes(int availableWidth);
    QHBoxLayout* createGroupRow(int group);
    void applyState(const LottoViewState &state);      // 渲染唯一入口(机械控件写入)

    Ui::MainWindow *ui;
    LottoController *m_controller = nullptr;  /*!< 输入侧适配器(不持有所有权) */
    LottoPresenter *m_presenter = nullptr;    /*!< 输出侧适配器(不持有所有权) */

    QLabel       *m_frontLabels[GROUP_COUNT][FRONT_COUNT] = {};
    QLabel       *m_backLabels[GROUP_COUNT][BACK_COUNT] = {};
    QLabel       *m_separators[GROUP_COUNT] = {};
    QLabel       *m_timeLabel;
    QPushButton  *m_btnGenerate;
    QPushButton  *m_btnLock;
    QVBoxLayout  *m_mainLayout      = nullptr;
    QWidget      *m_groupsContainer = nullptr;
    QVBoxLayout  *m_groupsLayout    = nullptr;

    int m_labelWidth     = 40;
    int m_spacing        = 6;
    int m_separatorWidth = 18;

    // 布局防抖: 避免 Android 端 resizeEvent 自激震荡
    QTimer *m_layoutDebounceTimer = nullptr;
    int     m_lastAppliedWidth    = -1;   // 上次实际执行的可用宽度, 相同则跳过
};

#endif // MAINWINDOW_H
