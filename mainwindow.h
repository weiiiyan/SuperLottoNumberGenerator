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
class LottoEngine;

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

private:
    void init();
    void rebuildLayout();                       // 横/竖屏切换时重建布局
    void updateLabelSizes();                    // 根据屏幕方向重新计算号码标签尺寸
    QHBoxLayout* createGroupRow(int group);     // 创建单组号码行（复用已有标签）
    void save() const;
    void restore();

    Ui::MainWindow *ui;
    LottoEngine    *m_lottoEngine = nullptr;

    QLabel       *m_frontLabels[5][5];
    QLabel       *m_backLabels[5][2];
    QLabel       *m_separators[5]   = {};       // 每组前/后区之间的 "+" 分隔标签
    QLabel       *m_timeLabel;
    QPushButton  *m_btnGenerate;
    QPushButton  *m_btnLock;
    QVBoxLayout  *m_mainLayout      = nullptr;
    QWidget      *m_groupsContainer = nullptr;  // 容纳五组号码的容器，横竖屏切换时内部布局重建
    QVBoxLayout  *m_groupsLayout    = nullptr;  // 容器上的布局（始终为 QVBoxLayout，不清空重建，只替换子项）

    int m_labelWidth  = 40;
    int m_labelHeight = 40;
    int m_spacing        = 6;                  // 号码之间的动态间距
    int m_separatorWidth = 18;                 // 分隔符 "+" 的动态宽度
    bool m_isLandscape = false;                 // 当前是否为横屏
    bool m_rebuilding  = false;                 // 防止 rebuildLayout 重入
    bool m_updatingSizes = false;               // 防止 updateLabelSizes 重入
};

#endif // MAINWINDOW_H
