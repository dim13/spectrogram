/* $Id$ */
/*
 * Copyright (c) 2010 Dimitri Sokolyuk <demon@dim13.org>
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/Intrinsic.h>
#include "Display.h"
//#include "Sgraph.h"

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "aux.h"
#include "fft.h"
#include "sio.h"
#include "widget.h"

extern	char *__progname;
int	die = 0;

void
catch(int notused)
{
	die = 1;
}

void
usage(void)
{
	#define USGFMT "\t%-12s%s\n"
	fprintf(stderr, "Usage: %s [-dfph] [-r <round>]\n", __progname);
	fprintf(stderr, USGFMT, "-d", "daemonize");
	fprintf(stderr, USGFMT, "-f", "fullscreen mode");
	fprintf(stderr, USGFMT, "-p", "don't hide pointer in fullscreen mode");
	fprintf(stderr, USGFMT, "-r <round>", "sio round count");
	fprintf(stderr, USGFMT, "-h", "this help");

	exit(0);
}

static XrmOptionDescRec options[] = {
	{"-mirror",	"*Sgraph.mirror",	XrmoptionNoArg, "TRUE" },
	{"-nomirror",	"*Sgprah.mirror",	XrmoptionNoArg, "FALSE" },
};

static void
quit(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	exit(0);
}

static XtActionsRec actionsList[] = {
	{ "quit", quit },
};

static Boolean
worker(XtPointer p)
{
	Arg	arg[10];
	int	n, samples;
	int	**data;

	n = 0;
	XtSetArg(arg[n], XtNnumSamples, &samples);	n++;
	XtSetArg(arg[n], XtNdata, &data);		n++;
	XtGetValues(p, arg, n);

	samples = read_sio(data, samples);
	exec_fft(data[0], samples);
	exec_fft(data[1], samples);

	n = 0;
	XtSetArg(arg[n], XtNnumSamples, samples);	n++;
	XtSetValues(p, arg, n);	/* trigger expose */

	return False; 	/* don't remove the work procedure from the list */
}

String fallback[] = {
	"*foreground:	#ff7f00",
	"*background:	#1f0000",
	"*borderColor:	#1f0000",
	"*borderWidth:	2",
	NULL,
};

int
main(int argc, char **argv)
{
	XtAppContext	app;
	Widget	toplevel, display;
	int	n, samples;
	Arg	args[10];
	XtAccelerators acs;

	toplevel = XtAppInitialize(&app, "Spectrograph",
		options, XtNumber(options), &argc, argv,
		fallback, NULL, 0);

	if (argc != 1)
		usage();

	samples = init_sio();
	init_fft(samples);
	warnx("%s samples: %d", __func__, samples);

	XtAppAddActions(app, actionsList, XtNumber(actionsList));
	acs = XtParseAcceleratorTable("<Key>q: quit()");

	n = 0;
	//XtSetArg(args[n], XtNaccelerators, acs);		n++;
	XtSetArg(args[n], XtNnumChannel, 2);			n++;
	XtSetArg(args[n], XtNnumSamples, samples);		n++;
	display = XtCreateManagedWidget("Display", displayWidgetClass,
		toplevel, args, n);

	XtAppAddWorkProc(app, worker, display);

	XtRealizeWidget(toplevel);
	XtAppMainLoop(app);

	return 0;
}
