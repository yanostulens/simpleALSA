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
