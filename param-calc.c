/*
  * param-calc.c
  *
  * Calculate parameters required by main for each MIDI note
  * 
  *
*/

#include <stdio.h>
#include <stdlib.h>
//#include <alsa/asoundlib.h>
#include <math.h>

//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

//#include "fft.h"
#define PI 3.141592653589793

#define MIN_KEY 1
#define MAX_KEY 127
#define SILENCE	0.001
#define N_REP	3
#define N_WIN	2	//mantenere

#define IF_ERR(errcode, msg, extra) if(errcode) {printf msg; extra;} 
#define IF_ERR_EXIT(errcode, msg) if(errcode) {fprintf msg; exit(1);} 

//typedef enum {FALSE = 0, TRUE} bool_t;

typedef struct note_struct {
  //bool_t	on;
  int		vel;
  float		harm_pwr_start;
  float		octave_ratio;
} note_info_t;

float MIDI_freq[128] = {8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817, 82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708, 130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220.0000, 233.0819, 246.9416, 261.6256, 277.1826, 293.6648, 311.1270, 329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833, 523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9889, 783.9909, 830.6094, 880.0000, 932.3276, 987.7666, 1046.5022, 1108.7305, 1174.6590, 1244.5079, 1318.5103, 1396.9129, 1479.9777, 1567.9818, 1661.2188, 1760.0000, 1864.6549, 1975.5334, 2093.0045, 2217.4609, 2349.3183, 2489.0158, 2637.0203, 2793.8260, 2959.9554, 3135.9632, 3322.4377, 3520.0000, 3729.3098, 3951.0668, 4186.0089, 4434.9218, 4698.6366, 4978.0317, 5274.0406, 5587.6521, 5919.9109, 6271.9265, 6644.8755, 7040.0000, 7458.6214, 7902.1319, 8372.0178, 8869.8452, 9397.2715, 9956.0633, 10548.0829, 11175.3024, 11839.8217, 12543.8554};

//int stop = 0;

//#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

/*
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
*/

float dft_square_arg(float freq, float* signal, unsigned int dim, float fs)
{
	float arg, res_real = 0, res_img = 0, th = -2.0*PI*freq/fs;

	arg = th;
	for (int i = 0; i < dim; i++) {
		res_real += signal[i] * cosf(arg);
		res_img += signal[i] * sinf(arg);
		arg += th;
	}

	return (powf(res_real, 2) + powf(res_img, 2)) / dim;
}
/*
int get_velocity(int key, float harmonic_pwr)
{
  return 127;
}
*/
float harm_pwr(float *vect, int dim, int frames)
{
        //power associated with fundamental frequency and harmonics
        float res = 0;

	//dbg_printf("frames %d \n", frames);
        for (int i = 0; i < dim; i ++) {
                res += vect[i];
		dbg_printf("cumul %.4f\n", res);
        }
        res /= frames;

        return res;
}

int main (int argc, char *argv[])
{
  int count, i, fin;
  unsigned int samplerate = 48000, frames = 1024, channels = 2;
  char fname[18] = "noteXXX-127-6.dat";
  float *buf, *x, oc_r[N_REP], hrm_p[N_REP];
  float  octave_ratio_keys[MAX_KEY - MIN_KEY + 1], harm_pwr_keys[MAX_KEY - MIN_KEY + 1];
  //note_info_t table[MAX_KEY - MIN_KEY + 1];
  float dft_vect[MAX_KEY - MIN_KEY + 1], rms = 0;

  for (int j = 0; j < N_REP; j++) {
  }

  IF_ERR_EXIT(((buf = malloc(sizeof(float) * frames * channels)) == 0), (stderr, "failed memory allocation\n"))
  IF_ERR_EXIT(((x = malloc(sizeof(float) * frames)) == 0), (stderr, "failed memory allocation\n"))

  for (int n = MIN_KEY; n <= MAX_KEY; n++) {
    float fr = MIDI_freq[n];
    dbg_printf("key %d freq %.4f\n", n, fr);

    octave_ratio_keys[n - MIN_KEY] = 0;
    harm_pwr_keys[n - MIN_KEY] = 0;
    i = 0;

    fname[4] = (char)(n / 100 + 0x30);
    fname[5] = (char)((n%100) / 10 + 0x30);
    fname[6] = (char)(n % 10 + 0x30);
    //fname[8] = (char)(vels[0] / 100 + 0x30);
    //fname[9] = (char)((vels[0]%100) / 10 + 0x30);
    //fname[10] = (char)(vels[0] % 10 + 0x30);
    //dbg_printf("file %s\n", fname);
    IF_ERR(((fin = open(fname, O_RDONLY)) < 0), ("Unable to read %s\n", fname), ;)
  //IF_ERR(((fd = open(fname, O_CREAT | O_WRONLY, 0666)) < 0), ("Unable to record\n"), ;)


    while (i < N_REP) {
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

//RMS normalization
      rms = 0;

      for (int j = 0; j < frames; j++) {
        x[j] = buf[j*channels];
        rms += x[j] * x[j];
      }
      rms = sqrt(rms / frames);
      dbg_printf("energy %.4f\n", rms);
      if (rms < SILENCE) {
	      count = 0;
	      oc_r[i] = 0;
	      hrm_p[i] = 0;

	      continue;
      }

      if (count >= N_WIN)

	      continue;

      count++;

      for (int j = 0; j < frames; j++) {
        x[j] /= rms;
      }

      int octaves = 0;

      for (int j = n; j <= MAX_KEY; j += 12) { 
        dft_vect[octaves] = dft_square_arg(MIDI_freq[j], x, frames, samplerate);
        octaves++;
      }

      //dbg_printf("octaves %d\n", octaves);
      oc_r[i] += dft_square_arg(fr, x, frames, samplerate) / dft_square_arg(2 * fr, x, frames, samplerate);

      float tmp = harm_pwr(dft_vect, octaves, frames);

      //dbg_printf("tmp %.4f\n", tmp);
      hrm_p[i] += tmp;
      dbg_printf("octave ratio  %.4f harm pwr %.4f\n", oc_r[i], hrm_p[i]);

      oc_r[i] /= count;
      hrm_p[i] /= count;
      i += (count - 1);
    }

    for (int j = 0; j < N_REP; j++) {
      octave_ratio_keys[n - MIN_KEY] += oc_r[j];
      harm_pwr_keys[n - MIN_KEY] += hrm_p[j];
    }
    octave_ratio_keys[n - MIN_KEY] /= N_REP;
    harm_pwr_keys[n - MIN_KEY] /= N_REP;
    dbg_printf("oct %.4f harm pwr %.4f\n", octave_ratio_keys[n - MIN_KEY], harm_pwr_keys[n - MIN_KEY]);

end:
    if (fin >= 0) {
      close(fin);
      //dbg_printf("closing %s\n", fname);
    }

  }

  printf("OCTAVE RATIO FROM %d TO %d\n", MIN_KEY, MAX_KEY);
  for (int j = MIN_KEY; j <= MAX_KEY; j++) {
    printf("%.4f, ", octave_ratio_keys[j - MIN_KEY]);
  }
  printf("\nHARMONIC POWER START FROM %d TO %d\n", MIN_KEY, MAX_KEY);
  for (int j = MIN_KEY; j <= MAX_KEY; j++) {
    printf("%.4f, ", harm_pwr_keys[j - MIN_KEY]);
  }
  printf("\n");

  exit(0);
}
