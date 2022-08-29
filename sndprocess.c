/*
  * parte principale del programma: registra audio, ne calcola la trasformata,
  * la confronta e salva su file
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "audiostream.h"
#define PI 3.141592653589793

#define MIN_KEY			69
#define MAX_KEY			105
#define SATURATION_LEVEL	3.4e35f
#define MAX_SATUR_SAMPLES	3
#define NOISE_RMS		0.002
#define PREV_PEAK		0.7
#define CONFIDENCE		0.4
#define HARM_PWR_NOTE		17	//to be calculated key by key
#define GAIN			1.5

#define IF_ERR(errcode, msg, extra) if(errcode) {printf msg; extra;} 
#define IF_ERR_EXIT(errcode, msg) if(errcode) {fprintf msg; exit(1);} 

#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

//typedef enum {FALSE = 0, TRUE} bool_t;

typedef struct note_struct {
  bool_t	on;
  //int		vel;
  float		rms_at_vel[128];
  //float	harmonic_power_old;	//to be used to detect velocity
  //float	harmonic_power;
} note_info_t;

float MIDI_freq[128] = {8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817, 82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708, 130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220.0000, 233.0819, 246.9416, 261.6256, 277.1826, 293.6648, 311.1270, 329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833, 523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9889, 783.9909, 830.6094, 880.0000, 932.3276, 987.7666, 1046.5022, 1108.7305, 1174.6590, 1244.5079, 1318.5103, 1396.9129, 1479.9777, 1567.9818, 1661.2188, 1760.0000, 1864.6549, 1975.5334, 2093.0045, 2217.4609, 2349.3183, 2489.0158, 2637.0203, 2793.8260, 2959.9554, 3135.9632, 3322.4377, 3520.0000, 3729.3098, 3951.0668, 4186.0089, 4434.9218, 4698.6366, 4978.0317, 5274.0406, 5587.6521, 5919.9109, 6271.9265, 6644.8755, 7040.0000, 7458.6214, 7902.1319, 8372.0178, 8869.8452, 9397.2715, 9956.0633, 10548.0829, 11175.3024, 11839.8217, 12543.8554};

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
  fprintf(stderr, "    -v		: verbose mode\n");
  fprintf(stderr, "    -i device-id	: ALSA input PCM device\n");
  fprintf(stderr, "    -o port-name	: ALSA output MIDI port\n");
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
	if ((res > 12) && (vec[res - 12] >= PREV_PEAK * (*val))) {
		res -= 12;
		*val = vec[res];
	}
  
	//energy associated with fundamental frequency
	*cumul = 0;
	for (int i = res; i < dim; i += 12) {
		*cumul += vec[res];
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
	int i, count, err, fd, verbose = 0, port_id, last_note = 0, saturation_count = 0;
	unsigned int samplerate = 48000, frames = 1024, channels = 2;
	char *fname = NULL, *audio_in = NULL, *midi_out = NULL, *port_name = NULL;
	float *buf, dft_on_note[MAX_KEY - MIN_KEY + 1], *x;
	note_info_t table[MAX_KEY - MIN_KEY + 1];
	float rms, rms_old = 0;
	bool_t note_detec = FALSE;

	if (argc==1) {
		usage();
		
		exit(0);
	}

	for (i = 1 ; i<argc ; i++) {
		if (argv[i][0]=='-') switch (argv[i][1]) {
			case 'h':
				usage();
				break;
			case 'v':
				verbose = 1;
				break;
			case 'i':
				audio_in = argv[++i];
				dbg_printf("audio in %s\n", audio_in);
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

	for (i = 0; i < MAX_KEY - MIN_KEY + 1; i++)
		table[i].on = FALSE;

	IF_ERR_EXIT(((buf = malloc(sizeof(float) * frames * channels)) == 0), (stderr, "failed memory allocation\n"))
	IF_ERR_EXIT(((x = malloc(sizeof(float) * frames)) == 0), (stderr, "failed memory allocation\n"))
  
	IF_ERR(((fd = open(fname, O_CREAT | O_WRONLY, 0666)) < 0), ("Not recording\n"), ;)
	dbg_printf("Recording to file: %s\n", fname);
  //inizializzazione con libreria da fare
	pcm_stream_t sett = {NULL, "default", buf, frames, samplerate, channels, CAPTURE, TRUE};
	midi_stream_t seq = {NULL, NULL, NULL, NULL, 0, OUTPUT, 0};
  
	pcm_init(&sett);
	midi_init(&seq);

	dbg_printf("entrata ciclo\n");
	for (i = 0; !stop; i++) {
		pcm_capture(&sett);
		//RMS normalization
		rms = 0;

		for (int i = 0; i < frames; i++) {
			x[i] = buf[i*channels];
	      		if (x[i] > SATURATION_LEVEL || x[i] < -SATURATION_LEVEL) {
	      			saturation_count++;
	      			if (saturation_count > MAX_SATUR_SAMPLES)
	      				printf("results may not be reliable due to saturation levels. Please move device further from source.\n");
	      		} else
	      			saturation_count = 0;
			rms += x[i] * x[i];
		}
		rms = sqrt(rms / frames);
		dbg_printf("rms %.4f\n", rms);
    
		if (rms <= NOISE_RMS) {
			//no detection
			last_note = 0;
			if (note_detec) {
				printf("possibly missed a note\n");
				note_detec = FALSE;
			}
			//turn off previous notes, if any
			for (int i = 0; i < MAX_KEY - MIN_KEY + 1; i++)
			if (table[i].on) {
				//send NOTEOFF event
				//dbg_printf("turning off note %d\n", MIN_KEY + i);
				midi_noteoff(&seq, 0, i + MIN_KEY);
				table[i].on = FALSE;
			}
			rms_old = NOISE_RMS;
			dbg_printf("noise\n");
			continue;	//wait for next frame
		}

		//to avoid computations
		if (!(note_detec) && rms < GAIN * rms_old && rms_old > NOISE_RMS) {
			//same note as before
			dbg_printf("same note\n");
			rms_old = rms;
			continue;
		}
		
		note_detec = TRUE;
		
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
		dbg_printf("match: key %d\n", index + MIN_KEY);
		if (hrm_pwr < CONFIDENCE * HARM_PWR_NOTE) {
			//unable to identify note. Delayed to next frame
			dbg_printf("something detected");
			continue;
		}
		
		//note identified
		if (last_note == index + MIN_KEY) {
			dbg_printf("same note pressed again\n");
		} else {
			//possible noteoff...
			dbg_printf("detected note %d\n", index + MIN_KEY);
			last_note = index + MIN_KEY;
			table[index].on = TRUE;
		}
		
		note_detec = FALSE;

		//output to MIDI port
		midi_noteon(&seq, 0, index + MIN_KEY, vel);
		
		rms_old = rms;

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

	midi_end(&seq);
	pcm_end(&sett);
	
	exit(0);
}
