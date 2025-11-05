#ifndef FORMATTERS_H
#define FORMATTERS_H

#include <QString>
#include <QColor>
#include <QtGlobal>

/**
 * @brief Utility functions for formatting values
 */
namespace Formatters {
    /**
     * @brief Format bytes to human-readable string (KB, MB, GB, TB)
     */
    inline QString formatBytes(qint64 bytes) {
        const qint64 KB = 1024;
        const qint64 MB = KB * 1024;
        const qint64 GB = MB * 1024;
        const qint64 TB = GB * 1024;

        if (bytes >= TB) {
            return QString::number(bytes / static_cast<double>(TB), 'f', 2) + " TB";
        } else if (bytes >= GB) {
            return QString::number(bytes / static_cast<double>(GB), 'f', 2) + " GB";
        } else if (bytes >= MB) {
            return QString::number(bytes / static_cast<double>(MB), 'f', 2) + " MB";
        } else if (bytes >= KB) {
            return QString::number(bytes / static_cast<double>(KB), 'f', 2) + " KB";
        } else {
            return QString::number(bytes) + " B";
        }
    }

    /**
     * @brief Format speed in KB/s to human-readable string
     */
    inline QString formatSpeed(double kbps) {
        if (kbps >= 1024.0) {
            return QString::number(kbps / 1024.0, 'f', 2) + " MB/s";
        } else {
            return QString::number(kbps, 'f', 2) + " KB/s";
        }
    }

    /**
     * @brief Format percentage with one decimal place
     */
    inline QString formatPercentage(double percentage) {
        return QString::number(percentage, 'f', 1) + "%";
    }

    /**
     * @brief Get color based on usage percentage
     */
    inline QColor getUsageColor(double percentage) {
        if (percentage < 60.0) {
            return QColor("#4CAF50"); // Green
        } else if (percentage < 80.0) {
            return QColor("#FFC107"); // Amber
        } else {
            return QColor("#F44336"); // Red
        }
    }
}

#endif // FORMATTERS_H
