#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QApplication>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class LottoEngine;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void generateFiveGroups();
    void onLockToggled(bool checked);

private:
    void adaptToMobile();        // 移动端适配逻辑
    void init();                 // 初始化界面
    void save() const;           // 保存状态(android程序退出时基本不调用closeEvent，所以不能在closeEvent里保存)
    void restore();              // 恢复界面

    QLabel* m_frontLabels[5][5];  // 5组 × 前区5个
    QLabel* m_backLabels[5][2];   // 5组 × 后区2个
    QPushButton* m_btnGenerate;
    QPushButton* m_btnLock;
    QVBoxLayout* m_mainLayout;

    int m_labelWidth;
    int m_labelHeight;

    LottoEngine *m_lottoEngine = nullptr;
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
