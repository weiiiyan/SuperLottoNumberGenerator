#include "mainwindow.h"

#include "./ui_mainwindow.h"

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

#include "lottocontroller.h"

// 工具函数

// formatNumber() 已提取至 mainwindow.h 供 UI 与测试复用

/// 根据竖屏可用宽度推算动态间距分档
static void spacingForWidth(int availableWidth, int &outSpacing, int &outSeparatorWidth)
{
    if (availableWidth < MainWindow::WIDTH_TIER_XS) {
        outSpacing        = 2;
        outSeparatorWidth = 10;
    } else if (availableWidth < MainWindow::WIDTH_TIER_SM) {
        outSpacing        = 3;
        outSeparatorWidth = 12;
    } else if (availableWidth < MainWindow::WIDTH_TIER_MD) {
        outSpacing        = 4;
        outSeparatorWidth = 14;
    } else {
        outSpacing        = 6;
        outSeparatorWidth = 18;
    }
}

// 构造 / 析构

MainWindow::MainWindow(LottoController *controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(controller)
{
    init();
    m_controller->load();   // 排定异步恢复, 不阻塞 Android 启动
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 状态渲染(视图唯一渲染入口, 不做任何业务判断)

void MainWindow::onTicketChanged(const LottoTicket &ticket)
{
    applyTicketToLabels(ticket);
    updateTimeLabel(ticket.generateTime());

    const bool hasTicket = !ticket.groups().isEmpty() && !ticket.groups().first().front.isEmpty();
    m_btnLock->setEnabled(hasTicket);
    m_btnLock->setChecked(ticket.isLocked());
    m_btnLock->setText(ticket.isLocked() ? "🔒 解锁生成" : "🔓 锁定号码");
    m_btnGenerate->setEnabled(!ticket.isLocked());
}

void MainWindow::applyTicketToLabels(const LottoTicket &ticket)
{
    for (int g = 0; g < GROUP_COUNT; g++) {
        const LottoResult result = ticket.groupAt(g);
        for (int i = 0; i < FRONT_COUNT; i++) {
            m_frontLabels[g][i]->setText(
                i < result.front.size() ? formatNumber(result.front[i]) : "?");
        }
        for (int i = 0; i < BACK_COUNT; i++) {
            m_backLabels[g][i]->setText(
                i < result.back.size() ? formatNumber(result.back[i]) : "?");
        }
    }
}

void MainWindow::updateTimeLabel(const QDateTime &time)
{
    if (time.isValid()) {
        m_timeLabel->setText("🕐 生成时间：" + time.toString("yyyy-MM-dd HH:mm:ss"));
    } else {
        m_timeLabel->setText("🕐 生成时间：年-月-日 时:分:秒");
    }
}

// 窗口尺寸变化 → 更新标签尺寸

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 防抖: 每次 resize 重启定时器, 50ms 内无新事件才执行布局更新
    // 解决 Android 端 centralWidget 宽度在 448/800 间反复跳动导致的
    // resizeEvent → updateLabelSizes → rebuildGroupRows → resizeEvent 自激震荡
    if (m_layoutDebounceTimer) {
        m_layoutDebounceTimer->start(50);
    }
}

// 标签尺寸计算(竖屏: 根据可用宽度反推标签尺寸)

void MainWindow::updateLabelSizes(int availableWidth)
{
    qDebug() << "[LOTTO] updateLabelSizes availableWidth=" << availableWidth;

    spacingForWidth(availableWidth, m_spacing, m_separatorWidth);

    m_labelWidth = (availableWidth - m_separatorWidth - LABELS_PER_ROW * m_spacing) / LABELS_PER_ROW;
    m_labelWidth = std::clamp(m_labelWidth, LABEL_MIN_SIZE_PORTRAIT, LABEL_MAX_SIZE);

    // 动态字号: 标签尺寸的 50%, 不窄于 10px
    int fontSize = std::max(10, static_cast<int>(m_labelWidth * 0.50));

    qDebug() << "[LOTTO] updateLabelSizes result labelW=" << m_labelWidth
             << "spacing=" << m_spacing
             << "sepW=" << m_separatorWidth
             << "fontSize=" << fontSize
             << "contentW=" << (LABELS_PER_ROW * m_labelWidth + LABELS_PER_ROW * m_spacing + m_separatorWidth);

    // 将尺寸/字体应用到一组标签的 lambda
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
        sizeLabelGroup(m_backLabels[g],  BACK_COUNT);
        if (m_separators[g]) {
            m_separators[g]->setFixedSize(m_separatorWidth, m_labelWidth);
            m_separators[g]->setContentsMargins(0, 0, 0, 0);
            QFont f = m_separators[g]->font();
            f.setPixelSize(std::max(8, fontSize * 3 / 5));
            m_separators[g]->setFont(f);
        }
    }
}

