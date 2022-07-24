#include <pthread.h>

#include "./ALSAfunctions.h"

#include "../logger/logger.h"

sa_result init_alsa_device(sa_device *device) {
    int err;
    snd_pcm_hw_params_alloca(&(device->hwparams));
    snd_pcm_sw_params_alloca(&(device->swparams));

    if((err = snd_pcm_open(&(device->handle), device->config->device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        SA_LOG(ERROR, "ALSA: playback open error:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if((err = set_hardware_parameters(device, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        SA_LOG(ERROR, "ALSA: setting hardware parameters failed:", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_software_parameters(device)) < 0)
    {
        SA_LOG(ERROR, "ALSA: setting software parameters failed:", snd_strerror(err));
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
        SA_LOG(DEBUG, "Device supports snd_pcm_pause()");
    } else
    { SA_LOG(DEBUG, "Device does not support snd_pcm_pause()"); }

    if(prepare_playback_thread(device) != SA_SUCCESS)
    {
        SA_LOG(ERROR, "Failed to perpare the playback thread");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

sa_result set_hardware_parameters(sa_device *device, snd_pcm_access_t access) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    err = snd_pcm_hw_params_any(device->handle, device->hwparams);
    if(err < 0)
    {
        SA_LOG(ERROR,
               "ALSA: broken configuration for playback: no configurations available:", snd_strerror(err));
        return SA_ERROR;
    }
    /** Set the samplerate */
    err = snd_pcm_hw_params_set_rate_resample(device->handle, device->hwparams, 1);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: resampling setup failed for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(device->handle, device->hwparams, access);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: access type not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(device->handle, device->hwparams, device->config->format);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: sample format not available for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(device->handle, device->hwparams, device->config->channels);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: channels count not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Set the stream rate */
    rrate = device->config->sampleRate;
    err   = snd_pcm_hw_params_set_rate_near(device->handle, device->hwparams, &rrate, 0);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: samplerate not available for playback", snd_strerror(err));
        return SA_ERROR;
    }
    if(rrate != device->config->sampleRate)
    {
        SA_LOG(ERROR, "ALSA: sample rate does not match the requested rate");
        return SA_ERROR;
    }
    /* Set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(device->handle, device->hwparams,
                                                 (unsigned int *) &(device->config->bufferTime), &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set the buffer time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_buffer_size(device->hwparams, &size);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to get the buffer size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->bufferSize = size;
    /* Set the period time */
    err                = snd_pcm_hw_params_set_period_time_near(device->handle, device->hwparams,
                                                 (unsigned int *) &(device->config->periodTime), &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set the period time for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    err = snd_pcm_hw_params_get_period_size(device->hwparams, &size, &dir);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to get period size for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    device->periodSize = size;
    /* Write the parameters to device */
    err                = snd_pcm_hw_params(device->handle, device->hwparams);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set hardware parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result set_software_parameters(sa_device *device) {
    int err;

    /* Get the current swparams */
    err = snd_pcm_sw_params_current(device->handle, device->swparams);
    if(err < 0)
    {
        SA_LOG(ERROR,
               "ALSA: unable to determine current software parameters for playback:", snd_strerror(err));
        return SA_ERROR;
    }
    /* Start the transfer when the buffer is almost full: (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(
      device->handle, device->swparams, (device->bufferSize / device->periodSize) * device->periodSize);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to start set threshold mode for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    err = snd_pcm_sw_params_set_avail_min(device->handle, device->swparams,
                                          0 ? device->bufferSize : device->periodSize);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set available minimum for playback", snd_strerror(err));
        return SA_ERROR;
    }
    /* Enable period events when requested */
    if(0)
    {
        err = snd_pcm_sw_params_set_period_event(device->handle, device->swparams, 1);
        if(err < 0)
        {
            SA_LOG(ERROR, "ALSA: unable to set period event", snd_strerror(err));
            return SA_ERROR;
        }
    }
    /* Write the parameters to the playback device */
    err = snd_pcm_sw_params(device->handle, device->swparams);
    if(err < 0)
    {
        SA_LOG(ERROR, "ALSA: unable to set software parameters for playback", snd_strerror(err));
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result prepare_playback_thread(sa_device *device) {
    /** Prepare communication pipe */
    int pipe_fds[2];
    if(pipe(pipe_fds))
    {
        SA_LOG(ERROR, "Cannot create poll_pipe");
        return SA_ERROR;
    }
    /** Makes read end nonblocking */
    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        SA_LOG(ERROR, "Failed to make pipe non-blocking");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return SA_ERROR;
    }
    /** Store the write end */
    device->pipe_write_end          = pipe_fds[1];
    /** Prepare read polling structure of the read end */
    struct pollfd *pipe_read_end_fd = malloc(sizeof(struct pollfd));
    pipe_read_end_fd->fd            = pipe_fds[0];
    pipe_read_end_fd->events        = POLLIN;
    /** Startup the playback thread */
    sa_thread_data *thread_data     = malloc(sizeof(sa_thread_data));
    thread_data->device             = device;
    thread_data->pipe_read_end_fd   = pipe_read_end_fd;
    if(pthread_create(&device->playbackThread, NULL, &init_playback_thread, (void *) thread_data) != 0)
    {
        SA_LOG(ERROR, "Failed to create the playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result start_alsa_device(sa_device *device) {
    return unpause_alsa_device(device);
}

void *init_playback_thread(void *data) {
    char command;
    /** Unwrap the data packet */
    sa_thread_data *thread_data     = (sa_thread_data *) data;
    sa_device *device               = thread_data->device;
    struct pollfd *pipe_read_end_fd = thread_data->pipe_read_end_fd;
    free(thread_data);

    /** Actual playback loop, lives and dies with the device */
    while(1)
    {
        /** Poll on read end, wait for initial play command */
        poll(pipe_read_end_fd, 1, -1);

        if(pipe_read_end_fd->revents & POLLIN)
        {
            if(read(pipe_read_end_fd->fd, &command, 1) != 1)
            {
                SA_LOG(ERROR, "Pipe read error");
            } else
            {
                switch(command)
                {
                /** Play command */
                case 'u':
                    if(start_write_and_poll_loop(device, pipe_read_end_fd) == SA_ERROR)
                    { break; }
                    /** Received no frames anymore from the callback so we stop and prepare the alsa device again */
                    //  drain_alsa_device(device);
                    prepare_alsa_device(device);
                    continue;
                /** Destroy command, no continue; break out of while */
                case 'd':
                    break;
                default:
                    SA_LOG(WARNING, "Invalid command send to the pipe");
                    continue;
                }
                SA_LOG(DEBUG, "Attempting to destroy the device");
                break;
            }
        }
    }
    close(pipe_read_end_fd->fd);
    free(pipe_read_end_fd);
    return NULL;
}

sa_result start_write_and_poll_loop(sa_device *device, struct pollfd *pipe_read_end_fd) {
    sa_poll_management *poll_manager = NULL;
    /** Init the poll manager */
    if(init_poll_management(device, &poll_manager, pipe_read_end_fd) != SA_SUCCESS)
    {
        SA_LOG(ERROR, "Could not initialize the poll manager");
        return SA_ERROR;
    }
    write_and_poll_loop(device, poll_manager);
    /** Cleanup */
    free(poll_manager->ufds);
    free(poll_manager);
    poll_manager = NULL;
    return SA_SUCCESS;
}

sa_result init_poll_management(sa_device *device, sa_poll_management **poll_manager,
                               struct pollfd *pipe_read_end_fd) {
    sa_poll_management *poll_manager_temp = (sa_poll_management *) malloc(sizeof(sa_poll_management));
    int err;

    poll_manager_temp->count = 1 + snd_pcm_poll_descriptors_count(device->handle);
    /** There must be at least one alsa descriptor */
    if(poll_manager_temp->count <= 1)
    {
        SA_LOG(ERROR, "Invalid poll descriptor count");
        return SA_ERROR;
    }

    poll_manager_temp->ufds = malloc(sizeof(struct pollfd) * (poll_manager_temp->count));
    if(poll_manager_temp->ufds == NULL)
    {
        SA_LOG(ERROR, "Not enough memory to allocate ufds");
        return SA_ERROR;
    }
    /** Store read end of pipe in the array */
    poll_manager_temp->ufds[0] = *pipe_read_end_fd;

    /** Don't give ALSA the first poll descriptor */
    if((err = snd_pcm_poll_descriptors(device->handle, poll_manager_temp->ufds + 1,
                                       poll_manager_temp->count - 1)) < 0)
    {
        SA_LOG(ERROR, "Unable to obtain poll descriptors for playback", snd_strerror(err));
        return SA_ERROR;
    }
    *poll_manager = poll_manager_temp;
    return SA_SUCCESS;
}

sa_result close_playback_thread(sa_device *device) {
    if(pthread_join(device->playbackThread, NULL) != 0)
    {
        SA_LOG(ERROR, "Could not join playback thread");
        return SA_ERROR;
    }
    return SA_SUCCESS;
}

sa_result write_and_poll_loop(sa_device *device, sa_poll_management *poll_manager) {
    signed short *ptr;
    int err, cptr, init, readcount;
    readcount = 1;
    init      = 1;
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
                        SA_LOG(ERROR, "Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(ERROR, "Wait for poll failed");
                    return SA_ERROR;
                }
            } else if(err == SA_CANCEL)
            { return SA_CANCEL; }
        }
        /** If the callback has not written any frames in the previous call- there are no frames left so we stop the callback loop */
        if(!readcount)
        { break; }
        int (*callbackFunction)(int framesToSend, void *audioBuffer, sa_device *sa_device,
                                void *myCustomData) =
          (int (*)(int, void *, sa_device *, void *myCustomData)) device->config->callbackFunction;
        readcount = callbackFunction(device->periodSize, device->samples, device, device->myCustomData);

        ptr  = device->samples;
        cptr = device->periodSize;
        while(cptr > 0)
        {
            err = snd_pcm_writei(device->handle, ptr, cptr);
            if(err < 0)
            {
                if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                {
                    SA_LOG(ERROR, "Write error:", snd_strerror(err));
                    return SA_ERROR;
                }
                init = 1;
                break;
            }
            if(snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING)
                init = 0;
            ptr += err * device->config->channels;
            cptr -= err;
            if(cptr == 0)
                break;
            /* It is possible, that the initial buffer cannot store all data from the last period, so wait a while */
            err = wait_for_poll(device, poll_manager);
            if(err < 0)
            {
                if(snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(device->handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(device->handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(device->handle, err) != SA_SUCCESS)
                    {
                        SA_LOG(ERROR, "Write error:", snd_strerror(err));
                        return SA_ERROR;
                    }
                    init = 1;
                } else
                {
                    SA_LOG(ERROR, "Wait for poll failed");
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
                SA_LOG(ERROR, "Pipe read error");
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
                    SA_LOG(WARNING, "Invalid command send to pipe");
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
    SA_LOG(ERROR, "Poll loop ended without proper return");
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
            SA_LOG(ERROR, "Pipe read error");
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
    { /* Underrun */
        err = snd_pcm_prepare(handle);
        if(err < 0)
        {
            SA_LOG(ERROR, "Can't recover from underrun, prepare failed:", snd_strerror(err));
            return SA_ERROR;
        }
    } else if(err == -ESTRPIPE)
    {
        while((err = snd_pcm_resume(handle)) == -EAGAIN)
        {
            /* Wait until the suspend flag is released */
            sleep(1);
        }
        if(err < 0)
        {
            err = snd_pcm_prepare(handle);
            if(err < 0)
            {
                SA_LOG(ERROR, "Can't recover from suspend, prepare failed:", snd_strerror(err));
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
        SA_LOG(ERROR, "Could not send pause command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result unpause_alsa_device(sa_device *device) {
    if(message_pipe(device, 'u') == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not send unpause command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result stop_alsa_device(sa_device *device) {
    if(message_pipe(device, 's') == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not send cancel command to the message pipe");
        return SA_ERROR;
    };
    return SA_SUCCESS;
}

sa_result pause_PCM_handle(sa_device *device) {
    if(device->supportsPause)
    {
        if(snd_pcm_pause(device->handle, 1) != 0)
        {
            SA_LOG(ERROR, "Failed to snd_pcm_pause the pcm handle (when pausing)");
            return SA_ERROR;
        }
        return SA_SUCCESS;
    } else
    {
        if(drain_alsa_device(device) == SA_SUCCESS && prepare_alsa_device(device) == SA_SUCCESS)
            return SA_SUCCESS;
    }
    return SA_ERROR;
}

sa_result unpause_PCM_handle(sa_device *device) {
    if(device->supportsPause)
    {
        if(snd_pcm_pause(device->handle, 0) != 0)
        {
            SA_LOG(ERROR, "Failed to snd_pcm_pause the pcm handle (when resuming)");
            return SA_ERROR;
        }
    }
    return SA_SUCCESS;
}

sa_result drain_alsa_device(sa_device *device) {
    if(device->handle &&
       (snd_pcm_state(device->handle) == SND_PCM_STATE_RUNNING ||
        snd_pcm_state(device->handle) == SND_PCM_STATE_PAUSED) &&
       snd_pcm_drop(device->handle) == 0)
    { return SA_SUCCESS; }
    SA_LOG(ERROR, "Failed to drop samples from the ALSA device");
    exit(EXIT_FAILURE);
}

sa_result prepare_alsa_device(sa_device *device) {
    if(device->handle && snd_pcm_prepare(device->handle) == 0)
    { return SA_SUCCESS; }
    SA_LOG(ERROR, "Failed to prepare the ALSA device");
    exit(EXIT_FAILURE);
}

sa_result message_pipe(sa_device *device, char toSend) {
    int result = write(device->pipe_write_end, &toSend, 1);
    if(result != 1)
    {
        SA_LOG(ERROR, "Failed to write to the pipe");
        exit(EXIT_FAILURE);
    }
    return SA_SUCCESS;
}

sa_result cleanup_device(sa_device *device) {
    if(device)
    {
        close(device->pipe_write_end);

        if(device->config)
        { free(device->config); }

        if(device->samples)
        { free(device->samples); }
        if(device->handle)
        {
            int err = snd_pcm_close(device->handle);
            if(err < 0)
            { SA_LOG(ERROR, "Could not close handle : ", snd_strerror(err)); }
        }
        free(device);
        snd_config_update_free_global();
    }
    return SA_SUCCESS;
}

sa_result destroy_alsa_device(sa_device *device) {
    stop_alsa_device(device);
    message_pipe(device, 'd');
    if(close_playback_thread(device) == SA_ERROR)
    {
        SA_LOG(ERROR, "Could not close thread");
        exit(EXIT_FAILURE);
    }
    return cleanup_device(device);
}
