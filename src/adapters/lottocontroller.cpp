#include "lottocontroller.h"

#include "lottoinputboundary.h"

LottoController::LottoController(LottoInputBoundary *interactor, QObject *parent)
    : QObject(parent)
    , m_interactor(interactor)
{
}

void LottoController::onGenerateRequested()
{
    if (m_interactor)
        m_interactor->generateNewTicket();
}

void LottoController::onLockRequested()
{
    if (m_interactor)
        m_interactor->toggleLock();
}