// 创建单组号码行(复用已有标签和分隔符)

QHBoxLayout* MainWindow::createGroupRow(int group)
{
    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(0);
    row->addStretch(1);

    for (int i = 0; i < FRONT_COUNT; i++) {
        if (i > 0) row->addSpacing(m_spacing);
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

// 清空布局中所有项(widget 保留, 子布局/spacer 销毁)

static void clearLayout(QLayout *layout)
{
    if (!layout) return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout()) {
            clearLayout(item->layout());
        }
        delete item;
    }
}

// 构建固定竖屏布局(仅在 init 中调用一次)

void MainWindow::buildLayout()
{
    if (!m_mainLayout || !m_groupsContainer || !m_groupsLayout)
        return;

    qDebug() << "[LOTTO] buildLayout labelW=" << m_labelWidth
             << "spacing=" << m_spacing;

    // 竖屏: 5 行纵向
    m_groupsLayout->setSpacing(14);
    for (int g = 0; g < GROUP_COUNT; g++) {
        m_groupsLayout->addLayout(createGroupRow(g));
    }

    // 按钮行
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    btnLayout->addWidget(m_btnLock);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch(1);

    // 主布局(竖屏固定间距/边距)
    m_mainLayout->setSpacing(18);
    m_mainLayout->addStretch(1);
    m_mainLayout->addWidget(m_timeLabel, 0, Qt::AlignHCenter);
    m_mainLayout->addSpacing(18);
    m_mainLayout->addWidget(m_groupsContainer);
    m_mainLayout->addSpacing(40);
    m_mainLayout->addLayout(btnLayout);
    m_mainLayout->addStretch(1);
}

// 间距档位变化时重建组行(保留主布局不变)

void MainWindow::rebuildGroupRows()
{
    if (!m_groupsLayout)
        return;

    qDebug() << "[LOTTO] rebuildGroupRows labelW=" << m_labelWidth
             << "spacing=" << m_spacing;

    clearLayout(m_groupsLayout);

    for (int g = 0; g < GROUP_COUNT; g++) {
        m_groupsLayout->addLayout(createGroupRow(g));
    }
}

// UI 初始化

