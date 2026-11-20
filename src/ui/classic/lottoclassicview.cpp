#include "lottoclassicview.h"

#include <QActionGroup>
#include <QResource>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#include "lottostylehost.h"

namespace {

// 根据竖屏可用宽度推算动态间距分档
void spacingForWidth(int availableWidth, int &outSpacing, int &outSeparatorWidth)
{
    if (availableWidth < LottoClassicView::WIDTH_TIER_XS) {
        outSpacing        = 2;
        outSeparatorWidth = 10;
    } else if (availableWidth < LottoClassicView::WIDTH_TIER_SM) {
        outSpacing        = 3;
        outSeparatorWidth = 12;
    } else if (availableWidth < LottoClassicView::WIDTH_TIER_MD) {
        outSpacing        = 4;
        outSeparatorWidth = 14;
    } else {
        outSpacing        = 6;
        outSeparatorWidth = 18;
    }
}

// 清空布局中所有项(widget 保留, 子布局/spacer 销毁)
void clearLayout(QLayout *layout)
{
    if (!layout)
        return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout())
            clearLayout(item->layout());
        delete item;
    }
}

} // namespace

LottoClassicView::LottoClassicView(LottoStyleHost *host, QWidget *parent)
    : LottoView(parent)
    , m_host(host)
{
    // 本组件的 qrc 由自己负责初始化: 静态库资源不经外部入口也不会被丢弃
    Q_INIT_RESOURCE(resources);
    setObjectName(QStringLiteral("classicRoot"));
    initUi();
}

QString LottoClassicView::styleId() const
{
    return QString::fromLatin1(STYLE_ID);
}

// 状态渲染: 机械写入控件, 不做任何推导

void LottoClassicView::renderState(const LottoViewState &state)
{
    applyState(state);
}

void LottoClassicView::applyState(const LottoViewState &state)
{
    // 防御: 对异常展示状态做边界保护, 避免越界访问
    if (state.groups.size() < GROUP_COUNT)
        return;

    for (int g = 0; g < GROUP_COUNT; g++) {
        const QStringList &row = state.groups.at(g);
        for (int i = 0; i < FRONT_COUNT; i++)
            if (i < row.size() && m_frontLabels[g][i])
                m_frontLabels[g][i]->setText(row.at(i));
        for (int i = 0; i < BACK_COUNT; i++)
            if (FRONT_COUNT + i < row.size() && m_backLabels[g][i])
                m_backLabels[g][i]->setText(row.at(FRONT_COUNT + i));
    }
    for (int g = 0; g < GROUP_COUNT; g++) {
        if (m_separators[g])
            m_separators[g]->setText(state.separatorText);
    }
    if (m_timeLabel)
        m_timeLabel->setText(state.timeLabelText);
    if (m_timeValueLabel)
        m_timeValueLabel->setText(state.timeValueText);
    if (m_btnLock) {
        m_btnLock->setEnabled(state.lockButtonEnabled);
        m_btnLock->setChecked(state.lockButtonChecked);
        m_btnLock->setText(state.lockButtonText);
    }
    if (m_btnGenerate) {
        m_btnGenerate->setText(state.generateButtonText);
        m_btnGenerate->setEnabled(state.generateEnabled);
    }
}

void LottoClassicView::resizeEvent(QResizeEvent *event)
{
    LottoView::resizeEvent(event);

    // 防抖: 每次 resize 重启定时器, 50ms 内无新事件才执行布局更新
    // 解决 Android 端宽度在相近值间反复跳动导致的
    // resizeEvent → updateLabelSizes → rebuildGroupRows → resizeEvent 自激震荡
    if (m_layoutDebounceTimer)
        m_layoutDebounceTimer->start(50);
}

void LottoClassicView::updateLabelSizes(int availableWidth)
{
    spacingForWidth(availableWidth, m_spacing, m_separatorWidth);

    m_labelWidth = (availableWidth - m_separatorWidth - LABELS_PER_ROW * m_spacing)
                   / LABELS_PER_ROW;
    m_labelWidth = std::clamp(m_labelWidth, LABEL_MIN_SIZE_PORTRAIT, LABEL_MAX_SIZE);

    // 动态字号: 标签尺寸的 50%, 不窄于 10px
    const int fontSize = std::max(10, static_cast<int>(m_labelWidth * 0.50));

    auto sizeLabelGroup = [&](QLabel *const labels[], int count) {
        for (int i = 0; i < count; i++) {
            if (labels[i]) {
                labels[i]->setFixedSize(m_labelWidth, m_labelWidth);
                labels[i]->setContentsMargins(0, 0, 0, 0);
                QFont f = labels[i]->font();
                f.setPixelSize(fontSize);
                labels[i]->setFont(f);
            }
        }
    };

    for (int g = 0; g < GROUP_COUNT; g++) {
        sizeLabelGroup(m_frontLabels[g], FRONT_COUNT);
        sizeLabelGroup(m_backLabels[g], BACK_COUNT);
        if (m_separators[g]) {
            m_separators[g]->setFixedSize(m_separatorWidth, m_labelWidth);
            m_separators[g]->setContentsMargins(0, 0, 0, 0);
            QFont f = m_separators[g]->font();
            f.setPixelSize(std::max(8, fontSize * 3 / 5));
            m_separators[g]->setFont(f);
        }
    }
}

