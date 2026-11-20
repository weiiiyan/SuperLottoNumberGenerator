#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "lottostylecatalog.h"
#include "lottoviewstate.h"

class LottoController;
class LottoPresenter;
class LottoStyleStore;
class LottoViewShell;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/*!
 * \brief MainWindow 窗口壳: 承载视图宿主, 不含任何展示逻辑
 *
 * 视图的创建、状态渲染、风格切换与偏好持久化全部由 LottoViewShell
 * (UI 层的视图宿主)承担; 本类只负责窗口框架(菜单栏/状态栏骨架)与把
 * 依赖转交宿主, 以便视图成为可独立替换、可独立测试的组件。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief 构造主窗口
     * \param controller 输入侧适配器(不持有所有权)
     * \param presenter 输出侧适配器(不持有所有权)
     * \param store 风格偏好端口(不持有所有权)
     * \param catalog 风格目录(默认取应用目录, 测试可注入假目录)
     * \param initialState 初始展示状态(由 Main 提供)
     * \param parent 父窗口
     */
    MainWindow(LottoController *controller, LottoPresenter *presenter,
               LottoStyleStore *store,
               QVector<LottoStyleDefinition> catalog = lottoStyleCatalog(),
               const LottoViewState &initialState = LottoViewState(),
               QWidget *parent = nullptr);
    ~MainWindow() override;

    /*! 当前窗口承载的视图宿主(供测试与调试) */
    LottoViewShell *viewShell() const;

private:
    Ui::MainWindow *ui;
    LottoViewShell *m_shell = nullptr;   /*!< 视图宿主(由本窗口持有) */
};

#endif // MAINWINDOW_H
