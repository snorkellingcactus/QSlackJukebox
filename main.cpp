#include <QCoreApplication>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "QSlackJukebox.h"

int volume;
int *pa_error;
bool pulseActionReady;
pa_context *_pa_context;
pa_mainloop *_pa_mainloop;

// We may want to handle errors (timeout (tryout)) here.
// We may want a max.
void pulseAudioExec(){
    pulseActionReady = false;

    while(!pa_mainloop_iterate(_pa_mainloop, 0, pa_error) || !pulseActionReady);
}

void pulseAudioActionCompleted(){
    pulseActionReady = true;
}

void onPulseAudioVolumeGet(pa_context *c, const pa_sink_info *sink_info, int eol, void *userdata){
    qWarning() << "onPulseAudioVolumeGet()";

    if(eol){
        pulseAudioActionCompleted();
    } else {
        volume = pa_cvolume_avg(&sink_info->volume) / (PA_VOLUME_NORM / 100);
    }
};

void onPulseAudioVolumeSet(pa_context *_pa_context, int success, void *data) {
    pulseAudioActionCompleted();
    qWarning() << "onPulseAudioVolumeSet " << success;
};

void pulseAudioVolumeGet(){
    pa_context_get_sink_info_by_index(_pa_context, 0, onPulseAudioVolumeGet, NULL);

    pulseAudioExec();
}

void pulseAudioVolumeSet(){
    void *data;

    pa_cvolume *pa_volume = new pa_cvolume;
    pa_cvolume_set(pa_volume, 2, (pa_volume_t) volume * (PA_VOLUME_NORM / 100));

    pa_context_set_sink_volume_by_index(_pa_context, 0, pa_volume, onPulseAudioVolumeSet, data);

    pulseAudioExec();
}

void onPulseAudioStateChanged(pa_context *_pa_context, void *data){
    qWarning() << "QSlackJukebox::onPulseAudioStateChanged";

    switch( pa_context_get_state( _pa_context ) ){
        case PA_CONTEXT_READY: {
            qWarning() << "  PulseAudio context ready.";
            pulseAudioActionCompleted();
        }
    default: {
            qWarning()  << "  Intentionally unhandled pa context state.";
            break;
        }
    }
};

void pulseAudioInitialize(){
    _pa_mainloop = pa_mainloop_new();

    _pa_context = pa_context_new(pa_mainloop_get_api(_pa_mainloop), "QSlackJukeboxx");

    pa_context_connect(_pa_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    pa_context_set_state_callback(
        _pa_context,
        onPulseAudioStateChanged,
        pa_error
    );

    pulseAudioExec();

    pulseAudioVolumeGet();
}

int main(int argc, char *argv[])
{
    pulseAudioInitialize();

    QCoreApplication app(argc, argv);

    QSlackJukebox client(QString(argv[1]), &app);

    int return_code = app.exec();

    pa_mainloop_free(_pa_mainloop);

    return return_code;
}
