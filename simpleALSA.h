#ifndef SIMPLEALSA_H
#define SIMPLEALSA_H
/*============================== INCLUDES ==============================*/
#include <alsa/asoundlib.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

/*=============================== MACROS ===============================*/
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

#define SA_LOG_2_ARGS(type, msg0)       sa_log(type, msg0, "")
#define SA_LOG_3_ARGS(type, msg0, msg1) sa_log(type, msg0, msg1)

#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define SA_LOG_MACRO_CHOOSER(...)                GET_4TH_ARG(__VA_ARGS__, SA_LOG_3_ARGS, SA_LOG_2_ARGS, )

#if defined SA_NO_LOGS
    #define SA_LOG(...) ((void) 0)
#else
    #define SA_LOG(...) SA_LOG_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#endif

/*================================ ENUMS ================================*/
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

/**
 * @brief enum to identify different types of logs
 *
 */
typedef enum
{
    SA_LOG_LEVEL_DEBUG   = 0,
    SA_LOG_LEVEL_WARNING = 1,
    SA_LOG_LEVEL_ERROR   = 2
} sa_log_type;

/*=============================== STRUCTS ===============================*/
typedef struct sa_device sa_device;
typedef struct sa_device_config sa_device_config;
typedef struct sa_condition_variable sa_condition_variable;
/**
 * @brief struct used to encapsulate a simple ALSA device
 *
 */
struct sa_device
{
    /** State of the device */
    sa_device_state state;

    /** Mutex to protect the state */
    pthread_mutex_t stateMutex;

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

    /** A condition variable and mutex used to communicate device stoppage */
    sa_condition_variable *condition_var;

    /** The current volume as a percentage between [0;100] */
    float volume_percentage;

    /** The current volume as a decibel value between [-100; 0] */
    float volume_dB;

    /** Mixer element for volume control */
    snd_mixer_elem_t *volume_handle;

    /** Mixer handle */
    snd_mixer_t *mixer_handle;
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
    char *alsa_device_name;

    /** Some pointer to custom set data*/
    void *my_custom_data;

    /** Callback function that will be called whenever the internal buffer is running
                                    empty and new audio samples are required */
    int (*data_callback)(int amount_of_frames, void *audio_buffer, sa_device *sa_device,
                         void *my_custom_data);

    /** Callback function that will be called whenever the other callback function fails to provide more samples */
    void (*eof_callback)(sa_device *sa_device, void *my_custom_data);

    /** Name that will show in the alsamixer */
    char *device_name;
};

/**
 * @brief a struct used to indicate wether the devices has stopped in a thread safe way
 *
 */
struct sa_condition_variable
{
    /** Condition variable */
    pthread_cond_t cond;
    /** Mutex to protect variable */
    pthread_mutex_t mutex;
    /** The variable */
    bool is_stopped;
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

/*************************************************************************************************************************************************************/
/*************************************************************************************************************************************************************/
/*****************************************************************DECLARATIONS/DEFINITIONS*********************************************************************/
/*************************************************************************************************************************************************************/
/*************************************************************************************************************************************************************/

/** Prevent parsers from greying out the following */
#if defined(__INTELLISENSE__) || defined(Q_CREATOR_RUN)
    #define SA_IMPLEMENTATION
#endif

#if defined SA_IMPLEMENTATION
/*=========================== API DECLARATIONS ===========================*/
/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
extern sa_result sa_init_device_config(sa_device_config **config);

/**
 * @brief initializes a new audio device
 * @param config - configuration used to initialize the device
 * @param device - pointer to the initialized audio device
 * @return sa_return_status
 */
extern sa_result sa_init_device(sa_device_config *config, sa_device **device);

/**
 * @brief starts the simple ALSA device - which starts the callback loop
 * This function will block untill the device is actually stopped.

 * @param device - device to start
 * @return sa_return_status
 */
extern sa_result sa_start_device(sa_device *device);

/**
 * @brief pauses the simple ALSA device - which pauses the callback loop
 *
 * @param device - device to pause
 * @return sa_return_status
 */
extern sa_result sa_pause_device(sa_device *device);

    #ifdef SA_ASYNC_API

/**
 * @brief stops a simple ALSA device - same as pause but in addition, all buffer data is dropped
 * This function will return directly which means there is no guarantee about the state after a call to this function
 *
 * @param device - device to stop
 * @return sa_return_status
 */
extern sa_result sa_stop_device_async(sa_device *device);

/**
 * @brief starts the simple ALSA device - which starts the callback loop
 * This function will return directly which means there is no guarantee about the state after a call to this function
 * @param device - device to start
 * @return sa_return_status
 */
extern sa_result sa_start_device_async(sa_device *device);

