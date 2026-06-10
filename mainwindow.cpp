#include "mainwindow.h"
#include "lottoengine.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_lottoEngine(new LottoEngine(this))
{
    init();
    restore();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ────────────────────────────────────────────
//  号码生成
// ────────────────────────────────────────────

void MainWindow::generateFiveGroups()
{
    QVector<LottoResult> numbers = m_lottoEngine->generateBatch(5);
    for (int g = 0; g < 5; g++) {
        for (int i = 0; i < 5; i++)
            m_frontLabels[g][i]->setText(QString("%1").arg(numbers[g].frontVec()[i], 2, 10, QChar('0')));
        for (int i = 0; i < 2; i++)
            m_backLabels[g][i]->setText(QString("%1").arg(numbers[g].backVec()[i], 2, 10, QChar('0')));
    }
    m_timeLabel->setText("🕐 生成时间：" + QDateTime::currentDateTimeUtc().toLocalTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_btnLock->setEnabled(true);
    m_btnLock->setChecked(false);
}

// ────────────────────────────────────────────
//  锁定 / 保存 / 恢复
// ────────────────────────────────────────────

void MainWindow::onLockToggled(bool checked)
{
    m_btnGenerate->setEnabled(!checked);
    m_btnLock->setText(checked ? "🔒 解锁生成" : "🔓 锁定号码");
}

void MainWindow::save() const
{
    QSettings settings("SuperLottoNumberGenerator.ini", QSettings::IniFormat);
    if (m_btnLock->isChecked()) {
        QVariantList frontList, backList;
        for (int g = 0; g < 5; ++g) {
            QVariantList fg, bg;
            for (int i = 0; i < 5; ++i) fg << m_frontLabels[g][i]->text().toInt();
            for (int i = 0; i < 2; ++i) bg << m_backLabels[g][i]->text().toInt();
            frontList << fg;
            backList  << bg;
        }
        settings.setValue("isLocked",     true);
        settings.setValue("frontNumbers", frontList);
        settings.setValue("backNumbers",  backList);
        settings.setValue("generateTime", m_timeLabel->text());
    } else {
        settings.setValue("isLocked", false);
    }
    settings.sync();
}

void MainWindow::restore()
{
    QTimer::singleShot(0, this, [this] {
        QSettings settings("SuperLottoNumberGenerator.ini", QSettings::IniFormat);
        if (!settings.value("isLocked", false).toBool())
            return;

        QVariantList frontNumbers = settings.value("frontNumbers").toList();
        QVariantList backNumbers  = settings.value("backNumbers").toList();
        for (int g = 0; g < 5; ++g) {
            for (int i = g * 5; i < (g + 1) * 5 && i < frontNumbers.size(); ++i)
                m_frontLabels[g][i % 5]->setText(QString("%1").arg(frontNumbers[i].toInt(), 2, 10, QChar('0')));
            for (int i = g * 2; i < (g + 1) * 2 && i < backNumbers.size(); ++i)
                m_backLabels[g][i % 2]->setText(QString("%1").arg(backNumbers[i].toInt(), 2, 10, QChar('0')));
        }

        QString genTime = settings.value("generateTime").toString();
        if (!genTime.isEmpty())
            m_timeLabel->setText(genTime);

        m_btnLock->setChecked(true);
        m_btnLock->setEnabled(true);
    });
}

// ────────────────────────────────────────────
//  窗口尺寸变化 → 横/竖屏切换
// ────────────────────────────────────────────

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (m_rebuilding) return;  // 防止 rebuildLayout 重入

    QRect screen = QApplication::primaryScreen()->availableGeometry();
    bool landscape = screen.width() > screen.height();

    if (landscape != m_isLandscape) {
        // 横竖屏切换：全面重建布局
        m_isLandscape = landscape;
        m_rebuilding = true;

        updateLabelSizes();
        rebuildLayout();

        // 通知样式表切换模式
        ui->centralwidget->setProperty("layoutMode", m_isLandscape ? "landscape" : "portrait");
        ui->centralwidget->style()->unpolish(ui->centralwidget);
        ui->centralwidget->style()->polish(ui->centralwidget);

        m_rebuilding = false;
    } else {
        // 同方向 resize（折叠屏展开、分屏等）：仅刷新标签尺寸
        updateLabelSizes();
    }
}

// ────────────────────────────────────────────
//  标签尺寸计算
// ────────────────────────────────────────────

void MainWindow::updateLabelSizes()
{
    if (m_updatingSizes) return;   // 防重入
    m_updatingSizes = true;

    QRect screen = QApplication::primaryScreen()->availableGeometry();

    if (m_isLandscape) {
        // ── 横屏：高度是瓶颈，五组号码网格 3 行 ──
        int refSize = screen.height() / 16;
        m_labelWidth  = std::max(24, std::min(refSize, 44));
        m_labelHeight = m_labelWidth;

        m_spacing        = 6;
        m_separatorWidth = 18;
    } else {
        // ── 竖屏：从 centralWidget 实际宽度反推标签尺寸 ──
        int cwWidth = centralWidget() ? centralWidget()->width() : this->width();
        if (cwWidth <= 0) cwWidth = screen.width();

        // 读取主布局真实 margins
        QMargins m = m_mainLayout ? m_mainLayout->contentsMargins() : QMargins(40, 30, 40, 30);
        int availableWidth = cwWidth - m.left() - m.right();

        // 5% 安全系数，消化 QLabel 内边距 / 系统圆角 / 亚像素误差
        availableWidth = (int)(availableWidth * 0.94);

        // 动态间距分档
        if (availableWidth < 280) {
            m_spacing        = 2;
            m_separatorWidth = 10;
        } else if (availableWidth < 350) {
            m_spacing        = 3;
            m_separatorWidth = 12;
        } else if (availableWidth < 440) {
            m_spacing        = 4;
            m_separatorWidth = 14;
        } else {
            m_spacing        = 6;
            m_separatorWidth = 18;
        }

        // 每行: 7 标签 + 1 分隔符 + 7 个显式间距（createGroupRow 已设 setSpacing(0)）
        m_labelWidth = (availableWidth - m_separatorWidth - 7 * m_spacing) / 7;
        m_labelWidth = std::max(22, std::min(m_labelWidth, 44));   // 竖屏上限 44
        m_labelHeight = m_labelWidth;
    }

    // ── 动态字号：标签尺寸的 50%，不窄于 10px ──
    int fontSize = std::max(10, (int)(m_labelWidth * 0.50));

    for (int g = 0; g < 5; g++) {
        for (int i = 0; i < 5; i++) {
            if (m_frontLabels[g][i]) {
                m_frontLabels[g][i]->setFixedSize(m_labelWidth, m_labelHeight);
                m_frontLabels[g][i]->setContentsMargins(0, 0, 0, 0);
                QFont f = m_frontLabels[g][i]->font();
                f.setPixelSize(fontSize);
                m_frontLabels[g][i]->setFont(f);
            }
        }
        for (int i = 0; i < 2; i++) {
            if (m_backLabels[g][i]) {
                m_backLabels[g][i]->setFixedSize(m_labelWidth, m_labelHeight);
                m_backLabels[g][i]->setContentsMargins(0, 0, 0, 0);
                QFont f = m_backLabels[g][i]->font();
                f.setPixelSize(fontSize);
                m_backLabels[g][i]->setFont(f);
            }
        }
        if (m_separators[g]) {
            m_separators[g]->setFixedSize(m_separatorWidth, m_labelHeight);
            m_separators[g]->setContentsMargins(0, 0, 0, 0);
            QFont f = m_separators[g]->font();
            f.setPixelSize(std::max(8, fontSize * 3 / 5));
            m_separators[g]->setFont(f);
        }
    }

    m_updatingSizes = false;
}

// ────────────────────────────────────────────
//  创建单组号码行（复用已有标签和分隔符）
// ────────────────────────────────────────────

QHBoxLayout* MainWindow::createGroupRow(int group)
{
    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(0);          // ⚠️ 关键：禁用布局默认间距，只用显式 addSpacing
    row->addStretch();

    // 前区 5 个号码，之间加间距
    for (int i = 0; i < 5; i++) {
        if (i > 0) row->addSpacing(m_spacing);
        row->addWidget(m_frontLabels[group][i]);
    }

    // 分隔符 + 间距
    row->addSpacing(m_spacing);
    row->addWidget(m_separators[group]);

    // 后区 2 个号码，之间加间距
    for (int i = 0; i < 2; i++) {
        row->addSpacing(m_spacing);
        row->addWidget(m_backLabels[group][i]);
    }

    row->addStretch();
    return row;
}

// ────────────────────────────────────────────
//  清空布局中所有项（widget 保留，子布局/spacer 销毁）
// ────────────────────────────────────────────

static void clearLayout(QLayout *layout)
{
    if (!layout) return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout()) {
            // 递归清空子布局
            clearLayout(item->layout());
        }
        delete item;
    }
}

