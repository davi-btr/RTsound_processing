/*
  * parte principale del programma: registra audio, ne calcola la trasformata,
  * la confronta, la mostra a schermo e salva su file
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include "fft.h"
//#include "graphics.h"
//int grph_display(short *vect);
#define PI 3.141592653589793

#define MIN_KEY 64
#define MAX_KEY 72
//#define N_SEQ 5		//sequenze di frames prima di scegliere nota più simile

#define IF_ERR(errcode, msg, extra) if(errcode) {printf msg; extra} 
#define IF_ERR_EXIT(errcode, msg) if(errcode) {fprintf msg; exit(1);} 

typedef enum {OFF = 0, ON} bool_t;

typedef struct note_struct {
  bool_t	on;
  int		vel;
  float		harmonic_power_old;
  float		harmonic_power;
} note_info_t;

float MIDI_freq[128] = {8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817, 82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708, 130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220.0000, 233.0819, 246.9416, 261.6256, 277.1826, 293.6648, 311.1270, 329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833, 523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9889, 783.9909, 830.6094, 880.0000, 932.3276, 987.7666, 1046.5022, 1108.7305, 1174.6590, 1244.5079, 1318.5103, 1396.9129, 1479.9777, 1567.9818, 1661.2188, 1760.0000, 1864.6549, 1975.5334, 2093.0045, 2217.4609, 2349.3183, 2489.0158, 2637.0203, 2793.8260, 2959.9554, 3135.9632, 3322.4377, 3520.0000, 3729.3098, 3951.0668, 4186.0089, 4434.9218, 4698.6366, 4978.0317, 5274.0406, 5587.6521, 5919.9109, 6271.9265, 6644.8755, 7040.0000, 7458.6214, 7902.1319, 8372.0178, 8869.8452, 9397.2715, 9956.0633, 10548.0829, 11175.3024, 11839.8217, 12543.8554};

int stop = 0;

#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

void sighandler(int dum)
{
    stop=1;
}

static void usage(void)
{
  fprintf(stderr, "usage: sndprocess [options]\n");
  fprintf(stderr, "  options:\n");
  fprintf(stderr, "    -h		: display this help window\n");
  fprintf(stderr, "    -v		: verbose mode\n");
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

void init_pcm(snd_pcm_t **capture_handle, char* dev, unsigned int* rate_p, unsigned int* channels)
{
  int err;
  snd_pcm_hw_params_t *hw_params;

  if (dev == NULL)
    dev = "default";
  IF_ERR_EXIT(((err = snd_pcm_open(capture_handle, dev, SND_PCM_STREAM_CAPTURE, 0)) < 0), (stderr, "cannot open audio device (%s)\n",  snd_strerror(err)))

  IF_ERR_EXIT(((err = snd_pcm_hw_params_malloc (&hw_params)) < 0), (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err)))

  IF_ERR_EXIT(((err = snd_pcm_hw_params_any (*capture_handle, hw_params)) < 0), (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err)))

  IF_ERR_EXIT(((err = snd_pcm_hw_params_set_access (*capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0), (stderr, "cannot set acces type (%s)\n", snd_strerror(err)))

  IF_ERR_EXIT((err = snd_pcm_hw_params_set_format (*capture_handle, hw_params, SND_PCM_FORMAT_FLOAT_LE) < 0), (stderr, "cannot set sample format (%s)\n", snd_strerror(err)))

  IF_ERR_EXIT(((err = snd_pcm_hw_params_set_rate_near (*capture_handle, hw_params, rate_p, 0)) < 0), (stderr, "cannot set sample rate (%s)\n", snd_strerror(err)))

  dbg_printf("Got actual rate: %u\n", *rate_p);

  IF_ERR_EXIT(((err = snd_pcm_hw_params_set_channels (*capture_handle, hw_params, *channels)) < 0), (stderr, "cannot set channel count (%s)\n", snd_strerror(err)))

  IF_ERR_EXIT(((err = snd_pcm_hw_params (*capture_handle, hw_params)) < 0), (stderr, "cannot set parameters (%s)\n", snd_strerror(err)))

  snd_pcm_hw_params_free(hw_params);

  IF_ERR_EXIT(((err = snd_pcm_prepare (*capture_handle)) < 0), (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err)))

}

void init_seq(snd_seq_t **seq_handle, char* dev, char* port_name, int *port_id)
{
  int err;
  
  if (dev == NULL)
    dev = "default";
  IF_ERR_EXIT(((err = snd_seq_open(seq_handle, dev, SND_SEQ_OPEN_OUTPUT, 0)) < 0), (stderr, "cannot open audio device (%s)\n", snd_strerror (err)))
  //snd_seq_set_client_name()
  
  IF_ERR_EXIT(((err = snd_seq_create_simple_port(*seq_handle, port_name, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC)) < 0), (stderr, "cannot open audio device (%s)\n",  snd_strerror (err)))

  *port_id = err;
  IF_ERR_EXIT(((err = snd_seq_connect_to(*seq_handle, *port_id, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)) < 0), (stderr, "snd_seq_subscribe_to() failed: %s\n", snd_strerror(err)))
  
}

float dft_arg(float freq, float* signal, unsigned int dim, float fs)
{
	float arg, res_real = 0, res_img = 0, th = -2.0*PI*freq/fs;

	arg = th;
	for (int i = 0; i < dim; i++) {
		res_real += signal[i] * cosf(arg);
		res_img += signal[i] * sinf(arg);
		arg += th;
	}

	return (sqrtf(powf(res_real, 2) + powf(res_img, 2)) / dim);
}

int find_pitch(float *vec, int dim, float *val, float *cumul)
{
  int res = 0;

  *val = vec[0];

  for (int i = 0; i < dim; i++) {
    if (vec[i] >= *val) {
      *val = vec[i];
      res = i;
    }
  }
  if ((res > 12) && (vec[res - 12] >= 0.7 * (*val))) {
    res -= 12;
    *val = vec[res];
  }
  
  //energy associated with fundamental frequency
  *cumul = 0;
  for (int i = res; i < dim; i += 12) {
    *cumul += vec[res]*vec[res];
  }
  *cumul /= dim;

  return res;
}

int get_velocity(int key, float harmonic_pwr)
{
  return 127;
}


int main (int argc, char *argv[])
{
  int i, count, err, fd, verbose = 0, port_id;
  snd_pcm_t *pcm_stream;
  snd_seq_t *seq_stream;
  unsigned int samplerate = 48000, frames = 4096, channels = 1;
  const char *fname = NULL;
  char *audio_in = NULL, *midi_out = NULL, *port_name;
  float *buf, dft_on_note[MAX_KEY - MIN_KEY + 1], *x;
  note_info_t table[MAX_KEY - MIN_KEY + 1];
  snd_seq_event_t ev;
  float rms, hrm_pwr_old = 0;

  if (argc==1) {
    usage();
    exit(0);
  }
    
  for (i = 1 ; i<argc ; i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
        case 'h':
          usage();
          break;
        case 'v':
          verbose = 1;
          break;
        case 'i':
          audio_in = argv[++i];
          break;
        case 'o':
          midi_out = argv[++i];
          break;
        case 'p':
          port_name = argv[++i];
          break;
        case 'O':
          fname = argv[++i];
          break;
        case 'r':
          samplerate = atoi(argv[++i]);
          break;
        case 'c':
          channels = atoi(argv[++i]);
          break;
        case 'f':
          frames = atoi(argv[++i]);
          break;
        default:
          fprintf(stderr, "invalid option\n");
          usage();
      }           
    }
  }
  
  IF_ERR(((fd = open(fname, O_CREAT | O_WRONLY, 0666)) < 0), ("Unable to record\n"), ;)
  dbg_printf("Recording to file: %s\n", fname);

  init_pcm(&pcm_stream, audio_in, &samplerate, &channels);

  init_seq(&seq_stream, midi_out, port_name, &port_id);

  for (i = 0; i < MAX_KEY - MIN_KEY + 1; i++)
    table[i].on = OFF;

  IF_ERR_EXIT(((buf = malloc(sizeof(float) * frames * channels)) == 0), (stderr, "failed memory allocation\n"))
  IF_ERR_EXIT(((x = malloc(sizeof(float) * frames)) == 0), (stderr, "failed memory allocation\n"))

  for (i = 0; !stop; ++i) {
    IF_ERR_EXIT(((err = snd_pcm_readi(pcm_stream, buf, frames)) != frames), (stderr, "read from audio interface failed (%s)\n", snd_strerror(err)))
//RMS normalization
    rms = 0;

    for (int i = 0; i < frames; i++) {
      x[i] = buf[i*channels];
      //saturation check ??
      rms += x[i] * x[i];
    }
    rms /= frames;
    rms = sqrt(rms);			//RMS
    for (int i = 0; i < frames; i++) {
      x[i] /= rms;
    }

    //fft_real(arg, phase, frames);		//FFT
    for (int i = MIN_KEY; i <= MAX_KEY; i++) {
      dft_on_note[i-MIN_KEY] = dft_arg(MIDI_freq[i], x, frames, samplerate);
    }

//pitch detection, single note only
    int vel, index = -1;
    float peak, hrm_pwr = 0;

    index = find_pitch(dft_on_note, MAX_KEY - MIN_KEY + 1, &peak, &hrm_pwr);
    vel = get_velocity(MIN_KEY + index, hrm_pwr);

    dbg_printf("miglior risultato: key %.4f\n", MIDI_freq[index + MIN_KEY]);
    
//output to MIDI port
    if (vel == 0) {
      //no NOTE
      for (int i = 0; i < MAX_KEY - MIN_KEY + 1; i++) {
        if (table[i].on) {
	  //send NOTEOFF event
          snd_seq_ev_clear(&ev);
          snd_seq_ev_set_source(&ev, port_id);
          snd_seq_ev_set_subs(&ev);
          snd_seq_ev_set_direct(&ev);
          ev.type = SND_SEQ_EVENT_NOTEOFF;

          snd_seq_event_output_direct(seq_stream, &ev);
	}
      }
    } else if (!(table[index].on)) {
      //send NOTEON event
      snd_seq_ev_clear(&ev);
      snd_seq_ev_set_source(&ev, port_id);
      snd_seq_ev_set_subs(&ev);
      snd_seq_ev_set_direct(&ev);
      ev.type = SND_SEQ_EVENT_NOTEON;
      ev.data.note.channel = 0;
      ev.data.note.note = index + MIN_KEY;
      ev.data.note.velocity = vel;

      snd_seq_event_output_direct(seq_stream, &ev);
    } else if (hrm_pwr > hrm_pwr_old) {
      //send NOTEON event
      snd_seq_ev_clear(&ev);
      snd_seq_ev_set_source(&ev, port_id);
      snd_seq_ev_set_subs(&ev);
      snd_seq_ev_set_direct(&ev);
      ev.type = SND_SEQ_EVENT_NOTEON;
      ev.data.note.channel = 0;
      ev.data.note.note = index + MIN_KEY;
      ev.data.note.velocity = vel;

      snd_seq_event_output_direct(seq_stream, &ev);
    }

    hrm_pwr_old = hrm_pwr;

/* display dft
    if (err = grph_display(magnitude)) {
      printf("error while displaying\n");

//      exit(1);
    }
*/
//write recording
    if (fd >= 0) {
      unsigned long to_write = sizeof(float) * frames;	//1 channel only
      unsigned char *ptr = (unsigned char *) buf;

      do {
        int written = write(fd, buf, to_write);
	if (written < 0) {
	  printf("recording error\n");
	  break;
	}
        to_write -= written;
        ptr += written;
      } while (to_write > 0);
    }
  }
  if (fd >= 0)
    close(fd);
  
  //snd_seq_drain(&seq_stream)
  snd_seq_close(seq_stream);
  snd_pcm_close(pcm_stream);
  exit(0);
}
