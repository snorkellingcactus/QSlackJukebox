# QSlackJukebox
A slack client which launches bash commands with received messages

Currently, there are two hardcoded bash commands.

`streamlink ${RECEIVED_MESSAGE}`
And
`bash -c "youtube-dl -o - ${RECEIVED_MESSAGE} | vlc -"`

You should obtain a slack token and:

`./QSlackJukebox ${TOKEN}`

Slack app name is QSlackJukebox
