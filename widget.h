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

#ifndef _WIDGET_H
#define _WIDGET_H

#define	HGAP	4
#define	VGAP	1

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

enum mirror { LTR, RTL };

__BEGIN_DECLS
void draw_panel(Display *, struct panel *);
void flip_panel(Display *, struct panel *);
struct panel *init_panel(Display *, Window, XRectangle, enum mirror);
void free_panel(Display *, struct panel *);
void toggle_mirror(struct panel *);
double *dataptr(struct panel *);
__END_DECLS

#endif
