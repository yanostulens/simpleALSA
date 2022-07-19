#include "ALSAfunctions.h"

sa_result init_alsa_device(sa_device *device) {
    int err;
    snd_pcm_hw_params_alloca(&(device->hwparams));
    snd_pcm_sw_params_alloca(&(device->swparams));

    if((err = snd_pcm_open(&(device->handle), device->config->device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if((err = set_hwparams(device->handle, device->hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_swparams(device->handle, device->swparams)) < 0)
    {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    device->samples = (signed short *) malloc((device->periodSize * device->config->channels *
                                               snd_pcm_format_physical_width(device->config->format)) /
                                              8);
    if(device->samples == NULL)
    {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

sa_result start_alsa_device(sa_device *device) {
    // TODOO DAAN
}

sa_result pause_alsa_device(sa_device *device) {
    // TODOO DAAN: stop our callback loop here

    if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING) {
        snd_pcm_pause(device->handle);
    }
}

sa_result stop_alsa_device(sa_device *device) {
    // TODOO DAAN: stop our callback loop here
    
}



sa_result drain_alsa_device(sa_device *device) {
    if(device->handle)
    {
        if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING)
        {
            snd_pcm_drain(device->handle);
            return SA_SUCCESS;
        }
    }
    return SA_ERROR;
}
