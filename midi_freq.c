#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main() {

	double val;

	for (int i = 0; i < 128; i++) {
		val = 440.0*powf(2.0, (i - 69)/12.0);
		printf("%.4f, ", val);
	}

	return 0;
}
