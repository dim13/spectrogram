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
#include <sndio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include "fft.h"
#include "hsv2rgb.h"

#define PSIZE	250
#define SSIZE	(PSIZE >> 1)
#define GAP	2

extern		char *__progname;

Display		*dsp;
Window		win;
Colormap	cmap;
GC		gc;
Pixmap		pix, bg;
int		width, height;

unsigned long	*wf, *sp, black, white;

XRectangle	wf_from, wf_to;		/* waterfall blit */
XRectangle	wf_left, wf_right;	/* waterfall */
XRectangle	sp_left, sp_right;	/* spectrogram */

int	die = 0;

void
init_rect(int w, int h, int ssz)
{
	/* Blit */
	wf_from.x = 0;
	wf_from.y = 1;
	wf_from.width = w;
	wf_from.height = h - ssz - 1;

	wf_to.x = 0;
	wf_to.y = 0;
	wf_to.width = w;
	wf_to.height = h - ssz - 1;

	/* Watterfall */
	wf_left.x = 0;
	wf_left.y = h - ssz - 1;
	wf_left.width = w / 2 - GAP;
	wf_left.height = 1;

	wf_right.x = w / 2 + GAP;
	wf_right.y = h - ssz - 1;
	wf_right.width = w / 2 - GAP;
	wf_right.height = 1;

	/* Spectrogram */
	sp_left.x = 0;
	sp_left.y = h - ssz;
	sp_left.width = w / 2 - GAP;
	sp_left.height = ssz;

	sp_right.x = w / 2 + GAP;
	sp_right.y = h - ssz;
	sp_right.width = w / 2 - GAP;
	sp_right.height = ssz;
}

unsigned long
hsvcolor(float h, float s, float v)
{
	XColor c;
	float r, g, b;

	hsv2rgb(&r, &g, &b, h, s, v);

	c.red = UINT16_MAX * r;
	c.green = UINT16_MAX * g;
	c.blue = UINT16_MAX * b;
	c.flags = DoRed|DoGreen|DoBlue;

	XAllocColor(dsp, cmap, &c);

	return c.pixel;
}

unsigned long *
init_palette(float h, float dh, float s, float ds, float v, float dv, int n, int lg)
{
	unsigned long *p;
	float	hstep, sstep, vstep;
	int	i;

	p = calloc(n, sizeof(unsigned long));

	hstep = (dh - h) / n;
	sstep = (ds - s) / n;
	vstep = (dv - v) / n;

	for (i = 0; i < n; i++) {
		p[i] = hsvcolor(h, s, lg ? logf(100 * v + 1) / logf(101) : v);
		h += hstep;
		s += sstep;
		v += vstep;
	}

	return p;
}

void
createbg(void)
{
	int y;

	for (y = 0; y < sp_left.height; y++) {
		XSetForeground(dsp, gc, sp[y]);
		XDrawLine(dsp, bg, gc,
			sp_left.x, sp_left.y + sp_left.height - y - 1,
			sp_left.x + sp_left.width - 1,
			sp_left.y + sp_left.height - y - 1);
	}
	XCopyArea(dsp, bg, bg, gc,
		sp_left.x, sp_left.y, sp_left.width, sp_left.height,
		sp_right.x, sp_right.y);
}

int
draw(double *left, double *right, int p, int step)
{
	int             x, l, r, lx, rx;

	/* blit waterfall */
	XCopyArea(dsp, pix, pix, gc,
		wf_from.x, wf_from.y, wf_from.width, wf_from.height,
		wf_to.x, wf_to.y);

	/* restore spectrogram bg */
	XCopyArea(dsp, bg, pix, gc,
		sp_left.x, sp_left.y, width, sp_left.height,
		sp_left.x, sp_left.y);

	for (x = 0; x < wf_left.width; x++) {
		l = left[x] - 0.5;
		if (l >= p)
			l = p - 1;
		r = right[x] - 0.5;
		if (r >= p)
			r = p - 1;

		lx = wf_left.x + wf_left.width - x - 1;
		rx = wf_right.x + x;

		/* waterfall */
		XSetForeground(dsp, gc, wf[l]);
		XDrawPoint(dsp, pix, gc, lx, wf_left.y);

		XSetForeground(dsp, gc, wf[r]);
		XDrawPoint(dsp, pix, gc, rx, wf_right.y);

		/* spectrogram neg mask */
		XSetForeground(dsp, gc, black);
		XDrawLine(dsp, pix, gc,
			lx, sp_left.y,
			lx, sp_left.y + sp_left.height - l - 1);
		XDrawLine(dsp, pix, gc,
			rx, sp_right.y,
			rx, sp_right.y + sp_right.height - r - 1);
	}

	/* flip */
	XCopyArea(dsp, pix, win, gc, 0, 0, width, height, 0, 0);

	return 0;
}

