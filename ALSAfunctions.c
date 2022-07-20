#include <pthread.h>

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

    if((err = set_hardware_parameters(device, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_software_parameters(device)) < 0)
    {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    device->samples = (signed short *) malloc((device->periodSize * device->config->channels *
                                               snd_pcm_format_physical_width(device->config->format)) /
                                              8);

    if(device->samples == NULL)
    { exit(EXIT_FAILURE); }

    device->supportsPause = snd_pcm_hw_params_can_pause(device->hwparams);
    if(device->supportsPause)
    {
        printf("Device supports snd_pcm_pause()\n");
    } else
    { printf("Device does not support snd_pcm_pause()\n"); }

    return SA_SUCCESS;
}

sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access) {
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
                                                 (unsigned int *) &(device->config->bufferTime), &dir);
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
                                                 (unsigned int *) &(device->config->periodTime), &dir);
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

sa_result set_software_parameters(sa_device *device) {
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
    sa_poll_management *poll_manager = NULL;
    if(init_poll_management(device, &poll_manager) != SA_SUCCESS)
    {
        printf("Could not allocate poll descriptors and pipe\n");
        return SA_ERROR;
    }

    sa_thread_data *thread_data = (sa_thread_data *) malloc(sizeof(sa_thread_data));
    thread_data->device         = device;
    thread_data->poll_manager   = poll_manager;
    // NULL for default thread attributes
    if(pthread_create(&device->playbackThread, NULL, &init_playback_thread, (void *) thread_data) != 0)
    {
        printf("Couldnt create playback thread\n");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager) {
    sa_poll_management *poll_manager_temp = (sa_poll_management *) malloc(sizeof(sa_poll_management));
    int pipe_fds[2];  // store me somewhere (later)
    int err;
    if(pipe(pipe_fds))
    {
        printf("Cannot create poll_pipe\n");
        return SA_ERROR;
    }
    // TODO maybe remove me
    // Makes read end nonblocking
    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        printf("Failed to make pipe non-blocking\n");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return SA_ERROR;
    }

    poll_manager_temp->count = 1 + snd_pcm_poll_descriptors_count(device->handle);
    // there must be at least one alsa descriptor
    if(poll_manager_temp->count <= 1)
    {
        printf("Invalid poll descriptors count\n");
        return poll_manager_temp->count;
    }

    poll_manager_temp->ufds = malloc(sizeof(struct pollfd) * (poll_manager_temp->count));
    if(poll_manager_temp->ufds == NULL)
    {
        printf("No enough memory\n");
        return SA_ERROR;
    }
    // store read end of pipe
    poll_manager_temp->ufds[0].fd     = pipe_fds[0];
    poll_manager_temp->ufds[0].events = POLLIN;
    // store the write end
    device->pipe_write_end            = pipe_fds[1];
    // dont give ALSA the first poll descriptor
    if((err = snd_pcm_poll_descriptors(device->handle, poll_manager_temp->ufds + 1,
                                       poll_manager_temp->count - 1)) < 0)
    {
        printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
        return SA_ERROR;
    }
    *poll_manager = poll_manager_temp;
    return SA_SUCCESS;
}

void *init_playback_thread(void *data) {
    sa_thread_data *thread_data = (sa_thread_data *) data;
    write_and_poll_loop(thread_data->device, thread_data->poll_manager);
    printf("Playback has ended, can join the thread\n");
    return NULL;
}

sa_result close_playback_thread(sa_device *device) {
    if(pthread_join(device->playbackThread, NULL) != 0)
    {
        printf("Could not join playback thread\n");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager) {
    signed short *ptr;
    int err, cptr, init, readcount;
    init = 1;
    while(1)
    {
        if(!init)
        {
            err = wait_for_poll(device, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    printf("Wait for poll failed\n");
                    return SA_ERROR;
                }
            } else if(err == SA_CANCEL)
            { return SA_CANCEL; }
        }
        // CALL CALLBACK HERE to fill samples !!
        int (*callbackFunction)(int framesToSend, void *audioBuffer, sa_device *sa_device) =
          (int (*)(int, void *, sa_device *)) device->config->callbackFunction;

        readcount = callbackFunction(device->periodSize, device->samples, device);
        if(!readcount)
        { break; }
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
                    return SA_ERROR;
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
            err = wait_for_poll(device, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    printf("Wait for poll failed\n");
                    return SA_ERROR;
                }
            } else if(err == SA_CANCEL)
            { return SA_CANCEL; }
        }
    }
    return SA_SUCCESS;
}

