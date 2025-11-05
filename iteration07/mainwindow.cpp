#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "systemmonitor.h"
#include "widgets/infocard.h"
#include "widgets/chartwidget.h"
#include "widgets/processtablewidget.h"
#include "utils/formatters.h"
#include "utils/theme.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QLabel>
#include <QGroupBox>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <QPalette>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_monitor(new SystemMonitor(this))
    , m_cpuCard(nullptr)
    , m_memoryCard(nullptr)
    , m_diskCard(nullptr)
    , m_networkCard(nullptr)
    , m_cpuChart(nullptr)
    , m_memoryChart(nullptr)
    , m_networkChart(nullptr)
    , m_processTable(nullptr)
    , m_tabWidget(nullptr)
    , m_statusTimer(new QTimer(this))
    , m_updateCount(0)
{
    ui->setupUi(this);
    
    // Set application window icon
    setWindowIcon(QIcon(":/icons/resources/icons/app-icon.png"));
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    connectSignals();

    // Apply initial light mode explicitly
    setDarkMode(false);

    // Start monitoring
    m_monitor->startMonitoring(1000);

    // Update status bar every second
    m_statusTimer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    setWindowTitle("Desktop System Monitor - LearnQt");
    resize(1200, 600);

    // Central widget with main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // Setup cards and tabs
    setupCards();

    // Add cards to main layout
    QGridLayout *cardsLayout = new QGridLayout();
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(m_cpuCard, 0, 0);
    cardsLayout->addWidget(m_memoryCard, 0, 1);
    cardsLayout->addWidget(m_diskCard, 1, 0);
    cardsLayout->addWidget(m_networkCard, 1, 1);

    mainLayout->addLayout(cardsLayout);

    // Setup tabs
    setupTabs();

    // Add tabs to main layout
    m_tabWidget = new QTabWidget(this);

    // CPU Tab
    QWidget *cpuTab = new QWidget();
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuTab);
    cpuLayout->setContentsMargins(16, 16, 16, 16);
    cpuLayout->addWidget(m_cpuChart, 0);
    cpuLayout->addWidget(new QLabel("<b>Top Processes by Memory Usage</b>"), 0);
    cpuLayout->addWidget(m_processTable, 1);
    m_tabWidget->addTab(cpuTab, "CPU & Processes");

    // Memory Tab
    QWidget *memoryTab = new QWidget();
    QVBoxLayout *memoryLayout = new QVBoxLayout(memoryTab);
    memoryLayout->setContentsMargins(16, 16, 16, 16);
    memoryLayout->addWidget(m_memoryChart);
    m_tabWidget->addTab(memoryTab, "Memory");

    // Network Tab
    QWidget *networkTab = new QWidget();
    QVBoxLayout *networkLayout = new QVBoxLayout(networkTab);
    networkLayout->setContentsMargins(16, 16, 16, 16);
    networkLayout->addWidget(m_networkChart);
    m_tabWidget->addTab(networkTab, "Network");

    mainLayout->addWidget(m_tabWidget, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    QAction *themeAction = viewMenu->addAction("Toggle &Dark Mode");
    themeAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("System Monitor Ready");

    connect(m_statusTimer, &QTimer::timeout, [this]() {
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        statusBar()->showMessage(QString("Last Update: %1 | Updates: %2")
                                     .arg(timeStr)
                                     .arg(m_updateCount));
    });
}

void MainWindow::setupCards()
{
    // CPU Card
    m_cpuCard = new InfoCard("CPU Usage", this);
    m_cpuCard->setIcon(QIcon(":/icons/resources/icons/cpu.png"));
    m_cpuCard->setValue("0%");
    m_cpuCard->setPercentage(0);
    m_cpuCard->setSubtitle("Processor activity");

    // Memory Card
    m_memoryCard = new InfoCard("Memory", this);
    m_memoryCard->setIcon(QIcon(":/icons/resources/icons/memory.png"));
    m_memoryCard->setValue("0%");
    m_memoryCard->setPercentage(0);
    m_memoryCard->setSubtitle("RAM usage");

    // Disk Card
    m_diskCard = new InfoCard("Disk", this);
    m_diskCard->setIcon(QIcon(":/icons/resources/icons/disk.png"));
    m_diskCard->setValue("0%");
    m_diskCard->setPercentage(0);
    m_diskCard->setSubtitle("Storage usage");

    // Network Card
    m_networkCard = new InfoCard("Network", this);
    m_networkCard->setIcon(QIcon(":/icons/resources/icons/network.png"));
    m_networkCard->setValue("0 KB/s");
    m_networkCard->setPercentage(0);
    m_networkCard->setSubtitle("Network activity");
}

void MainWindow::setupTabs()
{
    // CPU Chart
    m_cpuChart = new ChartWidget("CPU Usage Over Time", this);
    m_cpuChart->setColor(QColor("#2196F3"));
    m_cpuChart->setMinimumHeight(180);
    m_cpuChart->setMaximumHeight(200);

    // Memory Chart
    m_memoryChart = new ChartWidget("Memory Usage Over Time", this);
    m_memoryChart->setColor(QColor("#4CAF50"));
    m_memoryChart->setMinimumHeight(300);

    // Network Chart
    m_networkChart = new ChartWidget("Network Activity (Download Speed)", this);
    m_networkChart->setColor(QColor("#FF9800"));
    m_networkChart->setYAxisRange(0, 1000);
    m_networkChart->setMinimumHeight(300);

    // Process Table
    m_processTable = new ProcessTableWidget(this);
}