    #endif

/**
 * @brief stops a simple ALSA device - same sa_stop_device, but blocks until the devices has actually stopped
 * This function will block untill the device is actually stopped.
 *
 * @param device - device to stop
 * @return sa_return_status
 */
extern sa_result sa_stop_device(sa_device *device);

/**
 * @brief destroys the device - device pointer is set to NULL
 *
 * @param device - the device to destroy
 * @return sa_return_status
 */
extern sa_result sa_destroy_device(sa_device *device);

/**
 * @brief function used to retrieve the device state in a thread safe manner
 *
 * @param device
 * @return sa_device_state
 */
extern sa_device_state sa_get_device_state(sa_device *device);

/**
 * @brief Sets the volume in dB
 * @param device - the device to set the volume on
 * @param volume - dB value between [-100;0]
 * @return sa_result
 */
extern sa_result sa_set_volume_dB(sa_device *device, float volume);

/**
 * @brief Returns the current volume
 * Returns the volume as a value between [-100;0] dB
 * Returns -1 (SA_ERROR) on failure
 * @note This function will just return the last successful volume settting that has been
 * set by sa_set_volume_dB() as I did not find a bug free way to retrieve the current volume setting
 * with the snd_mixer API
 * @param device
 * @return float
 */
extern float sa_get_volume_dB(sa_device *device);

/*=========================== LOG DECLARATIONS ===========================*/
static void sa_log(sa_log_type type, const char msg0[], const char msg1[]);

/*======================== ALSA FUNC DECLARATIONS ========================*/
/**
 * @brief Initialized an ALSA device and store some settings in de sa_device
 *
 * @return sa_result
 */
static sa_result init_alsa_device(sa_device *device);

/**
 * @brief Sets the ALSA hardware parameters
 *
 * @param device
 * @param access
 * @return sa_result
 */
static sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access);

/**
 * @brief Sets the ALSA software parameters
 *
 * @param device
 * @return sa_result
 */
static sa_result set_software_parameters(sa_device *device);

/**
 * @brief Prepares and starts the playback thread and creates a communication pipe
 *
 * @param device
 * @return sa_result
 */
static sa_result prepare_playback_thread(sa_device *device);

/**
 * @brief Starts the ALSA write and wait loop
 *
 * @param device
 * @return sa_result
 */
static sa_result start_alsa_device(sa_device *device);

/**
 * @brief Waits until the transfer loop starts
 *
 * @param device
 * @return sa_result
 */
static sa_result wait_for_start_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to the transfer loop to pause audio output
 *
 * @param device
 * @return sa_result
 */
static sa_result pause_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to the transfer loop to unpause audio output
 *
 * @param device
 * @return sa_result
 */
static sa_result unpause_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to stop the transfer loop
 *
 * @param device
 * @return sa_result
 */
static sa_result stop_alsa_device(sa_device *device);

/**
 * @brief Waits until the transfer loop stops
 *
 * @param device
 * @return sa_result
 */
static sa_result wait_for_stop_alsa_device(sa_device *device);

/**
 * @brief Drops the samples of the internal ALSA buffer and stop the ALSA pcm handle
 *
 * @param device
 * @return sa_result
 */
static sa_result drop_alsa_device(sa_device *device);

/**
 * @brief Drains the samples of the internal ALSA buffer and stops the ALSA pcm handle
 *
 * @param device
 * @return sa_result
 */
static sa_result drain_alsa_device(sa_device *device);

/**
 * @brief Initializes the polling filedescriptors for ALSA and links it to the communication pipe
 *
 * @param device
 * @param poll_manager, nullpointer to initialize
 * @param pipe_read_end_fd
 * @return sa_result
 */
static sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager,
                                      struct pollfd *pipe_read_end_fd);

/**
 * @brief Starts the audio playback thread by running the write and poll loop
 *
 * @param data: a sa_thread_data packet
 *
 */
static void *init_playback_thread(void *data);

/**
 * @brief Attempts to join the playback thread
 *
 * @param device
 * @return sa_result
 */
static sa_result close_playback_thread(sa_device *device);

/**
 * @brief Prepares and starts write_and_poll_loop
 *
 * @param device
 * @param pipe_read_end_fd
 * @return sa_result
 */
static sa_result start_write_and_poll_loop(sa_device *device, struct pollfd *pipe_read_end_fd);

/**
 * @brief Plays audio by repeatedly calling the callback function for framas
 *
 * @param device
 * @param poll_manager
 * @return sa_result
 */
static sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Waits on poll and checks pipe
 *
 * @param handle
 * @param poll_manager
 * @return int
 */
