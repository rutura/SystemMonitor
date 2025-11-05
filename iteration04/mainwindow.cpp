#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "systemmonitor.h"
#include "widgets/infocard.h"
#include "utils/formatters.h"
#include <QWidget>
#include <QGridLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_cpuCard(nullptr)
    , m_memoryCard(nullptr)
    , m_diskCard(nullptr)
    , m_networkCard(nullptr)
    , m_systemMonitor(nullptr)
{
    ui->setupUi(this);
    
    // Create central widget with grid layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QGridLayout *gridLayout = new QGridLayout(centralWidget);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(20, 20, 20, 20);
    
    // Setup the info cards
    setupCards();
    
    // Add cards to grid layout (2x2 grid)
    gridLayout->addWidget(m_cpuCard, 0, 0);
    gridLayout->addWidget(m_memoryCard, 0, 1);
    gridLayout->addWidget(m_diskCard, 1, 0);
    gridLayout->addWidget(m_networkCard, 1, 1);
    
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
        m_cpuCard->setValue(Formatters::formatPercentage(usage));
        m_cpuCard->setPercentage(usage);
    }
}

void MainWindow::onMemoryUsageChanged(qint64 used, qint64 total)
{
    if (m_memoryCard && total > 0) {
        double usagePercent = (static_cast<double>(used) / total) * 100.0;
        
        m_memoryCard->setValue(QString("%1 / %2")
                              .arg(Formatters::formatBytes(used))
                              .arg(Formatters::formatBytes(total)));
        m_memoryCard->setPercentage(usagePercent);
    }
}

void MainWindow::onNetworkActivityChanged(double downloadKBps, double uploadKBps)
{
    if (m_networkCard) {
        double totalKBps = downloadKBps + uploadKBps;
        
        m_networkCard->setValue(Formatters::formatSpeed(totalKBps));
        
        // Set percentage based on a reasonable scale (0-100 MB/s = 0-100%)
        double percentage = qMin(100.0, (totalKBps / 1024.0) / 100.0 * 100.0);
        m_networkCard->setPercentage(percentage);
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
            
            m_diskCard->setValue(QString("%1 / %2")
                                .arg(Formatters::formatBytes(disk.usedSpace))
                                .arg(Formatters::formatBytes(disk.totalSpace)));
            m_diskCard->setPercentage(usagePercent);
        }
    }
}
