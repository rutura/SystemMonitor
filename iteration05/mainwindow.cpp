#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "systemmonitor.h"
#include "widgets/infocard.h"
#include "widgets/chartwidget.h"
#include <QWidget>
#include <QGridLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_cpuCard(nullptr)
    , m_memoryCard(nullptr)
    , m_diskCard(nullptr)
    , m_networkCard(nullptr)
    , m_cpuChart(nullptr)
    , m_memoryChart(nullptr)
    , m_networkChart(nullptr)
    , m_systemMonitor(nullptr)
{
    ui->setupUi(this);
    
    // Create central widget with grid layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QGridLayout *gridLayout = new QGridLayout(centralWidget);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(20, 20, 20, 20);
    
    // Setup the charts
    setupCharts();
    
    // Add charts to grid layout (2x2 grid)
    gridLayout->addWidget(m_cpuChart, 0, 0);
    gridLayout->addWidget(m_memoryChart, 0, 1);
    gridLayout->addWidget(m_networkChart, 1, 0, 1, 2); // Span 2 columns
    
    // Setup system monitoring
    setupSystemMonitor();
}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::setupCharts()
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
}

void MainWindow::setupSystemMonitor()
{
    // Create SystemMonitor instance
    m_systemMonitor = new SystemMonitor(this);
    
    // Connect signals to update InfoCards
    connect(m_systemMonitor, &SystemMonitor::cpuUsageChanged,
            this, &MainWindow::onCpuUsageChanged);
    
    connect(m_systemMonitor, &SystemMonitor::memoryUsageChanged,
            this, &MainWindow::onMemoryUsageChanged);
    
    connect(m_systemMonitor, &SystemMonitor::networkActivityChanged,
            this, &MainWindow::onNetworkActivityChanged);
    
    connect(m_systemMonitor, &SystemMonitor::dataUpdated,
            this, &MainWindow::updateDiskUsage);
    
    // Start monitoring with 1 second intervals
    m_systemMonitor->startMonitoring(1000);
}

void MainWindow::onCpuUsageChanged(double usage)
{
    if (m_cpuCard) {
        m_cpuCard->setValue(QString::number(usage, 'f', 1) + "%");
        m_cpuCard->setPercentage(usage);
    }
    
    if (m_cpuChart) {
        m_cpuChart->addDataPoint(usage);
    }
}

void MainWindow::onMemoryUsageChanged(qint64 used, qint64 total)
{
    if (m_memoryCard && total > 0) {
        double usagePercent = (static_cast<double>(used) / total) * 100.0;
        
        // Format memory usage in GB
        double usedGB = used / (1024.0 * 1024.0 * 1024.0);
        double totalGB = total / (1024.0 * 1024.0 * 1024.0);
        
        m_memoryCard->setValue(QString("%1 / %2 GB")
                              .arg(usedGB, 0, 'f', 1)
                              .arg(totalGB, 0, 'f', 1));
        m_memoryCard->setPercentage(usagePercent);
    }
    
    if (m_memoryChart && total > 0) {
        double usagePercent = (static_cast<double>(used) / total) * 100.0;
        m_memoryChart->addDataPoint(usagePercent);
    }
}

void MainWindow::onNetworkActivityChanged(double downloadKBps, double uploadKBps)
{
    if (m_networkCard) {
        double totalKBps = downloadKBps + uploadKBps;
        QString speedText;
        
        if (totalKBps < 1024) {
            speedText = QString::number(totalKBps, 'f', 1) + " KB/s";
        } else {
            double mbps = totalKBps / 1024.0;
            speedText = QString::number(mbps, 'f', 1) + " MB/s";
        }
        
        m_networkCard->setValue(speedText);
        
        // Set percentage based on a reasonable scale (0-100 MB/s = 0-100%)
        double percentage = qMin(100.0, (totalKBps / 1024.0) / 100.0 * 100.0);
        m_networkCard->setPercentage(percentage);
    }
    
    if (m_networkChart) {
        // Add download speed to the chart
        m_networkChart->addDataPoint(downloadKBps);
    }
}

void MainWindow::updateDiskUsage()
{
    if (m_diskCard && m_systemMonitor) {
        auto diskInfo = m_systemMonitor->getDiskInfo();
        
        if (!diskInfo.isEmpty()) {
            // Use the first disk (usually C: on Windows or / on Linux)
            const auto &disk = diskInfo.first();
            double usagePercent = (static_cast<double>(disk.usedSpace) / disk.totalSpace) * 100.0;
            
            // Format disk usage in GB
            double usedGB = disk.usedSpace / (1024.0 * 1024.0 * 1024.0);
            double totalGB = disk.totalSpace / (1024.0 * 1024.0 * 1024.0);
            
            m_diskCard->setValue(QString("%1 / %2 GB")
                                .arg(usedGB, 0, 'f', 1)
                                .arg(totalGB, 0, 'f', 1));
            m_diskCard->setPercentage(usagePercent);
        }
    }
}
