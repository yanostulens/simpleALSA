#include <alsa/asoundlib.h>

/** ENUMS **/

/**
 * @brief enum used to return function results
 * 
 */
enum sa_result {
    SA_SUCCESS = 0,
    SA_ERROR = 1
};

/**
 * @brief enum used to indicate the status of a device
 * 
 */
enum sa_device_status {
    UNINITIALIZED = 0,
    INITIALIZED = 1,
    STARTED = 2,
};


/** STRUCTS **/

/**
 * @brief struct used to config a simple ALSA devicre
 * 
 */
struct sa_device_config
{
    int sampleRate; /** Rate at which samples are send through the soundcard */
    int channels;   /** Amount of desired audiochannels */
    int bufferTime; /** Defines the size (in µs) of the internal ALSA ring buffer */
    int periodTime; /** Defines the time (in µs) after which ALSA will wake up to check if the buffer is running empty - increasing this time will increase efficiency, but risk the buffer running empty */

    snd_pcm_format_t format;    /** Format of the frames that are send to the ALSA buffer */ 
    char *device;               /** Name of the device - this name indicates ALSA to which physical device it must send audio - the default devices can be used by assigning this variable to "default" */

    void (*callbackFunction)(int framesToSend, void* audioBuffer, sa_device* sa_device);   /** Callback function that will be called whenever the internal buffer is running empty and new audio samples are required */
};

/**
 * @brief struct used to encapsulate a simple ALSA device
 * 
 */
struct sa_device
{
    sa_device_config* config;
    sa_device_status status;
};


/** FUNCTIONS DEFINITIONS **/

/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
sa_return_status sa_init_device_config(sa_device_config* config);

/**
 * @brief initializes a new audio device
 * @param config - configuration used to initialize the device
 * @param device - pointer to the initialized audio device
 * @return sa_return_status 
 */
sa_return_status sa_init_device(sa_device_config* config, sa_device* device);

/**
 * @brief starts the simple ALSA device - which starts the callback loop
 * 
 * @param device - device to start 
 * @return sa_return_status 
 */
sa_return_status sa_start_device(sa_device* device);

/**
 * @brief pauses the simple ALSA device - which pauses the callback loop
 * 
 * @param device - device to pause
 * @return sa_return_status 
 */
sa_return_status sa_pause_device(sa_device* device);

/**
 * @brief destroys the device - device pointer is set to NULL
 * 
 * @param device - the device to destroy
 * @return sa_return_status 
 */
sa_return_status sa_destroy_device(sa_device* device);
