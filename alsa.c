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

struct alsa {
	snd_pcm_t *hdl;
	snd_pcm_hw_params_t *par;
	int16_t *buffer;
	unsigned int samples;
} alsa;

int
init_sio(void)
{
	snd_pcm_uframes_t round;
	unsigned int rate;
	int rc;

	rc = snd_pcm_open(&alsa.hdl, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0)
		errx(1, "unable to open pcm device: %s", snd_strerror(rc));

	snd_pcm_hw_params_malloc(&alsa.par);
	snd_pcm_hw_params_any(alsa.hdl, alsa.par);
	snd_pcm_hw_params_set_access(alsa.hdl, alsa.par,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(alsa.hdl, alsa.par,
		SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(alsa.hdl, alsa.par, STEREO);
	snd_pcm_hw_params_set_rate(alsa.hdl, alsa.par, RATE, 0);

	rc = snd_pcm_hw_params(alsa.hdl, alsa.par);
	if (rc < 0)
		errx(1, "unable to set hw parameters: %s", snd_strerror(rc));
	
	snd_pcm_hw_params_get_period_size(alsa.par, &round, NULL);
	snd_pcm_hw_params_get_rate(alsa.par, &rate, 0);
	snd_pcm_hw_params_free(alsa.par);
	snd_pcm_prepare(alsa.hdl);

	alsa.samples = rate / FPS;
	warnx("min samples: %d", alsa.samples);
	alsa.samples -= alsa.samples % round - round;
	warnx("max samples: %d", alsa.samples);
	alsa.buffer = calloc(alsa.samples * STEREO, sizeof(int16_t));
	assert(alsa.buffer);

	return 0;
}

unsigned int
max_samples_sio(void)
{
	return alsa.samples;
}

int16_t *
read_sio(size_t n)
{
	snd_pcm_sframes_t rc;

	if (n > alsa.samples)
		n = alsa.samples;

	rc = snd_pcm_readi(alsa.hdl, alsa.buffer, alsa.samples);
	if (rc != alsa.samples) {
		warnx("audio read error: %s", snd_strerror(rc));
		if (rc == -EPIPE)
			snd_pcm_prepare(alsa.hdl);
	}

	return alsa.buffer + alsa.samples - n;
}

void
free_sio(void)
{
	snd_pcm_drain(alsa.hdl);
	snd_pcm_close(alsa.hdl);
	free(alsa.buffer);
}
