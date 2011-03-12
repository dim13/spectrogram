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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct	buf {
	double	*buf;
	double	*end;
	double	*wr;
	double	*rd;
};

struct	fifo {
	struct	buf l;
	struct	buf r;
	size_t	sz;
};

struct fifo *
init_fifo(int n)
{
	struct fifo *ff;

	ff = malloc(sizeof(struct fifo));

	ff->l.buf = malloc(n * sizeof(double));
	ff->l.end = ff->l.buf + n;
	ff->l.wr = ff->l.buf;
	ff->l.rd = ff->l.buf;

	ff->r.buf = malloc(n * sizeof(double));
	ff->r.end = ff->r.buf + n;
	ff->r.wr = ff->r.buf;
	ff->r.rd = ff->r.buf;

	ff->sz = 0;

	return ff;
}

int
wr_fifo(struct fifo *ff, double *l, double *r, int n)
{
	int	i;

	for (i = 0; i < n; i++) {
		*ff->l.wr++ = *l++;
		*ff->r.wr++ = *r++;
		if (ff->l.wr >= ff->l.end) {
			ff->l.wr = ff->l.buf;
			ff->r.wr = ff->r.buf;
		}
	}
	ff->sz += n;

	return ff->sz;
}

int
rd_fifo(struct fifo *ff, double *l, double *r, int n, double *w)
{
	int	i;
	double	*lnext = ff->l.rd;
	double	*rnext = ff->r.rd;

	for (i = 0; i < n; i++) {
		if (i == n / 4) {
			lnext = ff->l.rd;
			rnext = ff->r.rd;
		}
		*l++ = *ff->l.rd++ * w[i];
		*r++ = *ff->r.rd++ * w[i];
		if (ff->l.rd >= ff->l.end) {
			ff->l.rd = ff->l.buf;
			ff->r.rd = ff->r.buf;
		}
	}
	ff->l.rd = lnext;
	ff->r.rd = rnext;

	ff->sz -= n / 4;

	return ff->sz;
}
