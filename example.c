/** This example is used to demonstrate the use of the simpleALSA lib.
 *  The example makes use of libsndfile in order to open and read from .wav
 * files.
 */

#include <sndfile.h>
#include <stdio.h>

#include "simpleALSA.h"

/** This function is simply here to initialize libsndfile - it has nothing to do
 * with simpleALSA */
void initSndFile(char *infilename, SF_INFO *sfinfo, SNDFILE **infile) {
  SNDFILE *infile_temp = sf_open(infilename, SFM_READ, sfinfo);
  if (!infile) {
    printf("Failed to open wav file");
    exit(1);
  }
  *infile = infile_temp;
}

/** Here we define the callback function. The callback function must comply to
 * the signature used below As soon as an sa_device is started it will try to
 * send audio to the soundcard. Whenever the audio back-end (ALSA) needs more
 * data it will fire this callback function.
 *
 */

int callback_function(int framesToSend, void *audioBuffer,
                      sa_device *sa_device) {
  SNDFILE *infile = (SNDFILE *)sa_device->myCustomData;
  return (sf_readf_short(infile, sa_device->samples, sa_device->periodSize) >
          0);
}

int main(int argc, char const *argv[]) {
  char *infilename = "./audioFiles/california.wav";
  // infilename       =
  // "/home/daan/Thesis/alsa/simpleALSA/audioFiles/california.wav";
  SF_INFO sfinfo;
  SNDFILE *infile = NULL;

  sa_device_config *config = NULL;
  sa_device *device = NULL;
  sa_init_device_config(&config);

  initSndFile(infilename, &sfinfo, &infile);
  config->callbackFunction = &callback_function;
  config->sampleRate = sfinfo.samplerate;
  config->channels = sfinfo.channels;

  sa_init_device(config, &device);
  device->myCustomData = (void *)infile;
  while (1) {
    SA_LOG(DEBUG, "Give a command please...");
    char input[20];
    fgets(input, 20, stdin);

    if (strcmp(input, "play\n") == 0) {
      sa_start_device(device);
    } else if (strcmp(input, "pause\n") == 0) {
      sa_pause_device(device);
    } else if (strcmp(input, "stop\n") == 0) {
      sa_stop_device(device);
      sf_seek(infile, 0, SEEK_SET);
    } else if (strcmp(input, "destroy\n") == 0) {
      sa_destroy_device(device);
      break;
    }
  }
  sf_close(infile);
  return 0;
}