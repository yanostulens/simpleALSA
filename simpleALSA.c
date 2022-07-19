#include "simpleALSA.h"

sa_result sa_init_device_config(sa_device_config *config) {
    config = malloc(sizeof(sa_device_config));

    config->sampleRate       = DEFAULT_SAMPLE_RATE;
    config->channels         = DEFAULT_NUMBER_OF_CHANNELS;
    config->bufferTime       = DEFAULT_BUFFER_TIME;
    config->periodTime       = DEFAULT_PERIOD_TIME;
    config->format           = DEFAULT_AUDIO_FORMAT;
    config->device           = DEFAULT_DEVICE;
    config->callbackFunction = NULL;

    return SA_SUCCESS;
}
