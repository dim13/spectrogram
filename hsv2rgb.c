/* $Id$ */

#include <math.h>

/* input: h, s, v [0..1]
 * output: r, g, b [0..1]
 */
void
hsv2rgb(float *r, float *g, float *b, float h, float s, float v)
{
	float   F, M, N, K;
	int     i;

	if (s == 0.0) {
		*r = *g = *b = v;
	} else {
		if (h == 1.0)
			h = 0.0;
		h *= 6.0;
		i = floorf(h);
		F = h - i;
		M = v * (1 - s);
		N = v * (1 - s * F);
		K = v * (1 - s * (1 - F));

		switch (i) {
		case 0: *r = v; *g = K; *b = M; break;
		case 1: *r = N; *g = v; *b = M; break;
		case 2: *r = M; *g = v; *b = K; break;
		case 3: *r = M; *g = N; *b = v; break;
		case 4: *r = K; *g = M; *b = v; break;
		case 5: *r = v; *g = M; *b = N; break;
		}
	}
}