static int wait_for_poll(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Try to recover from errors during playback
 *
 * @param handle
 * @param err
 * @return sa_result
 */
static sa_result xrun_recovery(snd_pcm_t *handle, int err);

/**
 * @brief reclaims sa_device
 *
 * @param device
 * @return sa_result
 */
static sa_result cleanup_device(sa_device *device);

/**
 * @brief Messages a char to the playback thread via a pipe
 *
 * @param device
 * @param toSend
 * @return sa_result
 */
static sa_result message_pipe(sa_device *device, char toSend);

/**
 * @brief Pauses the callback loop, this function will pause the PCM handle by calling pause_PCM_handle().
 * After that it will block and wait for further commands from the message pipe.
 *
 * @param poll_manager
 * @param handle
 * @return sa_result
 */
static sa_result pause_callback_loop(sa_poll_management *poll_manager, sa_device *device);

/**
 * @brief Saves the device state and signals if it has stopped
 *
 * @param device
 * @param new_state
 * @return void
 */
static void save_device_state(sa_device *device, sa_device_state new_state);
/**
 * @brief Checks whether the hardware support pausing, if so it pauses using snd_pcm_pause(). If the hw does
 * not support pausing it uses snd_pcm_drop() and prepares the the device using snd_pcm_prepare().
 *
 * @param device
 * @return sa_result
 */
static sa_result pause_PCM_handle(sa_device *device);

/**
 * @brief Checks whether the pcm was paused using snd_pcm_pause() or not and resume the pcm accordingly.
 *
 * @param device
 * @return sa_result
 */
static sa_result unpause_PCM_handle(sa_device *device);

/**
 * @brief Prepares the ALSA device so it is ready for a restart
 *
 * @param device
 * @return sa_result
 */
static sa_result prepare_alsa_device(sa_device *device);

/**
 * @brief Destroys the ALSA device, closes threads and frees all allocated memory
 *
 * @param device
 * @return sa_result
 */
static sa_result destroy_alsa_device(sa_device *device);

/**
 * @brief Function used to set the device state - thread safe
 *
 * @param device
 * @param state
 */
static void sa_set_device_state(sa_device *device, sa_device_state state);

/**
 * @brief Convert a dB value to a percentage value between [0;100]
 */
static float convert_dB_to_percentage(float dB);

/*========================= API DEFINITIONS ==========================*/
extern sa_result sa_init_device_config(sa_device_config **config) {
    sa_device_config *config_temp = (sa_device_config *) malloc(sizeof(sa_device_config));
    if(!config_temp)
        return SA_ERROR;

    config_temp->sample_rate      = DEFAULT_SAMPLE_RATE;
    config_temp->channels         = DEFAULT_NUMBER_OF_CHANNELS;
    config_temp->buffer_time      = DEFAULT_BUFFER_TIME;
    config_temp->period_time      = DEFAULT_PERIOD_TIME;
    config_temp->format           = DEFAULT_AUDIO_FORMAT;
    config_temp->alsa_device_name = (char *) "default";
    config_temp->data_callback    = NULL;
    config_temp->device_name      = (char *) "simpleALSA";
    *config                       = config_temp;
    return SA_SUCCESS;
}

extern sa_result sa_init_device(sa_device_config *config, sa_device **device) {
    sa_device *device_temp = (sa_device *) malloc(sizeof(sa_device));
    if(!device_temp)
        return SA_ERROR;

    device_temp->config = config;
    device_temp->state  = SA_DEVICE_STOPPED;
    *device             = device_temp;
    if(init_alsa_device(*device) != SA_SUCCESS)
    { return SA_ERROR; }
    if(sa_set_volume_dB(*device, 0) != SA_SUCCESS)
    { return SA_ERROR; }
    return SA_SUCCESS;
}

    #ifdef SA_ASYNC_API

static sa_result sa_start_device_async(sa_device *device) {
    if(sa_get_device_state(device) != SA_DEVICE_STARTED)
    {
        return start_alsa_device(device);
    } else
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Unable to start the device, the device is already started");
        return SA_INVALID_STATE;
    }
}

static sa_result sa_stop_device_async(sa_device *device) {
    if(sa_get_device_state(device) != SA_DEVICE_STOPPED)
        return stop_alsa_device(device);
    else
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Unable to stop the device, the device is already stopped");
        return SA_INVALID_STATE;
    }
}

    #endif

extern sa_result sa_start_device(sa_device *device) {
    pthread_mutex_lock(&(device->condition_var->mutex));
    if(sa_get_device_state(device) != SA_DEVICE_STARTED)
        if(start_alsa_device(device) == SA_SUCCESS)
        {
            sa_result result = wait_for_start_alsa_device(device);
            pthread_mutex_unlock(&(device->condition_var->mutex));
            return result;
        }
    pthread_mutex_unlock(&(device->condition_var->mutex));
    return SA_INVALID_STATE;
}

extern sa_result sa_stop_device(sa_device *device) {
    pthread_mutex_lock(&(device->condition_var->mutex));
    if(sa_get_device_state(device) != SA_DEVICE_STOPPED)
    {
        if(stop_alsa_device(device) == SA_SUCCESS)
        {
            sa_result result = wait_for_stop_alsa_device(device);
            pthread_mutex_unlock(&(device->condition_var->mutex));
            return result;
        }
    }
    pthread_mutex_unlock(&(device->condition_var->mutex));
    return SA_INVALID_STATE;
}

extern sa_result sa_pause_device(sa_device *device) {
    if(sa_get_device_state(device) == SA_DEVICE_STARTED)
        return pause_alsa_device(device);
    else
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Unable to pause the device, the device is already ");
        return SA_INVALID_STATE;
    }
}

extern sa_result sa_destroy_device(sa_device *device) {
    return destroy_alsa_device(device);
}

extern sa_device_state sa_get_device_state(sa_device *device) {
    pthread_mutex_lock(&(device->stateMutex));
    sa_device_state result = device->state;
    pthread_mutex_unlock(&(device->stateMutex));
    return result;
}

