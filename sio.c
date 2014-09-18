/* $Id$ */
/*
 * Copyright (c) 2013 Dimitri Sokolyuk <demon@dim13.org>
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
#include <err.h>
#include <stdint.h>
#include <sndio.h>
#include <stdlib.h>

#include "sio.h"

#define STEREO	2
#define BITS	16
#define SIGNED	1
#define FPS	25

static struct	sio_hdl *hdl;
static struct	sio_par par;
static int16_t	*buffer;
static unsigned int samples;

struct data {
	int16_t left;
	int16_t right;
};

int
init_sio(void)
{
	hdl = sio_open(SIO_DEVANY, SIO_REC, 0);
	if (!hdl)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&par);

	par.rchan = STEREO;
	par.bits = BITS;
	par.le = SIO_LE_NATIVE;
	par.sig = SIGNED;

	if (!sio_setpar(hdl, &par))
		errx(1, "SIO set params failed");
	if (!sio_getpar(hdl, &par))
		errx(1, "SIO get params failed");

	if (par.rchan != STEREO ||
	    par.bits != BITS ||
	    par.le != SIO_LE_NATIVE ||
	    par.sig != SIGNED)
		errx(1, "unsupported audio params");

	samples = par.rate / FPS;
	samples -= samples % par.round;
	if (samples < par.rate / FPS)
		samples += par.round;
	buffer = calloc(samples * par.rchan, sizeof(int16_t));
	assert(buffer);

	warnx("%d, %d, %d", samples, par.rate, par.round);

	sio_start(hdl);

	return samples;
}

size_t
read_sio(double *left, double *right, size_t n)
{
	int done, i;
	char *p = (char *)buffer;
	size_t bufsz, rndsz;
	struct data *data;

	if (n > samples)
		n = samples;

	bufsz = samples * par.rchan * sizeof(int16_t);
	rndsz = n * par.rchan * sizeof(int16_t);

	for (done = 0; bufsz > 0; p += done, bufsz -= done) {
		done = sio_read(hdl, p, bufsz);
		if (sio_eof(hdl))
			errx(1, "SIO EOF");
	}

	/*
	 * return a pointer to the latest ROUND samples (the most recent
	 * ones) to minimize latency between picture and sound
	 */
	data = (struct data *)(p - rndsz);

	/* split and normalize */
	for (i = 0; i < n; i++) {
		left[i] = data[i].left / (double)INT16_MAX;
		right[i] = data[i].right / (double)INT16_MAX;
	}

	return n;
}

void
free_sio(void)
{
	sio_stop(hdl);
	sio_close(hdl);
	free(buffer);
}
