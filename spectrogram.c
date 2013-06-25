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

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include "sio.h"
#include "fft.h"
#include "hsv2rgb.h"

#if defined(__linux__)
#ifndef __dead
#define __dead	__attribute__((noreturn))
#endif
#endif

#define	GAP	4
extern	char *__progname;
int	die = 0;

struct	panel {
	Pixmap	pix;		/* main pixmap */
	Pixmap	bg;		/* spectrogram bg */
	Pixmap	mask;		/* spectrogram mask */
	Pixmap	sbg;		/* shadow bg */
	Pixmap	smask;		/* shadow mask */

	GC	pgc;
	GC	mgc;
	GC	sgc;

	XRectangle p;		/* panel */
	XRectangle w;		/* waterfall */
	XRectangle s;		/* spectrogram */

	int	mirror;
	double	*data;
	int	*shadow;
	int	maxval;

	unsigned long *palette;
};

unsigned long
hsvcolor(Display *d, float h, float s, float v)
{
	float r, g, b;
	int scr = DefaultScreen(d);
	Colormap cmap = DefaultColormap(d, scr);
	XColor c;

	hsv2rgb(&r, &g, &b, h, s, v);

	c.red = UINT16_MAX * r;
	c.green = UINT16_MAX * g;
	c.blue = UINT16_MAX * b;
	c.flags = DoRed|DoGreen|DoBlue;

	XAllocColor(d, cmap, &c);

	return c.pixel;
}

unsigned long *
init_palette(Display *d, float h, float dh, float s, float ds, float v, float dv, int n, int lg)
{
	unsigned long *p;
	float	hstep, sstep, vstep, vv;
	int	i;

	p = calloc(n, sizeof(unsigned long));
	if (!p)
		errx(1, "malloc failed");

	hstep = (dh - h) / n;
	sstep = (ds - s) / n;
	vstep = (dv - v) / n;

	for (i = 0; i < n; i++) {
		vv = lg ? logf(100 * v + 1) / logf(101) : v;
		p[i] = hsvcolor(d, h, s, vv);
		h += hstep;
		s += sstep;
		v += vstep;
	}

	return p;
}

void
catch(int notused)
{
	die = 1;
}

__dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-hsd]\n", __progname);
	fprintf(stderr, "\t-h\tthis help\n");
	fprintf(stderr, "\t-d\tdon't fork\n");

	exit(0);
}

void
init_bg(Display *d, Pixmap pix, GC gc, int w, int h, unsigned long *pal)
{
	int i;

	for (i = 0; i < h; i++) {
		XSetForeground(d, gc, pal[i]);
		XDrawLine(d, pix, gc, 0, h - i - 1, w - 1, h - i - 1);
	}
}

void
draw_panel(Display *d, struct panel *p)
{
	int i, v, x;

	/* blit waterfall */
	XCopyArea(d, p->pix, p->pix, p->pgc,
		0, 1, p->w.width, p->w.height, 0, 0);

	/* clear spectrogram */
	XSetForeground(d, p->pgc, p->palette[0]);
	XFillRectangle(d, p->pix, p->pgc,
		p->s.x, p->s.y, p->s.width, p->s.height);

	/* clear mask */
	XSetForeground(d, p->mgc, 0);
	XFillRectangle(d, p->mask, p->mgc,
		0, 0, p->s.width, p->s.height);

	/* clear shadow mask */
	XSetForeground(d, p->sgc, 0);
	XFillRectangle(d, p->smask, p->sgc,
		0, 0, p->s.width, p->s.height);

	for (i = 0; i < p->p.width; i++) {
		/* limit maxval */
		v = p->data[i] >= p->maxval ? p->maxval - 1 : p->data[i];
		x = p->mirror ? p->p.width - i - 1 : i;

		/* draw waterfall */
		XSetForeground(d, p->pgc, p->palette[v]);
		XDrawPoint(d, p->pix, p->pgc,
			x, p->w.height);

		/* draw spectrogram */
		XSetForeground(d, p->mgc, 1);
		XDrawLine(d, p->mask, p->mgc,
			x, p->s.height - v,
			x, p->s.height);

		/* draw schadow */
		if (p->shadow[i] < v)
			p->shadow[i] = v;
		else if (p->shadow[i] > 0) {
			XSetForeground(d, p->sgc, 1);
			XDrawLine(d, p->smask, p->sgc,
				x, p->s.height - p->shadow[i]--,
				x, p->s.height);
		}
	}

	/* apply mask */
	XSetClipOrigin(d, p->pgc, p->s.x, p->s.y);

	/* shadow */
	XSetClipMask(d, p->pgc, p->smask);
	XCopyArea(d, p->sbg, p->pix, p->pgc, 0, 0, p->s.width, p->s.height,
		p->s.x, p->s.y);

	/* spectrogram */
	XSetClipMask(d, p->pgc, p->mask);
	XCopyArea(d, p->bg, p->pix, p->pgc, 0, 0, p->s.width, p->s.height,
		p->s.x, p->s.y);

	/* reset mask */
	XSetClipMask(d, p->pgc, None);
}