QHBoxLayout* LottoClassicView::createGroupRow(int group)
{
    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(0);
    row->addStretch(1);

    for (int i = 0; i < FRONT_COUNT; i++) {
        if (i > 0)
            row->addSpacing(m_spacing);
        row->addWidget(m_frontLabels[group][i]);
    }

    row->addSpacing(m_spacing);
    row->addWidget(m_separators[group]);

    for (int i = 0; i < BACK_COUNT; i++) {
        row->addSpacing(m_spacing);
        row->addWidget(m_backLabels[group][i]);
    }

    row->addStretch(1);
    return row;
}

void LottoClassicView::buildLayout()
{
    if (!m_mainLayout || !m_groupsContainer || !m_groupsLayout)
        return;

    // 竖屏: 5 行纵向
    m_groupsLayout->setSpacing(14);
    for (int g = 0; g < GROUP_COUNT; g++)
        m_groupsLayout->addLayout(createGroupRow(g));

    // 按钮行
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    btnLayout->addWidget(m_btnLock);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch(1);

    // 风格切换按钮行(右上角)
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->addStretch(1);
    topRow->addWidget(m_btnStyleSwitch);

    // 主布局(竖屏固定间距/边距)
    m_mainLayout->setSpacing(18);
    m_mainLayout->addLayout(topRow);
    m_mainLayout->addStretch(1);
    m_mainLayout->addLayout([this]() {
        auto *timeRow = new QHBoxLayout();
        timeRow->addStretch(1);
        timeRow->addWidget(m_timeLabel);
        timeRow->addSpacing(0);
        timeRow->addWidget(m_timeValueLabel);
        timeRow->addStretch(1);
        return timeRow;
    }());
    m_mainLayout->addSpacing(18);
    m_mainLayout->addWidget(m_groupsContainer);
    m_mainLayout->addSpacing(40);
    m_mainLayout->addLayout(btnLayout);
    m_mainLayout->addStretch(1);
}

void LottoClassicView::rebuildGroupRows()
{
    if (!m_groupsLayout)
        return;

    clearLayout(m_groupsLayout);
    for (int g = 0; g < GROUP_COUNT; g++)
        m_groupsLayout->addLayout(createGroupRow(g));
}

// 风格菜单: 由宿主提供的风格清单构建, 当前项勾选

void LottoClassicView::buildStyleMenu()
{
    if (!m_btnStyleSwitch)
        return;

    QMenu *menu = new QMenu(m_btnStyleSwitch);
    QActionGroup *group = new QActionGroup(menu);
    group->setExclusive(true);

    if (m_host) {
        const QVector<LottoStyleInfo> styles = m_host->availableStyles();
        for (const LottoStyleInfo &info : styles) {
            QAction *action = menu->addAction(info.displayName);
            action->setCheckable(true);
            action->setData(info.id);
            action->setChecked(info.id == m_host->currentStyleId());
            group->addAction(action);
            connect(action, &QAction::triggered, this, [this, action]() {
                if (m_host)
                    m_host->requestSwitchTo(action->data().toString());
            });
        }
    }

    m_btnStyleSwitch->setMenu(menu);
}

void LottoClassicView::styleChanged(const QString &currentStyleId)
{
    if (!m_btnStyleSwitch || !m_btnStyleSwitch->menu())
        return;
    const QList<QAction *> actions = m_btnStyleSwitch->menu()->actions();
    for (QAction *action : actions)
        action->setChecked(action->data().toString() == currentStyleId);
}

// UI 初始化

