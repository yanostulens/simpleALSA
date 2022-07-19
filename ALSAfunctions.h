#ifndef ALSAFUNCTIONS_H_
#define ALSAFUNCTIONS_H_
#include <alsa/asoundlib.h>

#include "config.h"

/** STRUCTS */

/**
 * @brief holds everything related to polling an playback
 */
typedef struct
{
    /** An array of file descriptors to poll, ufds[0] is the read end of the pipe */
    struct pollfd *ufds;
    /** The amount of file descriptors to poll */
    int count;
} sa_poll_management;

/** FUNCTIONS */

/**
 * @brief Initialized an ALSA device
 *
 * @return sa_result
 */
sa_result init_alsa_device(sa_device *device);

/**
 * @brief Sets the ALSA hardware parameters
 *
 * @param handle
 * @param params
 * @param access
 * @return sa_result
 */
sa_result set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access);

/**
 * @brief Sets the ALSA software parameters
 *
 * @param handle
 * @param swparams
 * @return sa_result
 */
sa_result set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams);

/**
 * @brief Starts the ALSA write and wait loop
 *
 * @param device
 * @return sa_result
 */
sa_result start_alsa_device(sa_device *device);

/**
 * @brief Pauses the ALSA device and stops the callback loop to the simpleALSA callback
 *
 * @param device
 * @return sa_result
 */
sa_result pause_alsa_device(sa_device *device);

/**
 * @brief Pauses the ALSA device and the callback loop and drains all the existing data
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
 * @brief Initializes the polling filedescriptors for alsa and a pipe for canceling playback
 *
 * @param device
 * @param poll_manager, nullpointer to initialize
 * @return sa_result
 */
sa_result init_poll_management(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Plays audio by repeatedly calling the callback function for framas
 *
 * @param device
 * @param poll_manager
 * @return int
 */
int write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager);

/**
 * @brief Waits on poll and checks pipe
 *
 * @param handle
 * @param poll_manager
 * @return int
 */
int wait_for_poll(snd_pcm_t *handle, sa_poll_management *poll_manager);

/**
 * @brief Try to recover from errors during playback
 *
 * @param handle
 * @param err
 * @return sa_result
 */
sa_result xrun_recovery(snd_pcm_t *handle, int err);
/**
 * @brief reclaims sa_device and poll_manager
 *
 * @param device
 * @param poll_manager
 * @return sa_result
 */
sa_result cleanup(sa_device *device, sa_poll_management *poll_manager);

#endif  // ALSAFUNCTIONS_H_
