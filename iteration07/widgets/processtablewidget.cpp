#include "processtablewidget.h"
#include "../utils/formatters.h"
#include "../utils/theme.h"
#include <QHeaderView>
#include <QApplication>

ProcessTableWidget::ProcessTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    setupColumns();
}

void ProcessTableWidget::setupColumns()
{
    setColumnCount(4);
    QStringList headers;
    headers << "PID" << "Name" << "Memory" << "Status";
    setHorizontalHeaderLabels(headers);

    // Column widths
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // Table styling
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->setVisible(false);
    setShowGrid(false);

    // Set minimum height to show at least 5 processes (header + 5 rows)
    setMinimumHeight(200);

}

void ProcessTableWidget::applyTheme()
{
    bool isDark = Theme::isDarkMode();

    if (isDark) {
        // Dark mode
        setStyleSheet(
            "QTableWidget {"
            "    background-color: #2C2C2C;"
            "    alternate-background-color: #383838;"
            "    border: 1px solid #404040;"
            "    border-radius: 4px;"
            "    color: #E0E0E0;"
            "}"
            "QTableWidget::item {"
            "    padding: 8px;"
            "    color: #E0E0E0;"
            "}"
            "QTableWidget::item:selected {"
            "    background-color: #2196F3;"
            "    color: white;"
            "}"
            "QHeaderView::section {"
            "    background-color: #1E1E1E;"
            "    padding: 8px;"
            "    border: none;"
            "    border-bottom: 2px solid #2196F3;"
            "    font-weight: bold;"
            "    color: #E0E0E0;"
            "}"
            );
    } else {
        // Light mode
        setStyleSheet(
            "QTableWidget {"
            "    background-color: white;"
            "    alternate-background-color: #F5F5F5;"
            "    border: 1px solid #E0E0E0;"
            "    border-radius: 4px;"
            "    color: #000000;"
            "}"
            "QTableWidget::item {"
            "    padding: 8px;"
            "    color: #000000;"
            "}"
            "QTableWidget::item:selected {"
            "    background-color: #2196F3;"
            "    color: white;"
            "}"
            "QHeaderView::section {"
            "    background-color: #FAFAFA;"
            "    padding: 8px;"
            "    border: none;"
            "    border-bottom: 2px solid #2196F3;"
            "    font-weight: bold;"
            "    color: #424242;"
            "}"
            );
    }
}

void ProcessTableWidget::updateProcessList(const QVector<ProcessInfo> &processes)
{
    setRowCount(processes.size());

    for (int i = 0; i < processes.size(); ++i) {
        const ProcessInfo &proc = processes[i];

        // PID
        QTableWidgetItem *pidItem = new QTableWidgetItem(QString::number(proc.pid));
        pidItem->setTextAlignment(Qt::AlignCenter);
        setItem(i, 0, pidItem);

        // Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(proc.name);
        setItem(i, 1, nameItem);

        // Memory
        QTableWidgetItem *memItem = new QTableWidgetItem(Formatters::formatBytes(proc.memoryUsage));
        memItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setItem(i, 2, memItem);

        // Status
        QTableWidgetItem *statusItem = new QTableWidgetItem(proc.status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        setItem(i, 3, statusItem);
    }
}

void ProcessTableWidget::updateTheme()
{
    applyTheme();
}


