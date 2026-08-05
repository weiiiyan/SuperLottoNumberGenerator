#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "lottoticket.h"

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
 * \brief MainWindow 谦卑视图: 只负责渲染控制器推送的状态与转发用户动作
 *
 * 所有业务状态(号码/锁定/时间)由 LottoController 持有, 展示推导由
 * LottoPresenter 承担, 本类通过 ticketChanged 信号做机械的控件写入,
 * 不做任何业务判断与展示推导。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief 构造主窗口
     * \param controller 应用控制器(不持有所有权, 组合根保证生命周期)
     * \param parent 父窗口
     */
    MainWindow(LottoController *controller, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /*! 渲染控制器推送的完整票据状态(视图唯一渲染入口) */
    void onTicketChanged(const LottoTicket &ticket);

public:
    // 彩票格式常量(别名, 实际定义于领域层 LottoTicket)
    static constexpr int GROUP_COUNT   = LottoTicket::GROUP_COUNT;  // 注数(5 组)
    static constexpr int FRONT_COUNT   = LottoTicket::FRONT_COUNT;  // 前区每注号码个数(1-35)
    static constexpr int BACK_COUNT    = LottoTicket::BACK_COUNT;   // 后区每注号码个数(1-12)
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

    Ui::MainWindow *ui;
    LottoController *m_controller = nullptr;

    QLabel       *m_frontLabels[GROUP_COUNT][FRONT_COUNT];
    QLabel       *m_backLabels[GROUP_COUNT][BACK_COUNT];
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
