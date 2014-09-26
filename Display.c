/* $Id$ */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "DisplayP.h"

//static ChangeManaged(Widget);
//static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry *, XtWidgetGeometry *);

#define Offset(field) XtOffsetOf(DisplayRec, display.field)
static XtResource resources[] = {
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
		.resize			= XtInheritResize,
		.expose			= XtInheritExpose,
		.set_values		= NULL,
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
		.change_managed		= XtInheritChangeManaged,
		.insert_child		= XtInheritInsertChild,
		.delete_child		= XtInheritDeleteChild,
		.extension		= NULL,
	},
	.display_class = {
		.extension		= NULL,
	}
};

WidgetClass displayWidgetClass = (WidgetClass) & displayClassRec;
