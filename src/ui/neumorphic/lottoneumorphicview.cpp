#include "lottoneumorphicview.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>

#include "lottostylehost.h"

namespace {

// 状态转 QML 期望的变体形态(字符串列表的列表)
QVariantList groupsToVariant(const QVector<QStringList> &groups)
{
    QVariantList out;
    out.reserve(groups.size());
    for (const QStringList &row : groups)
        out.append(QVariant(row));
    return out;
}

// 风格清单转 QML 期望的对象列表
QVariantList stylesToVariant(const QVector<LottoStyleInfo> &styles)
{
    QVariantList out;
    out.reserve(styles.size());
    for (const LottoStyleInfo &info : styles) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), info.id);
        map.insert(QStringLiteral("displayName"), info.displayName);
        out.append(map);
    }
    return out;
}

} // namespace

LottoNeumorphicView::LottoNeumorphicView(LottoStyleHost *host, QWidget *parent)
    : LottoView(parent)
    , m_host(host)
{
    setObjectName(QStringLiteral("neumorphicRoot"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_quick = new QQuickWidget(this);
    m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quick->setClearColor(Qt::transparent);
    m_quick->engine()->addImportPath(QStringLiteral(":/"));
    layout->addWidget(m_quick);

    connect(m_quick, &QQuickWidget::statusChanged, this,
            [this](QQuickWidget::Status status) {
        if (status == QQuickWidget::Error) {
            qWarning() << "[LOTTO] 新拟物视图 QML 加载失败:" << m_quick->errors();
            return;
        }
        if (status == QQuickWidget::Ready) {
            connectRootSignals();
            pushStyles();
            if (m_hasState)
                pushStateToQml();
        }
    });

    m_quick->loadFromModule(QStringLiteral("LottoUi"),
                            QStringLiteral("NeumorphicView"));

    if (m_quick->status() == QQuickWidget::Ready) {
        connectRootSignals();
        pushStyles();
    }
}

QString LottoNeumorphicView::styleId() const
{
    return QString::fromLatin1(STYLE_ID);
}

void LottoNeumorphicView::renderState(const LottoViewState &state)
{
    m_state = state;
    m_hasState = true;
    if (rootItem())
        pushStateToQml();
}

bool LottoNeumorphicView::isReady() const
{
    // 加载中允许使用(状态会在 Ready 后补推), 出错或根对象始终不存在则不可用
    if (!m_quick)
        return false;
    return m_quick->status() != QQuickWidget::Error
        || m_quick->rootObject() != nullptr;
}

void LottoNeumorphicView::styleChanged(const QString &currentStyleId)
{
    if (!rootItem())
        return;
    rootItem()->setProperty("currentStyleId", currentStyleId);
}

QPixmap LottoNeumorphicView::snapshot() const
{
    if (m_quick) {
        const QImage frame = m_quick->grabFramebuffer();
        if (!frame.isNull())
            return QPixmap::fromImage(frame);
    }
    return LottoView::snapshot();
}

QQuickWidget *LottoNeumorphicView::quickWidget() const
{
    return m_quick;
}

QQuickItem *LottoNeumorphicView::rootItem() const
{
    return m_quick ? m_quick->rootObject() : nullptr;
}

void LottoNeumorphicView::onStyleSwitchRequested(const QString &styleId)
{
    if (m_host)
        m_host->requestSwitchTo(styleId);
}

void LottoNeumorphicView::connectRootSignals()
{
    QQuickItem *root = rootItem();
    if (!root || root->property("_signalsConnected").toBool())
        return;
    root->setProperty("_signalsConnected", true);

    // QML 侧信号以字符串形式连接, 避免为桥接引入额外 QObject 类型
    connect(root, SIGNAL(generateClicked()), this, SIGNAL(generateRequested()));
    connect(root, SIGNAL(lockClicked()), this, SIGNAL(lockRequested()));
    connect(root, SIGNAL(styleSwitchRequested(QString)), this,
            SLOT(onStyleSwitchRequested(QString)));
}

void LottoNeumorphicView::pushStyles()
{
    QQuickItem *root = rootItem();
    if (!root)
        return;

    const QVector<LottoStyleInfo> styles =
        m_host ? m_host->availableStyles() : QVector<LottoStyleInfo>();
    root->setProperty("styles", stylesToVariant(styles));
    root->setProperty("currentStyleId",
                      m_host ? m_host->currentStyleId() : QString());
}

void LottoNeumorphicView::pushStateToQml()
{
    QQuickItem *root = rootItem();
    if (!root)
        return;

    root->setProperty("groups", groupsToVariant(m_state.groups));
    root->setProperty("timeLabelText", m_state.timeLabelText);
    root->setProperty("timeValueText", m_state.timeValueText);
    root->setProperty("generateButtonText", m_state.generateButtonText);
    root->setProperty("generateEnabled", m_state.generateEnabled);
    root->setProperty("lockButtonText", m_state.lockButtonText);
    root->setProperty("lockButtonEnabled", m_state.lockButtonEnabled);
    root->setProperty("lockButtonChecked", m_state.lockButtonChecked);
    root->setProperty("numberPlaceholder", m_state.numberPlaceholder);
    root->setProperty("separatorText", m_state.separatorText);
}
