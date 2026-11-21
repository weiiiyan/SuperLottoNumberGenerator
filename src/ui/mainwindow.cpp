#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottostylestore.h"
#include "lottoviewshell.h"

MainWindow::MainWindow(LottoController *controller, LottoPresenter *presenter,
                       LottoStyleStore *store,
                       QVector<LottoStyleDefinition> catalog,
                       const LottoViewState &initialState, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("大乐透随机号码生成器"));

    // 视图宿主承载全部 UI: 视图创建/状态渲染/风格切换均由 UI 层负责。
    // setCentralWidget 接管其所有权, 无需自建占位 central widget
    m_shell = new LottoViewShell(controller, presenter, store,
                                 std::move(catalog), initialState, 200, this);
    setCentralWidget(m_shell);

    // Android: 请求把客户区扩展到系统栏区域(edge-to-edge)。设计骨架的
    // 上/下安全边距(40/34)即充当状态栏与手势条的安全区; 否则窗口先被
    // 系统栏压小, 再叠一次设计边距, 上下各多出一条空白带。
#if defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    setWindowFlag(Qt::ExpandedClientAreaHint, true);
    // Qt 6.9 起顶层窗口默认把安全区并入 contentsMargins, 中央控件会被
    // 系统栏再缩一次; 安全区已由设计骨架的 40/34 承担, 这里关闭自动并入。
    // 必须在窗口创建(show)之前设置, 否则属性已被自动置位。
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    // 设计骨架没有状态栏, 它会在底部白占一行高度
    statusBar()->hide();
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

LottoViewShell *MainWindow::viewShell() const
{
    return m_shell;
}
