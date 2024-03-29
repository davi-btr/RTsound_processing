/*
  * test2.c
  *
  * sndprocess.c test code using file .dat input instead of microphone
  * 2nd version
  * 
  * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "audiostream.h"
#include "sndutils.h"
#include "fourier.h"

#define NOISE_SQRMS	0.02
#define CONFIDENCE	0.9
#define GAIN		1.5

#define IF_ERR(errcode, msg, extra) if(errcode) {printf msg; extra;} 
#define IF_ERR_EXIT(errcode, msg) if(errcode) {fprintf msg; exit(1);} 

#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

int stop = 0;

void sighandler(int dum)
{
	stop=1;
}

static void usage(void)
{
	fprintf(stderr, "usage: sndprocess [options]\n");
	fprintf(stderr, "  options:\n");
	fprintf(stderr, "    -h		: display this help window\n");
	fprintf(stderr, "    -i device-id	: ALSA input PCM device\n");
	fprintf(stderr, "    -o port-id	: ALSA output MIDI port\n");
	fprintf(stderr, "    -O filename	: raw data transcript file\n");
	fprintf(stderr, "    -r samplerate	: samples acquired per second\n");
	fprintf(stderr, "    -c channels	: number of channels\n");
	fprintf(stderr, "    -f frames	: number of samples processed simultaneously\n");
	fprintf(stderr, "  example:\n");
	fprintf(stderr, "    sndprocess -r 48000 -c 1 -f 1024 -O recorded.dat\n");
	fprintf(stderr, "    \n");

}

int main (int argc, char *argv[])
{
  int err, fin, fd;
  unsigned int samplerate = 48000, frames = 1024, channels = 2;
  const char *fname = NULL;
  char *audio_in = NULL, *midi_out = NULL, *port_name = NULL;
  float *x, dft_on_note[MAX_KEY - MIN_KEY + 1], *buf = NULL;
  float rms, sqrms, harm_pwr_old, harm_pwr, sqrms_old = 0;
  note_info_t current = {FALSE, 0, 0};
  bool_t note_search = FALSE;

  if (argc==1) {
    usage();
    exit(0);
  }
    
  for (int i = 1 ; i<argc ; i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
        case 'h':
          usage();
          break;
        case 'i':
          audio_in = argv[++i];
	  dbg_printf("audio file in %s\n", audio_in);
          break;
        case 'o':
          midi_out = argv[++i];
	  dbg_printf("midi out %s\n", midi_out);
          break;
        case 'p':
          port_name = argv[++i];
	  dbg_printf("port %s\n", port_name);
          break;
        case 'O':
          fname = argv[++i];
          break;
        case 'r':
          samplerate = atoi(argv[++i]);
	  dbg_printf("samplerate %d\n", samplerate);
          break;
        case 'c':
          channels = atoi(argv[++i]);
	  dbg_printf("channels %d\n", channels);
          break;
        case 'f':
          frames = atoi(argv[++i]);
	  dbg_printf("frames %d\n", frames);
          break;
        default:
          fprintf(stderr, "invalid option\n");
          usage();
      }           
    }
  }
  
  IF_ERR(((fin = open(audio_in, O_RDONLY)) < 0), ("Unable to read\n"), ;)
  IF_ERR(((fd = open(fname, O_CREAT | O_WRONLY, 0666)) < 0), ("Unable to record\n"), ;)
  //dbg_printf("Recording to file: %s\n", fname);

  pcm_stream_t sett = {NULL, "default", buf, frames, samplerate, channels, CAPTURE, TRUE};
  
  midi_stream_t seq = {NULL, midi_out, NULL, port_name, 0, OUTPUT, 0};
  
  IF_ERR_EXIT(((err = pcm_init(&sett)) < 0), (stderr, "Unable to capture\n"))
  IF_ERR_EXIT(((err = midi_init(&seq)) < 0), (stderr, "Unable to open MIDI sequencer\n"))
  //dbg_printf("portID %d port name %s output %s\n", seq.portid, seq.portname, midi_out);

  IF_ERR_EXIT(((buf = malloc(sizeof(float) * frames * channels)) == 0), (stderr, "failed memory allocation\n"))
  IF_ERR_EXIT(((x = malloc(sizeof(float) * frames)) == 0), (stderr, "failed memory allocation\n"))

  for (int j = 0; !stop; j++) {
    unsigned long to_read = frames * channels * sizeof(float);
    unsigned char *ptr = (unsigned char *) buf;
    do {
      unsigned long just_read = read(fin, ptr, to_read);
      assert (just_read >= 0);
      if (just_read == 0) {
        // EOF
        goto end;
      }
      to_read -= just_read;
      ptr += just_read;
    } while (to_read > 0);

    //RMS
    sqrms = 0;

    for (int i = 0; i < frames; i++) {
      x[i] = buf[i*channels];
      sqrms += x[i] * x[i];
    }
    sqrms /= frames;
    rms = sqrt(sqrms);
    dbg_printf("sqrms %.4f\n", sqrms);

    if (sqrms <= NOISE_SQRMS) {
      //no detection
      if (note_search) {
        printf("possibly missed a note\n");
	note_search = FALSE;
      }
      //silence previous note, if any
      current.on = FALSE;
      midi_noteoff(&seq, 0, current.key);
      current.key = -1;
      current.vel = 0;
      sqrms_old = NOISE_SQRMS;

      continue;			//wait for next frame
    }

    //RMS normalization
    for (int i = 0; i < frames; i++) {
      x[i] /= rms;
    }

    if (current.on) {
      harm_pwr_old = harm_pwr_calc(MIDI_freq[current.key], x, frames, samplerate);
    } else {
      harm_pwr_old = 0.0;
    }
    //dbg_printf("harm pwr old %.4f\n", harm_pwr_old);

    if (harm_pwr_old > CONFIDENCE * harmonic_power_start[current.key] && sqrms < GAIN * sqrms_old) {
      //same note as before
      dbg_printf("same note\n");
      sqrms_old = sqrms;
      continue;
    }
    sqrms_old = sqrms;

    //note pressed
    midi_noteoff(&seq, 0, current.key);
    current.on = FALSE;
    //current.key is kept
    current.vel = 0;
    note_search = TRUE;

    for (int i = MIN_KEY; i <= MAX_KEY; i++) {
      dft_on_note[i-MIN_KEY] = dft_sqr_arg(MIDI_freq[i], x, frames, samplerate);
    }

    //pitch detection, single note only
    int index;
    float peak;

    index = get_index(dft_on_note, MAX_KEY - MIN_KEY + 1, &peak);
    if (current.key == index + MIN_KEY) {
      harm_pwr = harm_pwr_old;
    } else {
      harm_pwr = harm_pwr_calc(MIDI_freq[index + MIN_KEY], x, frames, samplerate) / (1 - harm_pwr_old);
    }
    dbg_printf("miglior risultato: key %d fr %.4f val %.4f harm_pwr %.4f\n", index + MIN_KEY, MIDI_freq[index + MIN_KEY], peak, harm_pwr);

    if (harm_pwr < CONFIDENCE * harmonic_power_start[index + MIN_KEY]) {
      //unable to choose note. To be possibly decided in the following frame
      dbg_printf("some note detected\n");
      
      continue;
    }

    //note found
    current.on = TRUE;
    if (current.key == index + MIN_KEY) {
      printf("same note pressed again %d\n", current.key);
    }
    else {
      current.key = index + MIN_KEY;
      printf("detected note %d\n", index + MIN_KEY);
    }

    current.vel = get_velocity(current.key, (1 - harm_pwr_old) * sqrms);
    dbg_printf("vel %d\n", current.vel);
    //output to MIDI port
    note_search = FALSE;
    midi_noteon(&seq, 0, current.key, current.vel);

    //write recording when required
    if (fd >= 0) {
      unsigned long to_write = sizeof(float) * frames;	//1 channel only
      unsigned char *ptr = (unsigned char *) buf;

      do {
        int written = write(fd, buf, to_write);
	if (written < 0) {
	  fprintf(stderr, "recording error\n");
	  break;
	}
        to_write -= written;
        ptr += written;
      } while (to_write > 0);
    }
  }
end:
  if (fd >= 0)
    close(fd);
  
  if (fin >= 0)
    close(fd);
  midi_end(&seq);
  pcm_end(&sett);
  exit(0);
}

