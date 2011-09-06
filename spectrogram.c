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

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "fifo.h"
#include "fft.h"
#include "hsv2rgb.h"

#define PSIZE	250
#define SSIZE	(PSIZE >> 1)

#define TIMING 0

struct	buf {
	int16_t	left;
	int16_t right;
};

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

int *
init_scala(int n, int w)
{
	int	*s, i;
	float	k;

	s = malloc(w * sizeof(int));

	k = n / logf(w);

	for (i = 0; i < w; i++)
		s[i] = n - k * logf(w - i);

	return s;
}

int *
init_factor(int *scala, int w)
{
	int	*s, k, i;

	s = malloc(w * sizeof(int));
	k = 0;
	for (i = 0; i < w; i++) {
		s[i] = scala[i] - k;
		if (s[i] == 0)
			s[i] = 1;
		k = scala[i];
	}

	return s;
}

#if 1
inline void
drawpixel(SDL_Surface *s, int x, int y, SDL_Color *c)
{
	Uint32 *buf = (Uint32 *)s->pixels + y * (s->pitch >> 2) + x;
	Uint32 pixel = SDL_MapRGB(s->format, c->r, c->g, c->b);	
	*buf = pixel;
}
#else
#define	drawpixel(s, x, y, c) do {					\
	Uint32 *buf, pixel;						\
	buf = (Uint32 *)(s)->pixels + (y) * ((s)->pitch >> 2) + (x);	\
	pixel = SDL_MapRGB((s)->format, (c)->r, (c)->g, (c)->b);	\
	*buf = pixel;							\
	} while (0)
#endif

int
draw(double *left, double *right, int n, int *scala, int *fact, int p)
{
	int             x, y, l, r, i, lx, rx;

	if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen))
		return -1;

	SDL_BlitSurface(screen, &wf_from, screen, &wf_to);

	for (x = 0; x < wf_left.w; x++) {
		l = 0;
		r = 0;

		for (i = 0; i < fact[x]; i++) {
			l += left[scala[x] - i] + 0.5;
			r += right[scala[x] - i] + 0.5;
		}

		l /= fact[x];
		r /= fact[x];

		l >>= 1;
		r >>= 1;

		if (l >= p)
			l = p - 1;
		if (r >= p)
			r = p - 1;

		lx = wf_left.x + (flip_left ? wf_left.w - x - 1 : x);
		rx = wf_right.x + (flip_right ? wf_right.w - x - 1 : x);

		drawpixel(screen, lx, wf_left.y, &pane[l]);
		drawpixel(screen, rx, wf_right.y, &pane[r]);

		l >>= 1;
		r >>= 1;

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
init_wights(int n)
{
	double	*w;
	int	i;

	w = calloc(n, sizeof(double));
	assert(w);

	for (i = 0; i < n; i++)
		w[i] = (1 - cos((2 * M_PI * i) / (n - 1))) / 2;

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
	const SDL_VideoInfo	*vi;
	SDL_Event       event;

	struct		buf *buf;
	size_t		bufsz;

	struct		sio_hdl	*sio;
	struct		sio_par par;
	struct	fifo	*ff;
	struct	fft	*fft;

	double		*left, *right;
	int		c, i, n, k, r, delta;
	double		*wights;
	int		*scala;
	int		*factor;
	int		psize, ssize;
	int		height = 0;
	int		width = 0;
	int		sdlargs;
	int		pressed = 0;


	#if TIMING
	struct		timeval ta, te;
	#endif

	while ((c = getopt(argc, argv, "h:w:lr")) != -1)
		switch (c) {
		case 'h':
			height = atoi(optarg);
			break;
		case 'w':
			width = atoi(optarg);
			break;
		case 'l':
			flip_left ^= 1;
			break;
		case 'r':
			flip_right ^= 1;
			break;
		default:
			usage();
			break;
		}

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	signal(SIGINT, catch);
	atexit(SDL_Quit);

	vi = SDL_GetVideoInfo();
	if (!vi)
		return 1;
	warnx("%dx%d", vi->current_w, vi->current_h);

	sdlargs = SDL_HWSURFACE | SDL_HWPALETTE |  SDL_DOUBLEBUF;
	if (!height && !width)
		sdlargs |= SDL_FULLSCREEN;
	if (!height)
		height = vi->current_h - 1;
	if (!width)
		width = vi->current_w - 1;

	screen = SDL_SetVideoMode(width, height, 32, sdlargs);
	if (!screen)
		return 1;

	SDL_ShowCursor(SDL_DISABLE);

	psize = 2 * vi->current_h / 3;
	ssize = psize >> 2;

	init_rect(vi->current_w, vi->current_h, 1, ssize);

	pan2 = init_palette_small(ssize);
	pane = init_palette_big(psize);
	
	sio = sio_open(NULL, SIO_REC, 0);
	if (!sio)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&par);
	sio_getpar(sio, &par);
	
	delta = par.round / 2;
	n = 4 * delta;
	warnx("delta: %d, n: %d", delta, n);

	bufsz = delta * sizeof(struct buf);
	buf = malloc(bufsz);
	assert(buf);

	left = calloc(n, sizeof(double));
	right = calloc(n, sizeof(double));
	assert(left && right);

	sio_start(sio);

	fft = init_fft(n);
	wights = init_wights(n);
	ff = init_fifo(10 * n);
	scala = init_scala(n / 2, wf_left.w);
	factor = init_factor(scala, wf_left.w);

	#if TIMING
	gettimeofday(&ta, NULL);
	#endif

	while (!die) {
		k = sio_read(sio, buf, bufsz) / sizeof(struct buf);
		for (i = 0; i < k; i++) {
			left[i] = buf[i].left / (double)INT16_MAX;
			right[i] = buf[i].right / (double)INT16_MAX;
		}
		r = wr_fifo(ff, left, right, k);

		if (r >= n) {
			rd_fifo(ff, left, right, n, wights);

			dofft(fft, left);
			dofft(fft, right);

			draw(left, right, n / 2, scala, factor, psize);
		}

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
			default:
				break;
			}
			pressed = 1;
			break;
		case SDL_KEYUP:
			pressed = 0;
			break;
		}
		#if TIMING
		gettimeofday(&te, NULL);
		fprintf(stderr, "%8d time elapsed: %7.6lf sec\r",
			r, te.tv_sec - ta.tv_sec
			+ (te.tv_usec - ta.tv_usec)/1000000.0);
		memcpy(&ta, &te, sizeof(struct timeval));
		#endif
	}


	sio_stop(sio);
	sio_close(sio);

	free(left);
	free(right);
	free(buf);

	return 0;
}