// ────────────────────────────────────────────
//  重建整体布局
// ────────────────────────────────────────────

void MainWindow::rebuildLayout()
{
    if (!m_mainLayout || !m_groupsContainer || !m_groupsLayout)
        return;

    // 1. 清空主布局（widget 保留，子布局/spacer 销毁）
    clearLayout(m_mainLayout);

    // 2. 清空号码容器内部布局（关键：不 delete m_groupsLayout 本身，只清空子项，
    //    避免 m_groupsContainer 的 layout 指针变成悬空）
    clearLayout(m_groupsLayout);

    // 3. 按方向重建号码容器内部布局
    if (m_isLandscape) {
        // 横屏：3×2 网格
        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(10);
        grid->setContentsMargins(0, 0, 0, 0);

        grid->addLayout(createGroupRow(0), 0, 0);
        grid->addLayout(createGroupRow(1), 0, 1);
        grid->addLayout(createGroupRow(2), 1, 0);
        grid->addLayout(createGroupRow(3), 1, 1);

        // 组 4 居中跨两列
        QHBoxLayout *centerRow = new QHBoxLayout();
        centerRow->addStretch();
        centerRow->addLayout(createGroupRow(4));
        centerRow->addStretch();
        grid->addLayout(centerRow, 2, 0, 1, 2);

        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 1);

        m_groupsLayout->setSpacing(0);
        m_groupsLayout->addLayout(grid);
    } else {
        // 竖屏：5 行纵向
        m_groupsLayout->setSpacing(14);
        for (int g = 0; g < 5; g++) {
            m_groupsLayout->addLayout(createGroupRow(g));
        }
    }

    // 4. 按方向重新组装主布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnLock);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch();

    if (m_isLandscape) {
        m_mainLayout->setSpacing(8);
        m_mainLayout->setContentsMargins(24, 8, 24, 8);
        m_mainLayout->addWidget(m_timeLabel, 0, Qt::AlignHCenter);
        m_mainLayout->addSpacing(6);
        m_mainLayout->addWidget(m_groupsContainer);
        m_mainLayout->addSpacing(10);
        m_mainLayout->addLayout(btnLayout);
    } else {
        m_mainLayout->setSpacing(18);
        m_mainLayout->setContentsMargins(40, 30, 40, 30);
        m_mainLayout->addStretch();
        m_mainLayout->addWidget(m_timeLabel, 0, Qt::AlignHCenter);
        m_mainLayout->addSpacing(18);
        m_mainLayout->addWidget(m_groupsContainer);
        m_mainLayout->addSpacing(40);
        m_mainLayout->addLayout(btnLayout);
        m_mainLayout->addStretch();
    }
}

