#ifndef ALSAFUNCTIONS_H_
#define ALSAFUNCTIONS_H_
#include <alsa/asoundlib.h>

#include "../config.h"
#include "../simpleALSA_API/simpleALSA.h"

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
