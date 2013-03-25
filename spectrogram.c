/* $Id$ */
/*
 * Copyright (c) 2010 Dimitri Sokolyuk <demon@dim13.org>
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

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <assert.h>
#include <sndio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <SDL.h>
#include <SDL_thread.h>

#include "fft.h"
#include "hsv2rgb.h"

#define PSIZE	250
#define SSIZE	(PSIZE >> 1)

SDL_Surface	*screen;
SDL_Color	*pane;
SDL_Color	*pan2;
SDL_Color	black = { .r = 0, .g = 0, .b = 0 };
SDL_Color	white = { .r = 255, .g = 255, .b = 255 };

SDL_Rect	wf_from, wf_to;
SDL_Rect	wf_left, wf_right;
SDL_Rect	sp_left, sp_right;

int	die = 0;
int	flip_left = 1;
int	flip_right = 0;

void
init_rect(int w, int h, int off, int s)
{
	wf_from.x = off;
	wf_from.y = 1;
	wf_from.w = w - 2 * off;
	wf_from.h = h - s - 1;

	wf_to.x = off;
	wf_to.y = 0;
	wf_to.w = w - 2 * off;
	wf_to.h = h - s - 1;

	wf_left.x = off;
	wf_left.y = h - s - 1;
	wf_left.w = w / 2 - 2 * off;
	wf_left.h = 1;

	wf_right.x = w / 2 + off;
	wf_right.y = h - s - 1;
	wf_right.w = w / 2 - 2 * off;
	wf_right.h = 1;

	sp_left.x = off;
	sp_left.y = h - s;
	sp_left.w = w / 2 - 2 * off;
	sp_left.h = s;

	sp_right.x = w / 2 + off;
	sp_right.y = h - s;
	sp_right.w = w / 2 - 2 * off;
	sp_right.h = s;
}

SDL_Color *
init_palette_small(int n)
{
	SDL_Color *p;
	int	i;
	float	h, s, v;
	float	hstep, sstep, vstep;


	p = malloc(n * sizeof(SDL_Color));

	h = 0.3;
	hstep = -0.3 / n;

	s = 0.5;
	sstep = 0.5 / n;

	v = 0.75;
	vstep = 0.25 / n;


	for (i = 0; i < n; i++) {
		hsv2rgb(&p[i].r, &p[i].g, &p[i].b, h, s, v);
		h += hstep;
		s += sstep;
		v += vstep;
	}

	return p;
}

SDL_Color *
init_palette_big(int n)
{
	SDL_Color *p;
	int	i;
	float	h, s, v;
	float	hstep, sstep, vstep;
	float	k;
	int	f = 100;

	p = malloc(n * sizeof(SDL_Color));
	k = 1.0 / logf(f + 1);

	h = 0.65;
	hstep = -0.4 / n;

	s = 1.0;
	sstep = -1.0 / n;

	v = 0.0;
	vstep = 1.0 / n;

	for (i = 0; i < n; i++) {
		hsv2rgb(&p[i].r, &p[i].g, &p[i].b, h, s,
			k * logf(f * v + 1));

		h += hstep;
		s += sstep;
		v += vstep;
	}

	return p;
}

void
drawpixel(SDL_Surface *s, int x, int y, SDL_Color *c)
{
	Uint32 *buf = (Uint32 *)s->pixels + y * (s->pitch >> 2) + x;
	Uint32 pixel = SDL_MapRGB(s->format, c->r, c->g, c->b);	
	*buf = pixel;
}

int
draw(double *left, double *right, int p)
{
	int             x, y, l, r, lx, rx;

	if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen))
		return -1;

	SDL_BlitSurface(screen, &wf_from, screen, &wf_to);

	for (x = 0; x < wf_left.w; x++) {
		l = left[x] - 0.5;
		if (l >= p)
			l = p - 1;
		r = right[x] - 0.5;
		if (r >= p)
			r = p - 1;

		lx = wf_left.x + (flip_left ? wf_left.w - x - 1 : x);
		rx = wf_right.x + (flip_right ? wf_right.w - x - 1 : x);

		drawpixel(screen, lx, wf_left.y, &pane[l]);
		drawpixel(screen, rx, wf_right.y, &pane[r]);

		for (y = 0; y < sp_left.h; y++) {
			drawpixel(screen, lx,
				sp_left.y + sp_left.h - y - 1,
				l > y ? &pan2[y] : &black);
			drawpixel(screen, rx,
				sp_right.y + sp_right.h - y - 1,
				r > y ? &pan2[y] : &black);
		}
	}
			
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return 0;
}

double *
init_hamming(int n)
{
	double	*w;
	int	i;

	w = calloc(n, sizeof(double));
	assert(w);

	for (i = 0; i < n; i++)
		w[i] = 0.54 - 0.46 * cos((2 * M_PI * i) / (n - 1));

	return w;
}

void
catch(int notused)
{
	die = 1;
}

__dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-h <pixel>] [-w <pixel>] [-l] [-r]\n",
		__progname);
	exit(1);
}

int 
main(int argc, char **argv)
{
	SDL_Event       event;

	int16_t		*buffer;
	size_t		bufsz;

	struct		sio_hdl	*sio;
	struct		sio_par par;
	struct	fft	*fft;

	double		*left, *right;
	int		delta;
	double		*hamming;
	int		psize, ssize;
	int		width, height;
	int		pressed = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	signal(SIGINT, catch);
	atexit(SDL_Quit);
	
	sio = sio_open(NULL, SIO_REC, 0);
	if (!sio)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&par);
	sio_getpar(sio, &par);
	
	delta = par.round;
	warnx("delta %d", delta);

	width = delta + 4;	/* XXX */
	height = 3 * width / 4;

	screen = SDL_SetVideoMode(width, height, 32,
		SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF);
	if (!screen)
		return 1;

	bufsz = 2 * delta * sizeof(int16_t);
	buffer = malloc(bufsz);
	assert(buffer);

	left = calloc(delta, sizeof(double));
	right = calloc(delta, sizeof(double));
	assert(left && right);

	psize = 2 * height / 3;
	ssize = psize >> 2;

	init_rect(width, height, 1, ssize);

	pan2 = init_palette_small(ssize);
	pane = init_palette_big(psize);

	fft = init_fft(delta);
	hamming = init_hamming(delta);

	sio_start(sio);

	while (!die) {
		size_t done = 0;

		do {
			done += sio_read(sio, buffer + done, bufsz - done);
			if (done != bufsz)
				warnx("re-read %zu", done);
			assert(sio_eof(sio) == 0);
		} while (done < bufsz);

		dofft(fft, buffer, left, right, delta, hamming);
		draw(left, right, psize);

		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			die = 1;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_q:
				die = 1;
				break;
			case SDLK_l:
			case SDLK_1:
				if (!pressed)
					flip_left ^= 1;
				break;
			case SDLK_r:
			case SDLK_2:
				if (!pressed)
					flip_right ^= 1;
				break;
			case SDLK_0:
				if (!pressed) {
					flip_left ^= 1;
					flip_right ^= 1;
				}
				break;
			case SDLK_f:
				if (!pressed)
					screen = SDL_SetVideoMode(0, 0, 0,
						screen->flags ^ SDL_FULLSCREEN);
				break;
			default:
				break;
			}
			pressed = 1;
			break;
		case SDL_KEYUP:
			pressed = 0;
			break;
		}
	}

	sio_stop(sio);
	sio_close(sio);

	free(left);
	free(right);
	free(buffer);

	return 0;
}