// ────────────────────────────────────────────
//  UI 初始化
// ────────────────────────────────────────────

void MainWindow::init()
{
    ui->setupUi(this);
    setWindowTitle("大乐透随机号码生成器");

    if (!ui->centralwidget) {
        ui->centralwidget = new QWidget(this);
        setCentralWidget(ui->centralwidget);
    }

    // ── 主布局 ──
    m_mainLayout = new QVBoxLayout(ui->centralwidget);

    // ── 时间标签 ──
    m_timeLabel = new QLabel("🕐 生成时间：年-月-日 时:分:秒", ui->centralwidget);
    m_timeLabel->setObjectName("timeLabel");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // ── 检测初始方向 ──
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    m_isLandscape = screen.width() > screen.height();
    ui->centralwidget->setProperty("layoutMode", m_isLandscape ? "landscape" : "portrait");

    // ── 初始标签尺寸估算（首帧展示用，与 updateLabelSizes 同公式）──
    if (m_isLandscape) {
        int refSize = screen.height() / 16;
        m_labelWidth  = std::max(24, std::min(refSize, 44));
        m_labelHeight = m_labelWidth;
        m_spacing        = 6;
        m_separatorWidth = 18;
    } else if (screen.width() < 800) {
        // 竖屏手机：从屏幕宽反推（与 updateLabelSizes 同公式 + 同安全系数）
        int estWidth = (int)((screen.width() - 80) * 0.94);  // 80=margins, 0.94=safety
        if (estWidth < 280) {
            m_spacing = 2; m_separatorWidth = 10;
        } else if (estWidth < 350) {
            m_spacing = 3; m_separatorWidth = 12;
        } else if (estWidth < 440) {
            m_spacing = 4; m_separatorWidth = 14;
        } else {
            m_spacing = 6; m_separatorWidth = 18;
        }
        m_labelWidth  = std::max(22, std::min((estWidth - m_separatorWidth - 7 * m_spacing) / 7, 44));
        m_labelHeight = m_labelWidth;
    } // else 桌面竖屏保持头文件中的默认 40×40

    // ── 号码标签 + 分隔符（只创建一次）──
    for (int g = 0; g < 5; g++) {
        for (int i = 0; i < 5; i++) {
            m_frontLabels[g][i] = new QLabel("?", ui->centralwidget);
            m_frontLabels[g][i]->setObjectName(QString("frontLabel_%1_%2").arg(g).arg(i));
            m_frontLabels[g][i]->setFixedSize(m_labelWidth, m_labelHeight);
            m_frontLabels[g][i]->setAlignment(Qt::AlignCenter);
            m_frontLabels[g][i]->setContentsMargins(0, 0, 0, 0);
            m_frontLabels[g][i]->setProperty("area", "front");
        }
        for (int i = 0; i < 2; i++) {
            m_backLabels[g][i] = new QLabel("?", ui->centralwidget);
            m_backLabels[g][i]->setObjectName(QString("backLabel_%1_%2").arg(g).arg(i));
            m_backLabels[g][i]->setFixedSize(m_labelWidth, m_labelHeight);
            m_backLabels[g][i]->setAlignment(Qt::AlignCenter);
            m_backLabels[g][i]->setContentsMargins(0, 0, 0, 0);
            m_backLabels[g][i]->setProperty("area", "back");
        }
        m_separators[g] = new QLabel("+", ui->centralwidget);
        m_separators[g]->setObjectName(QString("separator_%1").arg(g));
        m_separators[g]->setFixedSize(m_separatorWidth, m_labelHeight);
        m_separators[g]->setAlignment(Qt::AlignCenter);
        m_separators[g]->setContentsMargins(0, 0, 0, 0);
    }

    // ── 号码容器（使用永久 QVBoxLayout，只清空子项不 delete 自身）──
    m_groupsContainer = new QWidget(ui->centralwidget);
    m_groupsContainer->setObjectName("groupsContainer");
    m_groupsLayout = new QVBoxLayout(m_groupsContainer);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);

    // ── 按钮 ──
    m_btnGenerate = new QPushButton("🎲 生成号码", ui->centralwidget);
    m_btnGenerate->setObjectName("btnGenerate");

    m_btnLock = new QPushButton("🔓 锁定号码", ui->centralwidget);
    m_btnLock->setObjectName("btnLock");
    m_btnLock->setCheckable(true);
    m_btnLock->setEnabled(false);

    // ── 构建初始布局 ──
    rebuildLayout();

    // ── 统一样式表 ──
    QFile qss(":/style.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QTextStream(&qss).readAll());
    }

    // ── 信号连接 ──
    connect(m_btnGenerate, &QPushButton::clicked,  this, &MainWindow::generateFiveGroups);
    connect(m_btnLock,     &QPushButton::toggled,  this, &MainWindow::onLockToggled);
    connect(m_btnLock,     &QPushButton::clicked,  this, [this]{ save(); });
}
