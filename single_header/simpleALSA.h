#ifndef SIMPLEASLA_H_
    #define SIMPLEALSA_H_

    #include <alsa/asoundlib.h>

    #ifndef _CONFIG_H_
#define _CONFIG_H_

#include <alsa/asoundlib.h>
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

/** FUNCTIONS DEFINITIONS **/

/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
sa_result sa_init_device_config(sa_device_config **config);

/**
 * @brief initializes a new audio device
 * @param config - configuration used to initialize the device
 * @param device - pointer to the initialized audio device
 * @return sa_return_status
 */
sa_result sa_init_device(sa_device_config *config, sa_device **device);

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

#endif  // SIMPLEALSA_H_

#ifndef ALSAFUNCTIONS_H_
#define ALSAFUNCTIONS_H_
#include <alsa/asoundlib.h>

/** STRUCTS */

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

/** FUNCTIONS */

/**
 * @brief Initialized an ALSA device and store some settings in de sa_device
 *
 * @return sa_result
 */
sa_result init_alsa_device(sa_device *device);

/**
 * @brief Sets the ALSA hardware parameters
 *
 * @param device
 * @param access
 * @return sa_result
 */
sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access);
/**
 * @brief Sets the ALSA software parameters
 *
 * @param device
 * @return sa_result
 */
sa_result set_software_parameters(sa_device *device);

/**
 * @brief Prepares and starts the playback thread and creates a communication pipe
 *
 * @param device
 * @return sa_result
 */
sa_result prepare_playback_thread(sa_device *device);
/**
 * @brief Starts the ALSA write and wait loop
 *
 * @param device
 * @return sa_result
 */
sa_result start_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to the transfer loop to pause audio output
 *
 * @param device
 * @return sa_result
 */
sa_result pause_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to the transfer loop to unpause audio output
 *
 * @param device
 * @return sa_result
 */
sa_result unpause_alsa_device(sa_device *device);

/**
 * @brief Sends out a message to stop the transfer loop
 *
 * @param device
 * @return sa_result
 */
sa_result stop_alsa_device(sa_device *device);

/**
 * @brief Drains the samples of the internal ALSA buffer
 *
 * @param device
 * @return sa_result
 */
sa_result drain_alsa_device(sa_device *device);

/**
 * @brief Initializes the polling filedescriptors for ALSA and links it to the communication pipe
 *
 * @param device
 * @param poll_manager, nullpointer to initialize
 * @param pipe_read_end_fd
 * @return sa_result
 */
sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager,
                               struct pollfd *pipe_read_end_fd);

/**
 * @brief Starts the audio playback thread by running the write and poll loop
 *
 * @param data: a sa_thread_data packet
 *
 */
void *init_playback_thread(void *data);

/**
 * @brief Attempts to join the playback thread
 *
 * @param device
 * @return sa_result
 */
sa_result close_playback_thread(sa_device *device);
/**
 * @brief Prepares and starts write_and_poll_loop
 *
 * @param device
 * @param pipe_read_end_fd
 * @return sa_result
 */
sa_result start_write_and_poll_loop(sa_device *device, struct pollfd *pipe_read_end_fd);

/**
 * @brief Plays audio by repeatedly calling the callback function for framas
 *
 * @param device
 * @param poll_manager
 * @return sa_result
 */
sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Waits on poll and checks pipe
 *
 * @param handle
 * @param poll_manager
 * @return int
 */
