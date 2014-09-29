/* $Id$ */
/*
 * Copyright (c) 2014 Dimitri Sokolyuk <demon@dim13.org>
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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrender.h>
#include "SgraphP.h"

#include <err.h>
#include <stdint.h>

#define Printd(w, s) do {						\
	warnx("Class %s: %s", XtClass(w)->core_class.class_name, s);	\
} while (0)

static void Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs);
static void Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region r);
static Boolean SetValues(Widget old, Widget reference, Widget new, ArgList args, Cardinal *num_args);

/* Initialization */
#define offset(field) XtOffsetOf(SgraphRec, sgraph.field)
#define goffset(field) XtOffsetOf(CoreRec, core.field)
static XtResource resources[] = {
	{ XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
		goffset(width), XtRImmediate, (XtPointer) 320 },
	{ XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
		goffset(height), XtRImmediate, (XtPointer) 200 },
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
		offset(foreground), XtRString, XtDefaultForeground },
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
		offset(background), XtRString, XtDefaultBackground },
	{ XtNmirror, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(mirror), XtRBoolean, False },
	{ XtNdata, XtCData, XtRPointer, sizeof(int *),
		offset(data), XtRPointer, NULL },
	/*
	{ XtNrightData, XtCParameter, XtRPointer, sizeof(XtPointer),
		offset(rightData), XtRPointer, NULL },
	 */
	{ XtNsize, XtCsize, XtRInt, sizeof(int),
		offset(size), XtRImmediate, (XtPointer)2048 },
	{ XtNsamples, XtCsamples, XtRInt, sizeof(int),
		offset(samples), XtRImmediate, (XtPointer)0 },
	/*
	{ XtNdataCallback, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
		offset(data), XtRCallback, NULL },
	{ XtNfftCallback, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
		offset(fft), XtRCallback, NULL },
	 */
};
#undef goffset
#undef offset

SgraphClassRec sgraphClassRec = {
	/* core */
	{
		&widgetClassRec,		/* superclass */
		"Sgraph",			/* class_name */
		sizeof(SgraphRec),		/* widget_size */
		NULL,				/* class_initialize */
		NULL,				/* class_part_initialize */
		False,				/* class_inited */
		Initialize,			/* initialize */
		NULL,				/* initialize_hook */
		Realize,			/* realize */
		NULL,				/* actions */
		0,				/* num_actions */
		resources,			/* resources */
		XtNumber(resources),		/* num_resources */
		NULLQUARK,			/* xrm_class */
		True,				/* compress_motion */
		True,				/* compress_exposure */
		True,				/* compress_enterleave */
		False,				/* visible_interest */
		NULL,				/* destroy */
		Resize,				/* resize */
		Redisplay,			/* expose */
		SetValues,			/* set_values */
		NULL,				/* set_values_hook */
		XtInheritSetValuesAlmost,	/* set_values_almost */
		NULL,				/* get_values_hook */
		NULL,				/* accept_focus */
		XtVersion,			/* version */
		NULL,				/* callback_private */
		XtInheritTranslations,		/* tm_table */
		XtInheritQueryGeometry,		/* query_geometry */
		XtInheritDisplayAccelerator,	/* display_accelerator */
		NULL,				/* extension */
	},
	/* sgraph */
	{
		NULL,				/* extension */
	}
};
WidgetClass sgraphWidgetClass = (WidgetClass)&sgraphClassRec;

/* Implementation */

static void
GetGC(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;
	XGCValues	xgcv;
	XtGCMask	gc_mask = GCForeground|GCBackground|GCPlaneMask;

	xgcv.plane_mask = AllPlanes;
	xgcv.background = sw->sgraph.background;

	xgcv.foreground = sw->sgraph.background;
	sw->sgraph.backGC = XtGetGC(w, gc_mask, &xgcv);

	xgcv.foreground = sw->sgraph.foreground;
	sw->sgraph.foreGC = XtGetGC(w, gc_mask, &xgcv);

	/*
	gc_mask |= GCClipMask;
	xgcv.clip_mask = sw->sgraph.mask;
	sw->sgraph.clipGC = XtGetGC(w, gc_mask, &xgcv);

	gc_mask ^= ~GCClipMask;
	 */
	xgcv.plane_mask = 1;
	sw->sgraph.maskGC = XtGetGC(w, gc_mask, &xgcv);
}

static void
Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs)
{
	SgraphWidget	sw = (SgraphWidget)w;
	int major, minor;
	Status ret;

	ret = XdbeQueryExtension(XtDisplay(w), &major, &minor);
	if (!ret)
		errx(1, "Xdbe %d.%d error %d", major, minor, ret);
	/*
	ret = XRenderQueryVersion(XtDisplay(w), &major, &minor);
	if (!ret)
		errx(1, "XRender %d.%d error %d", major, minor, ret);
	 */

	sw->sgraph.data = (int *)XtCalloc(sw->sgraph.size, sizeof(int));
	
	Printd(w, "Initialize");
	GetGC(w);
}

static void
Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
	SgraphWidget	sw = (SgraphWidget)w;

	if (XtIsRealized(w))
		return;

	XtCreateWindow(w, InputOutput, CopyFromParent, *mask, attr);
	sw->sgraph.backBuf = XdbeAllocateBackBufferName(XtDisplay(w),
		XtWindow(w), XdbeBackground);
	Resize(w);
}

static void
Resize(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;

	if (!XtIsRealized(w))
		return;

	Printd(w, "Resize");

	sw->sgraph.size = w->core.width;
	warnx("win: %dx%d", w->core.width, w->core.height);
	warnx("size: %d", sw->sgraph.size);
	warnx("samples: %d", sw->sgraph.samples);

	if (sw->sgraph.bg != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.bg);
	sw->sgraph.bg = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		w->core.width, w->core.height,
		DefaultDepthOfScreen(XtScreen(sw)));

	if (sw->sgraph.mask != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.mask);
	sw->sgraph.mask = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		w->core.width, w->core.height, 1);
}

static void
Redisplay(Widget w, XEvent *event, Region r)
{
	SgraphWidget	sw = (SgraphWidget)w;
	Dimension	x, y;
	Dimension	bottom;
	XdbeSwapInfo	swap;

	if (!XtIsRealized(w))
		return;

	bottom = sw->core.height - 1;

	for (x = 0; x < sw->sgraph.size - 1; x++) {
		y = sw->sgraph.data[x];

		XDrawLine(XtDisplay(sw), sw->sgraph.backBuf,
			sw->sgraph.foreGC,
			x, bottom,
			x, bottom - y);
	}

	swap.swap_window = XtWindow(sw);
	swap.swap_action = XdbeBackground;

	XdbeSwapBuffers(XtDisplay(sw), &swap, 1);
}

static Boolean
SetValues(Widget old, Widget reference, Widget new, ArgList args, Cardinal *num_args)
{
	XExposeEvent xeev;

	xeev.type = Expose;
	xeev.display = XtDisplay(new);
	xeev.window = XtWindow(new);
	xeev.x = 0;
	xeev.y = 0;
	xeev.width = new->core.width;
	xeev.height = new->core.height;
	Redisplay(new, (XEvent *)&xeev, NULL);

	return False;
}
