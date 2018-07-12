# QSlackJukebox
A slack client which launches bash commands with received messages

Currently, there are two hardcoded bash commands.

`streamlink ${RECEIVED_MESSAGE}`
and
`bash -c "youtube-dl -o - ${RECEIVED_MESSAGE} | vlc -"`

So you should manage to install those two by now.

I used pip to do it.

`sudo pip install youtube-dl`

`sudo pip install streamlink`

And VLC from your package manager.

To compile the project do:

`mkdir build`

`cd build`

`qmake-qt5 this/source/code`

`make`

You should obtain a slack token and:

`./QSlackJukebox ${TOKEN}`

Slack app name is QSlackJukebox


## DEPENDENCIES:
    Qt [>=5.10] with network enabled.
    QtWebSockets
