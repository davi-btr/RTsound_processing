#include <stdlib.h>
#include <math.h>
#include "fourier.h"

#define DIRETTA 1
#define INVERSA -1

typedef struct Complex
{
   float real;
   float image;
} CPLX;

float dft_sqr_arg(float freq, float* signal, unsigned int dim, float fs)
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

void _fft(CPLX buf[], CPLX out[], int n, int step, int dir)
{
  int i;
  CPLX t;
  float a, b, c, d, theta;

  if (step < n) 
  {
    _fft(out, buf, n, step * 2, dir);
    _fft(out + step, buf + step, n, step * 2, dir);

    for (i = 0; i < n; i += 2 * step) 
    {
      theta = -PI * dir * i / n;
      a = cosf(theta);
      b = sinf(theta);
      c = out[i + step].real;
      d = out[i + step].image;
      t.real = a * c - b * d;
      t.image = b * c + a * d;
      //printf("%.4f . %.4f\n", t.real, t.image);
      buf[i / 2].real = out[i].real + t.real;
      buf[i / 2].image = out[i].image + t.image;
      buf[(i + n) / 2].real = out[i].real - t.real;
      buf[(i + n) / 2].image = out[i].image - t.image;
    }
  }
}

int fft(float in_real[], float in_img[], int n) {return 1;}

int fft_inv(float in[], float phase[], int n)
{
  int i;
  CPLX *buf = (CPLX *)malloc(n * sizeof(CPLX));
  if (buf == NULL)
  {
	  return 0;
  }
  CPLX *out = (CPLX *)malloc(n * sizeof(CPLX));
  if (out == NULL)
  {
	  return 0;
  }

  for (i = 0; i < n; i++)
  {
	  buf[i].real = in[i];
	  out[i].real = in[i];
	  buf[i].image = 0.0;
	  out[i].image = 0.0;
  }

  _fft(buf, out, n, 1, DIRETTA);

  for (i = 0; i < n; i++) {
    in[i] = sqrtf(powf(buf[i].real, 2) + powf(buf[i].image, 2)) / n;
    //printf("modulo %.4f\n", buf[i]);
    if (buf[i].real == 0.0) {
      if (buf[i].image > 0.0)
        phase[i] = PI/2;
      else if (buf[i].image < 0.0)
        phase[i] = -PI/2;
      else
        phase[i] = 0.0;
    } else 
      phase[i] = atan2f(buf[i].image, buf[i].real);
  }

  free(out);
  out = NULL;
  return 1;
}

int fft_real(float in[], float phase[], int n) //real/img ==> mag/phase
{
  int i;
  CPLX *buf = (CPLX *)malloc(n * sizeof(CPLX));
  if (buf == NULL)
  {
	  return 0;
  }
  CPLX *out = (CPLX *)malloc(n * sizeof(CPLX));
  if (out == NULL)
  {
	  return 0;
  }

  for (i = 0; i < n; i++)
  {
	  buf[i].real = in[i];
	  out[i].real = in[i];
	  buf[i].image = 0.0;
	  out[i].image = 0.0;
  }

  _fft(buf, out, n, 1, DIRETTA);

  for (i = 0; i < n; i++) {
    in[i] = sqrtf(powf(buf[i].real, 2) + powf(buf[i].image, 2)) / n;
    //printf("modulo %.4f\n", buf[i]);
    if (buf[i].real == 0.0) {
      if (buf[i].image > 0.0)
        phase[i] = PI/2;
      else if (buf[i].image < 0.0)
        phase[i] = -PI/2;
      else
        phase[i] = 0.0;
    } else 
      phase[i] = atan2f(buf[i].image, buf[i].real);
  }

  free(out);
  out = NULL;
  return 1;
}
/*
int main()
{
  #define COUNT 1024
  float mag[COUNT];
  //CPLX buf[COUNT] = {
    {1.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {1.0, 0.0},
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}
  };
  
  //campioni sinusoidali
  float f_A4 = 234.75;
  float f_sample = 48000.0;
  float ampl = 1000.0;
  unsigned long off = 0;
  for (int j = 0; j < COUNT; j++) {
    float t_secs = off++ / f_sample;
    buf[j].real = ampl * sinf(2.0*PI*f_A4*t_secs);
    buf[j].image = 0.0;
    //printf("ampl %f\n", buf[j].real);
  }

  fft(buf, COUNT);

  for (int i = 0; i < COUNT; i++) {
    mag[i] = sqrt(pow(buf[i].real, 2) + pow(buf[i].image, 2));
    mag[i] /= COUNT;
    printf("modulo %.4f\n", mag[i]);
  }

  for (int i = 0; i < COUNT; i++)
  {
    if (buf[i].image >= 0.0)
    {
      printf("%.4f + %.4fi\n", buf[i].real, buf[i].image);
    } else {
      printf("%.4f - %.4fi\n", buf[i].real, fabs(buf[i].image));
    }
  }

  return 0;
}
*/
