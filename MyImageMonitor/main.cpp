#include "myimagemonitor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyImageMonitor w;
    w.show();
    return QCoreApplication::exec();
}