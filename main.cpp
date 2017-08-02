#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w1(32768);
    //w1.show();

    //MainWindow w2(32769);
    //w2.show();

    MainWindow w3(32770);
    w3.show();

    return a.exec();
}
