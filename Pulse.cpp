#include "Pulse.h"

int Pulse::VOLUME_MAX = PA_VOLUME_NORM;
int Pulse::VOLUME_PERCENT = (VOLUME_MAX / 100);

bool Pulse::exec(){
    action_ready = false;
    action_succeeded = false;

    while(!pa_mainloop_iterate(_pa_mainloop, 0, pa_error) || !action_ready);

    return action_succeeded;
}

Pulse::Pulse() {
    _pa_mainloop = pa_mainloop_new();
    _pa_context = pa_context_new(pa_mainloop_get_api(_pa_mainloop), "QSlackJukeboxx");

    pa_context_connect(_pa_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    pa_context_set_state_callback(
        _pa_context,
        static_cast<pa_context_notify_cb_t>(&Pulse::onStateChange),
        this
    );

    exec();
}

Pulse::~Pulse(){
    pa_mainloop_free(_pa_mainloop);
}

void Pulse::setCompleted(bool success){
    action_ready = true;
    action_succeeded = success;
}

short int Pulse::volume(){
    qWarning() << "Pulse::volume";
    pa_context_get_sink_info_by_index (
        _pa_context,
        0,
        static_cast<pa_sink_info_cb_t>(&Pulse::onVolumeGet),
        this
    );

    exec();

    return _volume;
}

bool Pulse::volumeSet(short int level) {
    if(level > 100) {
        level = 100;
    }

    if(level < 0){
        level = 0;
    }

    pa_cvolume *pa_volume = new pa_cvolume;
    pa_cvolume_set(pa_volume, 2, (pa_volume_t) level * VOLUME_PERCENT);

    pa_context_set_sink_volume_by_index(
        _pa_context,
        0,
        pa_volume,
        static_cast<pa_context_success_cb_t>(&Pulse::onVolumeSet),
        this
    );

    return exec();
}

bool Pulse::volumeSetRelative(short int volume_inc) {
    return volumeSet(volume() + volume_inc);
};

void Pulse::onVolumeGet(pa_context *c, const pa_sink_info *sink_info, int eol, void *data){
    qWarning() << "onPulseAudioVolumeGet()";
    auto _this = static_cast<Pulse *>(data);

    if(eol){
        _this->setCompleted();
    } else {
        _this->_volume = pa_cvolume_avg(&sink_info->volume) / VOLUME_PERCENT;
    }
};

void Pulse::onVolumeSet(pa_context *_pa_context, int success, void *_this) {
    qWarning() << "onPulseAudioVolumeSet " << success;

    (static_cast<Pulse *>(_this))->setCompleted(success);
};

void Pulse::onStateChange(pa_context *__pa_context, void *data){
    qWarning() << "QSlackJukebox::onPulseAudioStateChanged";
    auto _this = static_cast<Pulse *>(data);

    switch( pa_context_get_state( _this->_pa_context ) ){
        case PA_CONTEXT_READY: {
            qWarning() << "  PulseAudio context ready.";
            _this->setCompleted();
        }
    default: {
            qWarning()  << "  Intentionally unhandled pa context state.";
            break;
        }
    }
};

