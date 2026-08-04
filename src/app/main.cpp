#include <QApplication>
#include <QResource>
#include <QSettings>

#include "lottocontroller.h"
#include "lottoengine.h"
#include "mainwindow.h"
#include "qsettingsrepository.h"

/*!
 * \brief 组合根: 装配具体依赖后移交控制权
 *
 * 唯一接触 QSettings 具体类的点。此处可替换存储实现
 * (如测试/未来改用数据库), 业务层不受影响。
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
    LottoController controller(&repository, &engine);
    MainWindow window(&controller);
    window.showMaximized();
    return a.exec();
}
