#include "lottocontroller.h"

#include "lottoinputboundary.h"

LottoController::LottoController(LottoInputBoundary *interactor, QObject *parent)
    : QObject(parent)
    , m_interactor(interactor)
{
}

void LottoController::onGenerateRequested()
{
    m_interactor->generateNewTicket();
}

void LottoController::onLockRequested()
{
    m_interactor->toggleLock();
}
