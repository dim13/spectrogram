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

#include "sio.h"

#define STEREO	2
#define RATE	48000
#define FPS	25

struct sio {
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	int16_t *buffer;
	unsigned int samples;
};

struct sio *
init_sio(void)
{
	struct sio *sio;
	snd_pcm_uframes_t round;
	unsigned int rate;
	int rc;

	sio = malloc(sizeof(struct sio));
	assert(sio);

	rc = snd_pcm_open(&sio->handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0)
		errx(1, "unable to open pcm device: %s", snd_strerror(rc));

	snd_pcm_hw_params_malloc(&sio->params);
	snd_pcm_hw_params_any(sio->handle, sio->params);
	snd_pcm_hw_params_set_access(sio->handle, sio->params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(sio->handle, sio->params,
		SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(sio->handle, sio->params, STEREO);
	snd_pcm_hw_params_set_rate(sio->handle, sio->params, RATE, 0);

	rc = snd_pcm_hw_params(sio->handle, sio->params);
	if (rc < 0)
		errx(1, "unable to set hw parameters: %s", snd_strerror(rc));
	
	snd_pcm_hw_params_get_period_size(sio->params, &round, NULL);
	snd_pcm_hw_params_get_rate(sio->params, &rate, 0);
	snd_pcm_hw_params_free(sio->params);
	snd_pcm_prepare(sio->handle);

	sio->samples = rate / FPS;
	warnx("min samples: %d", sio->samples);
	sio->samples -= sio->samples % round - round;
	warnx("max samples: %d", sio->samples);
	sio->buffer = calloc(sio->samples * STEREO, sizeof(int16_t));
	assert(sio->buffer);


	return sio;
}

unsigned int
max_samples_sio(struct sio *sio)
{
	return sio->samples;
}

int16_t *
read_sio(struct sio *sio, unsigned int n)
{
	snd_pcm_sframes_t rc;

	if (n > sio->samples)
		n = sio->samples;

	rc = snd_pcm_readi(sio->handle, sio->buffer, sio->samples);
	if (rc != sio->samples) {
		warnx("audio read error: %s", snd_strerror(rc));
		if (rc == -EPIPE)
			snd_pcm_prepare(sio->handle);
	}

	return sio->buffer + sio->samples - n;
}

void
free_sio(struct sio *sio)
{
	snd_pcm_drain(sio->handle);
	snd_pcm_close(sio->handle);
	free(sio->buffer);
	free(sio);
}
