#include "maincatalog.h"

#include <QObject>
#include <QtGlobal>

#include "lottocontroller.h"
#include "lottopresenter.h"
#include "lottoview.h"
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

QVector<LottoStyleDefinition> wireStyleCatalog(
    QVector<LottoStyleDefinition> catalog,
    LottoController *controller,
    LottoPresenter *presenter)
{
    Q_ASSERT(controller);
    Q_ASSERT(presenter);

    QVector<LottoStyleDefinition> wired;
    wired.reserve(catalog.size());
    for (const LottoStyleDefinition &definition : catalog) {
        LottoStyleDefinition entry = definition;
        const auto create = definition.create;
        entry.create = [create, controller, presenter](LottoStyleHost *host,
                                                       QWidget *parent) -> LottoView * {
            LottoView *view = create ? create(host, parent) : nullptr;
            if (!view)
                return nullptr;
            QObject::connect(presenter, &LottoPresenter::viewStateChanged,
                             view, &LottoView::renderState);
            QObject::connect(view, &LottoView::generateRequested,
                             controller, &LottoController::onGenerateRequested);
            QObject::connect(view, &LottoView::lockRequested,
                             controller, &LottoController::onLockRequested);
            view->renderState(presenter->state());
            return view;
        };
        wired.append(entry);
    }
    return wired;
}
