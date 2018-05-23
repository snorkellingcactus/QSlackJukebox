#include <QCoreApplication>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "QSlackJukebox.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QSlackJukebox client(QString(argv[1]), &app);

    return app.exec();
}
