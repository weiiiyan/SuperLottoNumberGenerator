#include "mainwindow.h"
#include "lottoengine.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>
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

void MainWindow::generateFiveGroups()
{
    QVector<LottoResult> numbers = m_lottoEngine->generateBatch(5);
    for (int g = 0; g < 5; g++) {
        for (int i = 0; i < 5; i++)
            m_frontLabels[g][i]->setText(QString("%1").arg(numbers[g].frontVec()[i], 2, 10, QChar('0')));
        for (int i = 0; i < 2; i++)
            m_backLabels[g][i]->setText(QString("%1").arg(numbers[g].backVec()[i], 2, 10, QChar('0')));
    }
    m_timeLabel->setText("🕐 生成时间：" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_btnLock->setEnabled(true);
    m_btnLock->setChecked(false);
}

void MainWindow::onLockToggled(bool checked)
{
    m_btnGenerate->setEnabled(!checked);
    m_btnLock->setText(checked ? "🔒 解锁生成" : "🔓 锁定号码");
}

// 宽度 < 800 视为移动端，Label 尺寸按屏幕比例缩放以适应小屏
void MainWindow::adaptToMobile()
{
    QRect screenRect = QApplication::primaryScreen()->availableGeometry();
    int screenWidth = screenRect.width();

    if (screenWidth < 800) {
        m_labelWidth  = screenWidth * 0.08;
        m_labelHeight = screenWidth * 0.08;
    } else {
        m_labelWidth  = 40;
        m_labelHeight = 40;
    }

    m_labelWidth  = qMax(24, m_labelWidth);
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

    m_btnGenerate = new QPushButton("🎲 生成号码", ui->centralwidget);
    m_btnGenerate->setStyleSheet(R"(
        QPushButton {
            font-size:16px; padding:10px;
            background:#E63946; color:white;
            border-radius:8px;
        }
        QPushButton:hover    { background:#FF4D6D; }
        QPushButton:disabled { background:#CCCCCC; color:#666666; }
    )");

    m_btnLock = new QPushButton("🔓 锁定号码", ui->centralwidget);
    m_btnLock->setCheckable(true);
    m_btnLock->setEnabled(false);
    m_btnLock->setStyleSheet(R"(
        QPushButton {
            font-size:16px; padding:10px;
            background:#F4A261; color:#1A1A1A;
            border-radius:8px;
        }
        QPushButton:hover   { background:#E76F51; color:white; }
        QPushButton:checked { background:#C1121F; color:white; }
        QPushButton:disabled{ background:#CCCCCC; color:#666666; }
    )");

    m_mainLayout = new QVBoxLayout(ui->centralwidget);
    m_mainLayout->setSpacing(18);
    m_mainLayout->setContentsMargins(40, 30, 40, 30);
    m_mainLayout->addStretch();

    adaptToMobile();

    // 生成时间标签
    m_timeLabel = new QLabel("🕐 生成时间：年-月-日 时:分:秒", ui->centralwidget);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_timeLabel->setStyleSheet(R"(
        QLabel {
            font-size:16px; color:#C1121F;
            background:rgba(230,57,70,0.10);
            border:1px solid rgba(230,57,70,0.35);
            border-radius:12px;
            padding:4px 20px;
        }
    )");
    m_mainLayout->addWidget(m_timeLabel, 0, Qt::AlignHCenter);
    m_mainLayout->addSpacing(18);

    // 五组号码行
    for (int group = 0; group < 5; group++) {
        QHBoxLayout *row = new QHBoxLayout();
        row->addStretch();

        for (int i = 0; i < 5; i++) {
            m_frontLabels[group][i] = new QLabel("?", ui->centralwidget);
            m_frontLabels[group][i]->setFixedSize(m_labelWidth, m_labelHeight);
            m_frontLabels[group][i]->setStyleSheet(R"(
                QLabel {
                    font-size:20px; color:white;
                    background:red; border:2px solid red; border-radius:6px;
                    qproperty-alignment:AlignCenter;
                }
            )");
            row->addWidget(m_frontLabels[group][i]);
            row->addSpacing(8);
        }

        QLabel *sep = new QLabel("+", ui->centralwidget);
        sep->setFixedSize(20, m_labelHeight);
        sep->setAlignment(Qt::AlignCenter);
        row->addWidget(sep);
        row->addSpacing(8);

        for (int i = 0; i < 2; i++) {
            m_backLabels[group][i] = new QLabel("?", ui->centralwidget);
            m_backLabels[group][i]->setFixedSize(m_labelWidth, m_labelHeight);
            m_backLabels[group][i]->setStyleSheet(R"(
                QLabel {
                    font-size:20px; color:white;
                    background:#0077B6; border:2px solid #0077B6; border-radius:6px;
                    qproperty-alignment:AlignCenter;
                }
            )");
            row->addWidget(m_backLabels[group][i]);
            row->addSpacing(8);
        }

        row->addStretch();
        m_mainLayout->addLayout(row);
    }

    // 按钮行
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnLock);
    btnLayout->addSpacing(12);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch();
    m_mainLayout->addSpacing(40);
    m_mainLayout->addLayout(btnLayout);
    m_mainLayout->addStretch();

    connect(m_btnGenerate, &QPushButton::clicked,  this, &MainWindow::generateFiveGroups);
    connect(m_btnLock,     &QPushButton::toggled,  this, &MainWindow::onLockToggled);
    connect(m_btnLock,     &QPushButton::clicked,  this, [this]{ save(); });
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
    // 异步加载避免启动时 UI 卡顿（Android 尤为明显）
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
