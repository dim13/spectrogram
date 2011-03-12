/* $Id$ */
/* hsv2rgb.c
 * Convert Hue Saturation Value to Red Green Blue
 *
 * P.J. 08-Aug-98
 *
 * Reference:
 * D. F. Rogers
 * Procedural Elements for Computer Graphics
 * McGraw Hill 1985
 */

#include <stdint.h>

int
hsv2rgb(uint8_t *r, uint8_t *g, uint8_t *b, float h, float s, float v)
{
	/*
	 * Purpose:
	 * Convert HSV values to RGB values
	 * All values are in the range [0.0 .. 1.0]
	 */
	float   F, M, N, K;
	int     i;

	/* normalize */
	if (s < 0.0) {
		h += 0.5;
		s = -s;
	}

	if (v < 0.0)
		v = -v;

	while (h > 1.0)
		h -= 1.0;
	while (h < 0.0)
		h += 1.0;


	if (s == 0.0) {
		/* 
		 * Achromatic case, set level of grey 
		 */
		*r = v * 255;
		*g = v * 255;
		*b = v * 255;
	} else {
		/* 
		 * Determine levels of primary colours. 
		 */

		h = (h >= 1.0) ? 0.0 : h * 6;
		i = (int)h;		/* should be in the range 0..5 */
		F = h - i;		/* fractional part */

		M = v * (1 - s);
		N = v * (1 - s * F);
		K = v * (1 - s * (1 - F));

		switch (i) {
		case 0:
			*r = v * 255;
			*g = K * 255;
			*b = M * 255;
			break;
		case 1:
			*r = N * 255;
			*g = v * 255;
			*b = M * 255;
			break;
		case 2:
			*r = M * 255;
			*g = v * 255;
			*b = K * 255;
			break;
		case 3:
			*r = M * 255;
			*g = N * 255;
			*b = v * 255;
			break;
		case 4:
			*r = K * 255;
			*g = M * 255;
			*b = v * 255;
			break;
		case 5:
			*r = v * 255;
			*g = M * 255;
			*b = N * 255;
			break;
		}
	}

	return 0;
}
