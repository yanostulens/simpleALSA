#ifndef ALSAFUNCTIONS_H
#define ALSAFUNCTIONS_H

#include <alsa/asoundlib.h>

int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access);

int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams);

#endif  // ALSAFUNCTIONS_H
