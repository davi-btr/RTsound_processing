/*
 *sndutils.c
 *
 *library for sndprocess
 *
*/

#include <stdlib.h>

#include "sndutils.h"
#include "fourier.h"


int get_index(float *vec, unsigned int dim, float *val)
{	//finds max peak
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

	return res;
}

float harm_pwr_calc(float freq, float* signal, int dim, double fs)
{	//selects harmonics and computes cumulative energy
	int count = 0;
	float res, harmonics[(int)(1 + MIDI_freq[MAX_KEY] / freq)], fr = freq;

	while (fr <= MIDI_freq[MAX_KEY]) {
		harmonics[count] = dft_sqr_arg(fr, signal, dim, fs);
		fr += freq;
		count++;
	}

	res = 0;

	for (int i = 0; i < count; i++) {
		res += harmonics[i];
	}
	res /= dim;

	return res;
}

int get_velocity(int key, float sqrms, float harm_pwr)
{
	return 127;
}

