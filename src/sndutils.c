/*
 * sndutils.c
 *
 * Library for sndprocess
 *
*/

#include <stdlib.h>
#include <math.h>

#include "sndutils.h"
#include "fourier.h"

#define VELOCITY_STEP	1.03552808f

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
{	//selects harmonics
	int count = 0;
	float res, harmonics[(int)(1 + MIDI_freq[MAX_KEY] / freq)], fr = freq;

	while (fr <= MIDI_freq[MAX_KEY]) {
		harmonics[count] = dft_sqr_arg(fr, signal, dim, fs);
		fr += freq;
		count++;
	}

	res = 0;
	//harmonics cumulative power
	for (int i = 0; i < count; i++) {
		res += harmonics[i];
	}
	res /= dim;

	return res;
}

int get_velocity(int key, float sqrms)
{	//sqrms_at_velocity(V) = VELOCITY_STEP * sqrms_at_velocity(V - 1)
	float res, ratio, step;

	step = powf(VELOCITY_STEP, 10);
	ratio = velocity_reference[key] / sqrms;
	res = 127;

	do {
		res -= 10;
		ratio /= step;
	} while (ratio > 1.0);

	ratio *= step;
	res += 10;

	do {
		res --;
		ratio /= VELOCITY_STEP;
	} while (ratio > 1.0);

	ratio *= VELOCITY_STEP;
	res++;

	return res;
}

