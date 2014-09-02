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
#include <stdlib.h>
#include <sndio.h>

#define RCHAN	2
#define BITS	16
#define SIGNED	1
#define FPS	24

struct sio {
	struct	sio_hdl *sio;
	struct	sio_par par;
	int16_t	*buffer;
	size_t	bufsz;
	size_t	round;
};

struct sio *
init_sio(unsigned int round)
{
	struct sio *sio;
	size_t bufsz;

	sio = malloc(sizeof(struct sio));
	assert(sio);

	sio->sio = sio_open(SIO_DEVANY, SIO_REC, 0);
	if (!sio->sio)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&sio->par);

	sio->par.rchan = RCHAN;
	sio->par.bits = BITS;
	sio->par.le = SIO_LE_NATIVE;
	sio->par.sig = SIGNED;

	if (!sio_setpar(sio->sio, &sio->par))
		errx(1, "SIO set params failed");
	if (!sio_getpar(sio->sio, &sio->par))
		errx(1, "SIO get params failed");

	if (sio->par.rchan != RCHAN ||
	    sio->par.bits != BITS ||
	    sio->par.le != SIO_LE_NATIVE ||
	    sio->par.sig != SIGNED)
		errx(1, "unsupported audio params");

	sio->round = round;

	bufsz = sio->par.rate / FPS;		/* 24 pictures/second */
	bufsz -= bufsz % sio->par.round;	/* round to block size */
	while (bufsz < sio->round)		/* not less than block size */
		bufsz += sio->par.round;
	sio->bufsz = bufsz * sio->par.rchan * sizeof(int16_t);
	sio->buffer = malloc(sio->bufsz);
	assert(sio->buffer);

	sio_start(sio->sio);

	return sio;
}

int16_t *
read_sio(struct sio *sio, size_t n)
{
	int done = 0;
	char *buffer = (char *)sio->buffer;
	size_t sz = sio->bufsz;
	size_t roundsz = n * sio->par.rchan * sizeof(int16_t);

	do {
		done = sio_read(sio->sio, buffer, sz);
		if (sio_eof(sio->sio))
			errx(1, "SIO EOF");
		buffer += done;
		sz -= done;
	} while (sz);

	/*
	 * return a pointer to the latest ROUND samples (the most recent
	 * ones) to minimize latency between picture and sound
	 */
	return (int16_t *)(buffer - roundsz);
}

void
del_sio(struct sio *sio)
{
	sio_stop(sio->sio);
	sio_close(sio->sio);
	free(sio->buffer);
	free(sio);
}
