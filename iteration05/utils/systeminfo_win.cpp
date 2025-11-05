#ifdef _WIN32

#include "systeminfo.h"
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <QStorageInfo>
#include <QDebug>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

namespace SystemInfo {

// Static variables for system-wide CPU monitoring via PDH
static PDH_HQUERY cpuQuery = nullptr;
static PDH_HCOUNTER cpuTotal = nullptr;
static bool pdhInitialized = false;

double getCpuUsage() {
    // Initialize PDH for system-wide CPU monitoring on first call
    if (!pdhInitialized) {
        PdhOpenQuery(nullptr, 0, &cpuQuery);
        PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal);
        PdhCollectQueryData(cpuQuery);
        pdhInitialized = true;
        return 0.0; // Return 0 on first call as we need baseline data
    }

    // Collect and return system-wide CPU usage
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal);

    return counterVal.doubleValue;
}

MemoryInfo getMemoryInfo() {
    MemoryInfo info = {};

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    info.totalPhysical = memInfo.ullTotalPhys;
    info.availablePhysical = memInfo.ullAvailPhys;
    info.usedPhysical = info.totalPhysical - info.availablePhysical;
    info.totalVirtual = memInfo.ullTotalPageFile;
    info.availableVirtual = memInfo.ullAvailPageFile;
    info.usagePercentage = memInfo.dwMemoryLoad;

    return info;
}

QVector<DiskInfo> getDiskInfo() {
    QVector<DiskInfo> disks;

    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        if (!storage.isValid() || storage.isReadOnly() || storage.isRoot()) {
            continue;
        }

        DiskInfo disk;
        disk.name = storage.name();
        disk.mountPoint = storage.rootPath();
        disk.totalSpace = storage.bytesTotal();
        disk.availableSpace = storage.bytesAvailable();
        disk.usedSpace = disk.totalSpace - disk.availableSpace;
        disk.fileSystem = QString::fromUtf8(storage.fileSystemType());

        if (disk.totalSpace > 0) {
            disk.usagePercentage = (disk.usedSpace * 100.0) / disk.totalSpace;
            disks.append(disk);
        }
    }

    // Also add the root drive
    QStorageInfo root = QStorageInfo::root();
    if (root.isValid()) {
        DiskInfo disk;
        disk.name = "System";
        disk.mountPoint = root.rootPath();
        disk.totalSpace = root.bytesTotal();
        disk.availableSpace = root.bytesAvailable();
        disk.usedSpace = disk.totalSpace - disk.availableSpace;
        disk.fileSystem = QString::fromUtf8(root.fileSystemType());

        if (disk.totalSpace > 0) {
            disk.usagePercentage = (disk.usedSpace * 100.0) / disk.totalSpace;
            disks.prepend(disk);
        }
    }

    return disks;
}

NetworkStats getNetworkStats() {
    NetworkStats stats = {};

    PMIB_IFTABLE pIfTable = nullptr;
    ULONG ulSize = 0;

    // Get required buffer size
    if (GetIfTable(nullptr, &ulSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE *) malloc(ulSize);

        if (pIfTable != nullptr) {
            if (GetIfTable(pIfTable, &ulSize, FALSE) == NO_ERROR) {
                qint64 totalRx = 0;
                qint64 totalTx = 0;

                for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                    MIB_IFROW *pIfRow = &pIfTable->table[i];

                    // Skip loopback (type 24)
                    if (pIfRow->dwType != MIB_IF_TYPE_LOOPBACK &&
                        pIfRow->dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL) {
                        totalRx += pIfRow->dwInOctets;
                        totalTx += pIfRow->dwOutOctets;

                        if (stats.interfaceName.isEmpty()) {
                            stats.interfaceName = QString::fromUtf8(reinterpret_cast<char*>(pIfRow->bDescr));
                        }
                    }
                }

                stats.bytesReceived = totalRx;
                stats.bytesSent = totalTx;
            }

            free(pIfTable);
        }
    }

    return stats;
}

QVector<ProcessInfo> getTopProcesses(int count) {
    QVector<ProcessInfo> processes;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            ProcessInfo proc;
            proc.pid = pe32.th32ProcessID;
            proc.name = QString::fromWCharArray(pe32.szExeFile);
            proc.cpuUsage = 0.0; // CPU usage per process is complex on Windows
            proc.memoryUsage = 0; // Initialize to 0
            proc.status = "Running";

            // Get memory usage
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    proc.memoryUsage = pmc.WorkingSetSize;
                }
                CloseHandle(hProcess);
            }

            processes.append(proc);

        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    // Sort by memory usage since CPU per-process is complex
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo &a, const ProcessInfo &b) {
        return a.memoryUsage > b.memoryUsage;
    });

    if (count > 0 && count < processes.size()) {
        processes.resize(count);
    }

    return processes;
}

} // namespace SystemInfo

#endif // _WIN32
