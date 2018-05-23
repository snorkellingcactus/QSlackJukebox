#include "QSlackJukebox.h"
#include <QtCore/QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

QT_USE_NAMESPACE

QSlackJukebox::QSlackJukebox(QString _token, QObject *parent) :
    QObject(parent)
{
    token = _token;

    connect(&m_webSocket, &QWebSocket::connected, this, &QSlackJukebox::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &QSlackJukebox::onClose);

    players[0] = new Streamlink();
    players[1] = new YoutubeDL<VLC>(VLC());

    players_count = 2;

    //player.setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedChannels);

    reconnect();
}

void QSlackJukebox::onConnected()
{
    qWarning() << "QSlackJukebox::onConnected";

    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &QSlackJukebox::onMessage);

    //m_webSocket.sendTextMessage(QStringLiteral("Hello, world!"));
}

void QSlackJukebox::onMessage(QString message)
{
    qWarning() << "QSlackJukebox::onMessage";

    QJsonObject _message(QJsonDocument::fromJson(QByteArray::fromStdString(message.toStdString()) , new QJsonParseError()).object());

    qWarning() << _message;

    if(_message["type"].toString() == "message") {
        last_command = QString(_message["text"].toString());

        if(last_command.startsWith("play")){

        }

        if(last_command.startsWith("<")){
            last_command = last_command.mid(1, last_command.size() -2);
        }

        if(!last_command.isEmpty()){
            player_current = 0;

            tryNextPlayer();
        }
    }
}

void QSlackJukebox::kill(QProcess *process) {
    qWarning() << "QSlackJukebox::kill";

    qint64 pid = process->processId();

    if(pid > 0){
        QString command = QString("pkill -9 -P ") + QString::number(pid);

        qWarning() << "  " << command;

        QProcess::execute(command);

        disconnect(process, 0, 0, 0);

        process->kill();
    }
}

void QSlackJukebox::startPlayer(Player *player_type) {
    qWarning() << "QSlackJukebox::startPlayer";

    kill(&player);

    qWarning() << "  Starting player: " << player_type->display_name;
    qWarning() << "  " << player_type->command(last_command);

    player.setProgram(player_type->program);
    player.setArguments(player_type->arguments(last_command));

    player.start();

    connect(&player,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &QSlackJukebox::onPlayerFinished);

    player.waitForStarted();
}

void QSlackJukebox::onHTTPFinished() {
    qWarning() << "QSlackJukebox::onHTTPFinished:";

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(status == 200) {
        QJsonObject response(QJsonDocument::fromJson(reply->readAll(), new QJsonParseError()).object());

        if(response["ok"].toBool() == true){
            QString url(response["url"].toString());

            qWarning() << "  Ok. Open " << url;

            m_webSocket.open(url);
        } else {
            qWarning() << "  Error: " << response["error"].toString();
        }

        //qWarning() << "WebSocket server:" << url;

    } else {
        qWarning() << "  Error: " << status << " " << reply->errorString() << endl;
    }
}

void QSlackJukebox::onHTTPError(QNetworkReply::NetworkError error){
    qWarning() << "QSlackJukebox::onHTTPError()";

    qWarning() << error << " " << reply->errorString() << endl;
}

void QSlackJukebox::reconnect(){
    qWarning() << "QSlackJukebox::reconnect()";

    QNetworkRequest request;
                    request.setUrl(QUrl("https://slack.com/api/rtm.connect?token=" + token));

    qWarning() << "  GET " << request.url().toString();

    reply = qnam.get(request);

    connect(reply, &QNetworkReply::finished, this, &QSlackJukebox::onHTTPFinished);

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &QSlackJukebox::onHTTPError);
}

void QSlackJukebox::onClose(){
    qWarning() << "QSlackJukebox::onClose()";

    reconnect();
}

void QSlackJukebox::onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "QSlackJukebox::onPlayerFinished";

    qWarning() << "  Previous player finished with code: " << exitCode;

    if (( exitCode == 0 ) && ( exitStatus != QProcess::CrashExit )) {
        kill(&player);
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

        startPlayer(players[player_current]);

        ++player_current;
    }
}
