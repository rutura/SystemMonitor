#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QString>
#include <QVector>
#include <QtGlobal>

/**
 * @brief Information about a single process
 */
struct ProcessInfo {
    quint32 pid;
    QString name;
    double cpuUsage;
    qint64 memoryUsage; // in bytes
    QString status;

    bool operator<(const ProcessInfo &other) const {
        return cpuUsage > other.cpuUsage; // Sort descending
    }
};

/**
 * @brief Memory usage information
 */
struct MemoryInfo {
    qint64 totalPhysical;
    qint64 availablePhysical;
    qint64 usedPhysical;
    qint64 totalVirtual;
    qint64 availableVirtual;
    double usagePercentage;
};

/**
 * @brief Disk/drive information
 */
struct DiskInfo {
    QString name;
    QString mountPoint;
    qint64 totalSpace;
    qint64 usedSpace;
    qint64 availableSpace;
    double usagePercentage;
    QString fileSystem;
};

/**
 * @brief Network statistics
 */
struct NetworkStats {
    QString interfaceName;
    qint64 bytesReceived;
    qint64 bytesSent;
    double downloadSpeedKBps;
    double uploadSpeedKBps;

    // For session totals
    qint64 sessionBytesReceived;
    qint64 sessionBytesSent;
};

// Platform-specific system information functions
namespace SystemInfo {
    double getCpuUsage();
    MemoryInfo getMemoryInfo();
    QVector<DiskInfo> getDiskInfo();
    NetworkStats getNetworkStats();
    QVector<ProcessInfo> getTopProcesses(int count = 10);
}

#endif // SYSTEMINFO_H
