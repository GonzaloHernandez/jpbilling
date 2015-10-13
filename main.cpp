#include "mainwindow.h"
#include <QApplication>
/**
 * @brief main is the initial step in the application.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
