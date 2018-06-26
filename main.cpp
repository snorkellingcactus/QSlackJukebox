#include <QCoreApplication>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "QSlackJukebox.h"
#include "Pulse.h"

int main(int argc, char *argv[])
{
    Pulse *audio_engine = new Pulse();

    QCoreApplication app(argc, argv);

    QSlackJukebox client(QString(argv[1]), audio_engine, &app);

    int return_code = app.exec();

    return return_code;
}
