// UI 核心库的链接隔离测试
// 本目标只链接 lotto_ui: 不引入任何具体风格组件, 也不引入 Qt Quick。
//
// 下面的编译期守门是关键: Qt 模块的 PUBLIC 依赖会以宏的形式传播给下游,
// 因此只要 lotto_ui 把 Quick/Qml 作为 PUBLIC 依赖泄漏出来, 本文件就会
// 编译失败——而不是靠人工 grep 链接行。

#if defined(QT_QUICK_LIB) || defined(QT_QUICKWIDGETS_LIB) || defined(QT_QML_LIB)
#  error "lotto_ui 把 Qt Quick/Qml 泄漏给了下游: 可插拔边界被破坏"
#endif

#include <QTest>
#include <QWidget>

#include "lottostylecatalog.h"
#include "lottoview.h"
#include "lottoviewshell.h"

class TestUiIsolation : public QObject
{
    Q_OBJECT

private slots:
    void coreLinksWithoutAnyStyle()
    {
        qDebug() << "验证只链接 lotto_ui 时端口与宿主类型可用";
        LottoStyleDefinition definition;   // 注册表类型可用
        QVERIFY(!definition.create);       // 默认构造无工厂
    }
};

QTEST_MAIN(TestUiIsolation)
#include "tst_uiisolation.moc"
