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

/* Class Methods */
static void Initialize(Widget, Widget, ArgList, Cardinal *);

/* Prototypes */
static Bool Function(SgraphWidget, int, int, Bool);

/* Actions */
static void Action(Widget, XEvent *, String *, Cardinal *);

/* Initialization */
#define offset(field) XtOffsetOf(SgraphRec, sgraph.field)
static XtResource resources[] = {
	{
		XtNsgraphResource,		/* name */
		XtCSgraphResource,		/* class */
		XtRSgraphResource,		/* type */
		sizeof(char *),			/* size */
		offset(resource),		/* offset */
		XtRString,			/* default_type */
		(XtPointer) "default"		/* default_addr */
	},
};
#undef offset

static XtActionsRec actions[] =
{
	{
		"sgraph",			/* name */
		Action				/* procedure */
	},
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
		NULL,				/* resize */
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

/*
 * Function:
 *	Initialize
 *
 * Parameters:
 *	request - requested widget
 *	w	- the widget
 *	args	- arguments
 *	num_args - number of arguments
 *
 * Description:
 *	Initializes widget instance.
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget w, ArgList args, Cardinal *num_args)
{
	SgraphWidget 	sw = (SgraphWidget)w;

	sw->sgraph.private = NULL;
}

/*
 * Function:
 *	Function
 *
 * Parameters:
 *	sw    - sgraph widget
 *	x     - x coordinate
 *	y     - y coordinate
 *	force - force action
 *
 * Description:
 *	This function does nothing.
 *
 * Return:
 *	Parameter force
 */
/* ARGSUSED */
static 		Bool
Function(SgraphWidget sw, int x, int y, Bool force)
{
	return (force);
}

/*
 * Function:
 *	Action
 *
 * Parameters:
 *	w	   - sgraph widget
 *	event	   - event that caused this action
 *	params	   - parameters
 *	num_params - number of parameters
 *
 * Description:
 *	This function does nothing.
 */
/* ARGSUSED */
static void
Action(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
}
