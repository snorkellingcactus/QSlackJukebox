#include "QSlackJukebox.h"
#include <QtCore/QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <unistd.h>

QT_USE_NAMESPACE

short int QSlackJukebox::LAST_VOLUME_NULL = -1;
short int QSlackJukebox::PING_INTERVAL = 5000;
short int QSlackJukebox::PING_INTERVAL_COUNT_MAX = 3;

QSlackJukebox::QSlackJukebox(QString _token, Pulse *_audio_engine, QObject *parent) :
    QObject(parent),
    intervals_from_last_pong(0),
    last_volume(-1),
    last_player_status(PLAYER_STATUS::DIED),
    last_message_id(0),
    ping_timer(new QTimer(this))
{
    connect(ping_timer, &QTimer::timeout, [=]{
        if(intervals_from_last_pong >= PING_INTERVAL_COUNT_MAX){
            qWarning() << "Connection lost. ( no pongs )";
            reconnect();
        } else {
            qWarning() << "ping";
            websocket.ping();

            ++intervals_from_last_pong;
        }
    });

    ping_timer->setInterval(PING_INTERVAL);

    audio_engine = _audio_engine;

    //pa_context_get_server_info()
    token = _token;

    players[0] = new YoutubeDL<VLC>(VLC());
    players[1] = new Streamlink();

    players_count = 2;

    //player.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedChannels);

    reconnect();
}

void QSlackJukebox::onConnected()
{
    qWarning() << "QSlackJukebox::onConnected";

    ping_timer->start();

    connect(&websocket, &QWebSocket::textMessageReceived, this, &QSlackJukebox::onMessage, Qt::UniqueConnection);
}

void QSlackJukebox::onMessage(QString message)
{
    qWarning() << "QSlackJukebox::onMessage";

    QJsonObject _message(QJsonDocument::fromJson(QByteArray::fromStdString(message.toStdString()) , new QJsonParseError()).object());

    qWarning() << _message;

    if(_message["type"].toString() == "message") {
        last_command = _message["text"].toString().trimmed();

        if(last_command.startsWith("\\")){
            last_command = last_command.mid(1);

            bool handled = false;

            if(last_command == QString("reconnect")){
                reconnect();

                handled = true;
            }

            if(last_command == QString("qsj")){
                QString new_channel = _message["channel"].toString();
                if(new_channel != last_channel){
                    last_channel = new_channel;

                    sendMessage("Te escucho.");
                } else {
                    sendMessage("Que tene alzheimer ameo.");
                }

                handled = true;
            }

            short int volume_inc = 0;

            if(last_command == QString("+")) {
                volume_inc = 10;
            }

            if(last_command == QString("-")) {
                volume_inc = -10;
            }

            if(!handled && volume_inc) {;
                audio_engine->volumeSetRelative(volume_inc);

                sendVolume();

                handled = true;
            }

            qWarning() << last_command;

            if(!handled && last_command.startsWith("v")) {
                bool conversion_was_ok;

                int new_volume = last_command.mid(last_command.indexOf("v") + 1).toInt(&conversion_was_ok);

                if(conversion_was_ok){
                    audio_engine->volumeSet(new_volume);

                    sendVolume();
                } else {
                    sendMessage("Eso no es un número queride amigue.");
                }

                handled = true;
            }

            if(!handled && last_command.startsWith("m")) {
                if(audio_engine->volume() != 0){
                    last_volume = audio_engine->volume();

                    audio_engine->volumeSet(0);

                    sendMessage(":zipper_mouth_face:");
                } else {
                    sendMessage("Ya me dijo eso doña.");
                }

                handled = true;
            }

            if(!handled && last_command.startsWith("u")) {
                if(last_volume != LAST_VOLUME_NULL) {
                    audio_engine->volumeSet(last_volume);

                    last_volume = LAST_VOLUME_NULL;

                    sendMessage(":i_love_you_hand_sign:");
                } else {
                    sendMessage("Imposible. Estoy rockandrolleando en este momento.");
                }

                handled = true;
            }

            if(!handled && (last_command == QString("p"))) {
                currentPlayerPause();

                sendMessage("Basta decía.");

                handled = true;
            }

            if(!handled && last_command.startsWith("r")) {
                currentPlayerResume();

                sendMessage("atr.");

                handled = true;
            }

            if(!handled && last_command.startsWith("play")){
                if(last_command.endsWith(">")){
                    last_command = last_command.mid(last_command.indexOf("<") + 1).chopped(1);

                    player_current = 0;

                    tryNextPlayer();
                } else {
                    sendMessage("keseso :astonished: ?");
                }
            }

            if(!handled && last_command == "h"){
                sendMessage(
                    QStringList({
                        "\\m mute.",
                        "\\u unmute.",
                        "\\qsj listen to the channel where this command is typed.",
                        "\\h this help.",
                        "\\play <url> plays the specified url.",
                        "\\p pause.",
                        "\\r resume.",
                        "\\+ +10% of volume.",
                        "\\- -10% of volume.",
                        "\\v<integer> set volume to <integer> percent.",
                        "copyboth (left & right), snorkellingcactus, todorotosoft (~R), some rights reserved."
                    }).join("\n")
                );
            }
        };
    }
}

