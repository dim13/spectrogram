/* $Id$ */

#include <err.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "DisplayP.h"
#include "SgraphP.h"

#define Trace(w) warnx("%s.%s", XtClass(w)->core_class.class_name, __func__)

static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void ChangeManaged(Widget);
static void Resize(Widget);
static void Redisplay(Widget, XEvent *, Region);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

#define Offset(field) XtOffsetOf(DisplayRec, display.field)
static XtResource resources[] = {
	{ XtNnumChannel, XtCNumChannel, XtRInt, sizeof(int),
		Offset(num_channel), XtRImmediate, (XtPointer)2 },
	{ XtNnumSamples, XtCNumSamples, XtRInt, sizeof(int),
		Offset(num_samples), XtRImmediate, (XtPointer)0 },
	{ XtNdata, XtCData, XtRPointer, sizeof(int **),
		Offset(data), XtRPointer, NULL },
};

#undef Offset

static CompositeClassExtensionRec	compositeExtension = {
	.next_extension			= NULL,
	.record_type			= NULLQUARK,
	.version			= XtCompositeExtensionVersion,
	.record_size			= sizeof(CompositeClassExtensionRec),
	.accepts_objects		= True,
	.allows_change_managed_set	= False,
};

DisplayClassRec displayClassRec = {
	.core_class = {
		.superclass		= (WidgetClass)&compositeClassRec,
		.class_name		= "Display",
		.widget_size		= sizeof(DisplayRec),
		.class_initialize	= NULL,
		.class_part_initialize	= NULL,
		.class_inited		= False,
		.initialize		= Initialize,
		.initialize_hook	= NULL,
		.realize		= XtInheritRealize,
		.actions		= NULL,
		.num_actions		= 0,
		.resources		= resources,
		.num_resources		= XtNumber(resources),
		.xrm_class		= NULLQUARK,
		.compress_motion	= True,
		.compress_exposure	= True,
		.compress_enterleave	= True,
		.visible_interest	= False,
		.destroy		= NULL,
		.resize			= Resize,
		.expose			= Redisplay,
		.set_values		= SetValues,
		.set_values_hook	= NULL,
		.set_values_almost	= XtInheritSetValuesAlmost,
		.get_values_hook	= NULL,
		.accept_focus		= XtInheritAcceptFocus,
		.version		= XtVersion,
		.callback_private	= NULL,
		.tm_table		= XtInheritTranslations,
		.query_geometry		= XtInheritQueryGeometry,
		.display_accelerator	= XtInheritDisplayAccelerator,
		.extension		= NULL,
	},
	.composite_class = {
		.geometry_manager	= XtInheritGeometryManager,
		.change_managed		= ChangeManaged,
		.insert_child		= XtInheritInsertChild,
		.delete_child		= XtInheritDeleteChild,
		.extension		= &compositeExtension,
	},
	.display_class = {
		.extension		= NULL,
	}
};

WidgetClass displayWidgetClass = (WidgetClass) & displayClassRec;

static void
Initialize(Widget req, Widget new, ArgList args, Cardinal *num_args)
{
	DisplayWidget dw = (DisplayWidget)new;
	Arg arg[10];
	int n, i;

	Trace(new);

	dw->display.data = (int **)XtCalloc(dw->display.num_channel, sizeof(int *));

	for (i = 0; i < dw->display.num_channel; i++) {
		dw->display.data[i] = (int *)XtCalloc(dw->display.num_samples,
			sizeof(int));
		n = 0;
		XtSetArg(arg[n], XtNvalues, dw->display.data[i]);	n++;
		XtSetArg(arg[n], XtNsize, dw->display.num_samples);	n++;
		XtSetArg(arg[n], XtNmirror, i % 2 ? False : True);	n++;
		XtCreateManagedWidget("SGraph", sgraphWidgetClass, new, arg, n);
	}
}

static void
ChangeManaged(Widget w)
{
	DisplayWidget dw = (DisplayWidget)w;
	Dimension width, height;
	Widget child;
	int i;

	Trace(w);

	width = w->core.width;
	height = w->core.height;

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child)) {
			XtMoveWidget(child,
				width, 0);
			width += child->core.width
				+ 2 * child->core.border_width;
			height = child->core.height
				+ 2 * child->core.border_width;
		}
	}
	w->core.width = width;
	w->core.height = height;

	/* XXX */
	dw->display.num_samples = dw->composite.children[0]->core.width * 2;
}

static void
Resize(Widget w)
{
	DisplayWidget dw = (DisplayWidget)w;
	Dimension x, y, width, height;
	Widget child;
	int n = dw->composite.num_children;
	int i;

	Trace(w);

	width = w->core.width / n;
	height = w->core.height;

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child)) {
			x = i * (width + child->core.border_width);
			y = 0;
			XtConfigureWidget(child, x, y,
				width - 2 * child->core.border_width,
				height - 2 * child->core.border_width,
				child->core.border_width);
		}
	}

	/* XXX */
	dw->display.num_samples = dw->composite.children[0]->core.width * 2;
}

static void
Redisplay(Widget w, XEvent *event, Region region)
{
	DisplayWidget dw = (DisplayWidget)w;
	Widget child;
	int i;

	//Trace(w);

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child))
			XtClass(child)->core_class.expose(child, event, region);
	}
}

static Boolean
SetValues(Widget old, Widget req, Widget new, ArgList args, Cardinal *n)
{
	XExposeEvent xeev;
	DisplayWidget dw = (DisplayWidget)new;
	Widget child;
	int i;

	//Trace(new);

	xeev.type = Expose;
	xeev.display = XtDisplay(new);
	xeev.window = XtWindow(new);
	xeev.x = 0;
	xeev.y = 0;
	xeev.width = new->core.width;
	xeev.height = new->core.height;

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child))
			XtSetValues(child, args, *n);
	}

	XtClass(new)->core_class.expose(new, (XEvent *)&xeev, NULL);

	return False;
}
