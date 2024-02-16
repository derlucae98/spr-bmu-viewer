#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
#ifdef Q_OS_LINUX
    w.hide();
#endif
    return a.exec();
}
