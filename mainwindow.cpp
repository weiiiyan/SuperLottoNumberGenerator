#include <QList>
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QSettings>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QFontMetrics> // 新增：字体尺寸计算
#include <QRandomGenerator>

#include "mainwindow.h"
#include "lottoengine.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), m_lottoEngine(new LottoEngine(this))
{
    init();
    restore();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 生成5组
void MainWindow::generateFiveGroups()
{
    QVector<LottoResult> numbers = m_lottoEngine->generateBatch(5);
    for(int g = 0; g < 5; g++) {

        for(int i = 0; i < 5; i++) {
            m_frontLabels[g][i]->setText(QString("%1").arg(numbers[g].frontVec()[i], 2, 10, QChar('0')));
        }

        for(int i = 0; i < 2; i++) {
            m_backLabels[g][i]->setText(QString("%1").arg(numbers[g].backVec()[i], 2, 10, QChar('0')));
        }
    }
    // 生成后启用锁定按钮
    m_btnLock->setEnabled(true);
    m_btnLock->setChecked(false);
}

// 锁定/解锁逻辑
void MainWindow::onLockToggled(bool checked)
{
    if (checked) {
        // 锁定状态：禁用生成按钮，保持当前号码
        m_btnGenerate->setEnabled(false);

        m_btnLock->setText("🔒 解锁生成");
    } else {
        // 解锁状态：恢复生成按钮可用
        m_btnGenerate->setEnabled(true);

        m_btnLock->setText("🔓 锁定号码");
    }
}

// 新增：移动端适配逻辑（核心：动态计算Label固定尺寸）
void MainWindow::adaptToMobile()
{
    // 获取屏幕尺寸
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    int screenWidth = screenRect.width();
    int screenHeight = screenRect.height();

    // 适配手机屏幕（宽度<800判定为移动端）
    if (screenWidth < 800) {
        // 计算Label尺寸：按屏幕宽度比例分配，保证5组+分隔符+间距能放下
        m_labelWidth = screenWidth * 0.08;  // 屏幕宽度8%作为Label宽度
        m_labelHeight = screenWidth * 0.08; // 宽高相等，正方形
    } else {
        // 桌面端：使用默认尺寸（保持原有逻辑）
        m_labelWidth = 40;
        m_labelHeight = 40;
    }

    // 确保尺寸为整数且最小值限制
    m_labelWidth = qMax(24, m_labelWidth);
    m_labelHeight = qMax(24, m_labelHeight);
}

void MainWindow::init()
{
    ui->setupUi(this);
    setWindowTitle("大乐透随机号码生成器");

    if (!ui->centralwidget) {
        ui->centralwidget = new QWidget(this);
        setCentralWidget(ui->centralwidget);
    }

    // 生成按钮
    m_btnGenerate = new QPushButton("🎲 生成号码", ui->centralwidget);
    // 生成按钮样式（启用：红色；禁用：灰色）
    m_btnGenerate->setStyleSheet(R"(
            QPushButton{
                font-size:16px; padding:10px;
                background:#E63946; color:white;
                border-radius:8px;
            }
            QPushButton:hover{ background:#FF4D6D; }
            QPushButton:disabled{
                background:#CCCCCC; color:#666666;
            }
        )");

    // 锁定按钮
    m_btnLock = new QPushButton("🔓 锁定号码", ui->centralwidget);
    m_btnLock->setCheckable(true);
    m_btnLock->setChecked(false);
    m_btnLock->setEnabled(false); // 初始禁用（无号码时不可锁）
    m_btnLock->setStyleSheet(R"(
            QPushButton{
                font-size:16px; padding:10px;
                background:#F4A261; color:#1A1A1A;   /* 沙橙色，深字 */
                border-radius:8px;
            }
            QPushButton:hover{
                background:#E76F51;
                color:white;
            }
            QPushButton:checked{
                background:#C1121F; color:white;    /* 深锈红 */
            }
            QPushButton:disabled{
                background:#CCCCCC; color:#666666;
            }
        )");

    // 布局初始化
    m_mainLayout = new QVBoxLayout(ui->centralwidget);
    m_mainLayout->addStretch();
    m_mainLayout->setSpacing(18);
    m_mainLayout->setContentsMargins(40,30,40,30);

    // 移动端适配（核心：根据屏幕尺寸计算Label固定大小）
    adaptToMobile();

    // 创建5组号码
    for(int group=0; group<5; group++)
    {
        QHBoxLayout* row = new QHBoxLayout();
        row->addStretch();

        // 前区5个（固定宽高）
        for(int i=0; i<5; i++){
            m_frontLabels[group][i] = new QLabel("?", ui->centralwidget);
            m_frontLabels[group][i]->setStyleSheet(R"(
                QLabel{
                    font-size:20px; color:white;
                    border:2px solid red; border-radius:6px;
                    qproperty-alignment:AlignCenter; /* 文本居中 */
                    background:red; /* 增加背景色，视觉更整齐 */
                }
            )");
            // 关键：固定Label尺寸（适配后的值）
            m_frontLabels[group][i]->setFixedSize(m_labelWidth, m_labelHeight);
            row->addWidget(m_frontLabels[group][i]);
            row->addSpacing(8); // 增大间距，视觉更舒适
        }

        // 分隔符
        QLabel* splitLabel = new QLabel("+", ui->centralwidget);
        splitLabel->setFixedSize(20, m_labelHeight); // 分隔符也固定尺寸
        splitLabel->setAlignment(Qt::AlignCenter);
        row->addWidget(splitLabel);
        row->addSpacing(8);

        // 后区2个（固定宽高）
        for(int i=0; i<2; i++){
            m_backLabels[group][i] = new QLabel("?", ui->centralwidget);
            m_backLabels[group][i]->setStyleSheet(R"(
                QLabel{
                    font-size:20px; color:white;
                    border:2px solid #0077B6; border-radius:6px;
                    qproperty-alignment:AlignCenter;
                    background:#0077B6;
                }
            )");
            // 关键：固定Label尺寸
            m_backLabels[group][i]->setFixedSize(m_labelWidth, m_labelHeight);
            row->addWidget(m_backLabels[group][i]);
            row->addSpacing(8);
        }

        row->addStretch();
        m_mainLayout->addLayout(row);
    }

    // 按钮行：生成 + 锁定
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnLock);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch();
    m_mainLayout->addSpacing(40);
    m_mainLayout->addLayout(btnLayout);
    m_mainLayout->addStretch();

    connect(m_btnGenerate, &QPushButton::clicked, this, &MainWindow::generateFiveGroups);
    connect(m_btnLock, &QPushButton::toggled, this, &MainWindow::onLockToggled);
    connect(m_btnLock, &QPushButton::clicked, this, [this](){
        save();
    });
}

void MainWindow::save() const
{
    QSettings settings("SuperLottoNumberGenerator", "MainWindow");
    if (m_btnLock->isChecked()) {
        settings.setValue("isLocked", true);
        // 保存5组号码
        QVariantList frontList, backList;
        for (int g = 0; g < 5; ++g) {
            QVariantList frontGroup, backGroup;
            for (int i = 0; i < 5; ++i) {
                frontGroup << m_frontLabels[g][i]->text().toInt();
            }
            for (int i = 0; i < 2; ++i) {
                backGroup << m_backLabels[g][i]->text().toInt();
            }
            frontList << frontGroup;
            backList << backGroup;
        }
        settings.setValue("frontNumbers", frontList);
        settings.setValue("backNumbers", backList);
    } else {
        settings.setValue("isLocked", false);
    }
    settings.sync();
}

void MainWindow::restore()
{
    // ✅ 2. 异步加载（避免 UI 卡顿，特别是andorid）
    QTimer::singleShot(0, this, [this] {
        QSettings settings("SuperLottoNumberGenerator", "MainWindow");
        bool isLocked = settings.value("isLocked", false).toBool();
        if (isLocked) {
            // 恢复锁定状态
            m_btnLock->setChecked(true);

            // 恢复5组号码
            QVariantList frontNumbers = settings.value("frontNumbers").toList();
            QVariantList backNumbers = settings.value("backNumbers").toList();
            for (int g = 0; g < 5; ++g) {
                for (int i = g * 5; i < (g + 1) * 5 && i < frontNumbers.size(); ++i) {
                    m_frontLabels[g][i % 5]->setText(QString("%1").arg(frontNumbers[i].toInt(), 2, 10, QChar('0')));
                }
                for (int i = g * 2; i < (g + 1) * 2 && i < backNumbers.size(); ++i) {
                    m_backLabels[g][i % 2]->setText(QString("%1").arg(backNumbers[i].toInt(), 2, 10, QChar('0')));
                }
            }

            // 启用锁定按钮（确保可见）
            m_btnLock->setEnabled(true);
        }
    });
}