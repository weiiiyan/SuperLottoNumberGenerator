#include "lottoviewshell.h"

#include <QDebug>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include "lottocontroller.h"
#include "lottostyleswitchoverlay.h"
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

    // 首个视图: 优先按偏好恢复, 未知/空偏好或视图不可用则回退目录首项
    if (m_catalog.isEmpty())
        return;

    LottoView *view = createInitialView();
    if (!view)
        return;

    m_stack->addWidget(view);
    m_stack->setCurrentWidget(view);
    m_currentView = view;
    m_currentStyleId = view->styleId();
    if (m_hasState)
        view->renderState(m_lastState);
    view->styleChanged(m_currentStyleId);
}

LottoView *LottoViewShell::createInitialView()
{
    const QString preferred = m_store ? m_store->loadStyleId() : QString();

    // 先尝试偏好项, 再逐一回退: 偏好未知、工厂返回空、视图未就绪都不采用,
    // 且不覆盖用户已保存的偏好(便于修复后恢复其选择)
    auto tryCreate = [this](const LottoStyleDefinition &definition) -> LottoView * {
        LottoView *view = createView(definition);
        if (!view)
            return nullptr;
        if (!view->isReady()) {
            qWarning() << "[LOTTO] 恢复的风格视图未就绪, 回退:" << definition.info.id;
            view->deleteLater();
            return nullptr;
        }
        return view;
    };

    if (!preferred.isEmpty()) {
        for (const LottoStyleDefinition &definition : m_catalog) {
            if (definition.info.id == preferred) {
                if (LottoView *view = tryCreate(definition))
                    return view;
                break;
            }
        }
    }

    for (const LottoStyleDefinition &definition : m_catalog) {
        if (definition.info.id == preferred)
            continue;
        if (LottoView *view = tryCreate(definition))
            return view;
    }
    return nullptr;
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
    if (styleId.isEmpty())
        return;

    // 去重基准是"已生效或已排队的最后一个目标", 而非仅已生效的风格:
    // 否则同一事件轮次内的"切走再切回"会让最后一次选择被丢弃
    const QString effective = m_pendingStyleId.isEmpty() ? m_currentStyleId
                                                         : m_pendingStyleId;
    if (styleId == effective)
        return;

    m_pendingStyleId = styleId;

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
    // 队列中可能积压多次请求: 只有最后一次目标生效
    if (styleId != m_pendingStyleId)
        return;
    m_pendingStyleId.clear();

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

    // 过渡起点: 旧视图快照(在隐藏旧视图之前抓取)
    QPixmap previousSnapshot;
    if (m_transitionMs > 0 && previous)
        previousSnapshot = previous->snapshot();

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

    if (!previousSnapshot.isNull())
        playTransition(previousSnapshot);
}

void LottoViewShell::playTransition(const QPixmap &snapshot)
{
    // 快速连续切换: 先结束上一次过渡, 保证同一时刻只有一个覆盖层
    finishTransition();

    m_overlay = new LottoStyleSwitchOverlay(snapshot, this);
    m_overlay->show();

    auto *animation = new QVariantAnimation(this);
    animation->setDuration(m_transitionMs);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
        if (m_overlay)
            m_overlay->setOpacity(value.toReal());
    });
    connect(animation, &QVariantAnimation::finished, this,
            [this]() { finishTransition(); });

    m_overlayAnimation = animation;
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void LottoViewShell::finishTransition()
{
    if (m_overlayAnimation) {
        m_overlayAnimation->stop();
        m_overlayAnimation.clear();
    }
    if (m_overlay) {
        m_overlay->hide();
        m_overlay->deleteLater();
        m_overlay.clear();
    }
}

void LottoViewShell::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_overlay)
        m_overlay->setGeometry(rect());
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
