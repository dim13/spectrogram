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

struct fft {
	fftw_plan plan;
	double	*in;
	fftw_complex *out;
	size_t	n;
	double	*window;
	double	*sq;
} fft;

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
	fft.n = n;
	fft.in = fftw_malloc(fft.n * sizeof(double));
	fft.out = fftw_malloc(fft.n * sizeof(fftw_complex) / 2);
	assert(fft.in && fft.out);

	fft.window = hamming(fft.n);
	fft.sq = squares(fft.n);

	fft.plan = fftw_plan_dft_r2c_1d(fft.n, fft.in, fft.out,
		FFTW_MEASURE);

	return 0;
}

int
exec_fft(double *io)
{
	int	i;

	for (i = 0; i < fft.n; i++)
		fft.in[i] = fft.window[i] * io[i];

	fftw_execute(fft.plan);

	for (i = 0; i < fft.n / 2; i++)
		io[i] = fft.sq[i] * cabs(fft.out[i]);

	return 0;
}

void
free_fft(void)
{
	fftw_free(fft.in);
	fftw_free(fft.out);
	free(fft.window);
	free(fft.sq);
}
