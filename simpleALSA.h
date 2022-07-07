#include <alsa/asoundlib.h>

struct sa_playback_device_config
{
    int sampleRate; /** Rate at which samples are send through the soundcard */
    int channels;   /** Amount of desired audiochannels */
    int bufferTime; /** Defines the size (in µs) of the internal ALSA ring buffer */
    int periodTime; /** Defines the time (in µs) after which ALSA will wake up to check if the buffer is running empty - increasing this time will increase efficiency, but risk the buffer running empty */

    snd_pcm_format_t format;    /** Format of the frames that are send to the ALSA buffer */ 
    char *device;               /** Name of the device 
    void (*callbackFunction)(int framesToSend, void* audioBuffer, sa_playback_device* sa_device);
};
