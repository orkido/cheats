#include "speedrunners.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SpeedRunners w;
    w.show();
    return a.exec();
}
