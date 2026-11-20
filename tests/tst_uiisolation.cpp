// UI 核心库的链接隔离测试
// 本目标只链接 lotto_ui: 不引入任何具体风格组件, 也不引入 Qt Quick。
// 若哪天核心库又直接编入某个风格, 这里的链接行会最先暴露问题。

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
