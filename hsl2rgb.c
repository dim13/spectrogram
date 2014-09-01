/* $Id$ */

/* input: H 0..360, S 0..100, L 0..100 */
/* output: R, G, B 0..65535 */

void
hsl2rgb(unsigned short *r, unsigned short *g, unsigned short *b,
	double h, double s, double l)
{
	double v, F, M, mv, K, N;
	int i;

	/* normalize */
	h /= 360.0;
	s /= 100.0;
	l /= 100.0;

	/* default to gray */
	*r = *g = *b = UINT16_MAX * l;

	v = (l <= 0.5) ? (l * (1.0 + s)) : (l + s - l * s);

	if (v > 0) {
		h *= 6.0;
		i = (int) h;
		F = h - i;
		M = 2.0 * l - v;
		K = M + F * (v - M);
		N = v - F * (v - M);

		/* scale up */
		v *= UINT16_MAX;
		M *= UINT16_MAX;
		K *= UINT16_MAX;
		N *= UINT16_MAX;

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
