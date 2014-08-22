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
#include <X11/Xutil.h>
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

#define	HGAP	4
#define	VGAP	1

extern	char *__progname;
int	die = 0;

struct pixmap {
	Pixmap	pix;
	GC	gc;
};

struct	panel {
	Window	win;		/* container */

	Window	wf;		/* waterfall */
	XRectangle w;
	struct	pixmap wfbuf;

	Window	sp;		/* spectrogram */
	XRectangle s;
	struct	pixmap spbuf;
	struct	pixmap spbg;
	struct	pixmap spmask;
	struct	pixmap shbg;
	struct	pixmap shmask;

	int	mirror;
	int	maxval;
	double	*data;
	unsigned long *palette;
};

struct hsv {
	float h, s, v;
};

struct palette {
	struct hsv from, to;
};

enum scale { LIN_SCALE, LOG_SCALE };

unsigned long
hsvcolor(Display *d, struct hsv hsv, enum scale scale)
{
	float r, g, b;
	int scr = DefaultScreen(d);
	Colormap cmap = DefaultColormap(d, scr);
	XColor c;

	if (scale == LOG_SCALE)
		hsv.v = logf(100 * hsv.v + 1) / logf(101);

	hsv2rgb(&r, &g, &b, hsv.h, hsv.s, hsv.v);

	c.red = UINT16_MAX * r;
	c.green = UINT16_MAX * g;
	c.blue = UINT16_MAX * b;
	c.flags = DoRed|DoGreen|DoBlue;

	XAllocColor(d, cmap, &c);

	return c.pixel;
}

unsigned long *
init_palette(Display *d, struct palette *pal, int n, enum scale scale)
{
	unsigned long *p;
	float	hstep, sstep, vstep;
	int	i;

	p = calloc(n, sizeof(unsigned long));
	if (!p)
		errx(1, "malloc failed");

	hstep = (pal->to.h - pal->from.h) / n;
	sstep = (pal->to.s - pal->from.s) / n;
	vstep = (pal->to.v - pal->from.v) / n;

	for (i = 0; i < n; i++) {
		p[i] = hsvcolor(d, pal->from, scale);
		pal->from.h += hstep;
		pal->from.s += sstep;
		pal->from.v += vstep;
	}

	return p;
}

void
catch(int notused)
{
	die = 1;
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s [-dh]\n", __progname);
	fprintf(stderr, "\t-d\tdaemonize\n");
	fprintf(stderr, "\t-f\tfullscreen\n");
	fprintf(stderr, "\t-h\tthis help\n");

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
	XCopyArea(d, p->wfbuf.pix, p->wfbuf.pix, p->wfbuf.gc,
		0, 0, p->w.width, p->w.height - 1, 0, 1);

	/* blit shadow mask */
	XCopyArea(d, p->shmask.pix, p->shmask.pix, p->shmask.gc,
		0, 0, p->s.width, p->s.height - 1, 0, 1);

	/* clear spectrogram */
	XSetForeground(d, p->spbuf.gc, p->palette[0]);
	XFillRectangle(d, p->spbuf.pix, p->spbuf.gc,
		0, 0, p->s.width, p->s.height);

	/* clear mask */
	XSetForeground(d, p->spmask.gc, 0);
	XFillRectangle(d, p->spmask.pix, p->spmask.gc,
		0, 0, p->s.width, p->s.height);

	for (i = 0; i < p->s.width; i++) {
		/* limit maxval */
		v = p->data[i] >= p->maxval ? p->maxval - 1 : p->data[i];
		x = p->mirror ? p->s.width - i - 1 : i;

		/* draw waterfall */
		XSetForeground(d, p->wfbuf.gc, p->palette[v]);
		XDrawPoint(d, p->wfbuf.pix, p->wfbuf.gc, x, 0);

		/* draw spectrogram */
		XSetForeground(d, p->spmask.gc, 1);
		XDrawLine(d, p->spmask.pix, p->spmask.gc,
			x, p->s.height - v,
			x, p->s.height);
	}

	/* copy mask to shadow mask */
	XSetClipMask(d, p->shmask.gc, p->spmask.pix);
	XCopyArea(d, p->spmask.pix, p->shmask.pix, p->shmask.gc,
		0, 0, p->s.width, p->s.height, 0, 0);
	XSetClipMask(d, p->shmask.gc, None);

	/* shadow */
	XSetClipMask(d, p->spbuf.gc, p->shmask.pix);
	XCopyArea(d, p->shbg.pix, p->spbuf.pix, p->spbuf.gc,
		0, 0, p->s.width, p->s.height, 0, 0);

	/* spectrogram */
	XSetClipMask(d, p->spbuf.gc, p->spmask.pix);
	XCopyArea(d, p->spbg.pix, p->spbuf.pix, p->spbuf.gc,
		0, 0, p->s.width, p->s.height, 0, 0);

	/* flip spectrogram */
	XSetClipMask(d, p->spbuf.gc, None);
	XCopyArea(d, p->spbuf.pix, p->sp, p->spbuf.gc, 0, 0,
		p->w.width, p->w.height, 0, 0);

	/* flip waterfall */
	XCopyArea(d, p->wfbuf.pix, p->wf, p->wfbuf.gc, 0, 0,
		p->w.width, p->w.height, 0, 0);

}

