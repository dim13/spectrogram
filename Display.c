/* $Id$ */

#include <err.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "DisplayP.h"

#define Printd(w, s) do {						\
	warnx("Class %s: %s", XtClass(w)->core_class.class_name, s);	\
} while (0)

static void Initialize(Widget, Widget, ArgList, Cardinal *);
static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry *, XtWidgetGeometry *);
static void ChangeManaged(Widget);
static void Resize(Widget);
static void Redisplay(Widget, XEvent *, Region);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

#define Offset(field) XtOffsetOf(DisplayRec, display.field)
static XtResource resources[] = {
	{ XtNspace, XtCSpace, XtRDimension, sizeof(Dimension),
		Offset(space), XtRImmediate, (XtPointer)2 },
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
		.initialize		= NULL,
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
		//.expose			= XtInheritExpose,
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
		.geometry_manager	= GeometryManager,
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
	warnx("Display initialize");
	new->core.width = 800;
	new->core.height = 600;
}

static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	warnx("Geometry Manager");
	return XtGeometryYes;
}

static void
ChangeManaged(Widget w)
{
	DisplayWidget dw = (DisplayWidget)w;
	Dimension width, height;
	Widget child;
	int **data;
	int i;
	Arg	arg;

	warnx("Change Managed %s", XtClass(w)->core_class.class_name);

	width = w->core.width;
	height = w->core.height;

	data = (int **)XtRealloc((char *)dw->display.data, dw->composite.num_children * sizeof(int *));
	dw->display.data = data;

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child)) {
			XtSetArg(arg, XtNdata, &dw->display.data[i]);
			XtGetValues(child, &arg, 1);
			XtMoveWidget(child,
				width + dw->display.space, dw->display.space);
			width += child->core.width
				+ 2 * child->core.border_width
				+ 2 * dw->display.space;
			height = child->core.height
				+ 2 * child->core.border_width
				+ 2 * dw->display.space;
		}
	}
	w->core.width = width;
	w->core.height = height;
}

static void
Resize(Widget w)
{
	DisplayWidget dw = (DisplayWidget)w;
	Dimension x, y, width, height, border;
	Widget child;
	int n = dw->composite.num_children;
	int i;

	width = w->core.width / n - (n + 1) * dw->display.space;
	height = w->core.height - 2 * dw->display.space;
	Printd(w, "Resize");

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child)) {
			border = child->core.border_width;
			x = dw->display.space;
			x += i * (width + 2 * border);
			y = dw->display.space;
			XtConfigureWidget(child, x, y, width - 2 * border,
				height - 2 * border, border);
		}
	}
}

static void
Redisplay(Widget w, XEvent *event, Region region)
{
	DisplayWidget dw = (DisplayWidget)w;
	Widget child;
	int i;

	for (i = 0; i < dw->composite.num_children; i++) {
		child = dw->composite.children[i];
		if (XtIsManaged(child))
			XtClass(child)->core_class.expose(child, event, region);
	}
}

static Boolean
SetValues(Widget old, Widget req, Widget new, ArgList args, Cardinal *n)
{
	return True;
}
