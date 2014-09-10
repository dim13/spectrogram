/* $Id$ */
/*
 * Copyright (c) 2010 Dimitri Sokolyuk <demon@dim13.org>
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>		/* must be prior fftw3 */
#include <fftw3.h>

#include "fft.h"

static fftw_plan plan;
static double	*in;
static fftw_complex *out;
static size_t	sz;
static double	*window;
static double	*sq;

static double *
hamming(size_t n)
{
	double	*p;
	int	i;

	p = calloc(n, sizeof(double));
	assert(p);

	for (i = 0; i < n; i++) {
		p[i] = 0.54 - 0.46 * cos((2 * M_PI * i) / (n - 1));
		p[i] /= INT16_MAX;
	}

	return p;
}

static double *
squares(size_t n)
{
	double	*p;
	int	i;

	p = calloc(n / 2, sizeof(double));
	assert(p);

	for (i = 0; i < n / 2; i++)
		p[i] = sqrt(i + 1);

	return p;
}

int
init_fft(size_t n)
{
	sz = n;
	in = fftw_malloc(sz * sizeof(double));
	out = fftw_malloc(sz * sizeof(fftw_complex) / 2);
	assert(in && out);

	window = hamming(sz);
	sq = squares(sz);

	plan = fftw_plan_dft_r2c_1d(sz, in, out, FFTW_MEASURE);

	return 0;
}

int
exec_fft(double *io)
{
	int	i;

	for (i = 0; i < sz; i++)
		in[i] = window[i] * io[i];

	fftw_execute(plan);

	for (i = 0; i < sz / 2; i++)
		io[i] = sq[i] * cabs(out[i]);

	return 0;
}

void
free_fft(void)
{
	fftw_free(in);
	fftw_free(out);
	free(window);
	free(sq);
}
