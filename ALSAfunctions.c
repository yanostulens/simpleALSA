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

    if((err = set_hwparams(device, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_swparams(device)) < 0)
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

sa_result set_hwparams(sa_device *device, snd_pcm_access_t access) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(device->handle, device->hwparams);
    if(err < 0)
    {
        printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(device->handle, device->hwparams, 1);
    if(err < 0)
    {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(device->handle, device->hwparams, access);
    if(err < 0)
    {
        printf("Access type not available for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(device->handle, device->hwparams, device->config->format);
    if(err < 0)
    {
        printf("Sample format not available for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(device->handle, device->hwparams, device->config->channels);
    if(err < 0)
    {
        printf("Channels count (%u) not available for playbacks: %s\n", device->config->channels,
               snd_strerror(err));
        return SA_ERROR;
    }
    /* set the stream rate */
    rrate = device->config->sampleRate;
    err   = snd_pcm_hw_params_set_rate_near(device->handle, device->hwparams, &rrate, 0);
    if(err < 0)
    {
        printf("Rate %uHz not available for playback: %s\n", device->config->sampleRate, snd_strerror(err));
        return SA_ERROR;
    }
    if(rrate != device->config->sampleRate)
    {
        printf("Rate doesn't match (requested %uHz, get %iHz)\n", device->config->sampleRate, err);
        // return -EINVAL;
        return SA_ERROR;
    }
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(device->handle, device->hwparams,
                                                 &(device->config->bufferTime), &dir);
    if(err < 0)
    {
        printf("Unable to set buffer time %u for playback: %s\n", device->config->bufferTime,
               snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_buffer_size(device->hwparams, &size);
    if(err < 0)
    {
        printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    device->bufferSize = size;
    /* set the period time */
    err                = snd_pcm_hw_params_set_period_time_near(device->handle, device->hwparams,
                                                 &(device->config->periodTime), &dir);
    if(err < 0)
    {
        printf("Unable to set period time %u for playback: %s\n", device->config->periodTime,
               snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_period_size(device->hwparams, &size, &dir);
    if(err < 0)
    {
        printf("Unable to get period size for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    device->periodSize = size;
    /* write the parameters to device */
    err                = snd_pcm_hw_params(device->handle, device->hwparams);
    if(err < 0)
    {
        printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result set_swparams(sa_device *device) {
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(device->handle, device->swparams);
    if(err < 0)
    {
        printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(
      device->handle, device->swparams, (device->bufferSize / device->periodSize) * device->periodSize);
    if(err < 0)
    {
        printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    err = snd_pcm_sw_params_set_avail_min(device->handle, device->swparams,
                                          0 ? device->bufferSize : device->periodSize);
    if(err < 0)
    {
        printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    /* enable period events when requested */
    if(0)
    {
        err = snd_pcm_sw_params_set_period_event(device->handle, device->swparams, 1);
        if(err < 0)
        {
            printf("Unable to set period event: %s\n", snd_strerror(err));
            return SA_ERROR;
        }
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(device->handle, device->swparams);
    if(err < 0)
    {
        printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result start_alsa_device(sa_device *device) {
    sa_poll_management *poll_manager;
    if(init_poll_management(device, poll_manager) != SA_SUCCESS)
    {
        printf("Could not allocate poll descriptors and pipe\n");
        return SA_ERROR;
    }
    write_and_poll_loop(device, poll_manager);
}

sa_result init_poll_management(sa_device *device, sa_poll_management *poll_manager) {
    poll_manager = (sa_poll_management *) malloc(sizeof(sa_poll_management));
    int pipe_fds[2];  // store me somewhere (later)
    int err;
    if(pipe(pipe_fds))
    {
        printf("Cannot create poll_pipe\n");
        return SA_ERROR;
    }

    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        printf("Failed to make pipe non-blocking\n");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return SA_ERROR;
    }

    poll_manager->count = 1 + snd_pcm_poll_descriptors_count(device->handle);
    // there must be at least one alsa descriptor
    if(poll_manager->count <= 1)
    {
        printf("Invalid poll descriptors count\n");
        return poll_manager->count;
    }

    poll_manager->ufds = malloc(sizeof(struct pollfd) * (poll_manager->count));
    if(poll_manager->ufds == NULL)
    {
        printf("No enough memory\n");
        return SA_ERROR;
    }
    // store read end of pipe
    poll_manager->ufds[0].fd     = pipe_fds[0];
    poll_manager->ufds[0].events = POLLIN;
    // store the write end
    device->pipe_write_end       = pipe_fds[1];
    // dont give ALSA the first poll descriptor
    if((err = snd_pcm_poll_descriptors(device->handle, poll_manager->ufds + 1, poll_manager->count - 1)) < 0)
    {
        printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

int write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager) {
    signed short *ptr;
    int err, cptr, init;

    init = 1;
    int readcount;
    while(1)
    {
        if(!init)
        {
            err = wait_for_poll(device->handle, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    init = 1;
                } else
                {
                    printf("Wait for poll failed\n");
                    return err;
                }
            }
        }
        // CALL CALLBACK HERE to fill samples !!
        void (*callbackFunction)(int framesToSend, void *audioBuffer,
                                 sa_device *sa_device) = device->config->callbackFunction;
        callbackFunction(device->periodSize, device->samples, device);
        // if(!(readcount = sf_readf_short(infile, samples, period_size) > 0))
        //{ break; }

        printf("Readcount: %i\n", readcount);
        printf("Periodsize: %ld\n", device->periodSize);

        ptr  = device->samples;
        cptr = device->periodSize;
        while(cptr > 0)
        {
            err = snd_pcm_writei(device->handle, ptr, cptr);
            if(err < 0)
            {
                if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                {
                    printf("Write error: %s\n", snd_strerror(err));
                    exit(EXIT_FAILURE);
                }
                init = 1;
                break; /* skip one period */
            }
            if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING)
                init = 0;
            ptr += err * device->config->channels;
            cptr -= err;
            if(cptr == 0)
                break;
            /* it is possible, that the initial buffer cannot store */
            /* all data from the last period, so wait awhile */
            err = wait_for_poll(device->handle, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    init = 1;
                } else
                {
                    printf("Wait for poll failed\n");
                    return err;
                }
            }
        }
    }
    return 1;
}

/*
 *   Transfer method - write and wait for room in buffer using poll
 */

int wait_for_poll(snd_pcm_t *handle, sa_poll_management *poll_manager) {
    unsigned short revents;

    while(1)
    {
        /** A period is the number of frames in between each hardware interrupt. The poll() will return once a period */
        poll(poll_manager->ufds, poll_manager->count, -1);

        if(poll_manager->ufds[0].revents & POLLIN)
        {
            int status;
            if((read(poll_manager->ufds[0].fd, &status, 1) == 1) && (status == 1))
            {
                // end has signaled
                printf("Polling has been ended prematurely by pipe\n");
                return -1;
            }
        }

        snd_pcm_poll_descriptors_revents(handle, poll_manager->ufds + 1, poll_manager->count - 1, &revents);
        if(revents & POLLERR)
            return -EIO;
        if(revents & POLLOUT)
            return 0;
    }
    return -1;
}

sa_result xrun_recovery(snd_pcm_t *handle, int err) {
    if(err == -EPIPE)
    { /* under-run */
        err = snd_pcm_prepare(handle);
        if(err < 0)
        {
            printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
            return SA_ERROR;
        }
    } else if(err == -ESTRPIPE)
    {
        while((err = snd_pcm_resume(handle)) == -EAGAIN)
        {
            /* wait until the suspend flag is released */
            sleep(1);
        }
        if(err < 0)
        {
            err = snd_pcm_prepare(handle);
            if(err < 0)
            {
                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                return SA_ERROR;
            }
        }
        return SA_SUCCESS;
    }
    return SA_ERROR;
}

sa_result pause_alsa_device(sa_device *device) {
    // TODOO DAAN: stop our callback loop here

    if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING)
    {
        // TODO YANO PAUSE THE DEVICE HERE IN SOME WAY
    }
    return SA_ERROR;
}

sa_result stop_alsa_device(sa_device *device) {
    // TODOO DAAN: stop our callback loop here
    return SA_ERROR;
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
