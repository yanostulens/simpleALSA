#include "simpleALSA.h"

#include "ALSAfunctions.h"

sa_result sa_init_device_config(sa_device_config *config) {
    config = malloc(sizeof(sa_device_config));
    if(!config)
        return SA_ERROR;

    config->sampleRate       = DEFAULT_SAMPLE_RATE;
    config->channels         = DEFAULT_NUMBER_OF_CHANNELS;
    config->bufferTime       = DEFAULT_BUFFER_TIME;
    config->periodTime       = DEFAULT_PERIOD_TIME;
    config->format           = DEFAULT_AUDIO_FORMAT;
    config->device           = DEFAULT_DEVICE;
    config->callbackFunction = NULL;

    return SA_SUCCESS;
}

sa_result sa_init_device(sa_device_config *config, sa_device *device) {
    device = malloc(sizeof(sa_device));
    if(!device)
        return SA_ERROR;

    device->config = config;
    device->status = SA_DEVICE_READY;
    return init_alsa_device(device);
}

sa_result sa_start_device(sa_device *device) {
    return start_alsa_device(device);
}
