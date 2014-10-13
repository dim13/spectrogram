/* $Id$ */

#ifndef _DisplayP_h
#define _DisplayP_h

#include "Display.h"

typedef struct {
	int			num_channel;
	int			num_samples;
	int			**data;
} DisplayPart;

typedef struct _DisplayRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	DisplayPart		display;
} DisplayRec;

typedef struct {
	XtPointer		extension;
} DisplayClassPart;

typedef struct _DisplayClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	DisplayClassPart	display_class;
} DisplayClassRec;

typedef struct {
	XtPointer		extension;
} DisplayConstraintPart;

typedef struct _DisplayConstraintRec {
	DisplayConstraintPart	display;
} DisplayConstraintRec, *DisplayConstraints;

extern DisplayClassRec displayClassRec;

#endif