// extern sa_result sa_set_volume_dB(sa_device *device, float volume_dB) {
//     // long min, max, volumeToSet;
//     // float volume_percentage;
//     // snd_mixer_t *handle;
//     // snd_mixer_selem_id_t *sid;
//     // const char *card       = device->config->alsa_device_name;
//     // const char *selem_name = "Master";
//     // if(volume_dB < -100 || volume_dB > 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Invalid volume value");
//     //     return SA_ERROR;
//     // }
//     // volume_percentage = convert_dB_to_percentage(volume_dB);
//     // if(snd_mixer_open(&handle, 0) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to open snd_mixer");
//     //     return SA_ERROR;
//     // }
//     // if(snd_mixer_attach(handle, card) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to attach a card to the mixer");
//     //     return SA_ERROR;
//     // }
//     // if(snd_mixer_selem_register(handle, NULL, NULL) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to register selem");
//     //     return SA_ERROR;
//     // }
//     // if(snd_mixer_load(handle) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to load snd_mixer");
//     //     return SA_ERROR;
//     // }
//     // snd_mixer_selem_id_alloca(&sid);
//     // snd_mixer_selem_id_set_index(sid, 0);
//     // snd_mixer_selem_id_set_name(sid, selem_name);
//     // snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);
//     // if(elem == NULL)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to find selem");
//     //     return SA_ERROR;
//     // }
//     // if(snd_mixer_selem_get_playback_volume_range(elem, &min, &max) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to get the playback volume range");
//     //     return SA_ERROR;
//     // }
//     // if(volume_percentage == 0)
//     // {
//     //     volumeToSet = 0;
//     // } else
//     // { volumeToSet = volume_percentage * max / 100; }
//     // if(snd_mixer_selem_set_playback_volume_all(elem, volumeToSet) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to set playback volume");
//     //     return SA_ERROR;
//     // }
//     // if(snd_mixer_close(handle) != 0)
//     // {
//     //     SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to close snd_mixer");
//     //     return SA_ERROR;
//     // }
//     // device->volume_dB         = volume_dB;
//     // device->volume_percentage = volume_percentage;
//     return SA_SUCCESS;
// }

// extern float sa_get_volume_dB(sa_device *device) {
//     // return device->volume_dB;
//     return 0.0;
// }

/*========================= LOG DEFINITIONS ==========================*/
static void sa_log(sa_log_type type, const char msg0[], const char msg1[]) {
    switch(type)
    {
    #ifndef SA_NO_ERROR_LOGS
    case SA_LOG_LEVEL_ERROR:
        printf("\e[1;31m[  SA ERROR   ] \e[0m %s %s\n", msg0, msg1);
        break;
    #endif
    #ifndef SA_NO_WARNING_LOGS
    case SA_LOG_LEVEL_WARNING:
        printf("\e[1;33m[  SA WARNING ] \e[0m %s %s\n", msg0, msg1);
        break;
    #endif
    #ifndef SA_NO_DEBUG_LOGS
    case SA_LOG_LEVEL_DEBUG:
        printf("\e[1;35m[  SA DEBUG   ] \e[0m %s %s\n", msg0, msg1);
        break;
    #endif
    default:
        break;
    }
    fflush(stdout);
}

/*======================= ALSA FUNC DEFINITIONS ======================*/
static sa_result init_alsa_device(sa_device *device) {
    int err;
    snd_pcm_hw_params_alloca(&(device->hw_params));
    snd_pcm_sw_params_alloca(&(device->sw_params));

    if((err = snd_mixer_open(&(device->mixer_handle), 0)) < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: could not open mixer handle:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = snd_pcm_open(&(device->handle), device->config->alsa_device_name, SND_PCM_STREAM_PLAYBACK, 0)) <
       0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: playback open error:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if((err = set_hardware_parameters(device, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: setting hardware parameters failed:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_software_parameters(device)) < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: setting software parameters failed:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = snd_mixer_attach(device->mixer_handle, device->config->alsa_device_name)) != 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: could not attach mixer:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = snd_mixer_selem_register(device->mixer_handle, NULL, NULL)) != 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: could not register selem:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = snd_mixer_load(device->mixer_handle)) != 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: could not load mixer:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    snd_mixer_selem_id_t *selem_handle;
    snd_mixer_selem_id_alloca(&selem_handle);
    snd_mixer_selem_id_set_index(selem_handle, 0);
    snd_mixer_selem_id_set_name(selem_handle, "Master");
    device->volume_handle = snd_mixer_find_selem(device->mixer_handle, selem_handle);
    if(device->volume_handle == NULL)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: could not find mixer selem");
        exit(EXIT_FAILURE);
    }

    device->samples = (int *) malloc((device->period_size * device->config->channels *
                                      snd_pcm_format_physical_width(device->config->format)) /
                                     8);

    if(device->samples == NULL)
    { exit(EXIT_FAILURE); }

    device->supports_pause = snd_pcm_hw_params_can_pause(device->hw_params);
    if(device->supports_pause)
    {
        SA_LOG(SA_LOG_LEVEL_DEBUG, "Device supports snd_pcm_pause()");
    } else
    { SA_LOG(SA_LOG_LEVEL_DEBUG, "Device does not support snd_pcm_pause()"); }

    if(prepare_playback_thread(device) != SA_SUCCESS)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to prepare the playback thread");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

static sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    err = snd_pcm_hw_params_any(device->handle, device->hw_params);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR,
               "ALSA: broken configuration for playback: no configurations available:", snd_strerror(err));
        return SA_ERROR;
    }
    /** Set the sample_rate */
    err = snd_pcm_hw_params_set_rate_resample(device->handle, device->hw_params, 1);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: resampling setup failed for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(device->handle, device->hw_params, access);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: access type not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(device->handle, device->hw_params, device->config->format);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: sample format not available for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(device->handle, device->hw_params, device->config->channels);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: channels count not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the stream rate */
    rrate = device->config->sample_rate;
    err   = snd_pcm_hw_params_set_rate_near(device->handle, device->hw_params, &rrate, 0);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: sample_rate not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    if(rrate != device->config->sample_rate)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: sample rate does not match the requested rate");
        return SA_ERROR;
    }
    /* Set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(device->handle, device->hw_params,
                                                 (unsigned int *) &(device->config->buffer_time), &dir);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to set the buffer time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_buffer_size(device->hw_params, &size);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to get the buffer size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->buffer_size = size;
    /* Set the period time */
    err                 = snd_pcm_hw_params_set_period_time_near(device->handle, device->hw_params,
                                                                 (unsigned int *) &(device->config->period_time), &dir);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to set the period time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_period_size(device->hw_params, &size, &dir);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to get period size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->period_size = size;
    /* Write the parameters to device */
    err                 = snd_pcm_hw_params(device->handle, device->hw_params);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR,
               "ALSA: unable to set hardware parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

static sa_result set_software_parameters(sa_device *device) {
    int err;

    /* Get the current sw_params */
    err = snd_pcm_sw_params_current(device->handle, device->sw_params);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR,
               "ALSA: unable to determine current software parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Start the transfer when the buffer is almost full: (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(
      device->handle, device->sw_params, (device->buffer_size / device->period_size) * device->period_size);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to start set threshold mode for playback",
               snd_strerror(err));
        return SA_ERROR;
    }
    /* Allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    err = snd_pcm_sw_params_set_avail_min(device->handle, device->sw_params,
                                          0 ? device->buffer_size : device->period_size);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to set available minimum for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Enable period events when requested */
    if(0)
    {
        err = snd_pcm_sw_params_set_period_event(device->handle, device->sw_params, 1);
        if(err < 0)
        {
            SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to set period event", snd_strerror(err));
            return SA_ERROR;
        }
    }
    /* Write the parameters to the playback device */
    err = snd_pcm_sw_params(device->handle, device->sw_params);
    if(err < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: unable to set software parameters for playback", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

static sa_result prepare_playback_thread(sa_device *device) {
    /** Prepare communication pipe */
    int pipe_fds[2];
    if(pipe(pipe_fds))
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Cannot create poll_pipe");
        return SA_ERROR;
    }
    /** Makes read end nonblocking */
    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to make pipe non-blocking");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return SA_ERROR;
    }
    /** Store the write end */
    device->pipe_write_end          = pipe_fds[1];
    /** Prepare read polling structure of the read end */
    struct pollfd *pipe_read_end_fd = (struct pollfd *) malloc(sizeof(struct pollfd));
    pipe_read_end_fd->fd            = pipe_fds[0];
    pipe_read_end_fd->events        = POLLIN;
    /** Prepare the condition variable*/
    device->condition_var           = (sa_condition_variable *) malloc(sizeof(sa_condition_variable));
    pthread_mutex_init(&(device->condition_var->mutex), NULL);
    pthread_cond_init(&(device->condition_var->cond), NULL);
    device->condition_var->is_stopped = true;
    /** Prepare stateMutex */
    pthread_mutex_init(&(device->stateMutex), NULL);
    /** Startup the playback thread */
    sa_thread_data *thread_data   = (sa_thread_data *) malloc(sizeof(sa_thread_data));
    thread_data->device           = device;
    thread_data->pipe_read_end_fd = pipe_read_end_fd;
    if(pthread_create(&device->playback_thread, NULL, &init_playback_thread, (void *) thread_data) != 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to create the playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

static sa_result start_alsa_device(sa_device *device) {
    return unpause_alsa_device(device);
}

static void *init_playback_thread(void *data) {
    char command;
    /** Unwrap the data packet */
    sa_thread_data *thread_data     = (sa_thread_data *) data;
    sa_device *device               = thread_data->device;
    struct pollfd *pipe_read_end_fd = thread_data->pipe_read_end_fd;
    free(thread_data);

    /** Actual playback loop, lives and dies with the device */
    while(1)
    {
        /** Poll on read end, wait for initial play command */
        poll(pipe_read_end_fd, 1, -1);

        if(pipe_read_end_fd->revents & POLLIN)
        {
            if(read(pipe_read_end_fd->fd, &command, 1) != 1)
            {
                SA_LOG(SA_LOG_LEVEL_ERROR, "Pipe read error");
            } else
            {
                switch(command)
                {
                /** Play command */
                case 'u':
                    {
                        /** Save state */
                        save_device_state(device, SA_DEVICE_STARTED);
                        /** Start playback */
                        sa_result res = start_write_and_poll_loop(device, pipe_read_end_fd);
                        /** The write and poll loop can end in three ways: error, a stop command is sent, or
                         * no more audio is send to the audio buffer */
                        if(res == SA_ERROR)
                        {
                            save_device_state(device, SA_DEVICE_STOPPED);
                            break;
                        } else if(res == SA_STOP)
                        {
                            drop_alsa_device(device);
                            prepare_alsa_device(device);
                            save_device_state(device, SA_DEVICE_STOPPED);

                        } else if(res == SA_AT_END)
                        {
                            /** Received no frames anymore from the callback so we stop and prepare the alsa device again */
                            drain_alsa_device(device);
                            prepare_alsa_device(device);
                            save_device_state(device, SA_DEVICE_STOPPED);
                            /** Signal eof */
                            void (*eof_callback)(sa_device * sa_device, void *my_custom_data) =
                              (void (*)(sa_device *, void *my_custom_data)) device->config->eof_callback;
                            eof_callback(device, device->config->my_custom_data);
                        }
                        continue;
                    }
                /** Destroy command, no continue; break out of while */
                case 'd':
                    break;
                default:
                    SA_LOG(SA_LOG_LEVEL_DEBUG, "Command sent to the pipe is ignored");
                    continue;
                }
                SA_LOG(SA_LOG_LEVEL_DEBUG, "Attempting to destroy the device");
                break;
            }
        }
    }
    close(pipe_read_end_fd->fd);
    free(pipe_read_end_fd);
    return NULL;
}

static sa_result start_write_and_poll_loop(sa_device *device, struct pollfd *pipe_read_end_fd) {
    sa_result result                 = SA_ERROR;
    sa_poll_management *poll_manager = NULL;
    /** Init the poll manager */
    if(init_poll_management(device, &poll_manager, pipe_read_end_fd) != SA_SUCCESS)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not initialize the poll manager");
        return result;
    }
    result = write_and_poll_loop(device, poll_manager);
    /** Cleanup */
    free(poll_manager->ufds);
    free(poll_manager);
    poll_manager = NULL;
    return result;
}

static sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager,
                                      struct pollfd *pipe_read_end_fd) {
    sa_poll_management *poll_manager_temp = (sa_poll_management *) malloc(sizeof(sa_poll_management));
    int err;

    poll_manager_temp->count = 1 + snd_pcm_poll_descriptors_count(device->handle);
    /** There must be at least one alsa descriptor */
    if(poll_manager_temp->count <= 1)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Invalid poll descriptor count");
        return SA_ERROR;
    }

    poll_manager_temp->ufds = (struct pollfd *) malloc(sizeof(struct pollfd) * (poll_manager_temp->count));
    if(poll_manager_temp->ufds == NULL)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Not enough memory to allocate ufds");
        return SA_ERROR;
    }
    /** Store read end of pipe in the array */
    poll_manager_temp->ufds[0] = *pipe_read_end_fd;

    /** Don't give ALSA the first poll descriptor */
    if((err = snd_pcm_poll_descriptors(device->handle, poll_manager_temp->ufds + 1,
                                       poll_manager_temp->count - 1)) < 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Unable to obtain poll descriptors for playback", snd_strerror(err));
        return SA_ERROR;
    }
    *poll_manager = poll_manager_temp;
    return SA_SUCCESS;
}

static sa_result close_playback_thread(sa_device *device) {
    if(pthread_join(device->playback_thread, NULL) != 0)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not join playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

static sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager) {
    int *ptr;
    int err, cptr, init, readcount;
    readcount = 1;
    init      = 1;
    while(1)
    {
        if(!init)
        {
            err = wait_for_poll(device, poll_manager);

            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(SA_LOG_LEVEL_ERROR, "Wait for poll failed");
                    return SA_ERROR;
                }
            } else if(err == SA_STOP)
            { return SA_STOP; }
        }
        /** If the callback has not written any frames in the previous call- there are no frames left so we stop the callback loop */

        int (*data_callback)(int framesToSend, void *audioBuffer, sa_device *sa_device,
                             void *my_custom_data) =
          (int (*)(int, void *, sa_device *, void *my_custom_data)) device->config->data_callback;
        readcount = data_callback(device->period_size, device->samples, device,
                                  device->config->my_custom_data);

        if(readcount == 0)
        { return SA_AT_END; }

        ptr  = device->samples;
        cptr = readcount;

        while(cptr > 0)
        {
            err = snd_pcm_writei(device->handle, ptr, cptr);
            if(err < 0)
            {
                if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                {
                    SA_LOG(SA_LOG_LEVEL_ERROR, "Write error:", snd_strerror(err));
                    return SA_ERROR;
                }
                init = 1;
                break;
            }
            if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING)
                init = 0;
            ptr += err * device->config->channels;
            cptr -= err;
            if(cptr == 0)
                break;
            /* It is possible, that the initial buffer cannot store all data from the last period, so wait a while */
            err = wait_for_poll(device, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        SA_LOG(SA_LOG_LEVEL_ERROR, "Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(SA_LOG_LEVEL_ERROR, "Wait for poll failed");
                    return SA_ERROR;
                }
            } else if(err == SA_STOP)
            { return SA_STOP; }
        }
    }
    return SA_SUCCESS;
}

