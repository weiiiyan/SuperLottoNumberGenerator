#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class QPushButton;
class QVBoxLayout;
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
    void adaptToMobile();
    void init();
    void save() const;   // Android 退出时不触发 closeEvent，须在锁定时主动保存
    void restore();

    Ui::MainWindow *ui;
    LottoEngine    *m_lottoEngine = nullptr;

    QLabel       *m_frontLabels[5][5];
    QLabel       *m_backLabels[5][2];
    QLabel       *m_timeLabel;
    QPushButton  *m_btnGenerate;
    QPushButton  *m_btnLock;
    QVBoxLayout  *m_mainLayout;

    int m_labelWidth;
    int m_labelHeight;
};

#endif // MAINWINDOW_H
