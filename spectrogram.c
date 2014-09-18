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
#include "Sgraph.h"

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
worker(XtPointer data)
{
	Arg	arg[10];
	int	n, size, samples;
	double	*left, *right;

	n = 0;
	XtSetArg(arg[n], XtNsize, &size);	n++;
	XtSetArg(arg[n], XtNsamples, &samples);	n++;
	XtSetArg(arg[n], XtNleftData, &left);	n++;
	XtSetArg(arg[n], XtNrightData, &right);	n++;
	XtGetValues(data, arg, n);

	size = read_sio(left, right, size);
	//warnx("samples: %d, size: %d, %d", samples, size, n);
	//warnx("l/r: %p (%lf) / %p (%lf)", left, *left, right, *right);

	n = 0;
	XtSetArg(arg[n], XtNsize, size);	n++;
	XtSetValues(data, arg, n);	/* trigger expose */

	usleep(40);	/* emulate 25 Hz */
	return False; 	/* don't remove the work procedure from the list */
}

int
main(int argc, char **argv)
{
	Widget	toplevel, sgraph;
	int	n, samples;
	Arg	args[10];

	toplevel = XtInitialize(__progname, "Spectrograph",
		options, XtNumber(options), &argc, argv);
	XtAddActions(actionsList, XtNumber(actionsList));

	if (argc != 1)
		usage();

	samples = init_sio();
	warnx("samples: %d", samples);

	n = 0;
	XtSetArg(args[n], XtNsamples, samples);	n++;
	sgraph = XtCreateManagedWidget(__progname, sgraphWidgetClass,
		toplevel, args, n);

	XtOverrideTranslations(sgraph,
		XtParseTranslationTable("<Key>q: quit()"));
	XtAddWorkProc(worker, sgraph);

	XtRealizeWidget(toplevel);
	XtMainLoop();

	return 0;
}

#if 0
int 
main(int argc, char **argv)
{
	Display		*dsp;
	Window		win, container;
	Atom		delwin;
	XClassHint	*class;
	XWindowAttributes wa;
	XRectangle	geo;
	int		scr;

	struct		panel *left, *right;
	double		*ldata, *rdata;

	int		dflag = 0;	/* daemonize */
	int		fflag = 0;	/* fullscreen */
	int		pflag = 1;	/* hide ptr */

	int		ch;
	int		width, height;
	unsigned int	maxwidth, maxheight;
	unsigned long	black, white;
	float		factor = 0.75;
	int		round = 1024;	/* FFT is fastest with powers of two */

	while ((ch = getopt(argc, argv, "dfpr:h")) != -1)
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'p':
			pflag = 0;
			break;
		case 'r':
			round = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	signal(SIGINT, catch);

	if (dflag)
		daemon(0, 0);

	init_sio();
	maxwidth = max_samples_sio();
	maxheight = wa.height;
		
	dsp = XOpenDisplay(NULL);
	if (!dsp)
		errx(1, "Cannot connect to X11 server");

	if (round > maxwidth)
		round = maxwidth;

	XGetWindowAttributes(dsp, DefaultRootWindow(dsp), &wa);
	width = round + HGAP;
	if (fflag || width > wa.width) {
		round = wa.width - HGAP;
		width = wa.width;
	}

	height = factor * width;
	if (height > wa.height)
		height = wa.height;

	scr = DefaultScreen(dsp);
	white = WhitePixel(dsp, scr);
	black = BlackPixel(dsp, scr);

	win = XCreateSimpleWindow(dsp, RootWindow(dsp, scr),
		0, 0, width, height, 0, white, black);
		
	XStoreName(dsp, win, __progname);
	class = XAllocClassHint();
	if (class) {
		class->res_name = __progname;
		class->res_class = __progname;
		XSetClassHint(dsp, win, class);
		XFree(class);
	}
	XSelectInput(dsp, win, KeyPressMask|StructureNotifyMask);

	/* catch delete window */
	delwin = XInternAtom(dsp, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(dsp, win, &delwin, 1);

	/* set minimal size */
	restrictsize(dsp, win, width, height, maxwidth, maxheight);

	container = XCreateSimpleWindow(dsp, win,
		0, 0, width, height, 0, white, black);
	XMapWindow(dsp, container);

	init_fft(maxwidth, round);

	geo.x = 0;
	geo.y = 0;
	geo.width = round / 2;
	geo.height = height;
	left = init_panel(dsp, container, geo, RTL);
	ldata = dataptr(left);

	geo.x = round / 2 + HGAP;
	geo.y = 0;
	geo.width = round / 2;
	geo.height = height;
	right = init_panel(dsp, container, geo, LTR);
	rdata = dataptr(right);

	XClearWindow(dsp, win);
	XMapRaised(dsp, win);	/* XMapWindow */

	if (fflag) {
		fullscreen(dsp, win);
		if (pflag)
			hide_ptr(dsp, win);
	}

	while (!die) {
		while (XPending(dsp)) {
			XEvent ev;

			XNextEvent(dsp, &ev);

			switch (ev.type) {
			case KeyPress:
				switch (XLookupKeysym(&ev.xkey, 0)) {
				case XK_q:
					die = 1;
					break;
				case XK_1:
					toggle_mirror(left);
					break;
				case XK_2:
					toggle_mirror(right);
					break;
				case XK_3:
					toggle_mirror(left);
					toggle_mirror(right);
					break;
				default:
					break;
				}
				break;
			case ClientMessage:
				die = *ev.xclient.data.l == delwin;
				break;
			case ConfigureNotify:
				move(dsp, win, container);
				break;
			default:
				break;
			}
		}

		read_sio(ldata, rdata, round);

		exec_fft(ldata);
		exec_fft(rdata);

		draw_panel(left);
		draw_panel(right);

		flip_panel(left);
		flip_panel(right);

		if (fflag)
			XResetScreenSaver(dsp);
	}

	free_sio();
	free_fft();
	free_panel(left);
	free_panel(right);

	XDestroyWindow(dsp, win);
	XCloseDisplay(dsp);

	return 0;
}
#endif
