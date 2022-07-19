#ifndef SIMPLEASLA_H
#define SIMPLEALSA_H


#include <alsa/asoundlib.h>

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
    #define DEFAULT_BUFFER_TIME 500000 /** in µS - so half a second here */
#endif

#if !defined(DEFAULT_PERIOD_TIME)
    #define DEFAULT_PERIOD_TIME 250000 /** in µS - so quarter of a second here */
#endif

/** ENUMS **/

/**
 * @brief enum used to return function results
 *
 */
typedef enum
{
    SA_SUCCESS = 0,
    SA_ERROR   = 1
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

/** STRUCTS **/

/**
 * @brief struct used to config a simple ALSA devicre
 *
 */
typedef struct
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
    void (*callbackFunction)(int framesToSend, void *audioBuffer, sa_device *sa_device);

} sa_device_config;

/**
 * @brief struct used to encapsulate a simple ALSA device
 *
 */
typedef struct
{
    sa_device_config *config;
    sa_device_status status;
} sa_device;

/** FUNCTIONS DEFINITIONS **/

/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
sa_result sa_init_device_config(sa_device_config *config);

/**
 * @brief initializes a new audio device
 * @param config - configuration used to initialize the device
 * @param device - pointer to the initialized audio device
 * @return sa_return_status
 */
sa_result sa_init_device(sa_device_config *config, sa_device *device);

/**
 * @brief starts the simple ALSA device - which starts the callback loop
 *
 * @param device - device to start
 * @return sa_return_status
 */
sa_result sa_start_device(sa_device *device);

/**
 * @brief pauses the simple ALSA device - which pauses the callback loop
 *
 * @param device - device to pause
 * @return sa_return_status
 */
sa_result sa_pause_device(sa_device *device);

/**
 * @brief stops a simple ALSA device - same as pause but in addation, all buffer data is dropped
 *
 * @param device - device to stop
 * @return sa_return_status
 */
sa_result sa_stop_device(sa_device *device);

/**
 * @brief destroys the device - device pointer is set to NULL
 *
 * @param device - the device to destroy
 * @return sa_return_status
 */
sa_result sa_destroy_device(sa_device *device);


#endif // SIMPLEALSA_H