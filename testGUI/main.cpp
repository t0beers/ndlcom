#include "testGUI.h"
#include <QApplication>
#include <QDebug>

#include <signal.h>

void termination_handler (int signum);

int main( int argc, char* argv[])
{
    Q_INIT_RESOURCE(serialcom);

    signal (SIGINT, termination_handler);

    QApplication a(argc, argv);

    testGUI w;
    w.show();

    a.setApplicationName(w.windowTitle());

    qDebug() << w.windowTitle() << ": compiled on"<<__DATE__<<"at"<<__TIME__;

    return a.exec();
}

void termination_handler (int signum)
{

    qDebug() << "received signal"<<signum<<"-- exiting NOW!";

    _exit(0);
}
