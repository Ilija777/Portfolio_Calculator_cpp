#include <QApplication>
#include "mainwindow.h"  // INCLUDEPATH korrekt gesetzt

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle("µC-Calculator");
    w.resize(400, 300);
    w.show();

    return app.exec();
}
