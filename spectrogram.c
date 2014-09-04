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

#include <assert.h>
#include <err.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cms.h"
#include "fft.h"
#include "sio.h"

#define	HGAP	4
#define	VGAP	1

extern	char *__progname;
int	die = 0;

struct pixmap {
	Pixmap	pix;
	GC	gc;
};

struct background {
	Pixmap	pix;
	Pixmap	mask;
	GC	gc;
	XRectangle geo;
};

struct subwin {
	Window	win;
	Pixmap	pix;		/* buffer */
	GC	gc;
	XRectangle geo;
};

struct	panel {
	Window	win;		/* container */
	struct subwin *wf;
	struct subwin *sp;
	struct background *bg;
	struct background *shadow;
	int	mirror;
	int	maxval;
	double	*data;
	unsigned long *palette;
};

struct hsl {
	float h, s, l;
};

struct palette {
	struct hsl from, to;
};

enum mirror { LTR, RTL };

struct palette p_spectr =    {{ 120.0, 100.0,  75.0 }, {   0.0, 100.0,  25.0 }};
struct palette p_shadow =    {{ 120.0, 100.0,  10.0 }, {   0.0, 100.0,  10.0 }};
struct palette p_waterfall = {{ 210.0,  75.0,   0.0 }, { 180.0, 100.0, 100.0 }};
struct hsl hsl_gray = {0.0, 0.0, 20.0};
unsigned long *sp_pal = NULL;
unsigned long *sh_pal = NULL;
unsigned long *wf_pal = NULL;

unsigned long
hslcolor(Display *d, struct hsl hsl)
{
	int scr = DefaultScreen(d);
	Colormap cmap = DefaultColormap(d, scr);
	XColor c;

	hsl2rgb(&c.red, &c.green, &c.blue, hsl.h, hsl.s, hsl.l);
	c.flags = DoRed|DoGreen|DoBlue;

	XAllocColor(d, cmap, &c);

	return c.pixel;
}

