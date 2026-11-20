#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottostylestore.h"
#include "lottoviewshell.h"

MainWindow::MainWindow(LottoController *controller, LottoPresenter *presenter,
                       LottoStyleStore *store,
                       QVector<LottoStyleDefinition> catalog, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("大乐透随机号码生成器"));

    // 视图宿主承载全部 UI: 视图创建/状态渲染/风格切换均由 UI 层负责。
    // setCentralWidget 接管其所有权, 无需自建占位 central widget
    m_shell = new LottoViewShell(controller, presenter, store,
                                 std::move(catalog), 200, this);
    setCentralWidget(m_shell);
}

MainWindow::~MainWindow()
{
    delete ui;
}

LottoViewShell *MainWindow::viewShell() const
{
    return m_shell;
}
