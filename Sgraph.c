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

static void Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs);
static void Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr);
static void Action(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region r);
static Boolean SetValues(Widget old, Widget reference, Widget new, ArgList args, Cardinal *num_args);

#define BORDER		1
#define SGRAPH_WIDTH	1024
#define SGRAPH_HEIGHT	768

static Dimension winwidth = SGRAPH_WIDTH;
static Dimension winheight = SGRAPH_HEIGHT;

/* Initialization */
#define offset(field) XtOffsetOf(SgraphRec, sgraph.field)
static XtResource resources[] = {
	{ XtNwidth, XtCWidth, XtRDimension,
		sizeof(Dimension), XtOffset(Widget, core.width),
		XtRDimension, &winwidth },
	{ XtNheight, XtCHeight, XtRDimension,
		sizeof(Dimension), XtOffset(Widget, core.height),
		XtRDimension, &winheight },
	{ XtNforeground, XtCForeground, XtRPixel,
		sizeof(Pixel), XtOffset(SgraphWidget, sgraph.foreground),
		XtRString, "white" },
	{ XtNbackground, XtCBackground, XtRPixel,
		sizeof(Pixel), XtOffset(SgraphWidget, sgraph.background),
		XtRString, "black" },
	{ XtNbackground, XtCBackground, XtRPixel,
		sizeof(Pixel), XtOffset(Widget, core.background_pixel),
		XtRString, "dark slate gray" },
	{ XtNmirror, XtCBoolean, XtRBoolean,
		sizeof(Boolean), XtOffset(SgraphWidget, sgraph.mirror),
		XtRBoolean, False },
	{ XtNleftData, XtCParameter, XtRPointer,
		sizeof(XtPointer), XtOffset(SgraphWidget, sgraph.leftData),
		XtRPointer, NULL },
	{ XtNrightData, XtCParameter, XtRPointer,
		sizeof(XtPointer), XtOffset(SgraphWidget, sgraph.rightData),
		XtRPointer, NULL },
	{ XtNsize, XtCsize, XtRInt,
		sizeof(int), XtOffset(SgraphWidget, sgraph.size),
		XtRImmediate, (XtPointer)2048 },
	{ XtNsamples, XtCsamples, XtRInt,
		sizeof(int), XtOffset(SgraphWidget, sgraph.samples),
		XtRImmediate, (XtPointer)0 },
};
#undef offset

static XtActionsRec actions[] = {
	{ "sgraph", Action },
};

static char translations[] = "<Key>:" "sgraph()\n";

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
		actions,			/* actions */
		XtNumber(actions),		/* num_actions */
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
		translations,			/* tm_table */
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
	xgcv.background = sw->core.background_pixel;

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

	sw->sgraph.leftData = (double *)XtCalloc(sw->sgraph.size, sizeof(double));
	sw->sgraph.rightData = (double *)XtCalloc(sw->sgraph.size, sizeof(double));
	
	warnx("Initialize");
	GetGC(w);
}

static void
Realize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
	XtCreateWindow(w, InputOutput, CopyFromParent, *mask, attr);
	Resize(w);
}

static void
Resize(Widget w)
{
	SgraphWidget	sw = (SgraphWidget)w;
	Dimension	width, height;

	if (!XtIsRealized(w))
		return;

	warnx("Resize");

	winwidth = w->core.width;
	winheight = w->core.height;

	width = winwidth / 2;
	height = winheight;
	warnx("win: %dx%d", winwidth, winheight);
	warnx("sub: %dx%d", width, height);
	warnx("size: %d", sw->sgraph.size);
	warnx("samples: %d", sw->sgraph.samples);

	if (sw->sgraph.bg != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.bg);
	sw->sgraph.bg = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		width, height / 4, DefaultDepthOfScreen(XtScreen(sw)));
	XSetForeground(XtDisplay(sw), sw->sgraph.foreGC, 0xFF0000);
	XFillRectangle(XtDisplay(sw), sw->sgraph.bg, sw->sgraph.foreGC,
		0, 0, width, height / 4);

	if (sw->sgraph.mask != None)
		XFreePixmap(XtDisplay(sw), sw->sgraph.mask);
	sw->sgraph.mask = XCreatePixmap(XtDisplay(sw), XtWindow(sw),
		width, height / 4, 1);

}

static void
Redisplay(Widget w, XEvent *event, Region r)
{
	SgraphWidget	sw = (SgraphWidget)w;
	Dimension	width = winwidth / 2;
	Dimension	height = winheight;
	static Dimension n;

	if (!XtIsRealized(w))
		return;

	//warnx("Redisplay");

	/*
	XFillRectangle(XtDisplay(sw), XtWindow(sw), sw->sgraph.backGC,
		BORDER, BORDER,
		width - 2 * BORDER, height - 2 * BORDER);
	XFillRectangle(XtDisplay(sw), XtWindow(sw), sw->sgraph.backGC,
		width + BORDER, BORDER,
		width - 2 * BORDER, height - 2 * BORDER);
	 */

	warnx("%lf : %lf" , sw->sgraph.leftData[0], sw->sgraph.rightData[0]);

	XClearWindow(XtDisplay(sw), XtWindow(sw));
	n = (n + 1) % width;
	XCopyArea(XtDisplay(sw), sw->sgraph.bg, XtWindow(sw), sw->sgraph.foreGC,
		0, 0, width, height / 4, BORDER + n, BORDER);
}

static Boolean
SetValues(Widget old, Widget reference, Widget new, ArgList args, Cardinal *num_args)
{
	return True;
}

static void
Action(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
}
