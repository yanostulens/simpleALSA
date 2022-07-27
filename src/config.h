#ifndef SIMPLEALSACONFIG_H
#define SIMPLEALSACONFIG_H

#include <alsa/asoundlib.h>
#include <stdbool.h>

#include "logger/logger.h"

/** MACROS **/

#if !defined(DEFAULT_DEVICE)
    #define DEFAULT_DEVICE "default"
#endif

#if !defined(DEFAULT_SAMPLE_RATE)
    #define DEFAULT_SAMPLE_RATE 48000
#endif

#if !defined(DEFAULT_NUMBER_OF_CHANNELS)
    #define DEFAULT_NUMBER_OF_CHANNELS 2
#endif

#if !defined(DEFAULT_AUDIO_FORMAT)
    #define DEFAULT_AUDIO_FORMAT SND_PCM_FORMAT_S16_LE
#endif

#if !defined(DEFAULT_BUFFER_TIME)
    #define DEFAULT_BUFFER_TIME 1000000 /** in µS - so one second here */
#endif

#if !defined(DEFAULT_PERIOD_TIME)
    #define DEFAULT_PERIOD_TIME \
        200000 /** in µS - so 200ms here - right now this value allows for low latency but at the cost of higher CPU load */
#endif

#if !defined(SA_DEBUG)
    #define SA_NO_DEBUG_LOGS
#endif

/** ENUMS **/

/**
 * @brief enum used to return function results
 *
 */
typedef enum sa_result
{
    SA_INVALID_STATE = -2,
    SA_ERROR         = -1,
    SA_SUCCESS       = 0,
    SA_AT_END        = 1,
    SA_STOP          = 2,
    SA_PAUSE         = 3,
    SA_UNPAUSE       = 4,

} sa_result;

/**
 * @brief enum used to identify the state of a device
 *
 */
typedef enum sa_device_state
{
    SA_DEVICE_STOPPED = 0,
    SA_DEVICE_PAUSED  = 1,
    SA_DEVICE_STARTED = 2,
} sa_device_state;

/** STRUCTS **/

typedef struct sa_device sa_device;
typedef struct sa_device_config sa_device_config;

/**
 * @brief struct used to encapsulate a simple ALSA device
 *
 */
struct sa_device
{
    /** State of the device */
    sa_device_state state;

    /** Pointer to the configuration settings of the device*/
    sa_device_config *config;

    /** Pointer to the ALSA PCM handle struct */
    snd_pcm_t *handle;

    /** Pointer to the ALSA hardware parameters */
    snd_pcm_hw_params_t *hw_params;

    /** Pointer to the ALSA hardware parameters */
    snd_pcm_sw_params_t *sw_params;

    /** Pointer to the place is memory where audio samples are written right before being send to the ALSA buffer */
    int *samples;

    /** Indicates support for the hardware to pause the pcm stream */
    bool supports_pause;

    /** TODO */
    snd_pcm_sframes_t buffer_size;

    /** TODO */
    snd_pcm_sframes_t period_size;

    /** Refers to the pipe which can cancel poll when playback is canceled*/
    int pipe_write_end;

    /** Reference to the playback thread */
    pthread_t playback_thread;
};

/**
 * @brief struct used to config a simple ALSA devicre
 *
 */
struct sa_device_config
{
    /** Rate at which samples are send through the soundcard */
    unsigned int sample_rate;

    /** Amount of desired audiochannels */
    int channels;

    /** Defines the size (in µs) of the internal ALSA ring buffer */
    int buffer_time;

    /** Defines the time (in µs) after which ALSA will wake up to check if the buffer is running
            empty - increasing this time will increase efficiency, but risk the buffer running empty */
    int period_time;

    /** Format of the frames that are send to the ALSA buffer */
    snd_pcm_format_t format;

    /** Name of the device - this name indicates ALSA to which physical device it must send
                     audio - the default devices can be used by assigning this variable to "default" */
    char *device;

    /** Some pointer to custom set data*/
    void *my_custom_data;

    /** Callback function that will be called whenever the internal buffer is running
                                    empty and new audio samples are required */
    int (*data_callback)(int amount_of_frames, void *audio_buffer, sa_device *sa_device,
                         void *my_custom_data);

    /** Callback function that will be called whenever the other callback function fails to provide more samples */
    void (*eof_callback)(sa_device *sa_device, void *my_custom_data);
};

/**
 * @brief holds everything related to polling
 */
typedef struct
{
    /** An array of file descriptors to poll, ufds[0] is the read end of the pipe */
    struct pollfd *ufds;
    /** The amount of file descriptors to poll */
    int count;
} sa_poll_management;

/**
 * @brief a struct with data to be passed to the playback thread
 */
typedef struct
{
    /** An array of file descriptors to poll, ufds[0] is the read end of the pipe */
    sa_device *device;
    /** The read end of the pipe */
    struct pollfd *pipe_read_end_fd;
} sa_thread_data;

#endif  // SIMPLEALSACONFIG_H
