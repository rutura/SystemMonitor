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

/*
 * getCpuUsage — system-wide CPU utilization on macOS
 *
 * API used: host_statistics() with the HOST_CPU_LOAD_INFO "flavor",
 *           accessed through the Mach host port returned by mach_host_self().
 * Headers:  <mach/mach.h>, <mach/mach_host.h>, <mach/host_info.h>
 *
 * What is Mach? macOS's kernel is a hybrid — a BSD layer on top of the Mach
 * microkernel. Mach exposes kernel objects (hosts, tasks, threads, ports) that
 * you talk to through "ports," which are message-passing handles. For anything
 * system-wide (CPU, memory, page size), you ask the host port. mach_host_self()
 * gives you a handle to it for free; no permission is needed for read-only
 * stats.
 *
 * What is a CPU tick? The kernel doesn't measure CPU time in seconds; it counts
 * short fixed intervals called "ticks" (historically ~1/100th of a second). On
 * every tick, the kernel looks at each CPU and bumps a counter based on what
 * that CPU is doing: running user code, kernel ("system") code, idling, or
 * running "nice"-priority user code. HOST_CPU_LOAD_INFO returns those four
 * counters summed across every core on the machine, as monotonically growing
 * numbers since boot.
 *
 * Why two samples? A single snapshot of tick counters tells you nothing on its
 * own — the numbers only grow. CPU *utilization* is a rate: you take a reading,
 * wait a while, take another reading, and compute how many of the ticks that
 * elapsed between the two samples were "busy" ticks versus idle ones. That's
 * why the first call seeds the baseline and returns 0; every later call
 * computes: busy_delta / (busy_delta + idle_delta).
 *
 * Linux/Windows analogues: this is the macOS equivalent of reading /proc/stat's
 * first line on Linux, or querying the "\\Processor(_Total)\\% Processor Time"
 * PDH counter on Windows. Same idea, three different operating-system dialects.
 */
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

/*
 * getMemoryInfo — physical RAM and swap usage on macOS
 *
 * APIs used:
 *   - sysctlbyname("hw.memsize")   — total installed physical RAM in bytes
 *   - sysctlbyname("vm.swapusage") — swap totals into a struct xsw_usage
 *   - host_page_size()             — size of one VM page (often 16 KB on Apple
 *                                    Silicon, 4 KB on Intel Macs)
 *   - host_statistics64(HOST_VM_INFO64) — counts of pages in each VM state
 * Headers: <sys/sysctl.h>, <mach/mach.h>, <mach/mach_host.h>,
 *          <mach/vm_statistics.h>
 *
 * What is sysctl? BSD-derived Unixes (macOS, FreeBSD) expose kernel state as a
 * tree of named variables, like a filesystem for kernel knobs. sysctlbyname()
 * lets you query one by its dotted name. "hw.memsize" and "vm.swapusage" are
 * the canonical read-only names for total RAM and swap accounting. The Linux
 * analogue is reading /proc/meminfo; on Windows it's GlobalMemoryStatusEx().
 *
 * What is a page? The kernel does not hand out memory one byte at a time —
 * it manages RAM in fixed-size blocks called "pages." Every number the kernel
 * reports about memory usage is a count of pages. To turn page counts into
 * bytes you multiply by the page size, which you get from host_page_size().
 * (Hardcoding 4096 is a bug waiting to happen on Apple Silicon, where the
 * default page size is 16 KB.)
 *
 * What do the VM counters mean? The kernel classifies every page of RAM:
 *   - free_count      — unused, immediately allocatable
 *   - inactive_count  — used but not touched recently; the kernel can evict
 *                       these to disk and reuse the RAM if something needs it
 *   - active_count    — being actively used by a process
 *   - wired_count     — locked in RAM (kernel data, drivers); cannot be paged
 *   - compressor_*    — compressed in RAM to save space before hitting swap
 * "Available" in the Activity Monitor sense roughly means free + inactive:
 * memory a new program could use without killing anyone. We use that here.
 *
 * What is swap? When RAM pressure is high the kernel writes idle pages to a
 * file on disk and reuses the RAM. "vm.swapusage" reports how big that swap
 * area is and how much of it is currently free. On a healthy Mac with modest
 * load, swap is usually zero.
 */
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

/*
 * getDiskInfo — mounted drives, sizes, and free space
 *
 * API used: Qt's QStorageInfo::mountedVolumes(). On macOS this is a thin
 *           wrapper around the BSD call getmntinfo(), which returns the same
 *           list of mounts you see from the `mount` or `df` commands.
 * Headers:  <QStorageInfo>
 *
 * Why Qt instead of the raw OS API? Disk enumeration is one of the few areas
 * where Qt's abstraction is genuinely useful: the concepts (mount point, total
 * bytes, free bytes, filesystem type) map cleanly across Windows, Linux, and
 * macOS, and Qt gives you one interface. We fall back to native APIs for the
 * things Qt doesn't cover (CPU ticks, per-process memory, etc.) but for disks
 * the portable wrapper is the right tool.
 *
 * macOS-specific caveats you must handle:
 *
 *   1. The system volume is read-only. Since macOS 10.15, the root filesystem
 *      (/) is a sealed, cryptographically signed, read-only APFS volume. User
 *      data lives on a separate read-write volume at /System/Volumes/Data,
 *      stitched into / with "firmlinks" so it looks like a single tree. A
 *      naive `if (storage.isReadOnly()) continue;` (correct on Linux) would
 *      hide the main drive on every modern Mac. We drop that filter.
 *
 *   2. APFS containers share space. A single physical disk can host multiple
 *      APFS volumes (Preboot, VM, Update, Data, and /) that all draw from a
 *      shared pool of free space. macOS surfaces each volume as its own mount
 *      point under /System/Volumes/. If we listed all of them, the user would
 *      see five "drives" that are really one disk, with confusing duplicated
 *      numbers. We filter out paths under /System/Volumes/ and let the root
 *      mount represent the container.
 *
 *   3. devfs, autofs, and tmpfs are virtual filesystems (device nodes,
 *      auto-mount maps, RAM-backed scratch space) — not "drives" in any sense
 *      a user cares about. We skip them by filesystem type.
 *
 * Usage percentage: (totalSpace - availableSpace) / totalSpace. On APFS the
 * "used" figure reflects all volumes in the container, not just the one you
 * queried — that's a property of APFS, not a bug in our code.
 */
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

