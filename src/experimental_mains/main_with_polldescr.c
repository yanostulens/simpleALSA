#include <alsa/asoundlib.h>
#include <math.h>
#include <sndfile.h>
#include <stdio.h>

static char *device             = "default";             /* playback device */
static snd_pcm_format_t format  = SND_PCM_FORMAT_S16_LE; /* sample format */
static unsigned int rate        = 44100;                 /* stream rate */
static unsigned int channels    = 2;                     /* count of channels */
static unsigned int buffer_time = 500000;                /* ring buffer length in us */
static unsigned int period_time = 250000;                /* period time in us */
static double freq              = 300;                   /* sinusoidal wave frequency in Hz */
static int resample             = 1;                     /* enable alsa-lib resampling */
static int period_event         = 0;                     /* produce poll event after each period */

static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;

char *infilename = "./audioFiles/california.wav";
SF_INFO sfinfo;
SNDFILE *infile = NULL;

static void initSndFile()
{
    infile = sf_open(infilename, SFM_READ, &sfinfo);
    if(!infile)
    {
        fprintf(stderr, "Failed to open wav file");
        exit(1);
    }
    fprintf(stderr, "Channels: %d\n", sfinfo.channels);
    fprintf(stderr, "Sample rate: %d\n", sfinfo.samplerate);
    fprintf(stderr, "Sections: %d\n", sfinfo.sections);
    fprintf(stderr, "Format: %d\n", sfinfo.format);
}