void
init_pixmap(struct pixmap *p, Display *d, Drawable dr, XRectangle r, int pl)
{
	p->pix = XCreatePixmap(d, dr, r.width, r.height, pl);
	p->gc = XCreateGC(d, p->pix, 0, NULL);	
}

struct panel *
init_panel(Display *d, Window win, int x, int y, int w, int h, int mirror)
{
	struct panel *p;
	int scr = DefaultScreen(d);
	int planes = DisplayPlanes(d, scr);
	unsigned long white = WhitePixel(d, scr);
	unsigned long black = BlackPixel(d, scr);
	struct hsv hsv_gray = {0.00, 0.00, 0.15};
	unsigned long gray = hsvcolor(d, hsv_gray, LIN_SCALE);
	unsigned long *palette;

	p = malloc(sizeof(struct panel));
	if (!p)
		errx(1, "malloc failed");

	p->data = calloc(w, sizeof(double));
	if (!p->data)
		errx(1, "malloc failed");

	/* main panel window */
	p->win = XCreateSimpleWindow(d, win, x, y, w, h, 0, white, gray);

	/* sperctrogram window and its bitmasks */
	p->s.x = 0;
	p->s.y = 0;
	p->s.width = w;
	p->s.height = h * 0.25;

	p->sp = XCreateSimpleWindow(d, p->win, p->s.x, p->s.y,
		p->s.width, p->s.height, 0, white, black);

	init_pixmap(&p->spbuf, d, p->sp, p->s, planes);
	init_pixmap(&p->spbg, d, p->sp, p->s, planes);
	init_pixmap(&p->spmask, d, p->sp, p->s, 1);
	init_pixmap(&p->shbg, d, p->sp, p->s, planes);
	init_pixmap(&p->shmask, d, p->sp, p->s, 1);

	/* waterfall window and double buffer */
	p->w.x = 0;
	p->w.y = p->s.height + VGAP;
	p->w.width = w;
	p->w.height = h - p->w.y;

	p->wf = XCreateSimpleWindow(d, p->win, p->w.x, p->w.y,
		p->w.width, p->w.height, 0, white, black);

	init_pixmap(&p->wfbuf, d, p->wf, p->w, planes);

	p->maxval = p->s.height;
	p->mirror = mirror;

	struct palette p_spectr = {
		{ 0.30, 0.30, 0.50 },
		{ 0.00, 1.00, 1.00 }
	};

	palette = init_palette(d, &p_spectr, p->maxval, LIN_SCALE);
	init_bg(d, p->spbg.pix, p->spbg.gc, p->s.width, p->s.height, palette);
	free(palette);

	struct palette p_shadow = {
		{ 0.30, 0.30, 0.10 },
		{ 0.00, 1.00, 0.20 }
	};

	palette = init_palette(d, &p_shadow, p->maxval, LIN_SCALE);
	init_bg(d, p->shbg.pix, p->shbg.gc, p->s.width, p->s.height, palette);
	free(palette);

	struct palette p_waterfall = {
		{ 0.55, 1.00, 0.00 },
		{ 0.45, 0.00, 1.00 }
	};

	p->palette = init_palette(d, &p_waterfall, p->maxval, LOG_SCALE);

	/* clear waterfall */
	XSetForeground(d, p->wfbuf.gc, p->palette[0]);
	XFillRectangle(d, p->wfbuf.pix, p->wfbuf.gc,
		0, 0, p->w.width, p->w.height);

	/* clear shadow mask */
	XSetForeground(d, p->shmask.gc, 0);
	XFillRectangle(d, p->shmask.pix, p->shmask.gc,
		0, 0, p->s.width, p->s.height);

	XMapWindow(d, p->wf);
	XMapWindow(d, p->sp);
	XMapWindow(d, p->win);
	
	return p;
}