void LottoClassicView::initUi()
{
    // 主布局
    m_mainLayout = new QVBoxLayout(this);

    // 时间标签: 标题与值分两个控件(排版由本风格决定)
    m_timeLabel = new QLabel(QString(), this);
    m_timeLabel->setObjectName(QStringLiteral("timeLabel"));
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_timeValueLabel = new QLabel(QString(), this);
    m_timeValueLabel->setObjectName(QStringLiteral("timeValueLabel"));
    m_timeValueLabel->setAlignment(Qt::AlignCenter);
    m_timeValueLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 计算初始可用宽度(竖屏: 以屏幕宽度为基准)
    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    const QMargins margins(40, 30, 40, 30);
    const int availableWidth = static_cast<int>(
        (screen.width() - margins.left() - margins.right()) * LABEL_ROW_WIDTH_FRACTION);

    // 号码标签 + 分隔符(只创建一次)
    auto createLabels = [&](const QString &prefix, const QString &area, int count,
                            QLabel *out[], int group) {
        for (int i = 0; i < count; i++) {
            out[i] = new QLabel(QString(), this);   // 文本由 renderState 填充
            out[i]->setObjectName(QString("%1_%2_%3").arg(prefix).arg(group).arg(i));
            out[i]->setFixedSize(m_labelWidth, m_labelWidth);
            out[i]->setAlignment(Qt::AlignCenter);
            out[i]->setContentsMargins(0, 0, 0, 0);
            out[i]->setProperty("area", area);
        }
    };

    for (int g = 0; g < GROUP_COUNT; g++) {
        createLabels(QStringLiteral("frontLabel"), QStringLiteral("front"),
                     FRONT_COUNT, m_frontLabels[g], g);
        createLabels(QStringLiteral("backLabel"), QStringLiteral("back"),
                     BACK_COUNT, m_backLabels[g], g);
        m_separators[g] = new QLabel(QString(), this);   // 文本由 renderState 填充
        m_separators[g]->setObjectName(QString("separator_%1").arg(g));
        m_separators[g]->setFixedSize(m_separatorWidth, m_labelWidth);
        m_separators[g]->setAlignment(Qt::AlignCenter);
        m_separators[g]->setContentsMargins(0, 0, 0, 0);
    }

    // 预设主布局边距(竖屏固定值)
    m_mainLayout->setContentsMargins(40, 30, 40, 30);

    // 统一计算初始尺寸
    updateLabelSizes(availableWidth);

    // 号码容器
    m_groupsContainer = new QWidget(this);
    m_groupsContainer->setObjectName(QStringLiteral("groupsContainer"));
    m_groupsLayout = new QVBoxLayout(m_groupsContainer);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);

    // 按钮(文字/启用状态由 renderState 填充)
    m_btnGenerate = new QPushButton(QString(), this);
    m_btnGenerate->setObjectName(QStringLiteral("btnGenerate"));

    m_btnLock = new QPushButton(QString(), this);
    m_btnLock->setObjectName(QStringLiteral("btnLock"));
    m_btnLock->setCheckable(true);

    // 风格切换按钮(角落, 弹出 N 项菜单)
    m_btnStyleSwitch = new QToolButton(this);
    m_btnStyleSwitch->setObjectName(QStringLiteral("btnStyleSwitch"));
    m_btnStyleSwitch->setPopupMode(QToolButton::InstantPopup);
    m_btnStyleSwitch->setText(QStringLiteral("风格"));
    buildStyleMenu();

    // 构建竖屏布局
    buildLayout();

    // 统一样式表
    QFile qss(QStringLiteral(":/classic/style.qss"));
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(qss.readAll());
    } else {
        qWarning() << "[LOTTO] 未能加载 :/classic/style.qss, 样式表未应用"
                   << "(本组件的 Q_INIT_RESOURCE 未能注册资源)";
    }

    // 信号连接: 按钮点击只发请求信号, 由宿主转发给输入侧适配器
    connect(m_btnGenerate, &QPushButton::clicked,
            this, &LottoView::generateRequested);
    connect(m_btnLock, &QPushButton::clicked,
            this, &LottoView::lockRequested);

    // 布局防抖定时器(解决 Android resizeEvent 自激震荡)
    m_layoutDebounceTimer = new QTimer(this);
    m_layoutDebounceTimer->setSingleShot(true);
    connect(m_layoutDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!m_mainLayout)
            return;

        const int cwWidth = width();
        const QMargins m = m_mainLayout->contentsMargins();
        const int availableWidth = static_cast<int>(
            (cwWidth - m.left() - m.right()) * LABEL_ROW_WIDTH_FRACTION);

        // 宽度未变化则跳过, 避免无意义的重绘
        if (availableWidth == m_lastAppliedWidth)
            return;

        m_lastAppliedWidth = availableWidth;

        const int oldSpacing = m_spacing;
        const int oldSepWidth = m_separatorWidth;

        updateLabelSizes(availableWidth);

        if (m_spacing != oldSpacing || m_separatorWidth != oldSepWidth)
            rebuildGroupRows();
    });
}
