#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "utils/systeminfo.h"
#include "utils/formatters.h"
#include <iostream>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Test SystemInfo functions
    std::cout << "=== Testing SystemInfo Functions ===" << std::endl << std::endl;

    // Initialize CPU monitoring (first call sets up PDH)
    std::cout << "Initializing CPU monitoring..." << std::endl;
    SystemInfo::getCpuUsage();
    
    // Use a timer to get accurate CPU usage after initialization
    QTimer::singleShot(1000, this, [this]() {
        std::cout << std::endl << "--- CPU Usage ---" << std::endl;
        double cpuUsage = SystemInfo::getCpuUsage();
        std::cout << "CPU Usage: " << cpuUsage << "%" << std::endl;
        

        // Test Memory Info
        std::cout << std::endl << "--- Memory Info ---" << std::endl;
        MemoryInfo memInfo = SystemInfo::getMemoryInfo();
        std::cout << "Total Physical: " << Formatters::formatBytes(memInfo.totalPhysical).toStdString() << std::endl;
        std::cout << "Available Physical: " << Formatters::formatBytes(memInfo.availablePhysical).toStdString() << std::endl;
        std::cout << "Used Physical: " << Formatters::formatBytes(memInfo.usedPhysical).toStdString() << std::endl;
        std::cout << "Total Virtual: " << Formatters::formatBytes(memInfo.totalVirtual).toStdString() << std::endl;
        std::cout << "Available Virtual: " << Formatters::formatBytes(memInfo.availableVirtual).toStdString() << std::endl;
        std::cout << "Usage Percentage: " << memInfo.usagePercentage << "%" << std::endl;


        // Test Disk Info
        std::cout << std::endl << "--- Disk Info ---" << std::endl;
        QVector<DiskInfo> disks = SystemInfo::getDiskInfo();
        for (int i = 0; i < disks.size(); ++i) {
            const DiskInfo &disk = disks[i];
            std::cout << "Disk " << (i + 1) << ":" << std::endl;
            std::cout << "  Name: " << disk.name.toStdString() << std::endl;
            std::cout << "  Mount Point: " << disk.mountPoint.toStdString() << std::endl;
            std::cout << "  Total Space: " << Formatters::formatBytes(disk.totalSpace).toStdString() << std::endl;
            std::cout << "  Used Space: " << Formatters::formatBytes(disk.usedSpace).toStdString() << std::endl;
            std::cout << "  Available Space: " << Formatters::formatBytes(disk.availableSpace).toStdString() << std::endl;
            std::cout << "  Usage: " << disk.usagePercentage << "%" << std::endl;
            std::cout << "  File System: " << disk.fileSystem.toStdString() << std::endl;
            std::cout << std::endl;
        }

        // Test Network Stats
        std::cout << "--- Network Stats ---" << std::endl;
        NetworkStats netStats = SystemInfo::getNetworkStats();
        std::cout << "Interface: " << netStats.interfaceName.toStdString() << std::endl;
        std::cout << "Bytes Received: " << Formatters::formatBytes(netStats.bytesReceived).toStdString() << std::endl;
        std::cout << "Bytes Sent: " << Formatters::formatBytes(netStats.bytesSent).toStdString() << std::endl;
        std::cout << "Download Speed: " << netStats.downloadSpeedKBps << " KB/s" << std::endl;
        std::cout << "Upload Speed: " << netStats.uploadSpeedKBps << " KB/s" << std::endl;
        
        // Test Top Processes
        std::cout << std::endl << "--- Top 10 Processes (by memory) ---" << std::endl;
        QVector<ProcessInfo> processes = SystemInfo::getTopProcesses(10);
        for (int i = 0; i < processes.size(); ++i) {
            const ProcessInfo &proc = processes[i];
            std::cout << (i + 1) << ". " << proc.name.toStdString() << std::endl;
            std::cout << "   PID: " << proc.pid << std::endl;
            std::cout << "   Memory: " << Formatters::formatBytes(proc.memoryUsage).toStdString() << std::endl;
            std::cout << "   CPU: " << proc.cpuUsage << "%" << std::endl;
            std::cout << "   Status: " << proc.status.toStdString() << std::endl;
        }
        
        std::cout << std::endl << "=== Testing Complete ===" << std::endl;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
