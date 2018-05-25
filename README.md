# QSlackJukebox
A slack client which launches bash commands with received messages

Currently, there are two hardcoded bash commands.

`streamlink ${RECEIVED_MESSAGE}`
and
`bash -c "youtube-dl -o - ${RECEIVED_MESSAGE} | vlc -"`

To compile de app you should:

`mkdir build`
`cd build`
`qmake-qt5 this/source/code`
`make`

You should obtain a slack token and:

`./QSlackJukebox ${TOKEN}`

Slack app name is QSlackJukebox
