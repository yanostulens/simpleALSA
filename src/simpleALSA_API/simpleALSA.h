#ifndef SIMPLEASLA_H_
    #define SIMPLEALSA_H_

    #include <alsa/asoundlib.h>
    #include "../config.h"

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
