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

#include <string.h>

void
fullscreen(Display *d, Window win)
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

void
move(Display *d, Window win, Window container)
{
	XWindowAttributes wa, wac;
	int dx, dy;

	XGetWindowAttributes(d, win, &wa);
	XGetWindowAttributes(d, container, &wac);

	dx = (wa.width - wac.width) / 2;
	dy = (wa.height - wac.height) / 2;
	if (dy < 0)
		dy = 0;

	XMoveWindow(d, container, dx, dy);
}

void
restrictsize(Display *d, Window win, int minw, int minh, int maxw, int maxh)
{
	Atom		nhints;
	XSizeHints	*hints;

	nhints = XInternAtom(d, "WM_NORMAL_HINTS", 0);
	hints = XAllocSizeHints();
	hints->flags = PMinSize|PMaxSize;
	hints->min_width = minw;
	hints->min_height = minh;
	hints->max_width = maxw;
	hints->max_height = maxh;
	XSetWMSizeHints(d, win, hints, nhints);
	XFree(hints);
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
