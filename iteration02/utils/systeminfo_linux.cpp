#ifdef __linux__

#include "systeminfo.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStorageInfo>
#include <QDebug>
#include <QRegularExpression>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <algorithm>

namespace SystemInfo {

// Static variables for system-wide CPU calculation
// Tracks total CPU time across all cores from /proc/stat
static unsigned long long lastTotalUser = 0, lastTotalUserLow = 0, lastTotalSys = 0, lastTotalIdle = 0;
static bool cpuInitialized = false;

double getCpuUsage() {
    // Read system-wide CPU statistics from /proc/stat
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly)) {
        return 0.0;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    // Parse first line (system-wide): cpu user nice system idle iowait irq softirq
    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5) {
        return 0.0;
    }

    unsigned long long totalUser = parts[1].toULongLong();
    unsigned long long totalUserLow = parts[2].toULongLong();
    unsigned long long totalSys = parts[3].toULongLong();
    unsigned long long totalIdle = parts[4].toULongLong();

    // On first call, store baseline values and return 0
    if (!cpuInitialized) {
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        cpuInitialized = true;
        return 0.0;
    }

    // Calculate system-wide CPU usage percentage
    unsigned long long total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                               (totalSys - lastTotalSys);
    unsigned long long totalDelta = total + (totalIdle - lastTotalIdle);

    double percent = 0.0;
    if (totalDelta > 0) {
        percent = (total * 100.0) / totalDelta;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

MemoryInfo getMemoryInfo() {
    MemoryInfo info = {};

    struct sysinfo memInfo;
    sysinfo(&memInfo);

    long long totalPhysical = memInfo.totalram;
    totalPhysical *= memInfo.mem_unit;

    long long availPhysical = memInfo.freeram;
    availPhysical *= memInfo.mem_unit;

    info.totalPhysical = totalPhysical;
    info.availablePhysical = availPhysical;
    info.usedPhysical = totalPhysical - availPhysical;
    info.totalVirtual = memInfo.totalswap * memInfo.mem_unit;
    info.availableVirtual = memInfo.freeswap * memInfo.mem_unit;

    if (info.totalPhysical > 0) {
        info.usagePercentage = (info.usedPhysical * 100.0) / info.totalPhysical;
    }

    return info;
}

QVector<DiskInfo> getDiskInfo() {
    QVector<DiskInfo> disks;

    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        if (!storage.isValid() || storage.isReadOnly()) {
            continue;
        }

        // Skip special filesystems
        QString fsType = QString::fromUtf8(storage.fileSystemType());
        if (fsType.startsWith("tmpfs") || fsType.startsWith("devtmpfs") ||
            fsType.startsWith("proc") || fsType.startsWith("sys")) {
            continue;
        }

        DiskInfo disk;
        disk.name = storage.name();
        if (disk.name.isEmpty()) {
            disk.name = storage.device();
        }
        disk.mountPoint = storage.rootPath();
        disk.totalSpace = storage.bytesTotal();
        disk.availableSpace = storage.bytesAvailable();
        disk.usedSpace = disk.totalSpace - disk.availableSpace;
        disk.fileSystem = fsType;

        if (disk.totalSpace > 0) {
            disk.usagePercentage = (disk.usedSpace * 100.0) / disk.totalSpace;
            disks.append(disk);
        }
    }

    return disks;
}

NetworkStats getNetworkStats() {
    NetworkStats stats = {};

    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly)) {
        return stats;
    }

    QTextStream in(&file);
    // Skip header lines
    in.readLine();
    in.readLine();

    qint64 totalRx = 0;
    qint64 totalTx = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() < 10) continue;

        QString iface = parts[0];
        iface.remove(':');

        // Skip loopback
        if (iface == "lo") continue;

        qint64 rxBytes = parts[1].toLongLong();
        qint64 txBytes = parts[9].toLongLong();

        totalRx += rxBytes;
        totalTx += txBytes;

        if (stats.interfaceName.isEmpty() && rxBytes > 0) {
            stats.interfaceName = iface;
        }
    }

    file.close();

    stats.bytesReceived = totalRx;
    stats.bytesSent = totalTx;

    return stats;
}

QVector<ProcessInfo> getTopProcesses(int count) {
    QVector<ProcessInfo> processes;

    QDir procDir("/proc");
    QStringList pidDirs = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &pidStr : pidDirs) {
        bool ok;
        quint32 pid = pidStr.toUInt(&ok);
        if (!ok) continue;

        ProcessInfo proc;
        proc.pid = pid;
        proc.cpuUsage = 0.0;
        proc.memoryUsage = 0; // Initialize to 0
        proc.status = "Running";

        // Read process name from /proc/[pid]/comm
        QFile commFile(QString("/proc/%1/comm").arg(pid));
        if (commFile.open(QIODevice::ReadOnly)) {
            proc.name = QString::fromUtf8(commFile.readAll().trimmed());
            commFile.close();
        }

        // Read memory usage from /proc/[pid]/statm
        // Format: total_program_size resident shared text lib data dt
        // We want resident (RSS) which is in pages
        QFile statmFile(QString("/proc/%1/statm").arg(pid));
        if (statmFile.open(QIODevice::ReadOnly)) {
            QString line = QString::fromUtf8(statmFile.readAll().trimmed());
            statmFile.close();
            
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                // Second field is RSS in pages, convert to bytes
                long pageSize = sysconf(_SC_PAGESIZE);
                qint64 rssPages = parts[1].toLongLong();
                proc.memoryUsage = rssPages * pageSize;
            }
        }

        if (!proc.name.isEmpty()) {
            processes.append(proc);
        }
    }

    // Sort by memory usage
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo &a, const ProcessInfo &b) {
        return a.memoryUsage > b.memoryUsage;
    });

    if (count > 0 && count < processes.size()) {
        processes.resize(count);
    }

    return processes;   
}

} // namespace SystemInfo

#endif // __linux__
