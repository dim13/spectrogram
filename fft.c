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
	size_t	n;
	double	*window;
};

double *
hamming(size_t n)
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

double *
fft_realloc(double *p, size_t n)
{
	if (p)
		fftw_free(p);
	p = fftw_malloc(n * sizeof(double));
	if (!p)
		errx(1, "malloc failed");

	return p;
}

struct fft *
resize_fft(struct fft *p, size_t n)
{
	if (n != p->n) {
		p->n = n;

		p->in = fft_realloc(p->in, p->n);
		p->out = fft_realloc(p->out, p->n);

		if (p->window)
			free(p->window);
		p->window = hamming(p->n);

		p->plan = fftw_plan_r2r_1d(p->n, p->in, p->out,
			FFTW_R2HC, FFTW_MEASURE);
	}
	return p;
}

struct fft *
init_fft(size_t n)
{
	struct	fft *p;

	p = calloc(1, sizeof(struct fft));
	p = resize_fft(p, n);

	return p;
}

int
dofft(struct fft *p, int16_t *data, double *out, int odd)
{
	int	i;

	for (i = 0; i < p->n; i++)
		p->in[i] = p->window[i] * data[2 * i + odd]
			/ (double)INT16_MAX;

	fftw_execute(p->plan);

	for (i = 1; i < p->n / 2; i++)
		out[i - 1] = sqrt(i * (pow(p->out[i], 2)
			+ pow(p->out[p->n - i], 2)));

	return 0;
}

void
del_fft(struct fft *p)
{
	fftw_free(p->in);
	fftw_free(p->out);
	free(p->window);
	free(p);
}