static int wait_for_poll(sa_device *device, sa_poll_management *poll_manager) {
    unsigned short revents;
    char command;
    while(1)
    {
        /** A period is the number of frames in between each hardware interrupt. The poll() will return once a period */
        poll(poll_manager->ufds, poll_manager->count, -1);

        if(poll_manager->ufds[0].revents & POLLIN)
        {
            if(read(poll_manager->ufds[0].fd, &command, 1) != 1)
            {
                SA_LOG(SA_LOG_LEVEL_ERROR, "Pipe read error");
            } else
            {
                switch(command)
                {
                /** Stop playback */
                case 's':
                    return SA_STOP;
                    break;
                /** Pause playback */
                case 'p':
                    {
                        sa_result res = pause_callback_loop(poll_manager, device);
                        /** The 'paused' state can end in 2 ways: either a stop command kills the device or an unpause command resumes playback */
                        if(res == SA_STOP)
                        { return SA_STOP; }

                        if(res == SA_UNPAUSE)
                        {
                            unpause_PCM_handle(device);
                            save_device_state(device, SA_DEVICE_STARTED);
                        }

                        break;
                    }
                default:
                    SA_LOG(SA_LOG_LEVEL_DEBUG, "Command sent to the pipe is ignored");
                    break;
                }
            }
        } else
        {
            snd_pcm_poll_descriptors_revents(device->handle, poll_manager->ufds + 1, poll_manager->count - 1,
                                             &revents);
            if(revents & POLLERR)
                return -EIO;
            if(revents & POLLOUT)
                return SA_SUCCESS;
        }
    }
    SA_LOG(SA_LOG_LEVEL_ERROR, "Poll loop ended without proper return");
    return -1;
}

static sa_result pause_callback_loop(sa_poll_management *poll_manager, sa_device *device) {
    pause_PCM_handle(device);

    int pauzed = 1;
    char command;
    while(pauzed)
    {
        poll(&(poll_manager->ufds[0]), 1, -1);
        if(read(poll_manager->ufds[0].fd, &command, 1) != 1)
        {
            SA_LOG(SA_LOG_LEVEL_ERROR, "Pipe read error");
        } else
        {
            switch(command)
            {
            /** Stop playback */
            case 's':
                {
                    return SA_STOP;
                    break;
                }
            /** Unpause */
            case 'u':
                {
                    return SA_UNPAUSE;
                    break;
                }
            default:
                break;
            }
        }
    }
    return SA_ERROR;
}

static void save_device_state(sa_device *device, sa_device_state new_state) {
    /** Save state */
    sa_set_device_state(device, new_state);
    pthread_mutex_lock(&(device->condition_var->mutex));
    device->condition_var->is_stopped = new_state == SA_DEVICE_STOPPED;
    pthread_mutex_unlock(&(device->condition_var->mutex));
    /** Broadcast change*/
    pthread_cond_signal(&(device->condition_var->cond));
}

static sa_result xrun_recovery(snd_pcm_t *handle, int err) {
    SA_LOG(SA_LOG_LEVEL_DEBUG, "ASLA: xrun occured");
    if(err == -EPIPE)
    { /* Underrun */
        err = snd_pcm_prepare(handle);
        if(err < 0)
        {
            SA_LOG(SA_LOG_LEVEL_ERROR,
                   "ALSA: Can't recover from underrun, prepare failed:", snd_strerror(err));
            return SA_ERROR;
        }
    } else if(err == -ESTRPIPE)
    {
        while((err = snd_pcm_resume(handle)) == -EAGAIN)
        {
            /* Wait until the suspend flag is released */
            sleep(1);
        }
        if(err < 0)
        {
            err = snd_pcm_prepare(handle);
            if(err < 0)
            {
                SA_LOG(SA_LOG_LEVEL_ERROR,
                       "ALSA: Can't recover from suspend, prepare failed:", snd_strerror(err));
                return SA_ERROR;
            }
        }
        return SA_SUCCESS;
    }
    return SA_ERROR;
}

static sa_result pause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'p') == SA_ERROR)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not send pause command to the message pipe");
        return SA_ERROR;
    };
    sa_set_device_state(device, SA_DEVICE_PAUSED);
    return SA_SUCCESS;
}

static sa_result unpause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'u') == SA_ERROR)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not send unpause command to the message pipe");
        return SA_ERROR;
    };
    sa_set_device_state(device, SA_DEVICE_STARTED);
    return SA_SUCCESS;
}

static sa_result wait_for_start_alsa_device(sa_device *device) {
    while((device->condition_var->is_stopped))
    { pthread_cond_wait(&(device->condition_var->cond), &(device->condition_var->mutex)); }
    return SA_SUCCESS;
}

