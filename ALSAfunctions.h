#ifndef ALSAFUNCTIONS_H
#define ALSAFUNCTIONS_H

#include <alsa/asoundlib.h>

#include "simpleALSA.h"

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

#endif  // ALSAFUNCTIONS_H
