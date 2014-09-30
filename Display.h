/* $Id$ */

#ifndef _Display_h
#define _Display_h

#define XtNdata		"data"
#define XtCData		"Data"

#define XtNround	"round"
#define XtCRound	"Round"

#define XtNmaxRound	"maxRound"
#define XtCMaxRound	"MaxRound"

typedef struct _DisplayClassRec *DisplayWidgetClass;
typedef struct _DisplayRec *DisplayWidget;

extern WidgetClass displayWidgetClass;

#endif
