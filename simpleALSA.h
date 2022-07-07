#include <alsa/asoundlib.h>

/** ENUMS **/

enum sa_result {
    SA_SUCCESS = 0,
    SA_ERROR = 1
}


/** STRUCTS **/

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


/** FUNCTIONS DEFINITIONS **/

/**
 * @brief creates an sa_device_config struct and sets it to default values
 * @param sa_device_config* device - empty pointer into which the config struct is set
 * @return sa_return_status
 */
sa_return_status sa_init_device_config(sa_device_config* config);

/**
 * @brief initializes a new audio device
 * @param sa_device_config* config - configuration used to initialize the device
 * @param sa_device* device - pointer to the initialized audio device
 * @return sa_return_status 
 */
sa_return_status sa_init_device(sa_device_config* config, sa_device* device);