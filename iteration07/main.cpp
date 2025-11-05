#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set application-wide icon
    a.setWindowIcon(QIcon(":/icons/resources/icons/app-icon.png"));

    MainWindow w;
    w.show();
    return a.exec();
}
