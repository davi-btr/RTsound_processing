/*
 * rawnotes.c
 *
 * Author:	Davide Bettarini
 * Date:	02/07/2022
 * License:	GPL
 *
*/


#include <stdlib.h>
#include <unistd.h>
#include <fluidsynth.h>

//#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

static void usage(void)
{
  fprintf(stderr, "usage: rawnotes [options]\n");
  fprintf(stderr, "  options:\n");
  fprintf(stderr, "    -h		: display this help window\n");
  fprintf(stderr, "    -S soundfont	: path to soundfont\n");
  fprintf(stderr, "    -f file-format	: output file rendering format\n");
  fprintf(stderr, "    -t sample-format	: audio samples type\n");
  fprintf(stderr, "    -b size		: buffer period size\n");
  fprintf(stderr, "    -r rate		: stream samplerate\n");
  fprintf(stderr, "    -g gain		: audio synthesizer gain\n");
  fprintf(stderr, "    -N data		: data is composed of: [total number of MIDI events K] [MIDI event #1] [delay #1] [MIDI event #2] ... [MIDI event 2*K + 1]\nEvery argument 'MIDI event' can match a 'note on MIDI event' or a 'note off MIDI event'\nA single 'MIDI event' is composed of [MIDI-channel MIDI-key MIDI-velocity]\nMIDI-velocity=0 matches a 'note off MIDI event, otherwise it represents a 'note on MIDI event''\nDelays are only measured in seconds. A null value implies no delay between contiguous events\n");
  fprintf(stderr, "  example:\n");
  fprintf(stderr, "    rawdata -S /usr/share/sounds/sf2/FluidR3_GM.sf2 -r 48000.0 -b 1024 -f float -N 2 0 64 127 0 75 127\n");
  fprintf(stderr, "    \n");

}


int main(int argc, char **argv)
{
  char *sfname = NULL, fname[18] = "noteXXX-YYY-N.dat", *sformat = "16bits", *fformat = "s16";
  int bsize = 64, count = 0;
  float rate = 44100.0, gain = 5.0;
  int *chans, *keys, *vels, *delays;	//to store info required to play MIDI notes

  if (argc <= 1) {
    usage();
    exit(0);
  }
 
  for (int i = 1 ; i < argc - 1; i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
        case 'h':
          usage();
          break;
        case 'S':
          sfname = argv[++i];
          break;
        case 'N':
          count = atoi(argv[++i]);
	  dbg_printf("count %d\n", count);
	  chans = malloc(count * sizeof(int));
	  keys = malloc(count * sizeof(int));
	  vels = malloc(count * sizeof(int));
	  delays = malloc(count * sizeof(int));
	  if (!chans || !keys || !vels || !delays) {
            fprintf(stderr, "memory allocation problems\n");

            exit(1);
	  }
	  for (int j = 0; j < count; j++) {
            chans[j] = atoi(argv[++i]);
            keys[j] = atoi(argv[++i]);
            vels[j] = atoi(argv[++i]);
	    dbg_printf("chann %d key %d vel %d\n", chans[j], keys[j], vels[j]);
	    if (j == count - 1)
		    delays[j] = 0;
	    else
		    delays[j] = atoi(argv[++i]);
	    dbg_printf("pause %d\n", delays[j]);
	  }
          break;
        case 'f':
          fformat = argv[++i];
          break;
        case 't':
          sformat = argv[++i];
          break;
        case 'b':
          bsize = atoi(argv[++i]);
          break;
        case 'r':
          rate = atof(argv[++i]);
          break;
        case 'g':
          gain = atof(argv[++i]);
          break;
        default:
          fprintf(stderr, "invalid option\n");
          usage();
      }           
    }
  }

  fluid_settings_t *settings;
  fluid_synth_t *synth;
  fluid_audio_driver_t *adriver;
  int sfont_id;

  //Handle output file name: note-KEY-VEL-N.dat
  fname[4] = (char)(keys[0] / 100 + 0x30);
  fname[5] = (char)((keys[0]%100) / 10 + 0x30);
  fname[6] = (char)(keys[0] % 10 + 0x30);
  fname[8] = (char)(vels[0] / 100 + 0x30);
  fname[9] = (char)((vels[0]%100) / 10 + 0x30);
  fname[10] = (char)(vels[0] % 10 + 0x30);
  fname[12] = (char)(count % 10 + 0x30);

  //Settings
  settings = new_fluid_settings();
  fluid_settings_setstr(settings, "audio.driver", "file");
  fluid_settings_setstr(settings, "audio.file.format", fformat);
  fluid_settings_setstr(settings, "audio.file.type", "raw");
  fluid_settings_setstr(settings, "audio.file.name", fname);
  fluid_settings_setint(settings, "audio.period-size", bsize);
  fluid_settings_setstr(settings, "audio.sample-format", sformat);
  fluid_settings_setnum(settings, "synth.gain", gain);
  fluid_settings_setnum(settings, "synth.sample-rate", rate);
  dbg_printf("settings done\n");

  synth = new_fluid_synth(settings);

  adriver = new_fluid_audio_driver(settings, synth);

  sfont_id = fluid_synth_sfload(synth, sfname, 1);
  if(sfont_id == FLUID_FAILED)
  {
      fprintf(stderr, "Loading the SoundFont failed!\n");

      goto err;
  }

  dbg_printf("start\n");
  for (int i = 0; i < count; i++) {
    //send MIDI event to synthesiser
    if (vels[i] == 0)
      fluid_synth_noteoff(synth, chans[i], keys[i]);
    else {
      fluid_synth_noteon(synth, chans[i], keys[i], vels[i]);
      dbg_printf("nota %d\n", keys[i]);
    }
    if (delays[i] > 0)
      sleep(delays[i]);
  }
err:
 
  delete_fluid_audio_driver(adriver);
  delete_fluid_synth(synth);
  delete_fluid_settings(settings);

  return 0;
}

