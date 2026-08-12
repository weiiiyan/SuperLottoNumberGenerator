#include <QApplication>
#include <QResource>
#include <QSettings>

#include "lottointeractor.h"
#include "lottoengine.h"
#include "lottocontroller.h"
#include "lottopresenter.h"
#include "mainwindow.h"
#include "qsettingsrepository.h"

/*!
 * \brief 组合根: 装配具体依赖后移交控制权
 *
 * 唯一接触 QSettings 具体类的点。此处可替换存储实现
 * (如测试/未来改用数据库), 业务层不受影响。装配顺序:
 * 仓储/引擎 → 输出侧适配器(实现输出端口) → 用例交互器(注入输出端口)
 * → 输入侧适配器(依赖输入边界) → 视图。
 */
int main(int argc, char *argv[])
{
    // 静态库中的 qrc 资源需显式初始化: 链接器会丢弃未被引用的 qrc 对象,
    // 不调用则 :/style.qss 在运行时不存在
    Q_INIT_RESOURCE(resources);

    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QSettings settings("SuperLottoNumberGenerator.ini", QSettings::IniFormat);
    QSettingsTicketRepository repository(&settings);
    LottoEngine engine;
    LottoPresenter presenter;                                   // 输出侧适配器: 实现输出端口
    LottoInteractor interactor(&repository, &engine, &presenter);  // 用例: 注入输出端口
    LottoController controller(&interactor);                    // 输入侧适配器: 依赖输入边界
    MainWindow window(&controller, &presenter);
    // 用例启动由组合根发起(视图不驱动用例): 异步恢复, 不阻塞 Android 启动
    interactor.load();
    window.showMaximized();
    return a.exec();
}