/*
 * getNetworkStats — total bytes in/out across network interfaces
 *
 * APIs used:
 *   - getifaddrs()  — returns a linked list of every network interface on the
 *                     system, along with associated address/statistics data.
 *   - freeifaddrs() — releases the list when you're done (required).
 *   - struct if_data — per-interface counters attached to AF_LINK entries;
 *                      fields ifi_ibytes and ifi_obytes are total bytes
 *                      received/sent since the interface came up.
 * Headers: <ifaddrs.h>, <net/if.h>, <net/if_dl.h>, <sys/socket.h>
 *
 * What is a "network interface"? Every way your machine talks to a network is
 * an interface with a short name: "en0" for the built-in Ethernet or Wi-Fi,
 * "lo0" for the loopback (traffic to yourself, i.e. 127.0.0.1), "utun0" for
 * VPN tunnels, "bridge0" for virtualization bridges, and so on. The kernel
 * tracks each one separately.
 *
 * Why AF_LINK? getifaddrs() returns multiple entries per interface — one for
 * each address family (AF_INET for IPv4, AF_INET6 for IPv6, AF_LINK for the
 * link-layer entry). Only the AF_LINK entry carries the byte counters we
 * want, and only one of those exists per interface, so filtering by family
 * both gives us the right data and avoids counting the same interface twice.
 *
 * Why skip lo0? Loopback traffic never leaves the machine. Counting it would
 * make your "download" number spike every time an app talks to localhost.
 * Pretty much every network monitor excludes it.
 *
 * Counters are monotonic. ifi_ibytes and ifi_obytes only grow; they reset only
 * when the interface is torn down. To get a *speed* (KB/s) you sample the
 * counter at two points in time and divide the delta by the elapsed seconds —
 * the same pattern as CPU usage. The caller of this function does that math.
 *
 * Linux/Windows analogues: this replaces parsing /proc/net/dev on Linux, and
 * calling GetIfTable() from iphlpapi.dll on Windows.
 */
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

/*
 * getTopProcesses — list running processes with PID, name, and memory usage
 *
 * APIs used (all from Apple's libproc):
 *   - proc_listpids(PROC_ALL_PIDS, ...)  — enumerate every process ID
 *   - proc_name(pid, buf, len)           — get a process's short executable
 *                                          name by PID
 *   - proc_pidinfo(pid, PROC_PIDTASKINFO, ..., &taskInfo, ...)
 *                                        — fill a struct proc_taskinfo with
 *                                          memory, CPU time, thread counts
 * Headers: <libproc.h>
 *
 * What is a PID? Every running program is a "process," and each process has a
 * unique integer ID (the PID). The kernel keeps a table of these; libproc is
 * Apple's user-space library for asking questions about them.
 *
 * The "ask for the size, then ask for the data" pattern. proc_listpids()
 * doesn't know how many processes exist until it counts them, so you call it
 * twice: first with a null buffer to get the required byte size, then with a
 * real buffer to receive the PIDs. This two-call dance shows up all over
 * system programming (Win32's GetIfTable() does it too). Calling it once with
 * a fixed buffer risks truncation on a busy system.
 *
 * What is "resident size"? When you look at how much memory a process uses,
 * there are several possible numbers:
 *   - Virtual size: everything the process has mapped, including files,
 *     libraries, and unused reservations. Often huge and misleading.
 *   - Resident Set Size (RSS): the portion of that memory actually kept in
 *     physical RAM right now. This is what users recognize as "memory usage"
 *     in Activity Monitor and Task Manager. That's pti_resident_size.
 * We sort by RSS because that's the number users expect to see.
 *
 * Why is CPU left at 0? Per-process CPU usage is surprisingly fiddly on every
 * platform. You need two snapshots of each process's cumulative CPU time,
 * divided by elapsed wall-clock time and the number of cores — and processes
 * can disappear between samples. The Windows implementation in this project
 * also leaves per-process CPU at 0 for the same reason; a correct version is
 * a whole mini-project of its own.
 *
 * Permissions note: some system processes can't be inspected by an
 * unprivileged user. proc_pidinfo() on those simply returns 0, and we skip
 * the memory field for that PID. We don't treat that as an error — it's just
 * how the kernel enforces privacy on sandboxed or root-owned processes.
 */
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
