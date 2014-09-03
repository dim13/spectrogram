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

#define STEREO	2
#define RATE	44100

struct sio {
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	int16_t *buffer;
	size_t bufsz;
	snd_pcm_uframes_t round;
};

struct sio *
init_sio(unsigned int round)
{
	struct sio *sio;
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
	snd_pcm_hw_params_set_rate(sio->handle, sio->params, RATE, 0);
	snd_pcm_hw_params_set_channels(sio->handle, sio->params, STEREO);
	snd_pcm_hw_params_set_period_size(sio->handle, sio->params, round, 0);

	rc = snd_pcm_hw_params(sio->handle, sio->params);
	if (rc < 0)
		errx(1, "unable to set hw parameters: %s", snd_strerror(rc));
	
	snd_pcm_hw_params_get_period_size(sio->params, &sio->round, NULL);
	snd_pcm_hw_params_free(sio->params);
	snd_pcm_prepare(sio->handle);

	if (sio->round != round)
		warnx("requested %d frames, got %d", round, sio->round);

	sio->bufsz = sio->round * STEREO * sizeof(int16_t);
	sio->buffer = malloc(sio->bufsz);
	assert(sio->buffer);

	return sio;
}

int16_t *
read_sio(struct sio *sio)
{
	int rc;

	rc = snd_pcm_readi(sio->handle, sio->buffer, sio->round);
	if (rc != sio->round) {
		warnx("error read from audio interface: %s", snd_strerror(rc));
		if (rc == -EPIPE)
			snd_pcm_prepare(sio->handle);
	}

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