static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access)
{
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, params);
    if(err < 0)
    {
        printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
        return err;
    }
    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
    if(err < 0)
    {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, params, access);
    if(err < 0)
    {
        printf("Access type not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, params, format);
    if(err < 0)
    {
        printf("Sample format not available for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, params, channels);
    if(err < 0)
    {
        printf("Channels count (%u) not available for playbacks: %s\n", channels, snd_strerror(err));
        return err;
    }
    /* set the stream rate */
    rrate = rate;
    err   = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if(err < 0)
    {
        printf("Rate %uHz not available for playback: %s\n", rate, snd_strerror(err));
        return err;
    }
    if(rrate != rate)
    {
        printf("Rate doesn't match (requested %uHz, get %iHz)\n", rate, err);
        return -EINVAL;
    }
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
    if(err < 0)
    {
        printf("Unable to set buffer time %u for playback: %s\n", buffer_time, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if(err < 0)
    {
        printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return err;
    }
    buffer_size = size;
    /* set the period time */
    err         = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
    if(err < 0)
    {
        printf("Unable to set period time %u for playback: %s\n", period_time, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if(err < 0)
    {
        printf("Unable to get period size for playback: %s\n", snd_strerror(err));
        return err;
    }
    period_size = size;
    /* write the parameters to device */
    err         = snd_pcm_hw_params(handle, params);
    if(err < 0)
    {
        printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if(err < 0)
    {
        printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if(err < 0)
    {
        printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
    if(err < 0)
    {
        printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* enable period events when requested */
    if(period_event)
    {
        err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
        if(err < 0)
        {
            printf("Unable to set period event: %s\n", snd_strerror(err));
            return err;
        }
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if(err < 0)
    {
        printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
    if(err == -EPIPE)
    { /* under-run */
        err = snd_pcm_prepare(handle);
        if(err < 0)
            printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
        return 0;
    } else if(err == -ESTRPIPE)
    {
        while((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1); /* wait until the suspend flag is released */
        if(err < 0)
        {
            err = snd_pcm_prepare(handle);
            if(err < 0)
                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
        }
        return 0;
    }
    return err;
}

/*
 *   Transfer method - write and wait for room in buffer using poll
 */

static int wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count)
{
    unsigned short revents;

    while(1)
    {
        /** A period is the number of frames in between each hardware interrupt. The poll() will return once a period */
        poll(ufds, count, -1);

        if(ufds[0].revents & POLLIN)
        {
            int status;
            if((read(ufds[0].fd, &status, 1) == 1) && (status == 1))
            {
                // end has signaled
                printf("Polling has been ended prematurely by pipe\n");
                return -1;
            }
        }

        snd_pcm_poll_descriptors_revents(handle, ufds + 1, count - 1, &revents);
        if(revents & POLLERR)
            return -EIO;
        if(revents & POLLOUT)
            return 0;
    }
    return -1;
}

static int write_and_poll_loop(snd_pcm_t *handle, signed short *samples)
{
    struct pollfd *ufds;
    signed short *ptr;
    int err, count, cptr, init;
    int pipe_fds[2];  // TODO store me somewhere (later)

    if(pipe(pipe_fds))
    {
        printf("Cannot create poll_pipe\n");
        return -1;
    }

    if(fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK))
    {
        printf("Failed to make pipe non-blocking\n");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return 0;
    }

    count = 1 + snd_pcm_poll_descriptors_count(handle);
    if(count <= 1)
    {  // there must be at least one alsa descriptor
        printf("Invalid poll descriptors count\n");
        return count;
    }

    ufds = malloc(sizeof(struct pollfd) * count);
    if(ufds == NULL)
    {
        printf("No enough memory\n");
        return -ENOMEM;
    }
    // store read end of pipe //TODO make pipe to signal end
    ufds[0].fd     = pipe_fds[0];
    ufds[0].events = POLLIN;

    if((err = snd_pcm_poll_descriptors(handle, ufds + 1, count - 1)) < 0)
    {  // dont give ALSA the first poll descriptor
        printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
        return err;
    }

    init = 1;
    int readcount;
    while(1)
    {
        if(!init)
        {
            err = wait_for_poll(handle, ufds, count);
            if(err < 0)
            {
                if(snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(handle, err) < 0)
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

        // generate_sine(areas, 0, period_size, &phase);
        if(!(readcount = sf_readf_short(infile, samples, period_size) > 0))
        {
            break;
        }

        printf("Readcount: %i\n", readcount);
        printf("Periodsize: %ld\n", period_size);

        ptr  = samples;
        cptr = period_size;
        while(cptr > 0)
        {
            err = snd_pcm_writei(handle, ptr, cptr);
            if(err < 0)
            {
                if(xrun_recovery(handle, err) < 0)
                {
                    printf("Write error: %s\n", snd_strerror(err));
                    exit(EXIT_FAILURE);
                }
                init = 1;
                break; /* skip one period */
            }
            if(snd_pcm_state(handle) == SND_PCM_STATE_RUNNING)
                init = 0;
            ptr += err * channels;
            cptr -= err;
            if(cptr == 0)
                break;
            /* it is possible, that the initial buffer cannot store */
            /* all data from the last period, so wait awhile */
            err = wait_for_poll(handle, ufds, count);
            if(err < 0)
            {
                if(snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                   snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED)
                {
                    err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                    if(xrun_recovery(handle, err) < 0)
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
    return 0;
}

struct transfer_method
{
    const char *name;
    snd_pcm_access_t access;
    int (*transfer_loop)(snd_pcm_t *handle, signed short *samples);
};

static struct transfer_method transfer_methods[] = {{"write_and_poll", SND_PCM_ACCESS_RW_INTERLEAVED,
                                                     write_and_poll_loop},
                                                    {NULL, SND_PCM_ACCESS_RW_INTERLEAVED, NULL}};

int startAlsa()
{
    snd_pcm_t *handle;
    int err;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    int method = 0;
    signed short *samples;
    // snd_pcm_channel_area_t *areas;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    printf("Playback device is %s\n", device);
    printf("Stream parameters are %uHz, %s, %u channels\n", rate, snd_pcm_format_name(format), channels);
    printf("Sine wave rate is %.4fHz\n", freq);
    printf("Using transfer method: %s\n", transfer_methods[method].name);

    if((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("Playback open error: %s\n", snd_strerror(err));
        return 0;
    }

    if((err = set_hwparams(handle, hwparams, transfer_methods[method].access)) < 0)
    {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if((err = set_swparams(handle, swparams)) < 0)
    {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    samples = (signed short *) malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
    if(samples == NULL)
    {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }

    /*areas = (snd_pcm_channel_area_t *) calloc(channels, sizeof(snd_pcm_channel_area_t));
    if (areas == NULL) {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }
    for (chn = 0; chn < channels; chn++) {
        areas[chn].addr = samples;
        areas[chn].first = chn * snd_pcm_format_physical_width(format);
        areas[chn].step = channels * snd_pcm_format_physical_width(format);
    }*/

    // Call to the transfermethod
    err = transfer_methods[method].transfer_loop(handle, samples);
    if(err < 0)
        printf("Transfer failed: %s\n", snd_strerror(err));

    // free(areas);
    free(samples);
    snd_pcm_close(handle);
    return 0;
}

int main(int argc, char const *argv[])
{
    printf("Hello world with polls\n");
    initSndFile();
    startAlsa();
    return 0;
}