int wait_for_poll(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Try to recover from errors during playback
 *
 * @param handle
 * @param err
 * @return sa_result
 */
sa_result xrun_recovery(snd_pcm_t *handle, int err);
/**
 * @brief reclaims sa_device
 *
 * @param device
 * @return sa_result
 */
sa_result cleanup_device(sa_device *device);
/**
 * @brief Messages a char to the playback thread via a pipe
 *
 * @param device
 * @param toSend
 * @return sa_result
 */
sa_result message_pipe(sa_device *device, char toSend);

/**
 * @brief Pauses the callback loop, this function will pause the PCM handle by calling pause_PCM_handle().
 * After that it will block and wait for further commands from the message pipe.
 *
 * @param poll_manager
 * @param handle
 * @return sa_result
 */
sa_result pause_callback_loop(sa_poll_management *poll_manager, sa_device *device);

/**
 * @brief Checks whether the hardware support pausing, if so it pauses using snd_pcm_pause(). If the hw does
 * not support pausing it uses snd_pcm_drop() and prepares the the device using snd_pcm_drain().
 *
 * @param device
 * @return sa_result
 */
sa_result pause_PCM_handle(sa_device *device);

/**
 * @brief Checks whether the pcm was paused using snd_pcm_pause() or not and resume the pcm accordingly.
 *
 * @param device
 * @return sa_result
 */
sa_result unpause_PCM_handle(sa_device *device);

/**
 * @brief Prepares the ALSA device so it is ready for a restart
 *
 * @param device
 * @return sa_result
 */
sa_result prepare_alsa_device(sa_device *device);

/**
 * @brief Destroys the ALSA device, closes threads and frees all allocated memory
 *
 * @param device
 * @return sa_result
 */
sa_result destroy_alsa_device(sa_device *device);

#endif  // ALSAFUNCTIONS_H_

sa_result sa_init_device_config(sa_device_config **config) {
    sa_device_config *config_temp = malloc(sizeof(sa_device_config));
    if(!config_temp)
        return SA_ERROR;

    config_temp->sampleRate       = DEFAULT_SAMPLE_RATE;
    config_temp->channels         = DEFAULT_NUMBER_OF_CHANNELS;
    config_temp->bufferTime       = DEFAULT_BUFFER_TIME;
    config_temp->periodTime       = DEFAULT_PERIOD_TIME;
    config_temp->format           = DEFAULT_AUDIO_FORMAT;
    config_temp->device           = "default";
    config_temp->callbackFunction = NULL;
    *config                       = config_temp;
    return SA_SUCCESS;
}

sa_result sa_init_device(sa_device_config *config, sa_device **device) {
    sa_device *device_temp = malloc(sizeof(sa_device));
    if(!device_temp)
        return SA_ERROR;

    device_temp->config = config;
    *device             = device_temp;
    return init_alsa_device(*device);
}

sa_result sa_start_device(sa_device *device) {
    return start_alsa_device(device);
}

sa_result sa_stop_device(sa_device *device) {
    return stop_alsa_device(device);
}

sa_result sa_pause_device(sa_device *device) {
    return pause_alsa_device(device);
}

sa_result sa_destroy_device(sa_device *device) {
    return destroy_alsa_device(device);
}
#include <pthread.h>

#ifndef LOGGER_H
#define LOGGER_H

#define SA_LOG_2_ARGS(type, msg0)        sa_log(type, msg0, "")
#define SA_LOG_3_ARGS(type, msg0, msg1)  sa_log(type, msg0, msg1)

#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define SA_LOG_MACRO_CHOOSER(...) \
    GET_4TH_ARG(__VA_ARGS__, SA_LOG_3_ARGS, \
                SA_LOG_2_ARGS, )

#if defined SA_NO_LOGS
    #define SA_LOG(...) ((void) 0)
#else
    #define SA_LOG(...) SA_LOG_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#endif

/**
 * @brief enum to identify different types of logs
 *
 */
typedef enum
{
    DEBUG   = 0,
    WARNING = 1,
    ERROR   = 2
} sa_log_type;

void sa_log(sa_log_type type, const char msg0[], const char msg1[]);

#endif  // LOGGER_H

sa_result init_alsa_device(sa_device *device) {
    int err;
    snd_pcm_hw_params_alloca(&(device->hwparams));
    snd_pcm_sw_params_alloca(&(device->swparams));

    if((err = snd_pcm_open(&(device->handle), device->config->device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        SA_LOG(ERROR, "ALSA: playback open error:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if((err = set_hardware_parameters(device, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        SA_LOG(ERROR, "ALSA: setting hardware parameters failed:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_software_parameters(device)) < 0)
    {
        SA_LOG(ERROR, "ALSA: setting software parameters failed:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    device->samples = (signed short *) malloc((device->periodSize * device->config->channels *
                                               snd_pcm_format_physical_width(device->config->format)) /
                                              8);

    if(device->samples == NULL)
    { exit(EXIT_FAILURE); }

    device->supportsPause = snd_pcm_hw_params_can_pause(device->hwparams);
    if(device->supportsPause)
    {
        SA_LOG(DEBUG, "Device supports snd_pcm_pause()");
    } else
    { SA_LOG(DEBUG, "Device does not support snd_pcm_pause()"); }

    if(prepare_playback_thread(device) != SA_SUCCESS)
    {
        SA_LOG(ERROR, "Failed to perpare the playback thread");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    err = snd_pcm_hw_params_any(device->handle, device->hwparams);
    if(err < 0)
    {
        SA_LOG(ERROR,
               "ALSA: broken configuration for playback: no configurations available:", snd_strerror(err));
        return SA_ERROR;
    }
    /** Set the samplerate */
    err = snd_pcm_hw_params_set_rate_resample(device->handle, device->hwparams, 1);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: resampling setup failed for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(device->handle, device->hwparams, access);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: access type not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(device->handle, device->hwparams, device->config->format);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: sample format not available for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(device->handle, device->hwparams, device->config->channels);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: channels count not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the stream rate */
    rrate = device->config->sampleRate;
    err   = snd_pcm_hw_params_set_rate_near(device->handle, device->hwparams, &rrate, 0);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: samplerate not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    if(rrate != device->config->sampleRate)
    {
        SA_LOG(ERROR, "ALSA: sample rate does not match the requested rate");
        return SA_ERROR;
    }
    /* Set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(device->handle, device->hwparams,
                                                 (unsigned int *) &(device->config->bufferTime), &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set the buffer time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_buffer_size(device->hwparams, &size);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to get the buffer size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->bufferSize = size;
    /* Set the period time */
    err                = snd_pcm_hw_params_set_period_time_near(device->handle, device->hwparams,
                                                 (unsigned int *) &(device->config->periodTime), &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set the period time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_period_size(device->hwparams, &size, &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to get period size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->periodSize = size;
    /* Write the parameters to device */
    err                = snd_pcm_hw_params(device->handle, device->hwparams);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set hardware parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result set_software_parameters(sa_device *device) {
    int err;

    /* Get the current swparams */
    err = snd_pcm_sw_params_current(device->handle, device->swparams);
    if(err < 0)
    {
        SA_LOG(ERROR,
               "ALSA: unable to determine current software parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Start the transfer when the buffer is almost full: (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(
      device->handle, device->swparams, (device->bufferSize / device->periodSize) * device->periodSize);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to start set threshold mode for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    err = snd_pcm_sw_params_set_avail_min(device->handle, device->swparams,
                                          0 ? device->bufferSize : device->periodSize);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set available minimum for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Enable period events when requested */
    if(0)
    {
        err = snd_pcm_sw_params_set_period_event(device->handle, device->swparams, 1);
        if(err < 0)
        {
            SA_LOG(ERROR, "ALSA: unable to set period event", snd_strerror(err));
            return SA_ERROR;
        }
    }
    /* Write the parameters to the playback device */
    err = snd_pcm_sw_params(device->handle, device->swparams);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set software parameters for playback", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result prepare_playback_thread(sa_device *device) {
    /** Prepare communication pipe */
    int pipe_fds[2];
    if(pipe(pipe_fds))
    {
        SA_LOG(ERROR, "Cannot create poll_pipe");
        return SA_ERROR;
    }
    /** Makes read end nonblocking */
    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        SA_LOG(ERROR, "Failed to make pipe non-blocking");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return SA_ERROR;
    }
    /** Store the write end */
    device->pipe_write_end          = pipe_fds[1];
    /** Prepare read polling structure of the read end */
    struct pollfd *pipe_read_end_fd = malloc(sizeof(struct pollfd));
    pipe_read_end_fd->fd            = pipe_fds[0];
    pipe_read_end_fd->events        = POLLIN;
    /** Startup the playback thread */
    sa_thread_data *thread_data     = malloc(sizeof(sa_thread_data));
    thread_data->device             = device;
    thread_data->pipe_read_end_fd   = pipe_read_end_fd;
    if(pthread_create(&device->playbackThread, NULL, &init_playback_thread, (void *) thread_data) != 0)
    {
        SA_LOG(ERROR, "Failed to create the playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result start_alsa_device(sa_device *device) {
    return unpause_alsa_device(device);
}

void *init_playback_thread(void *data) {
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
                SA_LOG(ERROR, "Pipe read error");
            } else
            {
                switch(command)
                {
                /** Play command */
                case 'u':
                    if(start_write_and_poll_loop(device, pipe_read_end_fd) == SA_ERROR)
                    { break; }
                    continue;
                /** Destroy command, no continue; break out of while */
                case 'd':
                    break;
                default:
                    SA_LOG(WARNING, "Invalid command send to the pipe");
                    continue;
                }
                SA_LOG(DEBUG, "Attempting to destroy the device");
                break;
            }
        }
    }
    close(pipe_read_end_fd->fd);
    free(pipe_read_end_fd);
    return NULL;
}

sa_result start_write_and_poll_loop(sa_device *device, struct pollfd *pipe_read_end_fd) {
    sa_poll_management *poll_manager = NULL;
    /** Init the poll manager */
    if(init_poll_management(device, &poll_manager, pipe_read_end_fd) != SA_SUCCESS)
    {
        SA_LOG(ERROR, "Could not initialize the poll manager");
        return SA_ERROR;
    }
    write_and_poll_loop(device, poll_manager);
    /** Cleanup */
    free(poll_manager->ufds);
    free(poll_manager);
    poll_manager = NULL;
    return SA_SUCCESS;
}

sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager,
                               struct pollfd *pipe_read_end_fd) {
    sa_poll_management *poll_manager_temp = (sa_poll_management *) malloc(sizeof(sa_poll_management));
    int err;

    poll_manager_temp->count = 1 + snd_pcm_poll_descriptors_count(device->handle);
    /** There must be at least one alsa descriptor */
    if(poll_manager_temp->count <= 1)
    {
        SA_LOG(ERROR, "Invalid poll descriptor count");
        return SA_ERROR;
    }

    poll_manager_temp->ufds = malloc(sizeof(struct pollfd) * (poll_manager_temp->count));
    if(poll_manager_temp->ufds == NULL)
    {
        SA_LOG(ERROR, "Not enough memory to allocate ufds");
        return SA_ERROR;
    }
    /** Store read end of pipe in the array */
    poll_manager_temp->ufds[0] = *pipe_read_end_fd;

    /** Don't give ALSA the first poll descriptor */
    if((err = snd_pcm_poll_descriptors(device->handle, poll_manager_temp->ufds + 1,
                                       poll_manager_temp->count - 1)) < 0)
    {
        SA_LOG(ERROR, "Unable to obtain poll descriptors for playback", snd_strerror(err));
        return SA_ERROR;
    }
    *poll_manager = poll_manager_temp;
    return SA_SUCCESS;
}

sa_result close_playback_thread(sa_device *device) {
    if(pthread_join(device->playbackThread, NULL) != 0)
    {
        SA_LOG(ERROR, "Could not join playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager) {
    signed short *ptr;
    int err, cptr, init, readcount;
    init = 1;
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
                        SA_LOG(ERROR, "Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(ERROR, "Wait for poll failed");
                    return SA_ERROR;
                }
            } else if(err == SA_CANCEL)
            { return SA_CANCEL; }
        }
        int (*callbackFunction)(int framesToSend, void *audioBuffer, sa_device *sa_device) =
          (int (*)(int, void *, sa_device *)) device->config->callbackFunction;
        readcount = callbackFunction(device->periodSize, device->samples, device);

        /** If the callback has not written any frames - there are no frames left so we stop the callback loop */
        if(!readcount)
        { break; }

        ptr  = device->samples;
        cptr = device->periodSize;
        while(cptr > 0)
        {
            err = snd_pcm_writei(device->handle, ptr, cptr);
            if(err < 0)
            {
                if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                {
                    SA_LOG(ERROR, "Write error:", snd_strerror(err));
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
                        SA_LOG(ERROR, "Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(ERROR, "Wait for poll failed");
                    return SA_ERROR;
                }
            } else if(err == SA_CANCEL)
            { return SA_CANCEL; }
        }
    }
    return SA_SUCCESS;
}

int wait_for_poll(sa_device *device, sa_poll_management *poll_manager) {
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
                SA_LOG(ERROR, "Pipe read error");
            } else
            {
                switch(command)
                {
                /** Stop playack */
                case 's':
                    drain_alsa_device(device);
                    prepare_alsa_device(device);
                    return SA_CANCEL;
                    break;
                /** Pause playback */
                case 'p':
                    if(pause_callback_loop(poll_manager, device) == SA_CANCEL)
                    { return SA_CANCEL; }
                    break;
                default:
                    SA_LOG(WARNING, "Invalid command send to pipe");
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
    SA_LOG(ERROR, "Poll loop ended without proper return");
    return -1;
}

sa_result pause_callback_loop(sa_poll_management *poll_manager, sa_device *device) {
    pause_PCM_handle(device);

    int pauzed = 1;
    char command;
    while(pauzed)
    {
        poll(&(poll_manager->ufds[0]), 1, -1);
        if(read(poll_manager->ufds[0].fd, &command, 1) != 1)
        {
            SA_LOG(ERROR, "Pipe read error");
        } else
        {
            switch(command)
            {
            /** Stop playback */
            case 's':
                drain_alsa_device(device);
                prepare_alsa_device(device);
                return SA_CANCEL;
                break;
            /** Unpause */
            case 'u':
                unpause_PCM_handle(device);
                return SA_SUCCESS;
                break;
            default:
                break;
            }
        }
    }
    return SA_ERROR;
}

sa_result xrun_recovery(snd_pcm_t *handle, int err) {
    if(err == -EPIPE)
    { /* Underrun */
        err = snd_pcm_prepare(handle);
        if(err < 0)
        {
            SA_LOG(ERROR, "Can't recover from underrun, prepare failed:", snd_strerror(err));
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
                SA_LOG(ERROR, "Can't recover from suspend, prepare failed:", snd_strerror(err));
                return SA_ERROR;
            }
        }
        return SA_SUCCESS;
    }
    return SA_ERROR;
}

sa_result pause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'p') == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not send pause command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result unpause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'u') == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not send unpause command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result stop_alsa_device(sa_device *device) {
    if(message_pipe(device, 's') == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not send cancel command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
    // TODO CLEANUP
}

sa_result pause_PCM_handle(sa_device *device) {
    if(device->supportsPause)
    {
        if(snd_pcm_pause(device->handle, 1) != 0)
        {
            SA_LOG(ERROR, "Failed to snd_pcm_pause the pcm handle (when pausing)");
            return SA_ERROR;
        }
        return SA_SUCCESS;
    } else
    {
        if(drain_alsa_device(device) == SA_SUCCESS && prepare_alsa_device(device) == SA_SUCCESS)
            return SA_SUCCESS;
    }
    return SA_ERROR;
}

sa_result unpause_PCM_handle(sa_device *device) {
    if(device->supportsPause)
    {
        if(snd_pcm_pause(device->handle, 0) != 0)
        {
            SA_LOG(ERROR, "Failed to snd_pcm_pause the pcm handle (when resuming)");
            return SA_ERROR;
        }
    }
    return SA_SUCCESS;
}

sa_result drain_alsa_device(sa_device *device) {
    if(device->handle &&
       (snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING ||
        snd_pcm_state(device->handle) == SND_PCM_STATE_PAUSED) &&
       snd_pcm_drop(device->handle) == 0)
    { return SA_SUCCESS; }
    SA_LOG(ERROR, "Failed to drop samples from the ALSA device");
    exit(EXIT_FAILURE);
}

sa_result prepare_alsa_device(sa_device *device) {
    if(device->handle && snd_pcm_prepare(device->handle) == 0)
    { return SA_SUCCESS; }
    SA_LOG(ERROR, "Failed to prepare the ALSA device");
    exit(EXIT_FAILURE);
}

sa_result message_pipe(sa_device *device, char toSend) {
    int result = write(device->pipe_write_end, &toSend, 1);
    if(result != 1)
    {
        SA_LOG(ERROR, "Failed to write to the pipe");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

sa_result cleanup_device(sa_device *device) {
    if(device)
    {
        close(device->pipe_write_end);

        if(device->config)
        { free(device->config); }

        if(device->samples)
        { free(device->samples); }
        if(device->handle)
        {
            int err = snd_pcm_close(device->handle);
            if(err < 0)
            { SA_LOG(ERROR, "Could not close handle : ", snd_strerror(err)); }
        }
        free(device);
        snd_config_update_free_global();
    }
    return SA_SUCCESS;
}

sa_result destroy_alsa_device(sa_device *device) {
    stop_alsa_device(device);
    message_pipe(device, 'd');
    if(close_playback_thread(device) == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not close thread");
        exit(EXIT_FAILURE);
    }
    return cleanup_device(device);
}
#include <stdio.h>

void sa_log(sa_log_type type, const char msg0[], const char msg1[]) {
    switch(type)
    {
#ifndef SA_NO_ERROR_LOGS
    case ERROR:
        printf("\e[1;31m[  ERROR   ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
#ifndef SA_NO_WARNING_LOGS
    case WARNING:
        printf("\e[1;33m[  WARNING ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
#ifndef SA_NO_DEBUG_LOGS
    case DEBUG:
        printf("\e[1;35m[  DEBUG   ] \e[0m %s %s\n", msg0, msg1);
        break;
#endif
    default:
        break;
    }
    fflush(stdout);
}
