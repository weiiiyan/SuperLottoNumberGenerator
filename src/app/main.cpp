#include <QApplication>
#include <QSettings>

#include "lottointeractor.h"
#include "lottoengine.h"
#include "lottocontroller.h"
#include "lottopresenter.h"
#include "maincatalog.h"
#include "mainwindow.h"
#include "qsettingsrepository.h"
#include "qsettingsstylestore.h"

/*!
 * \brief 组合根: 装配具体依赖后移交控制权
 *
 * 唯一接触 QSettings 具体类的点。此处可替换存储实现
 * (如测试/未来改用数据库), 业务层不受影响。装配顺序:
 * 仓储/引擎 → 输出侧适配器(实现输出端口) → 用例交互器(注入输出端口)
 * → 输入侧适配器(依赖输入边界) → 风格偏好 → 窗口(承载视图宿主)。
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QSettings settings("SuperLottoNumberGenerator.ini", QSettings::IniFormat);
    QSettingsTicketRepository repository(&settings);
    QSettingsStyleStore styleStore(&settings);              // 风格偏好(呈现层)
    LottoEngine engine;
    LottoPresenter presenter;                                   // 输出侧适配器: 实现输出端口
    LottoInteractor interactor(&repository, &engine, &presenter);  // 用例: 注入输出端口
    LottoController controller(&interactor);                    // 输入侧适配器: 依赖输入边界
    const LottoViewState seed = LottoPresenter::initialState();
    MainWindow window(&controller, &presenter, &styleStore,
                      buildStyleCatalog(), seed);
    // 用例启动由组合根发起(视图不驱动用例): 异步恢复, 不阻塞 Android 启动
    interactor.load();
    window.showMaximized();
    return a.exec();
}
