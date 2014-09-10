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

struct sio {
	struct	sio_hdl *hdl;
	struct	sio_par par;
	int16_t	*buffer;
	unsigned int samples;
} sio;

int
init_sio(void)
{
	sio.hdl = sio_open(SIO_DEVANY, SIO_REC, 0);
	if (!sio.hdl)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&sio.par);

	sio.par.rchan = STEREO;
	sio.par.bits = BITS;
	sio.par.le = SIO_LE_NATIVE;
	sio.par.sig = SIGNED;

	if (!sio_setpar(sio.hdl, &sio.par))
		errx(1, "SIO set params failed");
	if (!sio_getpar(sio.hdl, &sio.par))
		errx(1, "SIO get params failed");

	if (sio.par.rchan != STEREO ||
	    sio.par.bits != BITS ||
	    sio.par.le != SIO_LE_NATIVE ||
	    sio.par.sig != SIGNED)
		errx(1, "unsupported audio params");

	sio.samples = sio.par.rate / FPS;
	warnx("min samples: %d", sio.samples);
	sio.samples -= sio.samples % sio.par.round - sio.par.round;
	warnx("max samples: %d", sio.samples);
	sio.buffer = calloc(sio.samples * sio.par.rchan, sizeof(int16_t));
	assert(sio.buffer);

	return sio_start(sio.hdl);
}

unsigned int
max_samples_sio(void)
{
	/*
	 * maximal number of samples we're willing to provide
	 * with 1920 at 25 fps and 48000 Hz or
	 * with 1764 at 25 fps and 44100 Hz it shall fit on most screens
	 */
	return sio.samples;
}

int16_t *
read_sio(unsigned int n)
{
	int done;
	char *buffer = (char *)sio.buffer;
	size_t bufsz = sio.samples * sio.par.rchan * sizeof(int16_t);
	size_t rndsz = n * sio.par.rchan * sizeof(int16_t);

	if (rndsz > bufsz)
		rndsz = bufsz;

	for (done = 0; bufsz > 0; buffer += done, bufsz -= done) {
		done = sio_read(sio.hdl, buffer, bufsz);
		if (sio_eof(sio.hdl))
			errx(1, "SIO EOF");
	}

	/*
	 * return a pointer to the latest ROUND samples (the most recent
	 * ones) to minimize latency between picture and sound
	 */
	return (int16_t *)(buffer - rndsz);
}

void
free_sio(struct sio *sio)
{
	sio_stop(sio.hdl);
	sio_close(sio.hdl);
	free(sio.buffer);
}
