#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdbool.h>

/** ENUMS **/

/**
 * @brief enum used to return function results
 *
 */
typedef enum sa_result
{
    SA_ERROR   = -1,
    SA_SUCCESS = 0,
    SA_CANCEL  = 1,

} sa_result;

/**
 * @brief enum used to indicate the status of a device
 *
 */
typedef enum
{
    SA_DEVICE_UNINITIALIZED = 0,
    SA_DEVICE_READY         = 1,
    SA_DEVICE_STARTED       = 2,
} sa_device_status;

/**
 * @brief enum to identify different types of logs
 *
 */
typedef enum
{
    MESSAGE = 0,
    DEBUG   = 1,
    WARNING = 2,
    ERROR   = 3
} sa_log_type;

/** STRUCTS **/
typedef struct sa_device sa_device;
typedef struct sa_device_config sa_device_config;

/**
 * @brief struct used to encapsulate a simple ALSA device
 *
 */
struct sa_device
{
    /** Pointer to the configuration settings of the device*/
    sa_device_config *config;

    /** Enum indicating the status of the device */
    sa_device_status status;

    /** Pointer to the ALSA PCM handle struct */
    snd_pcm_t *handle;

    /** Pointer to the ALSA hardware parameters */
    snd_pcm_hw_params_t *hwparams;

    /** Pointer to the ALSA hardware parameters */
    snd_pcm_sw_params_t *swparams;

    /** Pointer to the place is memory where audio samples are written right before being send to the ALSA buffer */
    signed short *samples;

    /** Indicates support for the hardware to pause the pcm stream */
    bool supportsPause;

    /** TODO */
    snd_pcm_sframes_t bufferSize;

    /** TODO */
    snd_pcm_sframes_t periodSize;

    /** Refers to the pipe which can cancel poll when playback is canceled*/
    int pipe_write_end;

    /** Some pointer to custom set data*/
    void *myCustomData;

    pthread_t playbackThread;
};

/**
 * @brief struct used to config a simple ALSA devicre
 *
 */
struct sa_device_config
{
    /** Rate at which samples are send through the soundcard */
    int sampleRate;

    /** Amount of desired audiochannels */
    int channels;

    /** Defines the size (in µs) of the internal ALSA ring buffer */
    int bufferTime;

    /** Defines the time (in µs) after which ALSA will wake up to check if the buffer is running
            empty - increasing this time will increase efficiency, but risk the buffer running empty */
    int periodTime;

    /** Format of the frames that are send to the ALSA buffer */
    snd_pcm_format_t format;

    /** Name of the device - this name indicates ALSA to which physical device it must send
                     audio - the default devices can be used by assigning this variable to "default" */
    char *device;

    /** Callback function that will be called whenever the internal buffer is running
                                    empty and new audio samples are required */
    int (*callbackFunction)(int framesToSend, void *audioBuffer, sa_device *sa_device);
};

#endif /* _CONFIG_H_ */
