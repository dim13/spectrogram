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
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include "sio.h"

#define STEREO	2
#define RATE	48000
#define FPS	25

static snd_pcm_t *hdl;
static snd_pcm_hw_params_t *par;
static int16_t *buffer;
static unsigned int samples;

struct data {
	int16_t left;
	int16_t right;
};

int
init_sio(void)
{
	snd_pcm_uframes_t round;
	unsigned int rate;
	int rc;

	rc = snd_pcm_open(&hdl, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0)
		errx(1, "unable to open pcm device: %s", snd_strerror(rc));

	snd_pcm_hw_params_malloc(&par);
	snd_pcm_hw_params_any(hdl, par);
	snd_pcm_hw_params_set_access(hdl, par,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(hdl, par,
		SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(hdl, par, STEREO);
	snd_pcm_hw_params_set_rate(hdl, par, RATE, 0);

	rc = snd_pcm_hw_params(hdl, par);
	if (rc < 0)
		errx(1, "unable to set hw parameters: %s", snd_strerror(rc));
	
	snd_pcm_hw_params_get_period_size(par, &round, NULL);
	snd_pcm_hw_params_get_rate(par, &rate, 0);
	snd_pcm_hw_params_free(par);
	snd_pcm_prepare(hdl);

	samples = rate / FPS;
	samples -= samples % round;
	if (samples < rate / FPS)
		samples += round;
	warnx("%s round/rate/samples: %d/%d/%d", __func__,
		round, rate, samples);
	buffer = calloc(samples * STEREO, sizeof(int16_t));
	assert(buffer);

	return samples;
}

size_t
read_sio(int **out, size_t n)
{
	snd_pcm_sframes_t rc;
	int16_t *tmp;
	int i;

	if (n > samples)
		n = samples;

	rc = snd_pcm_readi(hdl, buffer, samples);
	if (rc != samples) {
		warnx("audio read error: %s", snd_strerror(rc));
		if (rc == -EPIPE)
			snd_pcm_prepare(hdl);
	}

	tmp = buffer[samples - n];

	/* split */
	for (i = 0; i < n * STEREO; i++)
		out[i % STEREO][i / STEREO] = tmp[i];

	return n;
}

void
free_sio(void)
{
	snd_pcm_drain(hdl);
	snd_pcm_close(hdl);
	free(buffer);
}
