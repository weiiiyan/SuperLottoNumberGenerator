#include "lottostylecatalog.h"

#include <QtGlobal>

#include "classic/lottoclassicview.h"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "neumorphic/lottoneumorphicview.h"
#endif

QVector<LottoStyleDefinition> lottoStyleCatalog()
{
    QVector<LottoStyleDefinition> catalog;

    catalog.append({ {QStringLiteral("classic"), QStringLiteral("经典")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoClassicView(host, parent);
        } });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    catalog.append({ {QStringLiteral("neumorphic"), QStringLiteral("新拟物")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoNeumorphicView(host, parent);
        } });
#endif

    return catalog;
}
