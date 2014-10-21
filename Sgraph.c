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
#include "SgraphP.h"

#include <err.h>
#include <stdint.h>

#define Trace(w) warnx("%s.%s", XtClass(w)->core_class.class_name, __func__)
#define TraceOnce(w) do {						\
	static int once = 1;						\
	if (once) {							\
		Trace(w);						\
		once = 0;						\
	}								\
} while (0)

static void Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs);
static void Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region r);
static void mirror(Widget, XEvent *, String *, Cardinal *);
static void GetGC(Widget);

/* Initialization */
#define Offset(field) XtOffsetOf(SgraphRec, sgraph.field)
static XtResource resources[] = {
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
		Offset(foreground), XtRString, XtDefaultForeground },
	{ XtNmirror, XtCBoolean, XtRBoolean, sizeof(Boolean),
		Offset(mirror), XtRBoolean, False },
	{ XtNvalues, XtCValues, XtRPointer, sizeof(int *),
		Offset(values), XtRPointer, NULL },
	{ XtNsize, XtCsize, XtRInt, sizeof(int),
		Offset(size), XtRImmediate, (XtPointer)0 },
};
#undef Offset

static XtActionsRec actions[] = {
	{ "mirror",	mirror },
};

static char translations[] = {
	"<Key>m:	mirror()\n",
};

SgraphClassRec sgraphClassRec = {
	.core_class = {
		.superclass			= (WidgetClass)&widgetClassRec,
		.class_name			= "Sgraph",
		.widget_size			= sizeof(SgraphRec),
		.class_initialize		= NULL,
		.class_part_initialize		= NULL,
		.class_inited			= False,
		.initialize			= Initialize,
		.initialize_hook		= NULL,
		.realize			= Realize,
		.actions			= actions,
		.num_actions			= XtNumber(actions),
		.resources			= resources,
		.num_resources			= XtNumber(resources),
		.xrm_class			= NULLQUARK,
		.compress_motion		= True,
		.compress_exposure		= True,
		.compress_enterleave		= True,
		.visible_interest		= False,
		.destroy			= NULL,
		.resize				= Resize,
		.expose				= Redisplay,
		.set_values			= NULL,
		.set_values_hook		= NULL,
		.set_values_almost		= XtInheritSetValuesAlmost,
		.get_values_hook		= NULL,
		.accept_focus			= NULL,
		.version			= XtVersion,
		.callback_private		= NULL,
		.tm_table			= translations,
		.query_geometry			= XtInheritQueryGeometry,
		.display_accelerator		= XtInheritDisplayAccelerator,
		.extension			= NULL,
	},
	.sgraph_class = {
		.extension			= NULL,
	}
};

WidgetClass sgraphWidgetClass = (WidgetClass)&sgraphClassRec;

static void
Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs)
{
	SgraphWidget	sw = (SgraphWidget)w;
	int major, minor;
	Status ret;

	Trace(w);

	ret = XdbeQueryExtension(XtDisplay(w), &major, &minor);
	if (!ret)
		errx(1, "Xdbe %d.%d error %d", major, minor, ret);

	sw->core.width = 320;
	sw->core.height = 120;

	sw->sgraph.bg = None;
	sw->sgraph.mask = None;
}

static void
Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
	SgraphWidget	sw = (SgraphWidget)w;

	if (XtIsRealized(w))
		return;

	Trace(w);

	XtSuperclass(w)->core_class.realize(w, mask, attr);
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

	Trace(w);

	if (sw->sgraph.bg != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.bg);

	sw->sgraph.bg = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		w->core.width, w->core.height,
		DefaultDepthOfScreen(XtScreen(sw)));

	if (sw->sgraph.mask != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.mask);

	sw->sgraph.mask = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		w->core.width, w->core.height, 1);

	GetGC(w);
}

static void
GetGC(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;
	XGCValues	xgcv;
	XtGCMask	gc_mask;

	Trace(w);

	gc_mask = GCForeground;
	xgcv.foreground = sw->core.background_pixel;
	sw->sgraph.backGC = XtGetGC(w, gc_mask, &xgcv);

	gc_mask = GCForeground;
	xgcv.foreground = sw->sgraph.foreground;
	sw->sgraph.foreGC = XtGetGC(w, gc_mask, &xgcv);

#if 1
	gc_mask = GCForeground|GCClipMask;
	xgcv.clip_mask = sw->sgraph.mask;
	sw->sgraph.clipGC = XtGetGC(w, gc_mask, &xgcv);
#endif

	gc_mask = GCForeground|GCPlaneMask;
	xgcv.plane_mask = 1;
	xgcv.foreground = 1;
	sw->sgraph.maskGC = XtGetGC(w, gc_mask, &xgcv);
}

static void
Redisplay(Widget w, XEvent *event, Region r)
{
	SgraphWidget	sw = (SgraphWidget)w;
	Dimension	i, x, y, width;
	XdbeSwapInfo	swap;

	if (!XtIsRealized(w))
		return;

	TraceOnce(w);

	width = sw->core.width;
	if (width > sw->sgraph.size)
		width = sw->sgraph.size;

	for (i = 0; i < width - 1; i++) {
		y = sw->sgraph.values[i];
		if (y > sw->core.height)
			y = sw->core.height - 1;
		x = sw->sgraph.mirror ? sw->core.width - i : i;

#if 0
		XDrawLine(XtDisplay(sw), sw->sgraph.backBuf,
			sw->sgraph.foreGC,
			x, sw->core.height,
			x, sw->core.height - y);
#else
		XDrawLine(XtDisplay(sw), sw->sgraph.bg,
			sw->sgraph.foreGC,
			x, sw->core.height,
			x, sw->core.height - y);
#endif
	}

#if 0
	swap.swap_window = XtWindow(sw);
	swap.swap_action = XdbeBackground;

	XdbeSwapBuffers(XtDisplay(sw), &swap, 1);
#else

	XCopyArea(XtDisplay(sw), sw->sgraph.bg, XtWindow(sw),
		sw->sgraph.foreGC, 0, 0,
		sw->core.width, sw->core.height,
		0, 0);
	XFillRectangle(XtDisplay(sw), sw->sgraph.bg,
		sw->sgraph.backGC,
		0, 0, sw->core.width, sw->core.height);
#endif
}

static void
mirror(Widget w, XEvent *event, String *param, Cardinal *n)
{
	SgraphWidget	sw = (SgraphWidget)w;

	Trace(w);

	sw->sgraph.mirror ^= 1;
}
