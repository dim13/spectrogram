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
#include "SgraphP.h"

#include <err.h>
#include <stdint.h>

#define Trace(w) do {							\
	warnx("%s.%s", XtClass(w)->core_class.class_name, __func__);	\
} while (0)

static void Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs);
static void Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region r);
static Boolean SetValues(Widget old, Widget reference, Widget new, ArgList args, Cardinal *num_args);
static void mirror(Widget, XEvent *, String *, Cardinal *);

/* Initialization */
#define Offset(field) XtOffsetOf(SgraphRec, sgraph.field)
static XtResource resources[] = {
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
		Offset(foreground), XtRString, XtDefaultForeground },
	{ XtNmirror, XtCBoolean, XtRBoolean, sizeof(Boolean),
		Offset(mirror), XtRBoolean, False },
	{ XtNdata, XtCData, XtRPointer, sizeof(int *),
		Offset(data), XtRPointer, NULL },
	{ XtNsize, XtCsize, XtRInt, sizeof(int),
		Offset(size), XtRImmediate, (XtPointer)2048 },
	{ XtNsamples, XtCsamples, XtRInt, sizeof(int),
		Offset(samples), XtRImmediate, (XtPointer)0 },
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
GetGC(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;
	XGCValues	xgcv;
	XtGCMask	gc_mask = GCForeground|GCBackground|GCPlaneMask;

	Trace(w);

	xgcv.plane_mask = AllPlanes;
	xgcv.background = sw->core.background_pixel;

	xgcv.foreground = sw->core.background_pixel;
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

	Trace(w);

	ret = XdbeQueryExtension(XtDisplay(w), &major, &minor);
	if (!ret)
		errx(1, "Xdbe %d.%d error %d", major, minor, ret);

	sw->core.width = 320;
	sw->core.height = 120;
	sw->sgraph.data = (int *)XtCalloc(sw->sgraph.size, sizeof(int));
	
	GetGC(w);
}

static void
Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
	SgraphWidget	sw = (SgraphWidget)w;

	Trace(w);

	if (XtIsRealized(w))
		return;

	XtSuperclass(w)->core_class.realize(w, mask, attr);
	sw->sgraph.backBuf = XdbeAllocateBackBufferName(XtDisplay(w),
		XtWindow(w), XdbeBackground);
}

static void
Resize(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;

	Trace(w);

	if (!XtIsRealized(w))
		return;

	sw->sgraph.size = w->core.width;
	warnx("win: %dx%d", w->core.width, w->core.height);
	warnx("size: %zu", sw->sgraph.size);
	warnx("samples: %zu", sw->sgraph.samples);

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
	Dimension	i, x, y;
	XdbeSwapInfo	swap;

	//Trace(w);

	if (!XtIsRealized(w))
		return;

	for (i = 0; i < sw->core.width - 1; i++) {
		y = sw->sgraph.data[i];
		if (sw->sgraph.mirror)
			x = sw->core.width - i;
		else
			x = i;

		XDrawLine(XtDisplay(sw), sw->sgraph.backBuf,
			sw->sgraph.foreGC,
			x, sw->core.height,
			x, sw->core.height - y);
	}

	swap.swap_window = XtWindow(sw);
	swap.swap_action = XdbeBackground;

	XdbeSwapBuffers(XtDisplay(sw), &swap, 1);
}

static void
mirror(Widget w, XEvent *event, String *param, Cardinal *n)
{
	SgraphWidget	sw = (SgraphWidget)w;
	sw->sgraph.mirror ^= 1;
}
