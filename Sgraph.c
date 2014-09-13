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

static void Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs);
static void Action(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void Resize(Widget w);

#define BORDER		2
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
	{ XtNmirror, XtCBoolean, XtRBoolean,
		sizeof(Boolean), XtOffset(SgraphWidget, sgraph.mirror),
		XtRBoolean, False },
};
#undef offset

static XtActionsRec actions[] = {
	{ "sgraph", Action },
};

static char translations[] = "<Key>:" "sgraph()\n";


#define Superclass (&widgetClassRec)
SgraphClassRec sgraphClassRec = {
	/* core */
	{
		(WidgetClass) Superclass,	/* superclass */
		"Sgraph",			/* class_name */
		sizeof(SgraphRec),		/* widget_size */
		NULL,				/* class_initialize */
		NULL,				/* class_part_initialize */
		False,				/* class_inited */
		Initialize,			/* initialize */
		NULL,				/* initialize_hook */
		XtInheritRealize,		/* realize */
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
		NULL,				/* expose */
		NULL,				/* set_values */
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
Initialize(Widget request, Widget w, ArgList args, Cardinal *nargs)
{
//	Display *dpy = XtDisplay(w);
}

static void
Resize(Widget w)
{
}

static void
Action(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
}
