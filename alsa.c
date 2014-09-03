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
#include <alsa/asoundlib.h>

#define	RCHAN	2

struct sio {
	int16_t *buffer;
	size_t bufsz;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
};

struct sio *
init_sio(unsigned int round)
{
	struct sio *sio;
	unsigned int rate = 44100;
	int rc;


	sio = malloc(sizeof(struct sio));
	assert(sio);

	rc = snd_pcm_open(&sio->handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0)
		errx(1, "unable to open pcm device: %s", snd_strerror(rc));

	snd_pcm_hw_params_malloc(&sio->params);
	snd_pcm_hw_params_any(sio->handle, sio->params);
	snd_pcm_hw_params_set_access(sio->handle, sio->params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(sio->handle, sio->params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(sio->handle, sio->params, &rate, NULL);
	snd_pcm_hw_params_set_channels(sio->handle, sio->params, RCHAN);
	snd_pcm_hw_params_set_period_size(sio->handle, sio->params, round, 0);

	rc = snd_pcm_hw_params(sio->handle, sio->params);
	if (rc < 0)
		errx(1, "unable to set hw parameters: %s", snd_strerror(rc));
	
	snd_pcm_hw_params_get_period_size(sio->params, &sio->frames, NULL);
	snd_pcm_hw_params_free(sio->params);
	snd_pcm_prepare(sio->handle);

	sio->bufsz = sio->frames * RCHAN * sizeof(int16_t);
	sio->buffer = malloc(sio->bufsz);
	assert(sio->buffer);

	return sio;
}

int16_t *
read_sio(struct sio *sio)
{
	int rc;

	rc = snd_pcm_readi(sio->handle, sio->buffer, sio->frames);
	if (rc == -EPIPE) {
		warnx("overrun occured");
		snd_pcm_prepare(sio->handle);
	} else if (rc < 0)
		warnx("error from read: %s", snd_strerror(rc));
	else if (rc != sio->frames)
		warnx("shor read, read %d frames", rc);

	return sio->buffer;
}

void
free_sio(struct sio *sio)
{
	snd_pcm_drain(sio->handle);
	snd_pcm_close(sio->handle);
	free(sio->buffer);
	free(sio);
}
