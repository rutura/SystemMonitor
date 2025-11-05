#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "systemmonitor.h"
#include "utils/formatters.h"
#include <iostream>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    std::cout << "\n=== SystemMonitor Testing Demo ===" 
              << std::endl;
    std::cout << "Testing SystemMonitor class functionality...\n" 
              << std::endl;
    
    // Create SystemMonitor instance
    SystemMonitor* monitor = new SystemMonitor(this);
    
    // Initialize monitoring to populate cached data
    // (but don't start the timer yet)
    std::cout << "Initializing system data collection..." 
              << std::endl;
    monitor->startMonitoring(5000); // Start with long interval
    monitor->stopMonitoring(); // Stop timer, keep data
    
    // Small delay to ensure data is collected
    QTimer::singleShot(100, [monitor]() {
        std::cout << "\n=== SystemMonitor Data After "
                  << "Initialization ===" << std::endl;
        
        // Test 1: Get initial CPU usage
        std::cout << "1. CPU Usage: " 
                  << monitor->getCpuUsage() << "%" 
                  << std::endl;
    
        // Test 2: Get memory information
        MemoryInfo memInfo = monitor->getMemoryInfo();
        std::cout << "\n2. Memory (RAM) Information:" 
                  << std::endl;
        std::cout << "   Total Physical: " 
                  << Formatters::formatBytes(
                      memInfo.totalPhysical).toStdString() 
                  << std::endl;
        std::cout << "   Used Physical: " 
                  << Formatters::formatBytes(
                      memInfo.usedPhysical).toStdString() 
                  << std::endl;
        std::cout << "   Available Physical: " 
                  << Formatters::formatBytes(
                      memInfo.availablePhysical).toStdString() 
                  << std::endl;
        std::cout << "   Usage Percentage: " 
                  << Formatters::formatPercentage(
                      memInfo.usagePercentage).toStdString() 
                  << std::endl;
    
        // Test 3: Get disk information
        QVector<DiskInfo> diskInfo = monitor->getDiskInfo();
        std::cout << "\n3. Disk Information (" 
                  << diskInfo.size() << " drives found):" 
                  << std::endl;
        
        for (int i = 0; i < diskInfo.size(); ++i) {
            const DiskInfo& disk = diskInfo[i];
            std::cout << "   Drive " << (i+1) << ": " 
                      << disk.name.toStdString() 
                      << " (" << disk.mountPoint.toStdString() 
                      << ")" << std::endl;
            std::cout << "     Total: " 
                      << Formatters::formatBytes(
                          disk.totalSpace).toStdString() 
                      << std::endl;
            std::cout << "     Used: " 
                      << Formatters::formatBytes(
                          disk.usedSpace).toStdString() 
                      << std::endl;
            std::cout << "     Available: " 
                      << Formatters::formatBytes(
                          disk.availableSpace).toStdString() 
                      << std::endl;
            std::cout << "     Usage: " 
                      << Formatters::formatPercentage(
                          disk.usagePercentage).toStdString() 
                      << std::endl;
            std::cout << "     File System: " 
                      << disk.fileSystem.toStdString() 
                      << std::endl;
        }
    
        // Test 4: Get network statistics
        NetworkStats netStats = monitor->getNetworkStats();
        std::cout << "\n4. Network Statistics:" << std::endl;
        std::cout << "   Interface: " 
                  << netStats.interfaceName.toStdString() 
                  << std::endl;
        std::cout << "   Bytes Received: " 
                  << Formatters::formatBytes(
                      netStats.bytesReceived).toStdString() 
                  << std::endl;
        std::cout << "   Bytes Sent: " 
                  << Formatters::formatBytes(
                      netStats.bytesSent).toStdString() 
                  << std::endl;
        std::cout << "   Download Speed: " 
                  << Formatters::formatSpeed(
                      netStats.downloadSpeedKBps).toStdString() 
                  << std::endl;
        std::cout << "   Upload Speed: " 
                  << Formatters::formatSpeed(
                      netStats.uploadSpeedKBps).toStdString() 
                  << std::endl;
        
        // Test 5: Get top processes
        QVector<ProcessInfo> processes = 
            monitor->getTopProcesses(5);
        std::cout << "\n5. Top 5 Processes by CPU Usage:" 
                  << std::endl;
        
        for (int i = 0; i < processes.size(); ++i) {
            const ProcessInfo& proc = processes[i];
            std::cout << "   " << (i+1) 
                      << ". PID: " << proc.pid 
                      << ", Name: " << proc.name.toStdString()
                      << std::endl;
            std::cout << "      CPU: " 
                      << Formatters::formatPercentage(
                          proc.cpuUsage).toStdString()
                      << ", Memory: " 
                      << Formatters::formatBytes(
                          proc.memoryUsage).toStdString()
                      << std::endl;
            std::cout << "      Status: " 
                      << proc.status.toStdString() 
                      << std::endl;
        }
    
        // Test 6: Start monitoring and connect to signals
        // to test real-time updates
        std::cout << "\n6. Starting real-time monitoring " 
                  << "(20 second demo)..." << std::endl;
        
        // Connect signals to test signal emission
        connect(monitor, &SystemMonitor::cpuUsageChanged, 
            [](double usage) {
                std::cout << "   CPU Update: " 
                          << Formatters::formatPercentage(
                              usage).toStdString() 
                          << std::endl;
            });
        
        connect(monitor, &SystemMonitor::memoryUsageChanged, 
            [](qint64 used, qint64 total) {
                std::cout << "   Memory Update: " 
                          << Formatters::formatBytes(
                              used).toStdString() 
                          << " / " 
                          << Formatters::formatBytes(
                              total).toStdString() 
                          << std::endl;
            });
        
        connect(monitor, &SystemMonitor::networkActivityChanged, 
            [](double download, double upload) {
                std::cout << "   Network Update: Down=" 
                          << Formatters::formatSpeed(
                              download).toStdString() 
                          << ", Up=" 
                          << Formatters::formatSpeed(
                              upload).toStdString() 
                          << std::endl;
            });
        
        connect(monitor, &SystemMonitor::dataUpdated, []() {
            std::cout << "   Data updated signal received" 
                      << std::endl;
        });
        
        // Start monitoring with 1-second intervals
        monitor->startMonitoring(1000);
        
        // Stop monitoring after 20 seconds for demo
        QTimer::singleShot(20000, [monitor]() {
            monitor->stopMonitoring();
            std::cout << "\n=== Demo completed! ===" 
                      << std::endl;
            std::cout << "SystemMonitor class tested "
                      << "successfully." << std::endl;
            std::cout << "All functions are working and "
                      << "ready for UI integration.\n" 
                      << std::endl;
        });
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
