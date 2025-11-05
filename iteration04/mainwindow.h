#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class InfoCard;
class SystemMonitor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCpuUsageChanged(double usage);
    void onMemoryUsageChanged(qint64 used, qint64 total);
    void onNetworkActivityChanged(double downloadKBps, double uploadKBps);
    void updateDiskUsage();

private:
    void setupCards();
    void setupSystemMonitor();
    
    Ui::MainWindow *ui;
    
    // InfoCard instances
    InfoCard *m_cpuCard;
    InfoCard *m_memoryCard;
    InfoCard *m_diskCard;
    InfoCard *m_networkCard;
    
    // System monitoring
    SystemMonitor *m_systemMonitor;
};
#endif // MAINWINDOW_H
