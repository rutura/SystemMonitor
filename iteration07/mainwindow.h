#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTabWidget;
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class InfoCard;
class SystemMonitor;
class ChartWidget;
class ProcessTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateUI();
    void showAboutDialog();
    void toggleTheme();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupCards();
    void setupTabs();
    void connectSignals();
    void setDarkMode(bool dark);
    void applyTheme();
    
    Ui::MainWindow *ui;
    
    // System monitoring
    SystemMonitor *m_monitor;
    
    // InfoCard instances
    InfoCard *m_cpuCard;
    InfoCard *m_memoryCard;
    InfoCard *m_diskCard;
    InfoCard *m_networkCard;
    
    // ChartWidget instances
    ChartWidget *m_cpuChart;
    ChartWidget *m_memoryChart;
    ChartWidget *m_networkChart;
    
    // Process table
    ProcessTableWidget *m_processTable;
    
    // UI Components
    QTabWidget *m_tabWidget;
    QTimer *m_statusTimer;
    
    // State
    int m_updateCount;
};
#endif // MAINWINDOW_H