void MainWindow::connectSignals()
{
    connect(m_monitor, &SystemMonitor::dataUpdated, this, &MainWindow::updateUI);
}

void MainWindow::updateUI()
{
    m_updateCount++;

    // Update CPU card and chart
    double cpuUsage = m_monitor->getCpuUsage();
    m_cpuCard->setValue(Formatters::formatPercentage(cpuUsage));
    m_cpuCard->setPercentage(cpuUsage);
    m_cpuChart->addDataPoint(cpuUsage);

    // Update Memory card and chart
    MemoryInfo memInfo = m_monitor->getMemoryInfo();
    m_memoryCard->setValue(Formatters::formatPercentage(memInfo.usagePercentage));
    m_memoryCard->setPercentage(memInfo.usagePercentage);
    m_memoryCard->setSubtitle(QString("%1 / %2")
                                  .arg(Formatters::formatBytes(memInfo.usedPhysical))
                                  .arg(Formatters::formatBytes(memInfo.totalPhysical)));
    m_memoryChart->addDataPoint(memInfo.usagePercentage);

    // Update Disk card
    QVector<DiskInfo> disks = m_monitor->getDiskInfo();
    if (!disks.isEmpty()) {
        const DiskInfo &mainDisk = disks.first();
        m_diskCard->setValue(Formatters::formatPercentage(mainDisk.usagePercentage));
        m_diskCard->setPercentage(mainDisk.usagePercentage);
        m_diskCard->setSubtitle(QString("%1 / %2")
                                    .arg(Formatters::formatBytes(mainDisk.usedSpace))
                                    .arg(Formatters::formatBytes(mainDisk.totalSpace)));
    }

    // Update Network card and chart
    NetworkStats netStats = m_monitor->getNetworkStats();
    m_networkCard->setValue(Formatters::formatSpeed(netStats.downloadSpeedKBps));
    m_networkCard->setSubtitle(QString("Up: %1").arg(Formatters::formatSpeed(netStats.uploadSpeedKBps)));

    // Update network chart (scale based on current speed)
    double maxSpeed = qMax(100.0, netStats.downloadSpeedKBps * 1.5);
    m_networkChart->setYAxisRange(0, maxSpeed);
    m_networkChart->addDataPoint(netStats.downloadSpeedKBps);

    // Update process table
    QVector<ProcessInfo> processes = m_monitor->getTopProcesses(10);
    m_processTable->updateProcessList(processes);
}

void MainWindow::showAboutDialog()
{
    QString aboutText =
        "<h2>Desktop System Monitor v1.0</h2>"
        "<p>A real-time system performance monitoring application built with Qt Widgets and modern C++.</p>"
        "<p><b>Learn to build professional Qt applications:</b><br>"
        "<a href='https://www.learnqt.guide/courses/qt-widgets-cpp/'>Qt Widgets C++ Course</a></p>"
        "<p>This project demonstrates fundamental Qt concepts. "
        "Want to learn advanced patterns, threading, Model/View architecture, and more?</p>"
        "<p>Visit <a href='https://www.learnqt.guide/'>LearnQt.Guide</a> for more resources!</p>"
        "<hr>"
        "<p><small>Built with Qt and C++<br>"
        "MIT License - Free for learning and personal projects</small></p>";

    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About System Monitor");
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutText);
    aboutBox.setIcon(QMessageBox::Information);
    aboutBox.exec();
}

void MainWindow::toggleTheme()
{
    bool currentlyDark = Theme::isDarkMode();
    setDarkMode(!currentlyDark);
}

void MainWindow::setDarkMode(bool dark)
{
    if (dark) {
        qApp->setPalette(Theme::createDarkPalette());
        if (centralWidget()) {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(Theme::DARK_BACKGROUND));
        }
        if (m_tabWidget) {
            m_tabWidget->setStyleSheet(Theme::getDarkTabWidgetStyle());
        }
        menuBar()->setStyleSheet(Theme::getDarkMenuBarStyle());
    } else {
        qApp->setPalette(Theme::createLightPalette());
        if (centralWidget()) {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(Theme::LIGHT_BACKGROUND));
        }
        if (m_tabWidget) {
            m_tabWidget->setStyleSheet(Theme::getLightTabWidgetStyle());
        }
        menuBar()->setStyleSheet(Theme::getLightMenuBarStyle());
    }

    // Apply theme the cards and charts
    applyTheme();
}

void MainWindow::applyTheme()
{
    // Update all cards
    if (m_cpuCard) m_cpuCard->updateTheme();
    if (m_memoryCard) m_memoryCard->updateTheme();
    if (m_diskCard) m_diskCard->updateTheme();
    if (m_networkCard) m_networkCard->updateTheme();

    // Update all charts
    if (m_cpuChart) m_cpuChart->updateTheme();
    if (m_memoryChart) m_memoryChart->updateTheme();
    if (m_networkChart) m_networkChart->updateTheme();

    // Update process table
    if (m_processTable) m_processTable->updateTheme();
}
