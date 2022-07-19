#ifndef _CONFIG_H_
#define _CONFIG_H_

/** ENUMS **/

/**
 * @brief enum used to return function results
 *
 */
typedef enum sa_result
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

#endif /* _CONFIG_H_ */