void QSlackJukebox::sendVolume() {
    sendMessage(QVariant(audio_engine->volume()).toString());
}

bool QSlackJukebox::currentPlayerSendKillSignal(const std::string signal) {
    qWarning() << "QSlackJukebox::currentPlayerSendKillSignal";

    qint64 pid = player.processId();

    bool applied = pid > 0;

    if(applied){
        QString command = QString::fromStdString("pkill -" + signal + " -P ") + QString::number(pid);

        qWarning() << "  " << command;

        QProcess::execute(command);
    }

    return applied;
}

void QSlackJukebox::sendMessage( QString message ){
     message = QString::fromStdString( QJsonDocument ( QJsonObject (
                 {
                     { "id"      ,    ++last_message_id   },
                     { "type"    ,    "message"           },
                     { "channel" ,    last_channel        },
                     { "text"    ,    message             }
                 }
            )
        ).toJson().toStdString()
    );

    qWarning() << "QSlackJukebox::sendMessage";
    qWarning() << "  " << message;

    if( !last_channel.isNull() ) {
        websocket.sendTextMessage(message);
    }
}

void QSlackJukebox::currentPlayerKill() {
    if(currentPlayerSendKillSignal("9")){
        disconnect(&player, 0, 0, 0);

        player.kill();

        player.waitForFinished(-1);

        last_player_status = PLAYER_STATUS::DIED;
    };
}

void QSlackJukebox::currentPlayerPause() {
    if(currentPlayerSendKillSignal("SIGSTOP")){
        last_player_status = PLAYER_STATUS::PAUSED;
    };
}

void QSlackJukebox::currentPlayerResume() {
    if(currentPlayerSendKillSignal("SIGCONT")){
        last_player_status = PLAYER_STATUS::RESUMED;
    };
}

void QSlackJukebox::currentPlayerToggle() {
    if( last_player_status == PLAYER_STATUS::PAUSED   ) { currentPlayerResume (); }
    if( last_player_status == PLAYER_STATUS::RESUMED  ) { currentPlayerPause  (); }
}

void QSlackJukebox::onHTTPFinished() {
    qWarning() << "QSlackJukebox::onHTTPFinished:";

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(status == 200) {
        QJsonObject response(QJsonDocument::fromJson(reply->readAll(), new QJsonParseError()).object());

        if(response["ok"].toBool() == true){
            QString url(response["url"].toString());

            qWarning() << "  Ok. Open " << url;

            websocket.open(url);
        } else {
            qWarning() << "  Error: " << response["error"].toString();
        }

    } else {
        qWarning() << "  Error: " << status << " " << reply->errorString() << endl;

        usleep(3000000);
        reconnect();
    }
}

void QSlackJukebox::onWebSocketError(QAbstractSocket::SocketError error){
    qWarning() << "QSlackJukebox::onWebSocketError()";
    qWarning() << websocket.errorString();
}

void QSlackJukebox::onHTTPError(QNetworkReply::NetworkError error){
    qWarning() << "QSlackJukebox::onHTTPError()";

    qWarning() << error << " " << reply->errorString() << endl;
}

void QSlackJukebox::reconnect(){
    ping_timer->stop();
    // May connect it before and just one time preventing this.
    websocket.disconnect();

    connect(&websocket, &QWebSocket::connected, this, &QSlackJukebox::onConnected);
    connect(&websocket, &QWebSocket::disconnected, this, &QSlackJukebox::reconnect);
    connect(&websocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &QSlackJukebox::onWebSocketError);
    connect(&websocket, &QWebSocket::pong, this, [=] {
        qWarning() << "pong";

        intervals_from_last_pong = 0;
    });

    qWarning() << "QSlackJukebox::reconnect()";

    QNetworkRequest request;
                    request.setUrl(QUrl("https://slack.com/api/rtm.connect?token=" + token));

    qWarning() << "  GET " << request.url().toString();

    if(reply != NULL){
        qWarning() << " Abort previous";
        reply->abort();
    }

    reply = qnam.get(request);

    connect(reply, &QNetworkReply::finished, this, &QSlackJukebox::onHTTPFinished);

    //connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &QSlackJukebox::onHTTPError);
}

void QSlackJukebox::onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "QSlackJukebox::onPlayerFinished";

    qWarning() << "  Previous player finished with code: " << exitCode;

    if (( exitCode == 0 ) && ( exitStatus != QProcess::CrashExit )) {
        currentPlayerKill();
    } else {
        tryNextPlayer();
    }
}

void QSlackJukebox::tryNextPlayer() {
    qWarning() << "QSlackJukebox::tryNextPlayer";

    if( player_current == players_count ) {
        qWarning() << "  There was no backend player able to play that resource.";
    } else {
        qWarning() << "  Exit code was about failure. Will try the next one.";
        qWarning() << "  Trying player number " << player_current + 1 << " of " << players_count;

        currentPlayerKill();

        Player *player_type = players[player_current];

        qWarning() << "  Starting player: " << player_type->display_name;
        qWarning() << "  " << player_type->command(last_command);

        player.setProgram(player_type->program);
        player.setArguments(player_type->arguments(last_command));

        player.start();

        connect(&player,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &QSlackJukebox::onPlayerFinished);

        player.waitForStarted();

        ++player_current;
    }
}
