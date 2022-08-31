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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "audiostream.h"
//#include "fft.h"
#define PI 3.141592653589793

#define MIN_KEY		1
#define MAX_KEY 	128
#define NOISE_RMS	0.002
#define CONFIDENCE	0.9
#define GAIN		1.1

#define IF_ERR(errcode, msg, extra) if(errcode) {printf msg; extra;} 
#define IF_ERR_EXIT(errcode, msg) if(errcode) {fprintf msg; exit(1);} 

typedef struct note_struct {
  bool_t	on;
  int		vel;
  float		harmonic_power_old;
  float		harmonic_power;
  float		octave_ratio;
} note_info_t;

float MIDI_freq[128] = {8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817, 82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708, 130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220.0000, 233.0819, 246.9416, 261.6256, 277.1826, 293.6648, 311.1270, 329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833, 523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9889, 783.9909, 830.6094, 880.0000, 932.3276, 987.7666, 1046.5022, 1108.7305, 1174.6590, 1244.5079, 1318.5103, 1396.9129, 1479.9777, 1567.9818, 1661.2188, 1760.0000, 1864.6549, 1975.5334, 2093.0045, 2217.4609, 2349.3183, 2489.0158, 2637.0203, 2793.8260, 2959.9554, 3135.9632, 3322.4377, 3520.0000, 3729.3098, 3951.0668, 4186.0089, 4434.9218, 4698.6366, 4978.0317, 5274.0406, 5587.6521, 5919.9109, 6271.9265, 6644.8755, 7040.0000, 7458.6214, 7902.1319, 8372.0178, 8869.8452, 9397.2715, 9956.0633, 10548.0829, 11175.3024, 11839.8217, 12543.8554};

float octave_ratio[127] = {0.5445, 0.4375, 1.0358, 1.8473, 1.7404, 1.0646, 0.3729, 0.6003, 0.4390, 0.4430, 0.9771, 0.8239, 0.5679, 0.5541, 0.6600, 0.6669, 0.4837, 0.5002, 0.2550, 0.4131, 0.3018, 0.4148, 0.4406, 0.4008, 0.2427, 0.2348, 0.3076, 0.2899, 0.1959, 0.0877, 0.2310, 0.2902, 0.5151, 0.2882, 6.0840, 6.4857, 9.9052, 3.3233, 24.2588, 25.5479, 51.3549, 221.5547, 65.8027, 69.7166, 19.3153, 118.6768, 0.5329, 0.6522, 0.5956, 0.7528, 0.7840, 0.7617, 0.7808, 0.7494, 9.5647, 15.6207, 12.3036, 15.6356, 2.1034, 2.3369, 2.3560, 2.3157, 2.6540, 2.4726, 2.5043, 2.6289, 25871.7402, 812.3201, 9371.0049, 564.9780, 197.5410, 273.3660, 189.7327, 216.9722, 1.1885, 1.2831, 1.3546, 1.4511, 8.0115, 8.2573, 8.6290, 8.6818, 8.5963, 9.2611, 175.4133, 133.8493, 182.0157, 155.8724, 140.5083, 180.3035, 56.0204, 73.1021, 101.3058, 91.4193, 1373.6012, 6365.3931, 4328.1685, 5417.5098, 7241.2695, 12583.2939, 18851.5469, 7884.6582, 87786.6875, 9609.6602, 3185.8020, 1096.9713, 474.6582, 494.5000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};

float harmonic_power_start[127] = {0.4025, 0.1783, 0.5714, 0.1645, 0.1315, 0.1136, 0.1044, 0.1837, 0.1610, 0.1969, 0.2324, 0.3663, 0.1838, 0.1731, 0.1879, 0.1836, 0.1714, 0.1550, 0.1697, 0.2096, 0.1608, 0.2682, 0.2622, 0.2450, 0.2192, 0.1722, 0.2724, 0.2333, 0.2606, 0.2403, 0.2123, 0.2131, 0.1997, 0.1772, 0.2379, 0.2186, 0.1951, 0.2215, 0.2609, 0.2617, 0.2776, 0.2732, 0.2246, 0.2705, 0.2671, 0.2755, 0.2632, 0.2569, 0.2840, 0.2728, 0.3681, 0.3673, 0.3742, 0.3644, 0.3259, 0.3234, 0.3189, 0.3203, 0.2707, 0.2711, 0.2693, 0.2770, 0.1456, 0.1549, 0.1605, 0.1699, 0.3441, 0.3451, 0.3508, 0.3504, 0.2750, 0.2821, 0.2804, 0.2871, 0.1723, 0.1779, 0.1812, 0.1805, 0.2991, 0.3074, 0.3168, 0.3256, 0.3342, 0.3404, 0.4227, 0.4203, 0.4194, 0.4187, 0.4200, 0.4224, 0.3586, 0.3966, 0.4149, 0.3875, 0.3134, 0.3014, 0.2975, 0.2897, 0.2878, 0.1533, 0.1318, 0.1086, 0.0899, 0.0742, 0.0139, 0.0134, 0.0127, 0.0124, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};

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
	//octave check
	if ((res > 12) && (vec[res - 12] > 0.5*octave_ratio[res - 12]*(*val))) {
		res -= 12;
		*val = vec[res];
	}

	*cumul = 0;
	for (int i = res; i < dim; i += 12) {
		*cumul += vec[i];
	}
	//*cumul /= dim;

	return res;
}

