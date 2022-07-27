#define SA_IMPLEMENTATION

#include <sndfile.h>
#include <stdio.h>

#include "./../simpleALSA.h"

void initSndFile(char *infilename, SF_INFO *sfinfo, SNDFILE **infile) {
    SNDFILE *infile_temp = sf_open(infilename, SFM_READ, sfinfo);
    if(!infile)
    {
        printf("Failed to open wav file");
        exit(1);
    }
    *infile = infile_temp;
}

void eof_callback(sa_device *sa_device, void *my_custom_data) {
    /** restart file (or signal that playback has ended to main thread) */
    SNDFILE *infile = (SNDFILE *) my_custom_data;
    sf_seek(infile, 0, SEEK_SET);
    sa_start_device(sa_device);
}

int data_callback(int frames_to_send, void *audio_buffer, sa_device *sa_device, void *my_custom_data) {
    SNDFILE *infile = (SNDFILE *) my_custom_data;
    int readcount   = sf_readf_int(infile, sa_device->samples, frames_to_send);
    return readcount;
}

int main(int argc, char const *argv[]) {
    if(argc != 2)
    {
        printf("Oops you did not provide enough arguments\n");
        exit(1);
    }

    char *infilename = (char *) argv[1];
    SF_INFO sfinfo;
    SNDFILE *infile = NULL;

    sa_device_config *config = NULL;
    sa_device *device        = NULL;
    sa_init_device_config(&config);

    initSndFile(infilename, &sfinfo, &infile);
    config->data_callback  = &data_callback;
    config->eof_callback   = &eof_callback;
    config->sample_rate    = sfinfo.samplerate;
    config->channels       = sfinfo.channels;
    config->my_custom_data = (void *) infile;
    /** We set the format to 32 bits here because libsndfile reads out frames (see callback_function) as integers in 32 bit words */
    config->format         = SND_PCM_FORMAT_S32_LE;

    sa_init_device(config, &device);

    while(1)
    {
        printf("Give a command please...\n");
        char input[20];
        if(fgets(input, 20, stdin))
        {
            if(strcmp(input, "play\n") == 0)
            {
                sa_start_device(device);
            } else if(strcmp(input, "pause\n") == 0)
            {
                sa_pause_device(device);
            } else if(strcmp(input, "stop\n") == 0)
            {
                sa_stop_device(device);
                sf_seek(infile, 0, SEEK_SET);
            } else if(strcmp(input, "destroy\n") == 0)
            {
                sa_destroy_device(device);
                break;
            } else if(strcmp(input, "rewind\n") == 0)
            {
                sf_seek(infile, 0, SEEK_SET);
            } else if(strcmp(input, "state\n") == 0)
            { printf("State = %i\n", device->state); }
        }
    }
    sf_close(infile);
    return 0;
}
