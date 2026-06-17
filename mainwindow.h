#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class LottoEngine;

/// 号码格式化：零填充两位数字（如 3 → "03"），供 UI 与测试复用
inline QString formatNumber(int n)
{
    return QString::number(n).rightJustified(2, '0');
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void generateFiveGroups();
    void onLockToggled(bool checked);

public:
    // 彩票格式常量
    static constexpr int GROUP_COUNT   = 5;    // 注数（5 组）
    static constexpr int FRONT_COUNT   = 5;    // 前区每注号码个数（1–35）
    static constexpr int BACK_COUNT    = 2;    // 后区每注号码个数（1–12）
    static constexpr int LABELS_PER_ROW = FRONT_COUNT + BACK_COUNT;  // 每行标签总数 = 7

    // 竖屏宽度分档阈值（逻辑像素）
    static constexpr int WIDTH_TIER_XS = 280;  // ~iPhone SE / 小屏 Android
    static constexpr int WIDTH_TIER_SM = 350;  // ~iPhone 6/7/8
    static constexpr int WIDTH_TIER_MD = 440;  // ~iPhone Plus / 中屏 Android

    // 标签尺寸约束
    static constexpr int LABEL_MAX_SIZE = 44;           // 标签尺寸上限
    static constexpr int LABEL_MIN_SIZE_PORTRAIT = 22;  // 标签尺寸下限
    // 宽度安全系数（消化 QLabel 内边距和系统圆角）
    static constexpr double LABEL_ROW_WIDTH_FRACTION = 0.94;

private:
    void init();
    void buildLayout();                                // 构建固定竖屏布局（仅在 init 中调用一次）
    void rebuildGroupRows();                           // 间距档位变化时重建组行
    void updateLabelSizes(int availableWidth);
    QHBoxLayout* createGroupRow(int group);
    void save() const;
    void restore();

    Ui::MainWindow *ui;
    LottoEngine    *m_lottoEngine = nullptr;

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

    // 布局防抖：避免 Android 端 resizeEvent 自激震荡
    QTimer *m_layoutDebounceTimer = nullptr;
    int     m_lastAppliedWidth    = -1;   // 上次实际执行的可用宽度，相同则跳过
};

#endif // MAINWINDOW_H
