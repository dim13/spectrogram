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

#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>

struct fft {
	fftw_plan	plan;
	double	*in;
	double	*out;
	int	n;
	double	*window;
};

double *
hamming(int n)
{
	double	*w;
	int	i;

	w = calloc(n, sizeof(double));
	if (!w)
		errx(1, "malloc failed");

	for (i = 0; i < n; i++)
		w[i] = 0.54 - 0.46 * cos((2 * M_PI * i) / (n - 1));

	return w;
}

struct fft *
init_fft(int n)
{
	struct	fft *p;

	p = malloc(2 * sizeof(struct fft));

	p[0].n = n;
	p[0].in = fftw_malloc(n * sizeof(double));
	p[0].out = fftw_malloc(n * sizeof(double));
	p[0].plan = fftw_plan_r2r_1d(n, p[0].in, p[0].out,
		FFTW_R2HC, FFTW_MEASURE);

	p[1].n = n;
	p[1].in = fftw_malloc(n * sizeof(double));
	p[1].out = fftw_malloc(n * sizeof(double));
	p[1].plan = fftw_plan_r2r_1d(n, p[1].in, p[1].out,
		FFTW_R2HC, FFTW_MEASURE);

	p->window = hamming(n);

	return p;
}

int
dofft(struct fft *p, int16_t *data, double *left, double *right, int n, float scala)
{
	int	i;

	for (i = 0; i < n; i++) {
		p[0].in[i] = p->window[i] * data[2 * i + 0] / (double)INT16_MAX;
		p[1].in[i] = p->window[i] * data[2 * i + 1] / (double)INT16_MAX;
	}

	fftw_execute(p[0].plan);
	fftw_execute(p[1].plan);

	for (i = 1; i < n / 2; i++) {
		left[i - 1] = sqrt(scala * i
			* (pow(p[0].out[i], 2) + pow(p[0].out[n - i], 2)));
		right[i - 1] =  sqrt(scala * i
			* (pow(p[1].out[i], 2) + pow(p[1].out[n - i], 2)));
	}

	return 0;
}

void
del_fft(struct fft *fft)
{
	fftw_free(fft->in);
	fftw_free(fft->out);
	free(fft->window);
	free(fft);
}