int wait_for_poll(sa_device *device, sa_poll_management *poll_manager) {
    unsigned short revents;
    char command;
    while(1)
    {
        /** A period is the number of frames in between each hardware interrupt. The poll() will return once a period */
        poll(poll_manager->ufds, poll_manager->count, -1);

        if(poll_manager->ufds[0].revents & POLLIN)
        {
            if(read(poll_manager->ufds[0].fd, &command, 1) != 1)
            {
                printf("Pipe read error\n");
            } else
            {
                switch(command)
                {
                /** Stop playack */
                case 's':
                    drain_alsa_device(device);
                    prepare_alsa_device(device);
                    return SA_CANCEL;
                    break;
                /** Pause playback */
                case 'p':
                    if(pause_callback_loop(poll_manager, device) == SA_CANCEL)
                    { return SA_CANCEL; }
                    break;
                default:
                    printf("Invalid command send to pipe\n");
                    break;
                }
            }
        } else
        {
            snd_pcm_poll_descriptors_revents(device->handle, poll_manager->ufds + 1, poll_manager->count - 1,
                                             &revents);
            if(revents & POLLERR)
                return -EIO;
            if(revents & POLLOUT)
                return SA_SUCCESS;
        }
    }
    printf("Poll loop ended without proper return\n");
    return -1;
}

sa_result pause_callback_loop(sa_poll_management *poll_manager, sa_device *device) {
    pause_PCM_handle(device);

    int pauzed = 1;
    char command;
    while(pauzed)
    {
        poll(&(poll_manager->ufds[0]), 1, -1);
        if(read(poll_manager->ufds[0].fd, &command, 1) != 1)
        {
            printf("Pipe read error\n");
        } else
        {
            switch(command)
            {
            /** Stop playback */
            case 's':
                drain_alsa_device(device);
                prepare_alsa_device(device);
                return SA_CANCEL;
                break;
            /** Unpause */
            case 'u':
                unpause_PCM_handle(device);
                return SA_SUCCESS;
                break;
            default:
                break;
            }
        }
    }
    return SA_ERROR;
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
    if(message_pipe(device, 'p') == SA_ERROR)
    {
        printf("Could not pauze playback\n");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result unpause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'u') == SA_ERROR)
    {
        printf("Could not unpauze playback\n");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result stop_alsa_device(sa_device *device) {
    if(message_pipe(device, 's') == SA_ERROR)
    {
        printf("Could not cancel playback\n");
        return SA_ERROR;
    };
    return close_playback_thread(device);
    // TODO CLEANUP
}

sa_result pause_PCM_handle(sa_device *device) {
    printf("Entered the pausePCMHandle function\n");
    if(device->supportsPause)
    {
        if(snd_pcm_pause(device->handle, 1) != 0)
        {
            printf("Failed to snd_pcm_pause() the pcm handle\n");
            return SA_ERROR;
        }
        printf("Device pause with snd_pcm_pause()\n");
        return SA_SUCCESS;
    } else
    {
        printf("Device paused alternatively\n");
        if(drain_alsa_device(device) == SA_SUCCESS && prepare_alsa_device(device) == SA_SUCCESS)
            return SA_SUCCESS;
    }
    return SA_ERROR;
}

sa_result unpause_PCM_handle(sa_device *device) {
    if(device->supportsPause)
    {
        printf("Resuming with snd_pcm_pause()\n");
        if(snd_pcm_pause(device->handle, 0) != 0)
        {
            printf("Failed to resume the paused pcm handle\n");
            return SA_ERROR;
        }
    }
    return SA_SUCCESS;
}

sa_result drain_alsa_device(sa_device *device) {
    if(device->handle && snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING &&
       snd_pcm_drop(device->handle) == 0)
    { return SA_SUCCESS; }
    printf("Failed to drain ALSA device");
    return SA_ERROR;
}

sa_result prepare_alsa_device(sa_device *device) {
    if(device->handle && snd_pcm_prepare(device->handle) == 0)
    { return SA_SUCCESS; }
    printf("Failed to prepare the ALSA device");
    return SA_ERROR;
}

sa_result message_pipe(sa_device *device, char toSend) {
    int result = write(device->pipe_write_end, &toSend, 1);
    if(result != 1)
    {
        printf("Pipe write error\n");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}
