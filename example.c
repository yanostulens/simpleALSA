/** This example is used to demonstrate the use of the simpleALSA lib.
 *  The example makes use of libsndfile in order to open and read from .wav
 * files.
 */

#include <sndfile.h>
#include <stdio.h>

#include "simpleALSA.h"

/** Disable any debug logs from simpleALSA */
#define SA_NO_DEBUG_LOGS

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
 * the signature used below.
 *
 * As soon as an sa_device is started it will try to
 * send audio to the soundcard. Whenever the audio back-end (ALSA) needs more
 * data it will fire this callback function. The user of the callback must then
 * write a number of audioframes (=framesToSend) to the audioBuffer.
 *
 * The most important thing about the callback
 * function is to return the amount of frames that have send to the audioBuffer!
 * Because when 0 is returned the sa_device will stop the callback loop, it is
 * then up to the user to restart the loop again.
 *
 * Furthermore the user can use the myCustomData pointer (which is set in the
 * config struct) in order to use custom data from within the callback loop.
 *
 * NOTE: do not call any API functions (such as sa_stop_device(),
 * sa_pause_device()...) from within the callback function as this will result
 * in undefined behaviour!
 */

int callback_function(int framesToSend, void *audioBuffer, sa_device *sa_device,
                      void *myCustomData) {
  SNDFILE *infile = (SNDFILE *)myCustomData;
  return sf_readf_int(infile, audioBuffer, framesToSend);
}

int main(int argc, char const *argv[]) {
  /** Init sndfile */
  char *infilename = "./audioFiles/mix.wav";
  SF_INFO sfinfo;
  SNDFILE *infile = NULL;
  initSndFile(infilename, &sfinfo, &infile);

  /** Declare a variable for an sa_device_config struct and an sa_device */
  sa_device_config *config = NULL;
  sa_device *device = NULL;
  /** Initialize the sa_device_config struct */
  sa_init_device_config(&config);

  /** Set the required configuration - note here that libsndfile will provide us
   * with the required infomartion regarding sampling rate, channelcount etc. */
  config->callbackFunction = &callback_function;
  config->sampleRate = sfinfo.samplerate;
  config->channels = sfinfo.channels;
  /** We set the format to signed 32 bit here because libsndfile reads out frames as 32 bit intergers (see callback_function) */
  config->format = SND_PCM_FORMAT_S32_LE;

  /** After the configuration we can initialize the device */
  sa_init_device(config, &device);

  /** Assign custom data to the device */
  device->myCustomData = (void *)infile;

  /** For demonstation purposes we start an endless loop that listens from
   * command from stdin */
  while (1) {
    printf("Give a command please...\n");
    char input[20];
    if (fgets(input, 20, stdin)) {
      if (strcmp(input, "play\n") == 0) {
        /** A play command will start the sa_device */
        sa_start_device(device);
      } else if (strcmp(input, "pause\n") == 0) {
        /** Pause */
        sa_pause_device(device);
      } else if (strcmp(input, "stop\n") == 0) {
        /** Stop */
        sa_stop_device(device);
      } else if (strcmp(input, "rewind\n") == 0) {
        /** Reset the file reader from sndfile to read from the start again */
        sf_seek(infile, 0, SEEK_SET);
      } else if (strcmp(input, "destroy\n") == 0) {
        /** Destory the sa_device - this will also clear all resources */
        sa_destroy_device(device);
        break;
      }
    }
  }
  sf_close(infile);
  return 0;
}