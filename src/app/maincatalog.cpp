#include "maincatalog.h"

#include <QtGlobal>

#include "ticket/lottoticketview.h"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "neumorphic/lottoneumorphicview.h"
#endif

QVector<LottoStyleDefinition> buildStyleCatalog()
{
    QVector<LottoStyleDefinition> catalog;

    catalog.append({ {QStringLiteral("ticket"), QStringLiteral("票据")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoTicketView(host, parent);
        } });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    catalog.append({ {QStringLiteral("neumorphic"), QStringLiteral("新拟物")},
        [](LottoStyleHost *host, QWidget *parent) -> LottoView * {
            return new LottoNeumorphicView(host, parent);
        } });
#endif

    return catalog;
}
