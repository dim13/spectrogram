/* $Id$ */

#ifndef _DisplayP_h
#define _DisplayP_h

#include "Display.h"

typedef struct {
	Dimension		space;
	int			**data;
} DisplayPart;

typedef struct _DisplayRec {
	CorePart		core;
	CompositePart		composite;
	DisplayPart		display;
} DisplayRec;

typedef struct {
	XtPointer		extension;
} DisplayClassPart;

typedef struct _DisplayClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	DisplayClassPart	display_class;
} DisplayClassRec;

extern DisplayClassRec displayClassRec;

#endif
