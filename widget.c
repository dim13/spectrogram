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

#include <assert.h>
#include <stdlib.h>

#include "aux.h"
#include "cms.h"
#include "widget.h"

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

struct panel {
	Display *dsp;
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

struct palette p_spectr =    {{ 120.0, 100.0,  75.0 }, {   0.0, 100.0,  25.0 }};
struct palette p_shadow =    {{ 120.0, 100.0,  10.0 }, {   0.0, 100.0,  10.0 }};
struct palette p_waterfall = {{ 210.0,  75.0,   0.0 }, { 180.0, 100.0, 100.0 }};

void
draw_panel(struct panel *p)
{
	int i, v, x;

	/* blit waterfall */
	blit(p->dsp, p->wf->pix, p->wf->gc, p->wf->geo);
	/* blit shadow mask */
	blit(p->dsp, p->shadow->mask, p->shadow->gc, p->shadow->geo);

	/* clear spectrogram */
	clear(p->dsp, p->sp->pix, p->sp->gc, p->sp->geo);
	/* clear mask */
	clear(p->dsp, p->bg->mask, p->bg->gc, p->bg->geo);

	for (i = 0; i < p->sp->geo.width; i++) {
		/* limit maxval */
		v = p->data[i] >= p->maxval ? p->maxval - 1 : p->data[i];
		x = p->mirror ? p->sp->geo.width - i - 1 : i;

		/* draw waterfall */
		XSetForeground(p->dsp, p->wf->gc, p->palette[v]);
		XDrawPoint(p->dsp, p->wf->pix, p->wf->gc, x, 0);

		/* draw spectrogram */
		XSetForeground(p->dsp, p->bg->gc, 1);
		XDrawLine(p->dsp, p->bg->mask, p->bg->gc,
			x, p->bg->geo.height - v,
			x, p->bg->geo.height);
	}

	/* copy mask to shadow mask */
	copy(p->dsp, p->bg->mask, p->shadow->mask, p->shadow->gc,
		p->shadow->geo, p->bg->mask);
	/* shadow to buffer */
	copy(p->dsp, p->shadow->pix, p->sp->pix, p->sp->gc, p->sp->geo,
		p->shadow->mask);
	/* spectrogram to buffer */
	copy(p->dsp, p->bg->pix, p->sp->pix, p->sp->gc, p->sp->geo,
		p->bg->mask);
}

void
flip_panel(struct panel *p)
{
	/* flip spectrogram */
	copy(p->dsp, p->sp->pix, p->sp->win, p->sp->gc, p->sp->geo, None);
	/* flip waterfall */
	copy(p->dsp, p->wf->pix, p->wf->win, p->wf->gc, p->wf->geo, None);
}

static void
draw_background(Display *d, Pixmap pix, GC gc, XRectangle r, unsigned long *pal)
{
	int i, x, y;

	x = r.width - 1;
	for (i = 0; i < r.height; i++) {
		y = r.height - i - 1;
		XSetForeground(d, gc, pal[i]);
		XDrawLine(d, pix, gc, 0, y, x, y);
	}
}

static struct background *
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

	clear(d, p->mask, p->gc, p->geo);

	return p;
}

static struct subwin *
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

	clear(d, p->pix, p->gc, p->geo);

	XMapWindow(d, p->win);
	
	return p;
}

struct panel *
init_panel(Display *d, Window win, XRectangle r, enum mirror m)
{
	struct panel *p;
	int scr = DefaultScreen(d);
	unsigned long white = WhitePixel(d, scr);
	unsigned long gray = hslcolor(d, 0.0, 0.0, 20.0);

	unsigned long *palette;
	unsigned int maxval = r.height / 4;
	XRectangle geo;

	p = malloc(sizeof(struct panel));
	assert(p);

	p->dsp = d;
	p->data = calloc(2 * r.width, sizeof(double));
	assert(p->data);

	/* main panel window */
	p->win = XCreateSimpleWindow(d, win,
		r.x, r.y, r.width, r.height,
		0, white, gray);

	/* sperctrogram window and its bitmasks */
	geo.x = 0;
	geo.y = 0;
	geo.width = r.width;
	geo.height = maxval;
	p->sp = init_subwin(d, p->win, geo);

	p->bg = init_background(d, p->sp->win, geo);
	palette = init_palette(d, p_spectr, maxval);
	draw_background(d, p->bg->pix, p->sp->gc, p->bg->geo, palette);
	free(palette);

	p->shadow = init_background(d, p->sp->win, geo);
	palette = init_palette(d, p_shadow, maxval);
	draw_background(d, p->shadow->pix, p->sp->gc, p->shadow->geo, palette);
	free(palette);

	p->palette = init_palette(d, p_waterfall, maxval);
	p->maxval = maxval;
	p->mirror = m;

	/* waterfall window and double buffer */
	geo.x = 0;
	geo.y = p->sp->geo.height + VGAP;
	geo.width = r.width;
	geo.height = r.height - maxval;
	p->wf = init_subwin(d, p->win, geo);

	XMapWindow(d, p->win);
	
	return p;
}

static void
free_background(Display *d, struct background *p)
{
	XFreePixmap(d, p->pix);
	XFreePixmap(d, p->mask);
	XFreeGC(d, p->gc);
}

static void
free_subwin(Display *d, struct subwin *p)
{
	XFreePixmap(d, p->pix);
	XFreeGC(d, p->gc);
	XUnmapWindow(d, p->win);
	XDestroyWindow(d, p->win);
}

void
free_panel(struct panel *p)
{
	free_background(p->dsp, p->bg);
	free_background(p->dsp, p->shadow);
	free_subwin(p->dsp, p->sp);
	free_subwin(p->dsp, p->wf);
	XUnmapWindow(p->dsp, p->win);
	XDestroyWindow(p->dsp, p->win);
	free(p->palette);
	free(p->data);
	free(p);
}

void
toggle_mirror(struct panel *p)
{
	p->mirror ^= 1;
}

double *
dataptr(struct panel *p)
{
	return p->data;
}
