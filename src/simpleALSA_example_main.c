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

void eof_callback(sa_device *sa_device, void *myCustomData) {
    /** restart file (or signal that playback has ended to main thread) */
    SNDFILE *infile = (SNDFILE *) myCustomData;
    sf_seek(infile, 0, SEEK_SET);
    sa_start_device(sa_device);
}

int callback_function(int framesToSend, void *audioBuffer, sa_device *sa_device, void *myCustomData) {
    SNDFILE *infile = (SNDFILE *) myCustomData;
    int readcount   = sf_readf_short(infile, sa_device->samples, framesToSend);
    printf("%d\n", readcount);
    return readcount;
}

int main(int argc, char const *argv[]) {
    char *infilename = "./audioFiles/afraid.wav";
    SF_INFO sfinfo;
    SNDFILE *infile = NULL;

    sa_device_config *config = NULL;
    sa_device *device        = NULL;
    sa_init_device_config(&config);

    initSndFile(infilename, &sfinfo, &infile);
    config->callbackFunction = &callback_function;
    config->eofCallback      = &eof_callback;
    config->sampleRate       = sfinfo.samplerate;
    config->channels         = sfinfo.channels;
    /** We set the format to 32 bits here because libsndfile reads out frames (see callback_function) as integers in 32 bit words */
    config->format           = SND_PCM_FORMAT_S32_LE;

    sa_init_device(config, &device);
    device->myCustomData = (void *) infile;
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
    }
    sf_close(infile);
    return 0;
}
