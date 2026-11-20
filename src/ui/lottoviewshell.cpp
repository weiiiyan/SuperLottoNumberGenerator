#include "lottoviewshell.h"

#include <QDebug>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "lottocontroller.h"
#include "lottostylestore.h"
#include "lottoview.h"

LottoViewShell::LottoViewShell(LottoController *controller,
                               LottoPresenter *presenter,
                               LottoStyleStore *store,
                               QVector<LottoStyleDefinition> catalog,
                               int transitionMs, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_presenter(presenter)
    , m_store(store)
    , m_catalog(std::move(catalog))
    , m_transitionMs(transitionMs)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);

    // 状态推送: 缓存最近状态供切换重放, 并转发给当前视图
    if (m_presenter) {
        connect(m_presenter, &LottoPresenter::viewStateChanged,
                this, [this](const LottoViewState &state) {
            m_lastState = state;
            m_hasState = true;
            if (m_currentView)
                m_currentView->renderState(m_lastState);
        });
    }

    // 首个视图: 优先按偏好恢复, 未知/空偏好回退目录首项且不回写
    if (m_catalog.isEmpty())
        return;

    QString preferred = m_store ? m_store->loadStyleId() : QString();
    int index = 0;
    if (!preferred.isEmpty()) {
        for (int i = 0; i < m_catalog.size(); ++i) {
            if (m_catalog.at(i).info.id == preferred) {
                index = i;
                break;
            }
        }
    }

    LottoView *view = createView(m_catalog.at(index));
    if (!view)
        return;

    m_stack->addWidget(view);
    m_stack->setCurrentWidget(view);
    m_currentView = view;
    m_currentStyleId = m_catalog.at(index).info.id;
    if (m_hasState)
        view->renderState(m_lastState);
    view->styleChanged(m_currentStyleId);
}

QVector<LottoStyleInfo> LottoViewShell::availableStyles() const
{
    QVector<LottoStyleInfo> styles;
    styles.reserve(m_catalog.size());
    for (const LottoStyleDefinition &definition : m_catalog)
        styles.append(definition.info);
    return styles;
}

QString LottoViewShell::currentStyleId() const
{
    return m_currentStyleId;
}

void LottoViewShell::requestSwitchTo(const QString &styleId)
{
    if (styleId.isEmpty() || styleId == m_currentStyleId)
        return;

    // 排队执行: 切换通常在视图自身的事件处理中发起, 不能在此时销毁/隐藏自己
    QMetaObject::invokeMethod(this, [this, styleId]() { applySwitch(styleId); },
                              Qt::QueuedConnection);
}

LottoView *LottoViewShell::currentView() const
{
    return m_currentView;
}

int LottoViewShell::transitionMs() const
{
    return m_transitionMs;
}

void LottoViewShell::applySwitch(const QString &styleId)
{
    if (styleId == m_currentStyleId)
        return;

    const LottoStyleDefinition *definition = nullptr;
    for (const LottoStyleDefinition &candidate : m_catalog) {
        if (candidate.info.id == styleId) {
            definition = &candidate;
            break;
        }
    }
    if (!definition) {
        qWarning() << "[LOTTO] 未知风格 id:" << styleId;
        emit styleSwitchFailed(styleId);
        return;
    }

    LottoView *view = findExistingView(styleId);
    if (!view) {
        view = createView(*definition);
        if (!view) {
            qWarning() << "[LOTTO] 风格视图创建失败:" << styleId;
            emit styleSwitchFailed(styleId);
            return;
        }
        if (!view->isReady()) {
            qWarning() << "[LOTTO] 风格视图未就绪:" << styleId;
            view->deleteLater();
            emit styleSwitchFailed(styleId);
            return;
        }
    }

    LottoView *previous = m_currentView;
    if (m_stack->indexOf(view) < 0)
        m_stack->addWidget(view);

    if (m_hasState)
        view->renderState(m_lastState);

    m_stack->setCurrentWidget(view);
    m_currentView = view;
    m_currentStyleId = styleId;

    if (previous && previous != view)
        previous->styleChanged(styleId);
    view->styleChanged(styleId);

    if (m_store)
        m_store->saveStyleId(styleId);
}

LottoView *LottoViewShell::createView(const LottoStyleDefinition &definition)
{
    if (!definition.create)
        return nullptr;

    LottoView *view = definition.create(this, this);
    if (!view)
        return nullptr;

    connect(view, &LottoView::generateRequested, this, [this]() {
        if (m_controller)
            m_controller->onGenerateRequested();
    });
    connect(view, &LottoView::lockRequested, this, [this]() {
        if (m_controller)
            m_controller->onLockRequested();
    });
    return view;
}

LottoView *LottoViewShell::findExistingView(const QString &styleId) const
{
    for (int i = 0; i < m_stack->count(); ++i) {
        auto *view = qobject_cast<LottoView *>(m_stack->widget(i));
        if (view && view->styleId() == styleId)
            return view;
    }
    return nullptr;
}
