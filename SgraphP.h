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

#ifndef _SgraphP_h
#define _SgraphP_h

#include "Sgraph.h"

/* include superclass private header file */
#include <X11/CoreP.h>

/* define unique representation types not found in <X11/StringDefs.h> */
#define XtRSgraphResource "SgraphResource"

typedef struct {
	XtPointer	extension;
} SgraphClassPart;

typedef struct _SgraphClassRec {
	CoreClassPart	core_class;
	SgraphClassPart	sgraph_class;
} SgraphClassRec;

extern SgraphClassRec sgraphClassRec;

typedef struct {
	Pixel		background;
	Pixel		foreground;
	Boolean		mirror;
	int		*data;
	size_t		size;
	size_t		samples;
	/*
	XtCallbackProc	data;
	XtCallbackProc	fft;
	 */

	XdbeBackBuffer backBuf;
	GC		foreGC;
	GC		backGC;
	GC		maskGC;
	GC		clipGC;
	Pixmap		bg;
	Pixmap		mask;
	Pixmap		waterfall;
} SgraphPart;

typedef struct _SgraphRec {
	CorePart	core;
	SgraphPart	sgraph;
} SgraphRec;

#endif
