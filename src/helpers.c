/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file src/helpers.c
  \ingroup CoreMtx
  \brief These are a bunch fo helper functions that are called from comm.xml
  
  These functions are referenced by the per ECU comm.xml which defines the
  I/O command structure and API for the device.  Most of these functions are
  helpers that do something important after some I/O operation has completed
  \author David Andruczyk 
  */

#include <3d_vetable.h>
#include <conversions.h>
#include <dashboard.h>
#include <datalogging_gui.h>
#include <init.h>
#include <listmgmt.h>
#include <notifications.h>
#include <plugin.h>
#include <runtime_sliders.h>
#include <runtime_status.h>
#include <runtime_text.h>
#include <timeout_handlers.h>
#include <widgetmgmt.h>

extern gconstpointer *global_data;

/*!
  \brief status the statuscounts timeout function
  */
G_MODULE_EXPORT void start_statuscounts_pf(void)
{
	start_tickler(SCOUNTS_TICKLER);
}


/*!
  \brief Enables the "get data" buttons on all tabs
  */
G_MODULE_EXPORT void enable_get_data_buttons_pf(void)
{
	gdk_threads_enter();
	g_list_foreach(get_list("get_data_buttons"),set_widget_sensitive,GINT_TO_POINTER(TRUE));
	gdk_threads_leave();
}


/*!
  \brief Starts the runtime var timeout if this is the first run of mtx
  */
G_MODULE_EXPORT void conditional_start_rtv_tickler_pf(void)
{
	static gboolean just_starting = TRUE;

	if (just_starting)
	{
		start_tickler(RTV_TICKLER);
		just_starting = FALSE;
	}
}


/*!
  \brief sets all burn buttons back to black 
 */
G_MODULE_EXPORT void set_store_black_pf(void)
{
	gint j = 0;
	Firmware_Details *firmware = NULL;
	static void (*slaves_set_color_f)(gint,const gchar *) = NULL;

	firmware = DATA_GET(global_data,"firmware");
	/* Only MS firmwares have TCP socket mode for now */
	if ((firmware->capabilities & MS1 ) ||
		(firmware->capabilities & MS2))
	{
		if (!slaves_set_color_f)
			get_symbol("slaves_set_color",(void *)&slaves_set_color_f);
	}

	gdk_threads_enter();
	set_group_color(BLACK,"burners");
	if (slaves_set_color_f)
		slaves_set_color_f(BLACK,"burners");
	for (j=0;j<firmware->total_tables;j++)
		set_reqfuel_color(BLACK,j);
	gdk_threads_leave();
}


/*!
  \brief Enables all applicable 3D display buttons
 */
G_MODULE_EXPORT void enable_3d_buttons_pf(void)
{
	g_list_foreach(get_list("3d_buttons"),set_widget_sensitive,GINT_TO_POINTER(TRUE));
}


/*!
  \brief Disables all burn to ecu buttons
 */
G_MODULE_EXPORT void disable_burner_buttons_pf(void)
{
	gdk_threads_enter();
	g_list_foreach(get_list("burners"),set_widget_sensitive,GINT_TO_POINTER(FALSE));
	gdk_threads_leave();
}


/*!
  \brief Resets and temperature dependant controls to rerender on temp scale
  changes
 */
G_MODULE_EXPORT void reset_temps_pf(void)
{
	gdk_threads_enter();
	set_title(g_strdup(_("Adjusting for local Temp units...")));
	reset_temps(DATA_GET(global_data,"mtx_temp_units"));
	gdk_threads_leave();
}


/*!
  \brief sets the time to be ready
  */
G_MODULE_EXPORT void ready_msg_pf(void)
{
	gdk_threads_enter();
	set_title(g_strdup(_("Ready...")));
	gdk_threads_leave();
	return;
}


/*!
  \brief Cleans up a postfunction array (special case for offline without
  chained helper/post functions
  \param message is the pointer to the Io_Message structure
  */
G_MODULE_EXPORT void cleanup_pf(Io_Message *message)
{
	dealloc_array(message->command->post_functions,POST_FUNCTIONS);
	return;
}


/*!
  \brief Starts up all the default timeout handlers for doing Gui updates,
  i.e. rtsliders, rttext, dashboards and so on.
  */
G_MODULE_EXPORT void startup_default_timeouts_pf(void)
{
	gint source = 0;
	gint rate = 0;

	gdk_threads_enter();
	set_title(g_strdup(_("Starting up data renderers...")));
	gdk_threads_leave();
	rate = (GINT)DATA_GET(global_data,"rtslider_fps");
	source = g_timeout_add_full(175,(GINT)(1000.0/(gfloat)rate),(GSourceFunc)update_rtsliders,NULL,NULL);
	DATA_SET(global_data,"rtslider_id", GINT_TO_POINTER(source));

	rate = (GINT)DATA_GET(global_data,"rttext_fps");
	source = g_timeout_add_full(180,(GINT)(1000.0/(gfloat)rate),(GSourceFunc)update_rttext,NULL,NULL);
	DATA_SET(global_data,"rttext_id", GINT_TO_POINTER(source));

	rate = (GINT)DATA_GET(global_data,"rttext_fps");
	source = g_timeout_add_full(220,(GINT)(2000.0/(gfloat)rate),(GSourceFunc)update_rtstatus,NULL,NULL);
	DATA_SET(global_data,"rtstatus_id", GINT_TO_POINTER(source));

	rate = (GINT)DATA_GET(global_data,"dashboard_fps");
	source = g_timeout_add_full(135,(GINT)(1000.0/(gfloat)rate),(GSourceFunc)update_dashboards,NULL,NULL);
	DATA_SET(global_data,"dashboard_id", GINT_TO_POINTER(source));

	source = g_timeout_add_full(500,1000,(GSourceFunc)run_datalog,NULL,NULL);
	DATA_SET(global_data,"datalog_id", GINT_TO_POINTER(source));

	source = g_timeout_add_full(210,(GINT)(1000.0/(gfloat)rate),(GSourceFunc)fire_off_rtv_watches,NULL,NULL);

	gdk_threads_enter();
	set_title(g_strdup(_("Data renderers running...")));
	gdk_threads_leave();
}

