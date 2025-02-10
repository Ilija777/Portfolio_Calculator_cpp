#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // Qt Anwendung starten
    MainWindow window;
    window.show();
    return app.exec();
}