void
del_panel(Display *d, struct panel *p)
{
	free(p->data);
	free(p->palette);

	XFreePixmap(d, p->shmask.pix);
	XFreeGC(d, p->shmask.gc);

	XFreePixmap(d, p->shbg.pix);
	XFreeGC(d, p->shbg.gc);

	XFreePixmap(d, p->spmask.pix);
	XFreeGC(d, p->spmask.gc);

	XFreePixmap(d, p->spbg.pix);
	XFreeGC(d, p->spbg.gc);

	XFreePixmap(d, p->spbuf.pix);
	XFreeGC(d, p->spbuf.gc);

	XDestroyWindow(d, p->sp);

	XFreePixmap(d, p->wfbuf.pix);
	XFreeGC(d, p->wfbuf.gc);

	XDestroyWindow(d, p->wf);

	XDestroyWindow(d, p->win);
	
	free(p);
}

int
move(Display *dsp, Window win, Window container)
{
	int dx, dy;
	XWindowAttributes wa, wac;

	XGetWindowAttributes(dsp, win, &wa);
	XGetWindowAttributes(dsp, container, &wac);

	dx = (wa.width - wac.width) / 2;
	dy = (wa.height - wac.height) / 2;
	if (dy < 0)
		dy = 0;

	XMoveWindow(dsp, container, dx, dy);

	return 0;
}

int
gofullscreen(Display *d, Window win)
{
	XEvent xev;
	Atom state = XInternAtom(d, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);

	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = win;
	xev.xclient.message_type = state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = fullscreen;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	return XSendEvent(d, DefaultRootWindow(d), False,
		SubstructureRedirectMask|SubstructureNotifyMask, &xev);
}

int 
main(int argc, char **argv)
{
	Display		*dsp;
	Window		win, container;
	Atom		delwin;
	Atom		nhints;
	XSizeHints	*hints;
	XClassHint	*class;
	XWindowAttributes wa;
	int		scr;

	struct		panel *left, *right;
	struct		sio *sio;
	struct		fft *fft;
	int16_t		*buffer;

	int		ch;
	int		dflag = 0;
	int		fflag = 0;
	int		width, height;
	unsigned long	black, white;
	float		factor = 0.75;
	int		round = 1024;	/* FFT is fastest with powers of two */
	while ((ch = getopt(argc, argv, "dfh")) != -1)
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	signal(SIGINT, catch);

	if (dflag)
		daemon(0, 0);
		
	dsp = XOpenDisplay(NULL);
	if (!dsp)
		errx(1, "Cannot connect to X11 server");

	if (fflag) {
		XGetWindowAttributes(dsp, DefaultRootWindow(dsp), &wa);
		round = wa.width - HGAP;
		width = wa.width;
		height = wa.height;
	} else {
		width = round + HGAP;
		height = factor * width;
	}

	sio = init_sio(round);

	scr = DefaultScreen(dsp);
	white = WhitePixel(dsp, scr);
	black = BlackPixel(dsp, scr);

	win = XCreateSimpleWindow(dsp, RootWindow(dsp, scr),
		0, 0, width, height, 0, white, black);

	if (fflag && gofullscreen(dsp, win) != Success) {
		fprintf(stderr, "failed to go fullscreen, trying resize\n");
		XMoveResizeWindow(dsp, win, 0, 0, wa.width, wa.height);
	}
		
	XStoreName(dsp, win, __progname);
	class = XAllocClassHint();
	if (class) {
		class->res_name = __progname;
		class->res_class = __progname;
		XSetClassHint(dsp, win, class);
		XFree(class);
	}
	XSelectInput(dsp, win, KeyPressMask|StructureNotifyMask);

	/* catch delete window */
	delwin = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(dsp, win, &delwin, 1);

	/* set minimal size */
	nhints = XInternAtom(dsp, "WM_NORMAL_HINTS", 0);
	hints = XAllocSizeHints();
	hints->flags = PMinSize;
	hints->min_width = width;
	hints->min_height = height;
	XSetWMSizeHints(dsp, win, hints, nhints);
	XFree(hints);

	container = XCreateSimpleWindow(dsp, win,
		0, 0, width, height, 0, white, black);
	XMapWindow(dsp, container);

	left = init_panel(dsp, container, 0, 0, round / 2, height, 1);
	right = init_panel(dsp, container, round / 2 + HGAP, 0, round / 2, height, 0);
	fft = init_fft(round);

	XClearWindow(dsp, win);
	XMapWindow(dsp, win);

	while (!die) {
		buffer = read_sio(sio, round);

		dofft(fft, buffer, left->data, 0);
		dofft(fft, buffer, right->data, 1);
		draw_panel(dsp, left);
		draw_panel(dsp, right);

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
			case ConfigureNotify:
				move(dsp, win, container);
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

	XDestroyWindow(dsp, win);
	XCloseDisplay(dsp);

	return 0;
}
