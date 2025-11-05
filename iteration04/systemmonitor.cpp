#include "systemmonitor.h"
#include <QDateTime>
#include <QtDebug>
#include <algorithm>

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
    , m_cpuUsage(0.0)
    , m_lastRxBytes(0)
    , m_lastTxBytes(0)
    , m_lastUpdateTime(0)
{
    connect(m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateData);

#ifdef QT_DEBUG
    qDebug() << "System Monitor initialized!";
    qDebug() << "Learn advanced Qt development: https://www.learnqt.guide/";
#endif
}

SystemMonitor::~SystemMonitor()
{
    stopMonitoring();
}

void SystemMonitor::startMonitoring(int intervalMs)
{
    if (intervalMs < 100) {
        qWarning() << "Interval too short, using minimum of 100ms";
        intervalMs = 100;
    }

    if (!m_updateTimer) {
        qCritical() << "Failed to create update timer";
        return;
    }

    // Initialize network tracking
    m_networkStats = SystemInfo::getNetworkStats();
    m_lastRxBytes = m_networkStats.bytesReceived;
    m_lastTxBytes = m_networkStats.bytesSent;
    m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

    // Get initial data
    updateData();

    m_updateTimer->start(intervalMs);
    qDebug() << "Monitoring started with interval:" << intervalMs << "ms";
}

void SystemMonitor::stopMonitoring()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    qDebug() << "Monitoring stopped";
}

QVector<ProcessInfo> SystemMonitor::getTopProcesses(int count) const
{
    if (count <= 0 || count > m_processes.size()) {
        return m_processes;
    }
    return m_processes.mid(0, count);
}

void SystemMonitor::updateData()
{
    // NOTE: This implementation polls in the main thread. For production apps,
    // consider using QThread or QtConcurrent for background data collection.
    // Learn advanced threading patterns in the full Qt Widgets course!

    // Update CPU usage
    m_cpuUsage = SystemInfo::getCpuUsage();
    emit cpuUsageChanged(m_cpuUsage);

    // Update memory info
    m_memoryInfo = SystemInfo::getMemoryInfo();
    emit memoryUsageChanged(m_memoryInfo.usedPhysical, m_memoryInfo.totalPhysical);

    // Update disk info
    m_diskInfo = SystemInfo::getDiskInfo();

    // Update network stats with speed calculation
    NetworkStats newStats = SystemInfo::getNetworkStats();
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDelta = currentTime - m_lastUpdateTime;

    if (timeDelta > 0) {
        qint64 rxDelta = newStats.bytesReceived - m_lastRxBytes;
        qint64 txDelta = newStats.bytesSent - m_lastTxBytes;

        // Convert to KB/s
        newStats.downloadSpeedKBps = (rxDelta / 1024.0) / (timeDelta / 1000.0);
        newStats.uploadSpeedKBps = (txDelta / 1024.0) / (timeDelta / 1000.0);

        m_lastRxBytes = newStats.bytesReceived;
        m_lastTxBytes = newStats.bytesSent;
        m_lastUpdateTime = currentTime;
    }

    m_networkStats = newStats;
    emit networkActivityChanged(m_networkStats.downloadSpeedKBps, m_networkStats.uploadSpeedKBps);

    // Update process list
    // TODO: Add caching mechanism to reduce system calls
    // See the course module on "Performance Optimization Patterns"
    m_processes = SystemInfo::getTopProcesses(20);
    std::sort(m_processes.begin(), m_processes.end());

    emit dataUpdated();
}
