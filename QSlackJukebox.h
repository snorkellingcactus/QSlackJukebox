#ifndef QSlackJukebox_H
#define QSlackJukebox_H

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>

class Player {
    public:
        Player(QString _program, QString _display_name){
            program = _program;
            display_name = _display_name;
        }

        virtual QStringList command(QString resource) {
            return QStringList(program) + this->arguments(resource);
        }

        virtual QStringList arguments(QString resource) {
            return QStringList(resource);
        };

        QString program;
        QString display_name;
};

class Streamlink : public Player {
    public:
        Streamlink() : Player("streamlink", "Streamlink") {}
};

class VLC : public Player {
    public:
        VLC() : Player("vlc", "VLC") {}
};

template <class PlayerType> class YoutubeDL : public Player {
    public:
        YoutubeDL(PlayerType _slave);
    private:
        QStringList arguments(QString resource) {
            return  QStringList("-c") <<
                    QString("youtube-dl -o - ") + resource + " | " +
                    slave.command("-").join(" ");
        }

        PlayerType slave;
};

template <class PlayerType> YoutubeDL<PlayerType>::YoutubeDL(PlayerType _slave) : Player("bash", "YoutubeDL") {
    slave = _slave;
}

class QSlackJukebox : public QObject
{
    Q_OBJECT
public:
    QSlackJukebox(QString _token, QObject *parent = nullptr);
private Q_SLOTS:
    void onConnected();
    void onMessage(QString message);
private slots:
    void onHTTPFinished();
    void onClose();
    void onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    void tryNextPlayer();
    void startPlayer(Player *player_type);
    void kill(QProcess *process);

    void onHTTPError(QNetworkReply::NetworkError error);
    void reconnect();

    QWebSocket m_webSocket;
    QNetworkReply *reply;
    QNetworkAccessManager qnam;
    QString token;
    QProcess player;
    QString last_command;

    unsigned int player_current;
    unsigned int players_count = 2;
    Player *players[2];
};

#endif // QSlackJukebox_H
