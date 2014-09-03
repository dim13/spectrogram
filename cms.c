/* $Id$ */
/*
 * Copyright (c) 2014 Dimitri Sokolyuk <demon@dim13.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * input: H 0..360, S 0..100, V 0..100
 * output: R, G, B 0..65535
 */

#include <stdint.h>

#include "cms.h"

void
hsv2rgb(unsigned short *r, unsigned short *g, unsigned short *b,
	double h, double s, double v)
{
	double	F, M, N, K;
	int	i;

	/* normalize */
	h /= 360.0;
	s /= 100.0;
	v /= 100.0;

	if (s > 0.0) {
		if (h >= 1.0)
			h -= 1.0;

		h *= 6.0;
		i = (int)h;
		F = h - i;
		M = v * (1 - s);
		N = v * (1 - s * F);
		K = v * (1 - s * (1 - F));

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
	} else {
		*r = *g = *b = UINT16_MAX * v;
	}
}

/*
 * input: H 0..360, S 0..100, L 0..100
 * output: R, G, B 0..65535
 */

void
hsl2rgb(unsigned short *r, unsigned short *g, unsigned short *b,
	double h, double s, double l)
{
	double v, F, M, K, N;
	int i;

	/* normalize */
	h /= 360.0;
	s /= 100.0;
	l /= 100.0;

	v = (l <= 0.5) ? (l + l * s) : (l - l * s + s);

	if (v > 0.0) {
		if (h >= 1.0)
			h -= 1.0;

		h *= 6.0;
		i = (int)h;
		F = h - i;
		M = l + l - v;
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
	} else {
		*r = *g = *b = UINT16_MAX * l;
	}
}
