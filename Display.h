/* $Id$ */

#ifndef _Display_h
#define _Display_h

#define XtNdata		"data"
#define XtCData		"Data"

#define XtNnumChannel	"numChannel"
#define XtCNumChannel	"NumChannel"

#define XtNnumSamples	"numSamples"
#define XtCNumSamples	"NumSamples"

typedef struct _DisplayClassRec *DisplayWidgetClass;
typedef struct _DisplayRec *DisplayWidget;

extern WidgetClass displayWidgetClass;

#endif
