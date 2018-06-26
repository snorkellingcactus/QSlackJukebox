#ifndef PULSE_H
#define PULSE_H

#include <pulse/pulseaudio.h>
#include <QtCore/QDebug>

class Pulse {
    public:
        static int VOLUME_MAX;
        static int VOLUME_PERCENT;

        Pulse();
        ~Pulse();

        short int volume();

        bool volumeSet(short int level);
        bool volumeSetRelative(short int volume_inc);
    private:
        bool action_ready;
        bool action_succeeded;

        int _volume;
        int *pa_error;

        pa_context *_pa_context;
        pa_mainloop *_pa_mainloop;

        bool exec();
        void setCompleted(bool success = true);
        static void onVolumeGet(pa_context *c, const pa_sink_info *sink_info, int eol, void *userdata);
        static void onVolumeSet(pa_context *_pa_context, int success, void *data);
        static void onStateChange(pa_context *_pa_context, void *data);
};


#endif // PULSE_H