unsigned long *
init_palette(Display *d, struct palette pal, int n)
{
	unsigned long *p;
	float	hstep, sstep, lstep;
	int	i;

	p = calloc(n, sizeof(unsigned long));
	assert(p);

	hstep = (pal.to.h - pal.from.h) / (n - 1);
	sstep = (pal.to.s - pal.from.s) / (n - 1);
	lstep = (pal.to.l - pal.from.l) / (n - 1);

	for (i = 0; i < n; i++) {
		p[i] = hslcolor(d, pal.from);
		pal.from.h += hstep;
		pal.from.s += sstep;
		pal.from.l += lstep;
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
	#define USGFMT "\t%-12s%s\n"
	fprintf(stderr, "Usage: %s [-dfph] [-r <round>]\n", __progname);
	fprintf(stderr, USGFMT, "-d", "daemonize");
	fprintf(stderr, USGFMT, "-f", "fullscreen mode");
	fprintf(stderr, USGFMT, "-p", "don't hide pointer in fullscreen mode");
	fprintf(stderr, USGFMT, "-r <round>", "sio round count");
	fprintf(stderr, USGFMT, "-h", "this help");

	exit(0);
}

void
blit(Display *d, Drawable p, GC gc, XRectangle r)
{
	XCopyArea(d, p, p, gc, 0, 0, r.width, r.height - 1, 0, 1);
}

void
clear(Display *d, Drawable p, GC gc, XRectangle r)
{
	XSetForeground(d, gc, BlackPixel(d, DefaultScreen(d)));
	XFillRectangle(d, p, gc, 0, 0, r.width, r.height);
}

void
copy(Display *d, Drawable from, Drawable to, GC gc, XRectangle r, Drawable mask)
{
	XSetClipMask(d, gc, mask);
	XCopyArea(d, from, to, gc, 0, 0, r.width, r.height, 0, 0);
	XSetClipMask(d, gc, None);
}

void
draw_panel(Display *d, struct panel *p)
{
	int i, v, x;

	/* blit waterfall */
	blit(d, p->wf->pix, p->wf->gc, p->wf->geo);
	/* blit shadow mask */
	blit(d, p->shadow->mask, p->shadow->gc, p->shadow->geo);


	/* clear spectrogram */
	clear(d, p->sp->pix, p->sp->gc, p->sp->geo);
	/* clear mask */
	clear(d, p->bg->mask, p->bg->gc, p->bg->geo);

	for (i = 0; i < p->sp->geo.width; i++) {
		/* limit maxval */
		v = p->data[i] >= p->maxval ? p->maxval - 1 : p->data[i];
		x = p->mirror ? p->sp->geo.width - i - 1 : i;

		/* draw waterfall */
		XSetForeground(d, p->wf->gc, p->palette[v]);
		XDrawPoint(d, p->wf->pix, p->wf->gc, x, 0);

		/* draw spectrogram */
		XSetForeground(d, p->bg->gc, 1);
		XDrawLine(d, p->bg->mask, p->bg->gc,
			x, p->bg->geo.height - v,
			x, p->bg->geo.height);
	}

	/* copy mask to shadow mask */
	copy(d, p->bg->mask, p->shadow->mask, p->shadow->gc, p->shadow->geo,
		p->bg->mask);
	/* shadow to buffer */
	copy(d, p->shadow->pix, p->sp->pix, p->sp->gc, p->sp->geo,
		p->shadow->mask);
	/* spectrogram to buffer */
	copy(d, p->bg->pix, p->sp->pix, p->sp->gc, p->sp->geo,
		p->bg->mask);
}

void
flip(Display *d, struct subwin *p)
{
	XCopyArea(d, p->pix, p->win, p->gc,
		0, 0, p->geo.width, p->geo.height, 0, 0);
}

void
flip_panel(Display *d, struct panel *p)
{
	/* flip spectrogram */
	flip(d, p->sp);
	/* flip waterfall */
	flip(d, p->wf);
}

void
init_bg(Display *d, Pixmap pix, GC gc, XRectangle r, unsigned long *pal)
{
	int i, x, y;

	x = r.width - 1;
	for (i = 0; i < r.height; i++) {
		y = r.height - i - 1;
		XSetForeground(d, gc, pal[i]);
		XDrawLine(d, pix, gc, 0, y, x, y);
	}
}

struct background *
init_background(Display *d, Drawable parent, XRectangle r)
{
	struct background *p;
	int scr = DefaultScreen(d);
	int planes = DisplayPlanes(d, scr);

	p = malloc(sizeof(struct subwin));
	assert(p);

	p->pix = XCreatePixmap(d, parent, r.width, r.height, planes);
	p->mask = XCreatePixmap(d, parent, r.width, r.height, 1);
	p->gc = XCreateGC(d, p->mask, 0, NULL);	
	p->geo = r;

	XSetForeground(d, p->gc, 0);
	XFillRectangle(d, p->mask, p->gc, 0, 0, r.width, r.height);

	return p;
}

struct subwin *
init_subwin(Display *d, Drawable parent, XRectangle r)
{
	struct subwin *p;
	int scr = DefaultScreen(d);
	int white = WhitePixel(d, scr);
	int black = BlackPixel(d, scr);
	int planes = DisplayPlanes(d, scr);

	p = malloc(sizeof(struct subwin));
	assert(p);

	p->win = XCreateSimpleWindow(d, parent, r.x, r.y,
		r.width, r.height, 0, white, black);
	p->pix = XCreatePixmap(d, p->win, r.width, r.height, planes);
	p->gc = XCreateGC(d, p->pix, 0, NULL);	
	p->geo = r;

	XSetForeground(d, p->gc, black);
	XFillRectangle(d, p->pix, p->gc, 0, 0, r.width, r.height);

	XMapWindow(d, p->win);
	
	return p;
}

struct panel *
init_panel(Display *d, Window win, XRectangle r, enum mirror m)
{
	struct panel *p;
	int scr = DefaultScreen(d);
	int planes = DisplayPlanes(d, scr);
	unsigned long white = WhitePixel(d, scr);
	unsigned long black = BlackPixel(d, scr);
	unsigned long gray = hslcolor(d, hsl_gray);
	XRectangle geo;

	p = malloc(sizeof(struct panel));
	assert(p);

	p->data = calloc(r.width, sizeof(double));
	assert(p->data);

	/* main panel window */
	p->win = XCreateSimpleWindow(d, win,
		r.x, r.y, r.width, r.height,
		0, white, gray);

	/* sperctrogram window and its bitmasks */
	geo.x = 0;
	geo.y = 0;
	geo.width = r.width;
	geo.height = r.height / 4;
	p->sp = init_subwin(d, p->win, geo);
	p->bg = init_background(d, p->sp->win, geo);
	p->shadow = init_background(d, p->sp->win, geo);

	/* waterfall window and double buffer */
	geo.x = 0;
	geo.y = p->sp->geo.height + VGAP;
	geo.width = r.width;
	geo.height = r.height - p->sp->geo.y;
	p->wf = init_subwin(d, p->win, geo);

	p->maxval = p->sp->geo.height;
	p->mirror = m;

	if (!sp_pal)
		sp_pal = init_palette(d, p_spectr, p->maxval);
	init_bg(d, p->bg->pix, p->sp->gc, p->bg->geo, sp_pal);

	if (!sh_pal)
		sh_pal = init_palette(d, p_shadow, p->maxval);
	init_bg(d, p->shadow->pix, p->sp->gc, p->shadow->geo, sh_pal);

	if (!wf_pal)
		wf_pal = init_palette(d, p_waterfall, p->maxval);
	p->palette = wf_pal;

	XMapWindow(d, p->win);
	
	return p;
}

void
free_background(Display *d, struct background *p)
{
	XFreePixmap(d, p->pix);
	XFreePixmap(d, p->mask);
	XFreeGC(d, p->gc);
}

void
free_subwin(Display *d, struct subwin *p)
{
	XFreePixmap(d, p->pix);
	XFreeGC(d, p->gc);
	XUnmapWindow(d, p->win);
	XDestroyWindow(d, p->win);
}

void
free_panel(Display *d, struct panel *p)
{
	free_background(d, p->bg);
	free_background(d, p->shadow);
	free_subwin(d, p->sp);
	free_subwin(d, p->wf);
	XUnmapWindow(d, p->win);
	XDestroyWindow(d, p->win);
	free(p->data);
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

void
gofullscreen(Display *d, Window win)
{
	XClientMessageEvent cm;

	bzero(&cm, sizeof(cm));
	cm.type = ClientMessage;
	cm.send_event = True;
	cm.message_type = XInternAtom(d, "_NET_WM_STATE", False);
	cm.window = win;
	cm.format = 32;
	cm.data.l[0] = 1;	/* _NET_WM_STATE_ADD */
	cm.data.l[1] = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);
	cm.data.l[2] = 0;	/* no secondary property */
	cm.data.l[3] = 1;	/* normal application */

	XSendEvent(d, DefaultRootWindow(d), False, NoEventMask, (XEvent *)&cm);
	XMoveWindow(d, win, 0, 0);
}

void
hide_ptr(Display *d, Window win)
{
	Pixmap		bm;
	Cursor		ptr;
	Colormap	cmap;
	XColor		black, dummy;
	static char empty[] = {0, 0, 0, 0, 0, 0, 0, 0};

	cmap = DefaultColormap(d, DefaultScreen(d));
	XAllocNamedColor(d, cmap, "black", &black, &dummy);
	bm = XCreateBitmapFromData(d, win, empty, 8, 8);
	ptr = XCreatePixmapCursor(d, bm, bm, &black, &black, 0, 0);

	XDefineCursor(d, win, ptr);
	XFreeCursor(d, ptr);
	if (bm != None)
		XFreePixmap(d, bm);
	XFreeColors(d, cmap, &black.pixel, 1, 0);
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
	XRectangle	geo;
	int		scr;

	struct		panel *left, *right;
	struct		sio *sio;
	struct		fft *fft;
	int16_t		*buffer;

	int		dflag = 0;	/* daemonize */
	int		fflag = 0;	/* fullscreen */
	int		pflag = 1;	/* hide ptr */

	int		ch;
	int		width, height;
	unsigned int	maxwidth, maxheight;
	unsigned long	black, white;
	float		factor = 0.75;
	int		round = 1024;	/* FFT is fastest with powers of two */

	while ((ch = getopt(argc, argv, "dfpr:h")) != -1)
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'p':
			pflag = 0;
			break;
		case 'r':
			round = atoi(optarg);
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

	sio = init_sio();
	maxwidth = max_samples_sio(sio);
	maxheight = wa.height;
		
	dsp = XOpenDisplay(NULL);
	if (!dsp)
		errx(1, "Cannot connect to X11 server");

	if (round > maxwidth)
		round = maxwidth;

	XGetWindowAttributes(dsp, DefaultRootWindow(dsp), &wa);
	width = round + HGAP;
	if (fflag || width > wa.width) {
		round = wa.width - HGAP;
		width = wa.width;
	}

	height = factor * width;
	if (height > wa.height)
		height = wa.height;

	scr = DefaultScreen(dsp);
	white = WhitePixel(dsp, scr);
	black = BlackPixel(dsp, scr);

	win = XCreateSimpleWindow(dsp, RootWindow(dsp, scr),
		0, 0, width, height, 0, white, black);
		
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
	hints->flags = PMinSize|PMaxSize;
	hints->min_width = width;
	hints->min_height = height;
	hints->max_width = maxwidth;
	hints->max_height = maxheight;
	XSetWMSizeHints(dsp, win, hints, nhints);
	XFree(hints);

	container = XCreateSimpleWindow(dsp, win,
		0, 0, width, height, 0, white, black);
	XMapWindow(dsp, container);

	fft = init_fft(round);

	geo.x = 0;
	geo.y = 0;
	geo.width = round / 2;
	geo.height = height;
	left = init_panel(dsp, container, geo, RTL);

	geo.x = round / 2 + HGAP;
	geo.y = 0;
	geo.width = round / 2;
	geo.height = height;
	right = init_panel(dsp, container, geo, LTR);

	free(sp_pal);
	free(sh_pal);

	XClearWindow(dsp, win);
	XMapRaised(dsp, win);	/* XMapWindow */

	if (fflag) {
		gofullscreen(dsp, win);
		if (pflag)
			hide_ptr(dsp, win);
	}

	while (!die) {
		while (XPending(dsp)) {
			XEvent ev;

			XNextEvent(dsp, &ev);

			switch (ev.type) {
			case KeyPress:
				switch (XLookupKeysym(&ev.xkey, 0)) {
				case XK_q:
					die = 1;
					break;
				case XK_1:
					left->mirror ^= 1;
					break;
				case XK_2:
					right->mirror ^= 1;
					break;
				case XK_3:
					left->mirror ^= 1;
					right->mirror ^= 1;
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

		buffer = read_sio(sio, round);

		exec_fft(fft, buffer, left->data, FFT_LEFT);
		exec_fft(fft, buffer, right->data, FFT_RIGHT);

		draw_panel(dsp, left);
		draw_panel(dsp, right);

		flip_panel(dsp, left);
		flip_panel(dsp, right);

		if (fflag)
			XResetScreenSaver(dsp);
	}

	free_sio(sio);
	free_fft(fft);
	free_panel(dsp, left);
	free_panel(dsp, right);
	free(wf_pal);

	XDestroyWindow(dsp, win);
	XCloseDisplay(dsp);

	return 0;
}
