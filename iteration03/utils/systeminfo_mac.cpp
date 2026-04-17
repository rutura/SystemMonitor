#ifdef __APPLE__

#include "systeminfo.h"
#include <QStorageInfo>
#include <QDebug>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <libproc.h>
#include <unistd.h>
#include <algorithm>

namespace SystemInfo {

// Static variables for system-wide CPU calculation
// Tracks aggregate CPU ticks returned by host_statistics(HOST_CPU_LOAD_INFO)
static unsigned long long lastTotalUser = 0, lastTotalNice = 0, lastTotalSys = 0, lastTotalIdle = 0;
static bool cpuInitialized = false;

double getCpuUsage() {
    // Read system-wide CPU tick counters via the Mach host_statistics API
    host_cpu_load_info_data_t cpuLoad;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&cpuLoad), &count) != KERN_SUCCESS) {
        return 0.0;
    }

    // CPU states reported by macOS: user, system, idle, nice
    unsigned long long totalUser = cpuLoad.cpu_ticks[CPU_STATE_USER];
    unsigned long long totalNice = cpuLoad.cpu_ticks[CPU_STATE_NICE];
    unsigned long long totalSys  = cpuLoad.cpu_ticks[CPU_STATE_SYSTEM];
    unsigned long long totalIdle = cpuLoad.cpu_ticks[CPU_STATE_IDLE];

    // On first call, store baseline values and return 0
    if (!cpuInitialized) {
        lastTotalUser = totalUser;
        lastTotalNice = totalNice;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        cpuInitialized = true;
        return 0.0;
    }

    // Calculate system-wide CPU usage percentage from deltas
    unsigned long long busy = (totalUser - lastTotalUser) + (totalNice - lastTotalNice) +
                              (totalSys - lastTotalSys);
    unsigned long long totalDelta = busy + (totalIdle - lastTotalIdle);

    double percent = 0.0;
    if (totalDelta > 0) {
        percent = (busy * 100.0) / totalDelta;
    }

    lastTotalUser = totalUser;
    lastTotalNice = totalNice;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

MemoryInfo getMemoryInfo() {
    MemoryInfo info = {};

    // Total physical memory from sysctl hw.memsize (64-bit safe)
    int64_t totalPhysical = 0;
    size_t len = sizeof(totalPhysical);
    if (sysctlbyname("hw.memsize", &totalPhysical, &len, nullptr, 0) != 0) {
        totalPhysical = 0;
    }

    // Per-page VM statistics from the Mach host
    vm_size_t pageSize = 0;
    host_page_size(mach_host_self(), &pageSize);

    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

    qint64 availPhysical = 0;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStats), &count) == KERN_SUCCESS) {
        // Treat free + inactive pages as available (inactive can be reclaimed)
        qint64 availablePages = static_cast<qint64>(vmStats.free_count) +
                                static_cast<qint64>(vmStats.inactive_count);
        availPhysical = availablePages * static_cast<qint64>(pageSize);
    }

    info.totalPhysical = totalPhysical;
    info.availablePhysical = availPhysical;
    info.usedPhysical = totalPhysical - availPhysical;

    // Swap usage via sysctl vm.swapusage
    struct xsw_usage swap = {};
    len = sizeof(swap);
    if (sysctlbyname("vm.swapusage", &swap, &len, nullptr, 0) == 0) {
        info.totalVirtual = static_cast<qint64>(swap.xsu_total);
        info.availableVirtual = static_cast<qint64>(swap.xsu_avail);
    }

    if (info.totalPhysical > 0) {
        info.usagePercentage = (info.usedPhysical * 100.0) / info.totalPhysical;
    }

    return info;
}

QVector<DiskInfo> getDiskInfo() {
    QVector<DiskInfo> disks;

    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        // Don't exclude read-only volumes: on modern macOS the sealed system
        // volume at / is intentionally read-only but is still a real drive.
        if (!storage.isValid()) {
            continue;
        }

        // Skip special/synthetic filesystems and internal APFS helper mounts
        QString fsType = QString::fromUtf8(storage.fileSystemType());
        if (fsType.startsWith("devfs") || fsType.startsWith("autofs") ||
            fsType.startsWith("tmpfs")) {
            continue;
        }
        if (storage.rootPath().startsWith("/System/Volumes/")) {
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

    // Enumerate network interfaces; AF_LINK entries carry per-interface byte counters
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0 || ifap == nullptr) {
        return stats;
    }

    qint64 totalRx = 0;
    qint64 totalTx = 0;

    for (struct ifaddrs *ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_LINK) continue;

        QString iface = QString::fromUtf8(ifa->ifa_name);

        // Skip loopback
        if (iface == "lo0") continue;

        struct if_data *data = reinterpret_cast<struct if_data*>(ifa->ifa_data);
        if (!data) continue;

        qint64 rxBytes = static_cast<qint64>(data->ifi_ibytes);
        qint64 txBytes = static_cast<qint64>(data->ifi_obytes);

        totalRx += rxBytes;
        totalTx += txBytes;

        if (stats.interfaceName.isEmpty() && rxBytes > 0) {
            stats.interfaceName = iface;
        }
    }

    freeifaddrs(ifap);

    stats.bytesReceived = totalRx;
    stats.bytesSent = totalTx;

    return stats;
}

QVector<ProcessInfo> getTopProcesses(int count) {
    QVector<ProcessInfo> processes;

    // First pass: ask libproc how many PIDs exist
    int bufBytes = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (bufBytes <= 0) {
        return processes;
    }

    QVector<pid_t> pids(bufBytes / sizeof(pid_t));
    bufBytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                             pids.size() * sizeof(pid_t));
    if (bufBytes <= 0) {
        return processes;
    }

    const int pidCount = bufBytes / sizeof(pid_t);
    for (int i = 0; i < pidCount; ++i) {
        pid_t pid = pids[i];
        if (pid <= 0) continue;

        ProcessInfo proc;
        proc.pid = static_cast<quint32>(pid);
        proc.cpuUsage = 0.0;
        proc.memoryUsage = 0; // Initialize to 0
        proc.status = "Running";

        // Read process name via proc_name
        char nameBuf[256] = {0};
        if (proc_name(pid, nameBuf, sizeof(nameBuf)) > 0) {
            proc.name = QString::fromUtf8(nameBuf);
        }

        // Read resident memory via PROC_PIDTASKINFO
        struct proc_taskinfo taskInfo;
        int rv = proc_pidinfo(pid, PROC_PIDTASKINFO, 0,
                              &taskInfo, sizeof(taskInfo));
        if (rv == sizeof(taskInfo)) {
            proc.memoryUsage = static_cast<qint64>(taskInfo.pti_resident_size);
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

#endif // __APPLE__
