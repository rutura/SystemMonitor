#ifndef PROCESSTABLEWIDGET_H
#define PROCESSTABLEWIDGET_H

#include <QTableWidget>
#include <QVector>
#include "../utils/systeminfo.h"

class ProcessTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit ProcessTableWidget(QWidget *parent = nullptr);

    void updateProcessList(const QVector<ProcessInfo> &processes);
    void updateTheme();

private:
    void setupColumns();
    void applyTheme();
};

#endif // PROCESSTABLEWIDGET_H
