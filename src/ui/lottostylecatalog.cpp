#include "lottostylecatalog.h"

#include "classic/lottoclassicview.h"

QVector<LottoStyleDefinition> lottoStyleCatalog()
{
    QVector<LottoStyleDefinition> catalog;

    catalog.append({ {QStringLiteral("classic"), QStringLiteral("经典")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoClassicView(host, parent);
        } });

    return catalog;
}
