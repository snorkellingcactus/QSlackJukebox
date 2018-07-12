#ifndef QSlackJukebox_H
#define QSlackJukebox_H

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTimer>

#include "Pulse.h"

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

        QStringList arguments(QString resource) {
            return  Player::arguments(resource) << QStringList("worst");
        }
};

class VLC : public Player {
    public:
        VLC() : Player("vlc", "VLC") {}
};

template <class PlayerType> class YoutubeDL : public Player {
    public:
        YoutubeDL(PlayerType _slave);

        QStringList arguments(QString resource) {
            return  QStringList("-c") <<
                    QString("youtube-dl -o - ") + resource + " | " +
                    slave.command("-").join(" ") + QString(" --file-caching 100000");
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
    static short int LAST_VOLUME_NULL;
    static short int PING_INTERVAL;
    static short int PING_INTERVAL_COUNT_MAX;

    enum PLAYER_STATUS { DIED, PAUSED, RESUMED };

    QSlackJukebox(QString _token, Pulse *_audio_engine, QObject *parent = nullptr);
private Q_SLOTS:
    void onConnected();
    void onMessage(QString message);
private slots:
    void onHTTPFinished();
    void onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    void tryNextPlayer();
    bool currentPlayerSendKillSignal(const std::string signal);
    void currentPlayerKill();
    void currentPlayerPause();
    void currentPlayerResume();
    void currentPlayerToggle();
    void sendMessage(QString message);
    void sendVolume();
    void onHTTPError(QNetworkReply::NetworkError error);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void reconnect();

    QTimer *ping_timer;
    QWebSocket websocket;
    QNetworkReply *reply = NULL;
    QNetworkAccessManager qnam;
    QString token;
    QString last_command;
    int last_message_id;
    QString last_channel;
    QProcess player;

    short int intervals_from_last_pong;
    short int last_volume;
    unsigned int player_current;
    unsigned int players_count = 2;
    Player *players[2];

    Pulse *audio_engine;
    PLAYER_STATUS last_player_status;
};

#endif // QSlackJukebox_H