static sa_result stop_alsa_device(sa_device *device) {
    if(message_pipe(device, 's') == SA_ERROR)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not send cancel command to the message pipe");
        return SA_ERROR;
    };
    sa_set_device_state(device, SA_DEVICE_STOPPED);
    return SA_SUCCESS;
}

static sa_result wait_for_stop_alsa_device(sa_device *device) {
    while(!(device->condition_var->is_stopped))
    { pthread_cond_wait(&(device->condition_var->cond), &(device->condition_var->mutex)); }
    return SA_SUCCESS;
}

static sa_result pause_PCM_handle(sa_device *device) {
    if(device->supports_pause)
    {
        if(snd_pcm_pause(device->handle, 1) != 0)
        {
            SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: Failed to snd_pcm_pause the pcm handle (when pausing)");
            return SA_ERROR;
        }
        return SA_SUCCESS;
    } else
    {
        if(drop_alsa_device(device) == SA_SUCCESS && prepare_alsa_device(device) == SA_SUCCESS)
            return SA_SUCCESS;
    }
    return SA_ERROR;
}

static sa_result unpause_PCM_handle(sa_device *device) {
    if(device->supports_pause)
    {
        if(snd_pcm_pause(device->handle, 0) != 0)
        {
            SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: Failed to snd_pcm_pause the pcm handle (when resuming)");
            return SA_ERROR;
        }
    }
    return SA_SUCCESS;
}

static sa_result drop_alsa_device(sa_device *device) {
    SA_LOG(SA_LOG_LEVEL_DEBUG, "ALSA drop called");
    int err = 0;
    if(device->handle && (snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING ||
                          snd_pcm_state(device->handle) == SND_PCM_STATE_PAUSED))
    {
        err = snd_pcm_drop(device->handle);
        if(err == 0)
        {
            return SA_SUCCESS;
        } else
        { SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: snd_pcm_drop() failed: ", snd_strerror(err)); }
    }
    SA_LOG(SA_LOG_LEVEL_ERROR,
           "Failed to drop samples from the ALSA device: pcm_hanlde not in runnning or paused state");
    exit(EXIT_FAILURE);
}

static sa_result drain_alsa_device(sa_device *device) {
    SA_LOG(SA_LOG_LEVEL_DEBUG, "ALSA drain called");
    int err = 0;
    if(device->handle && (snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING ||
                          snd_pcm_state(device->handle) == SND_PCM_STATE_PAUSED))
    {
        err = snd_pcm_drain(device->handle);
        if(err == 0)
        {
            return SA_SUCCESS;
        } else
        { SA_LOG(SA_LOG_LEVEL_ERROR, "ALSA: snd_pcm_drain() failed: ", snd_strerror(err)); }
    }
    SA_LOG(SA_LOG_LEVEL_ERROR,
           "Failed to drain samples from the ALSA device: pcm_handle not in runnning or paused state");
    return SA_ERROR;
}

static sa_result prepare_alsa_device(sa_device *device) {
    SA_LOG(SA_LOG_LEVEL_DEBUG, "ALSA prepare called");
    if(device->handle && snd_pcm_prepare(device->handle) == 0)
    { return SA_SUCCESS; }
    SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to prepare the ALSA device");
    exit(EXIT_FAILURE);
}

static sa_result message_pipe(sa_device *device, char toSend) {
    int result = write(device->pipe_write_end, &toSend, 1);
    if(result != 1)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Failed to write to the pipe");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

static sa_result cleanup_device(sa_device *device) {
    if(device)
    {
        close(device->pipe_write_end);

        if(device->config)
        { free(device->config); }
        if(device->condition_var)
        {
            pthread_mutex_destroy(&(device->condition_var->mutex));
            pthread_cond_destroy(&(device->condition_var->cond));
            free(device->condition_var);
        }
        if(device->samples)
        { free(device->samples); }
        if(device->handle)
        {
            int err = snd_pcm_close(device->handle);
            if(err < 0)
            { SA_LOG(SA_LOG_LEVEL_ERROR, "Could not close handle : ", snd_strerror(err)); }
        }
        pthread_mutex_destroy(&(device->stateMutex));
        free(device);
        snd_config_update_free_global();
    }
    return SA_SUCCESS;
}

static sa_result destroy_alsa_device(sa_device *device) {
    sa_stop_device(device);
    message_pipe(device, 'd');
    if(close_playback_thread(device) == SA_ERROR)
    {
        SA_LOG(SA_LOG_LEVEL_ERROR, "Could not close thread");
        exit(EXIT_FAILURE);
    }
    return cleanup_device(device);
}

static void sa_set_device_state(sa_device *device, sa_device_state state) {
    pthread_mutex_lock(&(device->stateMutex));
    device->state = state;
    pthread_mutex_unlock(&(device->stateMutex));
}

static float convert_dB_to_percentage(float dB) {
    float result;
    if(dB <= -100)
    {
        result = 0;
    } else if(dB > 0)
    {
        result = 100;
    } else
    { result = pow(10.0, (float) (dB / 20)) * 100; }
    return result;
}

#endif  // SA_IMPLEMENTATION
#endif  // SIMPLEALSA_H
