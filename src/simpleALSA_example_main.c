#include <sndfile.h>
#include <stdio.h>

#include "./ALSAfunctions/ALSAfunctions.h"
#include "./logger/logger.h"
#include "./simpleALSA_API/simpleALSA.h"

void initSndFile(char *infilename, SF_INFO *sfinfo, SNDFILE **infile) {
    SNDFILE *infile_temp = sf_open(infilename, SFM_READ, sfinfo);
    if(!infile)
    {
        printf("Failed to open wav file");
        exit(1);
    }
    *infile = infile_temp;
}

int callback_function(int framesToSend, void *audioBuffer, sa_device *sa_device, void *myCustomData) {
    SNDFILE *infile = (SNDFILE *) myCustomData;

    return sf_readf_short(infile, sa_device->samples, framesToSend);
}

int main(int argc, char const *argv[]) {
    char *infilename = "./audioFiles/mix.wav";
    SF_INFO sfinfo;
    SNDFILE *infile = NULL;

    sa_device_config *config = NULL;
    sa_device *device        = NULL;
    sa_init_device_config(&config);

    initSndFile(infilename, &sfinfo, &infile);
    config->callbackFunction = &callback_function;
    config->sampleRate       = sfinfo.samplerate;
    config->channels         = sfinfo.channels;

    sa_init_device(config, &device);
    device->myCustomData = (void *) infile;
    while(1)
    {
        printf("Give a command please...\n");
        char input[20];
        fgets(input, 20, stdin);

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
        } else if(strcmp(input, "prep\n") == 0)
        {
            prepare_alsa_device(device);
        } else if(strcmp(input, "destroy\n") == 0)
        {
            sa_destroy_device(device);
            break;
        } else if(strcmp(input, "rewind\n") == 0)
        { sf_seek(infile, 0, SEEK_SET); }
    }
    sf_close(infile);
    return 0;
}