void MainWindow::init()
{
    ui->setupUi(this);
    setWindowTitle("大乐透随机号码生成器");

    if (!ui->centralwidget) {
        ui->centralwidget = new QWidget(this);
        setCentralWidget(ui->centralwidget);
    }

    // 主布局
    m_mainLayout = new QVBoxLayout(ui->centralwidget);

    // 时间标签
    m_timeLabel = new QLabel("🕐 生成时间：年-月-日 时:分:秒", ui->centralwidget);
    m_timeLabel->setObjectName("timeLabel");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 计算初始可用宽度(竖屏: 以屏幕宽度为基准)
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    QMargins margins(40, 30, 40, 30);
    int availableWidth = static_cast<int>(
        (screen.width() - margins.left() - margins.right()) * LABEL_ROW_WIDTH_FRACTION);
    qDebug() << "[LOTTO] init screen=" << screen
             << "availableWidth=" << availableWidth;

    // 号码标签 + 分隔符(只创建一次)
    auto createLabels = [&](const QString &prefix, const QString &area, int count,
                             QLabel *out[], int group) {
        for (int i = 0; i < count; i++) {
            out[i] = new QLabel("?", ui->centralwidget);
            out[i]->setObjectName(QString("%1_%2_%3").arg(prefix).arg(group).arg(i));
            out[i]->setFixedSize(m_labelWidth, m_labelWidth);
            out[i]->setAlignment(Qt::AlignCenter);
            out[i]->setContentsMargins(0, 0, 0, 0);
            out[i]->setProperty("area", area);
        }
    };

    for (int g = 0; g < GROUP_COUNT; g++) {
        createLabels("frontLabel", "front", FRONT_COUNT, m_frontLabels[g], g);
        createLabels("backLabel",  "back",  BACK_COUNT,  m_backLabels[g],  g);
        m_separators[g] = new QLabel("+", ui->centralwidget);
        m_separators[g]->setObjectName(QString("separator_%1").arg(g));
        m_separators[g]->setFixedSize(m_separatorWidth, m_labelWidth);
        m_separators[g]->setAlignment(Qt::AlignCenter);
        m_separators[g]->setContentsMargins(0, 0, 0, 0);
    }

    // 预设主布局边距(竖屏固定值)
    m_mainLayout->setContentsMargins(40, 30, 40, 30);

    // 统一计算初始尺寸
    updateLabelSizes(availableWidth);
    qDebug() << "[LOTTO] init after updateLabelSizes labelW=" << m_labelWidth
             << "spacing=" << m_spacing
             << "sepW=" << m_separatorWidth
             << "contentW=" << (LABELS_PER_ROW * m_labelWidth + LABELS_PER_ROW * m_spacing + m_separatorWidth)
             << "cwMarginL+R=" << (m_mainLayout ? (m_mainLayout->contentsMargins().left() + m_mainLayout->contentsMargins().right()) : -1);

    // 号码容器
    m_groupsContainer = new QWidget(ui->centralwidget);
    m_groupsContainer->setObjectName("groupsContainer");
    m_groupsLayout = new QVBoxLayout(m_groupsContainer);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);

    // 按钮
    m_btnGenerate = new QPushButton("🎲 生成号码", ui->centralwidget);
    m_btnGenerate->setObjectName("btnGenerate");

    m_btnLock = new QPushButton("🔓 锁定号码", ui->centralwidget);
    m_btnLock->setObjectName("btnLock");
    m_btnLock->setCheckable(true);
    m_btnLock->setEnabled(false);

    // 构建竖屏布局
    buildLayout();

    // 统一样式表
    QFile qss(":/style.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(qss.readAll());
    } else {
        qWarning() << "[LOTTO] 未能加载 :/style.qss, 样式表未应用"
                   << "(静态库资源需在入口调用 Q_INIT_RESOURCE)";
    }

    // 信号连接: 用户动作转发给控制器, 状态变化由 ticketChanged 信号回推
    connect(m_btnGenerate, &QPushButton::clicked, this, [this]{ m_controller->generateNewTicket(); });
    connect(m_btnLock,     &QPushButton::clicked, this, [this]{ m_controller->toggleLock(); });
    connect(m_controller, &LottoController::ticketChanged, this, &MainWindow::onTicketChanged);

    // 布局防抖定时器(解决 Android resizeEvent 自激震荡)
    m_layoutDebounceTimer = new QTimer(this);
    m_layoutDebounceTimer->setSingleShot(true);
    connect(m_layoutDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!centralWidget() || !m_mainLayout)
            return;

        const int cwWidth = centralWidget()->width();
        QMargins m = m_mainLayout->contentsMargins();
        int availableWidth = static_cast<int>(
            (cwWidth - m.left() - m.right()) * LABEL_ROW_WIDTH_FRACTION);

        // 宽度未变化则跳过, 避免无意义的重绘
        if (availableWidth == m_lastAppliedWidth)
            return;

        qDebug() << "[LOTTO] debounced layout update cwWidth=" << cwWidth
                 << "availableWidth=" << availableWidth;

        m_lastAppliedWidth = availableWidth;

        const int oldSpacing = m_spacing;
        const int oldSepWidth = m_separatorWidth;

        updateLabelSizes(availableWidth);

        if (m_spacing != oldSpacing || m_separatorWidth != oldSepWidth) {
            qDebug() << "[LOTTO] spacing tier changed, rebuilding group rows";
            rebuildGroupRows();
        }
    });
}
