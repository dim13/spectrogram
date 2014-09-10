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
	warnx("min samples: %d", samples);
	samples -= samples % par.round - par.round;
	warnx("max samples: %d", samples);
	buffer = calloc(samples * par.rchan, sizeof(int16_t));
	assert(buffer);

	return sio_start(hdl);
}

unsigned int
max_samples_sio(void)
{
	/*
	 * maximal number of samples we're willing to provide
	 * with 1920 at 25 fps and 48000 Hz or
	 * with 1764 at 25 fps and 44100 Hz it shall fit on most screens
	 */
	return samples;
}

int16_t *
read_sio(size_t n)
{
	int done;
	char *p = (char *)buffer;
	size_t bufsz = samples * par.rchan * sizeof(int16_t);
	size_t rndsz = n * par.rchan * sizeof(int16_t);

	if (rndsz > bufsz)
		rndsz = bufsz;

	for (done = 0; bufsz > 0; p += done, bufsz -= done) {
		done = sio_read(hdl, p, bufsz);
		if (sio_eof(hdl))
			errx(1, "SIO EOF");
	}

	/*
	 * return a pointer to the latest ROUND samples (the most recent
	 * ones) to minimize latency between picture and sound
	 */
	return (int16_t *)(p - rndsz);
}

void
free_sio(struct sio *sio)
{
	sio_stop(hdl);
	sio_close(hdl);
	free(buffer);
}