double *
init_hamming(int n)
{
	double	*w;
	int	i;

	w = calloc(n, sizeof(double));
	if (!w)
		errx(1, "malloc failed");

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
	fprintf(stderr, "Usage: %s [-hsd]\n", __progname);
	fprintf(stderr, "\t-h\tthis help\n");
	fprintf(stderr, "\t-d\tdon't fork\n");

	exit(0);
}

int 
main(int argc, char **argv)
{

	int		scr;
	Atom		delwin;

	struct		sio_hdl	*sio;
	struct		sio_par par;
	struct		fft *fft;

	double		*left, *right;
	double		*hamming;

	int16_t		*buffer;
	size_t		bufsz;
	size_t		done;

	int		ch, dflag = 1;
	int		delta, resolution, fps;
	int		psize, ssize;

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
		
	dsp = XOpenDisplay(getenv("DISPLAY"));
	if (!dsp)
		errx(1, "Cannot connect to X11 server");
	scr = DefaultScreen(dsp);
	black = BlackPixel(dsp, scr);
	white = WhitePixel(dsp, scr);
	cmap = DefaultColormap(dsp, scr);

	signal(SIGINT, catch);

	sio = sio_open(NULL, SIO_REC, 0);
	if (!sio)
		errx(1, "cannot connect to sound server, is it running?");

	sio_initpar(&par);

	par.rchan = 2;
	par.bits = 16;
	par.le = SIO_LE_NATIVE;
	par.sig = 1;

	if (!sio_setpar(sio, &par))
		errx(1, "SIO set params failed");
	if (!sio_getpar(sio, &par))
		errx(1, "SIO get params failed");

	if (par.rchan != 2 ||
	    par.bits != 16 ||
	    par.le != SIO_LE_NATIVE ||
	    par.sig != 1)
		errx(1, "unsupported audio params");

#if 0	
	if (dflag)
		daemon(0, 0);
#endif

	delta = par.round;
	resolution = (par.rate / par.round) / par.rchan;
	fps = (par.rate / par.round) * par.rchan;

	width = delta + 2 * GAP;
	height = 3 * width / 4;

	win = XCreateSimpleWindow(dsp, RootWindow(dsp, scr), 0, 0,
		width, height, 2, white, black);
	XStoreName(dsp, win, __progname);
	delwin = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(dsp, win, &delwin, 1);

	gc = XCreateGC(dsp, win, 0, NULL);	
	XSetGraphicsExposures(dsp, gc, False);
	
	pix = XCreatePixmap(dsp, win, width, height, DisplayPlanes(dsp, scr));
	bg = XCreatePixmap(dsp, win, width, height, DisplayPlanes(dsp, scr));

	XSelectInput(dsp, win, ExposureMask|KeyPressMask);
	XMapWindow(dsp, win);

	bufsz = par.rchan * delta * sizeof(int16_t);
	buffer = malloc(bufsz);
	if (!buffer)
		errx(1, "malloc failed");

	left = calloc(delta, sizeof(double));
	right = calloc(delta, sizeof(double));
	if (!left || !right)
		errx(1, "malloc failed");

	psize = 2 * height / 3;
	ssize = psize >> 2;

	init_rect(width, height, ssize);

	sp = init_palette(0.30, 0.00, 0.50, 1.00, 0.75, 1.00, ssize, 0);
	wf = init_palette(0.65, 0.35, 1.00, 0.00, 0.00, 1.00, ssize, 1);

	createbg();

	fft = init_fft(delta);
	hamming = init_hamming(delta);

	sio_start(sio);

	done = 0;
	while (!die) {
		do {
			done += sio_read(sio, buffer + done, bufsz - done);
			if (sio_eof(sio))
				errx(1, "SIO EOF");
		} while (done < bufsz);
		done -= bufsz;

		dofft(fft, buffer, left, right, delta, hamming);
		draw(left, right, ssize, resolution);

		while (XPending(dsp)) {
			XEvent ev;

			XNextEvent(dsp, &ev);

			switch (ev.type) {
			case KeyPress:
				die = XLookupKeysym(&ev.xkey, 0) == XK_q;
				break;
			case ClientMessage:
				die = *ev.xclient.data.l == delwin;
				break;
			default:
				break;
			}
		}
	}

	sio_stop(sio);
	sio_close(sio);

	free(left);
	free(right);
	free(buffer);

	XCloseDisplay(dsp);

	return 0;
}