int get_velocity(int key, float harmonic_pwr)
{
	return 127;
}


int main (int argc, char *argv[])
{
  int i, err, fin, fd, last_note = 0, verbose = 0;
  unsigned int samplerate = 48000, frames = 1024, channels = 2;
  const char *fname = NULL;
  char *audio_in = NULL, *midi_out = NULL, *port_name = NULL;
  float *buf, dft_on_note[MAX_KEY - MIN_KEY + 1], *x;
  note_info_t table[MAX_KEY - MIN_KEY + 1];
  float rms, rms_old = 0;
  bool_t new_note = FALSE;

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
  
  midi_stream_t seq = {NULL, NULL, NULL, NULL, 0, OUTPUT, 0};
  
  IF_ERR_EXIT(((err = pcm_init(&sett)) < 0), (stderr, "Unable to capture\n"))
  IF_ERR_EXIT(((err = midi_init(&seq)) < 0), (stderr, "Unable to open MIDI sequencer\n"))
  //dbg_printf("portID %d port name %s output %s\n", seq.portid, seq.portname, midi_out);

  for (i = 0; i < MAX_KEY - MIN_KEY + 1; i++)
    table[i].on = FALSE;

  IF_ERR_EXIT(((buf = malloc(sizeof(float) * frames * channels)) == 0), (stderr, "failed memory allocation\n"))
  IF_ERR_EXIT(((x = malloc(sizeof(float) * frames)) == 0), (stderr, "failed memory allocation\n"))

  //dbg_printf("entrata ciclo\n");
  for (i = 0; !stop; ++i) {
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

    for (int i = 0; i < frames; i++) {
      x[i] = buf[i*channels];
      //saturation check
      rms += x[i] * x[i];
    }
    rms = sqrt(rms / frames);
    dbg_printf("rms %.4f\n", rms);

    if (rms <= NOISE_RMS) {
      //no detection
      last_note = 0;
      if (new_note) {
        printf("possibly missed a note\n");
	new_note = FALSE;
      }
      //silence previous notes still going
      for (int i = 0; i < MAX_KEY - MIN_KEY + 1; i++) {
        if (table[i].on) {
	  //send NOTEOFF event
          midi_noteoff(&seq, 0, i + MIN_KEY);
          
	  printf("turning off note %d\n", i + MIN_KEY);
	  table[i].on = FALSE;
	}
      }
      rms_old = NOISE_RMS;

      //dbg_printf("noise\n");
      continue;		//wait for next frame
    }

    bool_t debu = FALSE;
    // /*  to avoid computations in the final code
    if (!(new_note) && rms < GAIN * rms_old && rms_old > NOISE_RMS) {
      //same note as before
      dbg_printf("same note\n");
      rms_old = rms;
      debu = TRUE;
      //continue;
    }
 

    new_note = TRUE;
    rms_old = rms;

    for (int i = 0; i < frames; i++) {
      //rms normalization
      x[i] /= rms;
    }

    for (int i = MIN_KEY; i <= MAX_KEY; i++) {
      dft_on_note[i-MIN_KEY] = dft_square_arg(MIDI_freq[i], x, frames, samplerate);
    }

    //pitch detection, single note only
    int vel, index = -1;
    float peak, hrm_pwr = 0;

    index = find_pitch(dft_on_note, MAX_KEY - MIN_KEY + 1, &peak, &hrm_pwr);
    vel = get_velocity(MIN_KEY + index, hrm_pwr);
    hrm_pwr /= frames;
    dbg_printf("miglior risultato: key %d val %.4f harm_pwr %.4f\n", index + MIN_KEY, peak, hrm_pwr);

    if (debu) {
	    new_note = FALSE;
	    continue;
    }

    if (hrm_pwr < CONFIDENCE * harmonic_power_start[index + MIN_KEY]) {
      //unable to confirm note. To be decided in the following frame
      dbg_printf("some note detected\n");
      continue;
    }

    //note found
    midi_noteoff(&seq, 0, last_note);

    if (last_note == index + MIN_KEY)
      printf("same note pressed again %d\n", last_note);
    else {
      printf("detected note %d\n", index + MIN_KEY);
      last_note = index + MIN_KEY;
      table[last_note - MIN_KEY].on = FALSE;
      table[index].on = TRUE;
    }
    new_note = FALSE;
    //output to MIDI port
    midi_noteon(&seq, 0, index + MIN_KEY, vel);

//write recording
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
