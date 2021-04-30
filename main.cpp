#include "speedrunners.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT3D_RENDERER", "opengl");
    //qputenv("QSG_RHI_BACKEND", "gl");
    QApplication a(argc, argv);
    SpeedRunners w;
    w.show();
    return a.exec();
}