struct panel *
init_panel(Display *d, Window win, int w, int h, int mirror)
{
	struct panel *p;
	int scr = DefaultScreen(d);
	int planes = DisplayPlanes(d, scr);
	unsigned long *bgpalette, *shpalette;

	p = malloc(sizeof(struct panel));
	if (!p)
		errx(1, "malloc failed");

	p->mirror = mirror;

	/* whole panel */
	p->p.x = 0;
	p->p.y = 0;
	p->p.width = w;
	p->p.height = h;

	/* waterfall */
	p->w.x = 0;
	p->w.y = 0;
	p->w.width = w;
	p->w.height = h * 0.75;

	/* spectrogram */
	p->s.x = 0;
	p->s.y = p->w.height + 1;
	p->s.width = w;
	p->s.height = h * 0.25;

	p->data = calloc(w, sizeof(double));
	p->shadow = calloc(w, sizeof(int));
	p->maxval = p->s.height;

	if (!p->data || !p->shadow)
		errx(1, "malloc failed");

	p->pix = XCreatePixmap(d, win, w, h, planes);
	p->bg = XCreatePixmap(d, p->pix, p->s.width, p->s.height, planes);
	p->mask = XCreatePixmap(d, p->pix, p->s.width, p->s.height, 1);

	p->sbg = XCreatePixmap(d, p->pix, p->s.width, p->s.height, planes);
	p->smask = XCreatePixmap(d, p->pix, p->s.width, p->s.height, 1);

	p->pgc = XCreateGC(d, p->pix, 0, NULL);	
	p->mgc = XCreateGC(d, p->mask, 0, NULL);	
	p->sgc = XCreateGC(d, p->smask, 0, NULL);	

	p->palette = init_palette(d, 0.65, 0.35, 1.0, 0.0, 0.0, 1.0,
		p->maxval, 1);
	bgpalette = init_palette(d, 0.3, 0.0, 0.5, 1.0, 0.75, 1.0,
		p->maxval, 0);
	shpalette = init_palette(d, 0.3, 0.0, 0.5, 1.0, 0.1, 0.15,
		p->maxval, 0);

	init_bg(d, p->bg, p->pgc, p->s.width, p->s.height, bgpalette);
	init_bg(d, p->sbg, p->pgc, p->s.width, p->s.height, shpalette);

	/* clear all */
	XSetForeground(d, p->pgc, p->palette[0]);
	XFillRectangle(d, p->pix, p->pgc,
		p->p.x, p->p.y, p->p.width, p->p.height);

	free(bgpalette);
	free(shpalette);
	
	return p;
}

void
del_panel(Display *d, struct panel *p)
{
	XFreePixmap(d, p->pix);
	XFreePixmap(d, p->bg);
	XFreePixmap(d, p->mask);
	XFreeGC(d, p->pgc);
	XFreeGC(d, p->mgc);
	free(p->data);
	free(p->shadow);
	free(p->palette);
	free(p);
}

int 
main(int argc, char **argv)
{
	Display		*dsp;
	Window		win;
	Atom		delwin;
	int		scr;

	struct		panel *left, *right;
	struct		sio *sio;
	struct		fft *fft;
	int16_t		*buffer;

	int		ch, dflag = 1;
	int		delta;
	int		width, height;
	unsigned long	black, white;

	while ((ch = getopt(argc, argv, "hd")) != -1)
		switch (ch) {
		case 'd':
			dflag = 0;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	signal(SIGINT, catch);
		
	dsp = XOpenDisplay(NULL);
	if (!dsp)
		errx(1, "Cannot connect to X11 server");

	sio = init_sio(2, 16, 1);

	if (dflag)
		daemon(0, 0);

	delta = get_round(sio);
	width = delta + GAP;
	height = 0.75 * width;

	scr = DefaultScreen(dsp);
	white = WhitePixel(dsp, scr);
	black = BlackPixel(dsp, scr);

	win = XCreateSimpleWindow(dsp, RootWindow(dsp, scr), 0, 0,
		width, height, 2, white, black);
	XClearWindow(dsp, win);
		
	XStoreName(dsp, win, __progname);
	XSelectInput(dsp, win, KeyPressMask);

	delwin = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(dsp, win, &delwin, 1);

	left = init_panel(dsp, win, delta / 2, height, 1);
	right = init_panel(dsp, win, delta / 2, height, 0);
	fft = init_fft(delta);

	XMapWindow(dsp, win);

	while (!die) {
		buffer = read_sio(sio);

		dofft(fft, buffer, left->data, 0);
		dofft(fft, buffer, right->data, 1);
		draw_panel(dsp, left);
		draw_panel(dsp, right);

		/* flip */
		XCopyArea(dsp, left->pix, win, left->pgc, 0, 0,
			left->p.width, left->p.height,
			0, 0);
		XCopyArea(dsp, right->pix, win, right->pgc, 0, 0,
			right->p.width, right->p.height,
			right->p.width + GAP, 0);

		while (XPending(dsp)) {
			XEvent ev;

			XNextEvent(dsp, &ev);

			switch (ev.type) {
			case KeyPress:
				switch (XLookupKeysym(&ev.xkey, 0)) {
				case XK_q:
					die = 1;
					break;
				default:
					break;
				}
				break;
			case ClientMessage:
				die = *ev.xclient.data.l == delwin;
				break;
			default:
				break;
			}
		}
	}

	del_sio(sio);
	del_fft(fft);
	del_panel(dsp, left);
	del_panel(dsp, right);
	XCloseDisplay(dsp);

	return 0;
}
