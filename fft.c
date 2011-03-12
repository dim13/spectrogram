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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>

struct fft {
	fftw_plan	plan;
	double	*in;
	double	*out;
	int	n;
};

struct fft *
init_fft(int n)
{
	struct	fft *p;

	p = malloc(sizeof(struct fft));
	p->in = fftw_malloc(n * sizeof(double));
	p->out = fftw_malloc(n * sizeof(double));
	p->plan = fftw_plan_r2r_1d(n, p->in, p->out, FFTW_R2HC, FFTW_MEASURE);
	p->n = n;

	return p;
}

int
dofft(struct fft *p, double *data)
{
	int	i, n;

	memset(p->out, 0, p->n * sizeof(double));
	memcpy(p->in, data, p->n * sizeof(double));
	fftw_execute(p->plan);

	n = p->n / 2;
	for (i = 1; i < n; i++)
		data[i - 1] = sqrt(i * (p->out[i] * p->out[i] + p->out[p->n - i] * p->out[p->n - i]));

	return 0;
}
