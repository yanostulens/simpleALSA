#include <alsa/asoundlib.h>
#include <math.h>
#include <sndfile.h>
#include <stdio.h>

#include "ALSAfunctions.h"
#include "simpleALSA.h"

void initSndFile(char *infilename, SF_INFO *sfinfo, SNDFILE *infile) {
    infile = sf_open(infilename, SFM_READ, sfinfo);
    if(!infile)
    {
        fprintf(stderr, "Failed to open wav file");
        exit(1);
    }
    fprintf(stderr, "Channels: %d\n", sfinfo->channels);
    fprintf(stderr, "Sample rate: %d\n", sfinfo->samplerate);
    fprintf(stderr, "Sections: %d\n", sfinfo->sections);
    fprintf(stderr, "Format: %d\n", sfinfo->format);
}

int callback_function(int framesToSend, void *audioBuffer, sa_device *sa_device) {
    int readcount;
    char *infile = (char *) sa_device->myCustomData;
    readcount    = sf_readf_short(infile, sa_device->samples, sa_device->periodSize);
    return readcount;
}

int main(int argc, char const *argv[]) {
    char *infilename = "./audioFiles/california.wav";
    SF_INFO sfinfo;
    SNDFILE *infile = NULL;
    sa_device_config *config;
    sa_device *device;
    sa_init_device_config(config);
    config->callbackFunction = callback_function;
    sa_init_device(config, device);
    device->myCustomData = (void *) infile;
    initSndFile(infilename, &sfinfo, infile);
    sa_start_device(device);
    return 0;
}