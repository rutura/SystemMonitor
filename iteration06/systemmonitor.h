#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include "utils/systeminfo.h"

/**
 * @brief Monitors system resources in real-time
 *
 * This class provides periodic updates of CPU, memory, disk, and network statistics.
 * Data is collected at a configurable interval and emitted via signals.
 *
 * @note Current implementation runs in the main thread. For production use,
 *       consider moving data collection to a worker thread.
 *       Learn advanced threading patterns in the full Qt Widgets course!
 *       https://www.learnqt.guide/courses/qt-widgets-cpp/
 */
class SystemMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor();

    void startMonitoring(int intervalMs = 1000);
    void stopMonitoring();

    // Data retrieval methods
    double getCpuUsage() const { return m_cpuUsage; }
    MemoryInfo getMemoryInfo() const { return m_memoryInfo; }
    QVector<DiskInfo> getDiskInfo() const { return m_diskInfo; }
    NetworkStats getNetworkStats() const { return m_networkStats; }
    QVector<ProcessInfo> getTopProcesses(int count = 10) const;

signals:
    void dataUpdated();
    void cpuUsageChanged(double usage);
    void memoryUsageChanged(qint64 used, qint64 total);
    void networkActivityChanged(double downloadKBps, double uploadKBps);

private slots:
    void updateData();

private:
    QTimer *m_updateTimer;

    // Cached data
    double m_cpuUsage;
    MemoryInfo m_memoryInfo;
    QVector<DiskInfo> m_diskInfo;
    NetworkStats m_networkStats;
    QVector<ProcessInfo> m_processes;

    // For network speed calculation
    qint64 m_lastRxBytes;
    qint64 m_lastTxBytes;
    qint64 m_lastUpdateTime;
};

#endif // SYSTEMMONITOR_H
