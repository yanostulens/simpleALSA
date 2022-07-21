#include "simpleALSA.h"

#include "ALSAfunctions.h"

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
