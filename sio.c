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

#include <err.h>
#include <stdlib.h>
#include <sndio.h>

struct sio {
	struct	sio_hdl *sio;
	struct	sio_par par;
};

struct sio *
init_sio(int rchan, int bits, int sig)
{
	struct sio *sio;

	sio = malloc(sizeof(struct sio));
	if (!sio)
		errx(1, "malloc failed");

	sio->sio = sio_open(NULL, SIO_REC, 0);

	if (!sio->sio)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&sio->par);

	sio->par.rchan = rchan;
	sio->par.bits = bits;
	sio->par.le = SIO_LE_NATIVE;
	sio->par.sig = sig;

	if (!sio_setpar(sio->sio, &sio->par))
		errx(1, "SIO set params failed");
	if (!sio_getpar(sio->sio, &sio->par))
		errx(1, "SIO get params failed");

	if (sio->par.rchan != rchan ||
	    sio->par.bits != bits ||
	    sio->par.le != SIO_LE_NATIVE ||
	    sio->par.sig != sig)
		errx(1, "unsupported audio params");

	sio_start(sio->sio);

	return sio;
}

unsigned int
get_round(struct sio *sio)
{
	return sio->par.round;
}

int
read_sio(struct sio *sio, int16_t *buffer, size_t sz)
{
	int done = 0;

	do {
		done += sio_read(sio->sio, buffer, sz);
		if (sio_eof(sio->sio))
			errx(1, "SIO EOF");
		buffer += done;
		sz -= done;
	} while (sz);

	return done;
}

void
del_sio(struct sio *sio)
{
	sio_stop(sio->sio);
	sio_close(sio->sio);
	free(sio);
}
