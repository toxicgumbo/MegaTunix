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
  \file src/gui_handlers.c
  \ingroup CoreMtx
  \brief Handles all interations with Gui widgets
 
  This file contains nearly all the functions that will handle gui operations
  like updating text entries, clicking buttons, moving ranges/scales and so 
  on. Since most controls will be ecu persona/firmware specific these functions
  will check for and call the persona specific handlers which will then call
  firmware specific handlers if required from the persona/firmware plugins.
  Basically for any control that could exist in potentially more than one
  ECU persona, there should be a wrapper here that calls the apprpriate function
  out of the plugin to be reasonably extensible
  \author David Andruczyk
  */

#define _ISOC99_SOURCE
#include <3d_vetable.h>
#include <args.h>
#include <binlogger.h>
#include <conversions.h>
#include <dashboard.h>
#include <datalogging_gui.h>
#include <debugging.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <init.h>
#include <keyparser.h>
#include <listmgmt.h>
#include <locking.h>
#include <logviewer_events.h>
#include <logviewer_gui.h>
#include <math.h>
#include <offline.h>
#include <notifications.h>
#include <plugin.h>
#include <runtime_sliders.h>
#include <runtime_status.h>
#include <runtime_text.h>
#include <serialio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tableio.h>
#include <tabloader.h>
#include <threads.h>
#include <timeout_handlers.h>
#include <vetable_gui.h>
#include <widgetmgmt.h>

static gboolean grab_single_cell = FALSE;
static gboolean grab_multi_cell = FALSE;
extern gconstpointer *global_data;
extern GdkColor red;
extern GdkColor green;
extern GdkColor blue;
extern GdkColor black;
extern GdkColor white;

/*!
  \brief leave() is the main shutdown function for MegaTunix. It shuts down
  whatever runnign handlers are still going, deallocates memory and quits
  \param widget is  unused
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean leave(GtkWidget *widget, gpointer data)
{
	gint id;
	gboolean res = FALSE;
	/*
	   extern GThread * ascii_socket_id;
	   extern GThread * binary_socket_id;
	   extern GThread * control_socket_id;
	 */
	GMutex *serio_mutex = NULL;
	GMutex *rtv_mutex = NULL;
	GAsyncQueue *io_repair_queue = NULL;
	gboolean tmp = TRUE;
	GIOChannel * iochannel = NULL;
	GTimeVal now;
	GtkWidget *main_window = NULL;
	static GStaticMutex leave_mutex = G_STATIC_MUTEX_INIT;
	CmdLineArgs *args = DATA_GET(global_data,"args");
	GCond *pf_dispatch_cond = NULL;
	GCond *gui_dispatch_cond = NULL;
	GCond *io_dispatch_cond = NULL;
	GCond *statuscounts_cond = NULL;
	GMutex *pf_dispatch_mutex = NULL;
	GMutex *gui_dispatch_mutex = NULL;
	GMutex *io_dispatch_mutex = NULL;
	GMutex *statuscounts_mutex = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	if (DATA_GET(global_data,"leaving"))
		return TRUE;

	//MTXDBG(CRITICAL,_("Entered\n"));
	if ((!args->be_quiet) && (DATA_GET(global_data,"interrogated")))
	{
		DATA_SET(global_data,"might_be_leaving",GINT_TO_POINTER(TRUE));
		if(!prompt_r_u_sure())
		{
			DATA_SET(global_data,"might_be_leaving",GINT_TO_POINTER(FALSE));
			return TRUE;
		}
		prompt_to_save();
	}
	g_static_mutex_lock(&leave_mutex);
	/* Commits any pending data to ECU flash */
/*	BUGGY CODE, causes deadlock, disabled....
	if ((DATA_GET(global_data,"connected")) && 
			(DATA_GET(global_data,"interrogated")) && 
			(!DATA_GET(global_data,"offline")))
	{
		QUIET_MTXDBG(CRITICAL,_("Before burn\n"));
		io_cmd(firmware->burn_all_command,NULL);
		QUIET_MTXDBG(CRITICAL,_("After burn\n"));
	}
	*/
	/* Set global flag */
	DATA_SET(global_data,"leaving",GINT_TO_POINTER(TRUE));

	main_window = lookup_widget("main_window");

	/* Stop timeout functions */
	stop_tickler(RTV_TICKLER);
	//QUIET_MTXDBG(CRITICAL,_("After stop_realtime\n"));
	stop_tickler(LV_PLAYBACK_TICKLER);
	//QUIET_MTXDBG(CRITICAL,_("After stop_lv_playback\n"));

	stop_datalogging();
	//QUIET_MTXDBG(CRITICAL,_("Aafter stop_datalogging\n"));

	/* Message to trigger serial repair queue to exit immediately */
	io_repair_queue = DATA_GET(global_data,"io_repair_queue");
	if (io_repair_queue)
		g_async_queue_push(io_repair_queue,&tmp);


	//QUIET_MTXDBG(CRITICAL,_("Configuration saved\n"));

	/* Tell plugins to shutdown */
	plugins_shutdown();

	/* IO dispatch queue */
	g_get_current_time(&now);
	g_time_val_add(&now,250000);
	io_dispatch_cond = DATA_GET(global_data,"io_dispatch_cond");
	io_dispatch_mutex = DATA_GET(global_data,"io_dispatch_mutex");
	g_mutex_lock(io_dispatch_mutex);
	res = g_cond_timed_wait(io_dispatch_cond,io_dispatch_mutex,&now);
	/*QUIET_MTXDBG(CRITICAL,_("Result of waiting for io_dispatch_condition is %i\n"),res);*/
	g_mutex_unlock(io_dispatch_mutex);

	/* Binary Log flusher */
	id = (GINT)DATA_GET(global_data,"binlog_flush_id");
	if (id)
		g_source_remove(id);
	DATA_SET(global_data,"binlog_flush_id",NULL);

	/* PF dispatch queue */
	id = (GINT)DATA_GET(global_data,"pf_dispatcher_id");
	if (id)
		g_source_remove(id);
	DATA_SET(global_data,"pf_dispatcher_id",NULL);
	g_get_current_time(&now);
	g_time_val_add(&now,250000);
	pf_dispatch_cond = DATA_GET(global_data,"pf_dispatch_cond");
	pf_dispatch_mutex = DATA_GET(global_data,"pf_dispatch_mutex");
	g_mutex_lock(pf_dispatch_mutex);
	res = g_cond_timed_wait(pf_dispatch_cond,pf_dispatch_mutex,&now);
	/*QUIET_MTXDBG(CRITICAL,_("Result of waiting for pf_dispatch_condition is %i\n"),res);*/
	g_mutex_unlock(pf_dispatch_mutex);

	/* Statuscounts timeout */
	id = (GINT)DATA_GET(global_data,"statuscounts_id");
	if (id)
		g_source_remove(id);
	DATA_SET(global_data,"statuscounts_id",NULL);
	g_get_current_time(&now);
	g_time_val_add(&now,250000);
	statuscounts_cond = DATA_GET(global_data,"statuscounts_cond");
	statuscounts_mutex = DATA_GET(global_data,"statuscounts_mutex");
	g_mutex_lock(statuscounts_mutex);
	res = g_cond_timed_wait(statuscounts_cond,statuscounts_mutex,&now);
	/*QUIET_MTXDBG(CRITICAL,_("Result of waiting for statuscounts_condition is %i\n"),res);*/
	g_mutex_unlock(statuscounts_mutex);

	/* GUI Dispatch timeout */
	id = (GINT)DATA_GET(global_data,"gui_dispatcher_id");
	if (id)
		g_source_remove(id);
	DATA_SET(global_data,"gui_dispatcher_id",NULL);
	g_get_current_time(&now);
	g_time_val_add(&now,250000);
	gui_dispatch_cond = DATA_GET(global_data,"gui_dispatch_cond");
	gui_dispatch_mutex = DATA_GET(global_data,"gui_dispatch_mutex");
	g_mutex_lock(gui_dispatch_mutex);
	res = g_cond_timed_wait(gui_dispatch_cond,gui_dispatch_mutex,&now);
	/*QUIET_MTXDBG(CRITICAL,_("Result of waiting for gui dispatch_cond is %i\n"),res);*/
	g_mutex_unlock(gui_dispatch_mutex);

	save_config();

	/*
	 * Can';t do these until I can get nonblocking socket to behave. 
	 * not sure what I'm doing wrong,  but select loop doesn't detect the 
	 * connection for some reason, so had to go back to blocking mode, thus
	 * the threads sit permanently blocked and can't catch the notify.
	 *
	 if (ascii_socket_id)
	 g_thread_join(ascii_socket_id);
	 //QUIET_MTXDBG(CRITICAL,_("After ascii socket thread shutdown\n"));
	 if (binary_socket_id)
	 g_thread_join(binary_socket_id);
	 //QUIET_MTXDBG(CRITICAL,_("After binary socket thread shutdown\n"));
	 if (control_socket_id)
	 g_thread_join(control_socket_id);
	 //QUIET_MTXDBG(CRITICAL,_("After control socket thread shutdown\n"));
	 */

	if (lookup_widget("dlog_select_log_button"))
		iochannel = (GIOChannel *) OBJ_GET(lookup_widget("dlog_select_log_button"),"data");

	if (iochannel)	
	{
		g_io_channel_shutdown(iochannel,TRUE,NULL);
		g_io_channel_unref(iochannel);
	}
	//QUIET_MTXDBG(CRITICAL,_("After datalog iochannel shutdown\n"));

	rtv_mutex = DATA_GET(global_data,"rtv_mutex");
	g_mutex_trylock(rtv_mutex);  /* <-- this makes us wait */
	g_mutex_unlock(rtv_mutex); /* now unlock */

	close_serial();
	//QUIET_MTXDBG(CRITICAL,_("After close_serial()\n"));
	unlock_serial();
	//QUIET_MTXDBG(CRITICAL,_("After unlock_serial()\n"));
	close_binary_logs();
	//QUIET_MTXDBG(CRITICAL,_("After close_binary_logs()\n"));

	/* Grab and release all mutexes to get them to relinquish
	 */
	serio_mutex = DATA_GET(global_data,"serio_mutex");
	g_mutex_lock(serio_mutex);
	g_mutex_unlock(serio_mutex);

	/* Free all buffers */
	mem_dealloc();
	//MTXDBG(CRITICAL,_("After mem_dealloc()\n"));
	//MTXDBG(CRITICAL,_("Before close_debug(), exiting...\n"));
	close_debug();
	gtk_main_quit();
	exit(0);
	return TRUE;
}


/*!
  \brief comm_port_override() is called every time the comm port text entry
  changes. it'll try to open the port and if it does it'll setup the serial 
  parameters
  \param editable is the pointer to editable widget to extract text from
  \returns TRUE
  */
G_MODULE_EXPORT gboolean comm_port_override(GtkEditable *editable)
{
	gchar *port;

	port = gtk_editable_get_chars(editable,0,-1);
	gtk_widget_modify_text(GTK_WIDGET(editable),GTK_STATE_NORMAL,&black);
	DATA_SET_FULL(global_data,"override_port",g_strdup(port),g_free);
	g_free(port);
	close_serial();
	unlock_serial();
	/* I/O thread should detect the closure and spawn the serial
	 * repair thread which should take care of flipping to the 
	 * overridden port
	 */
	return TRUE;
}


/*!
  \brief toggle_button_handler() handles all toggle buttons in MegaTunix
  \param widget is the the toggle button that changes
  \param data is unused in most cases
  \returns TRUE
  */
G_MODULE_EXPORT gboolean toggle_button_handler(GtkWidget *widget, gpointer data)
{
	static GtkSettings *settings = NULL;
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;
	void *obj_data = NULL;
	void (*function)(void);
	gint handler = 0; 
	gchar * tmpbuf = NULL;
	gboolean *tracking_focus = NULL;
	extern gconstpointer * global_data;
	tracking_focus = (gboolean *)DATA_GET(global_data,"tracking_focus");

	if (GTK_IS_OBJECT(widget))
	{
		obj_data = (void *)OBJ_GET(widget,"data");
		handler = (ToggleHandler)OBJ_GET(widget,"handler");
	}
	if (gtk_toggle_button_get_inconsistent(GTK_TOGGLE_BUTTON(widget)))
		gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(widget),FALSE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
	{	/* It's pressed (or checked) */
		switch ((ToggleHandler)handler)
		{
			case TOGGLE_NETMODE:
				DATA_SET(global_data,"network_access",GINT_TO_POINTER(TRUE));
				get_symbol("open_tcpip_sockets",(void*)&function);
				function();
				break;
			case COMM_AUTODETECT:
				DATA_SET(global_data,"autodetect_port", GINT_TO_POINTER(TRUE));
				gtk_entry_set_editable(GTK_ENTRY(lookup_widget("active_port_entry")),FALSE);
				break;
			case OFFLINE_FIRMWARE_CHOICE:
				DATA_SET_FULL(global_data,"offline_firmware_choice", g_strdup(OBJ_GET(widget,"filename")),g_free);
				break;
			case TRACKING_FOCUS:
				tmpbuf = (gchar *)OBJ_GET(widget,"table_num");
				tracking_focus[(GINT)strtol(tmpbuf,NULL,10)] = TRUE;
				break;
			case TOOLTIPS_STATE:
				if (!settings)
					settings = gtk_settings_get_default();
				if (gtk_minor_version >= 14)
					g_object_set(settings,"gtk-enable-tooltips",TRUE,NULL);
				DATA_SET(global_data,"tips_in_use",GINT_TO_POINTER(TRUE));
				break;
			case LOG_RAW_DATASTREAM:
				DATA_SET(global_data,"log_raw_datastream",GINT_TO_POINTER(TRUE));
				open_binary_logs();
				break;
			case TOGGLE_FIXED_COLOR_SCALE:
				DATA_SET(global_data,"mtx_color_scale",GINT_TO_POINTER(FIXED_COLOR_SCALE));
				break;
			case TOGGLE_AUTO_COLOR_SCALE:
				DATA_SET(global_data,"mtx_color_scale",GINT_TO_POINTER(AUTO_COLOR_SCALE));
				break;
			case TOGGLE_FAHRENHEIT:
				DATA_SET(global_data,"mtx_temp_units",GINT_TO_POINTER(FAHRENHEIT));
				reset_temps(GINT_TO_POINTER(FAHRENHEIT));
				DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
				break;
			case TOGGLE_CELSIUS:
				DATA_SET(global_data,"mtx_temp_units",GINT_TO_POINTER(CELSIUS));
				reset_temps(GINT_TO_POINTER(CELSIUS));
				DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
				break;
			case TOGGLE_KELVIN:
				DATA_SET(global_data,"mtx_temp_units",GINT_TO_POINTER(KELVIN));
				reset_temps(GINT_TO_POINTER(KELVIN));
				DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
				break;
			case COMMA:
				DATA_SET(global_data,"preferred_delimiter",GINT_TO_POINTER(COMMA));
				update_logbar("dlog_view", NULL,_("Setting Log delimiter to a \"Comma\"\n"),FALSE,FALSE,FALSE);
				DATA_SET_FULL(global_data,"delimiter",g_strdup(","),cleanup);
				break;
			case TAB:
				DATA_SET(global_data,"preferred_delimiter",GINT_TO_POINTER(TAB));
				update_logbar("dlog_view", NULL,_("Setting Log delimiter to a \"Tab\"\n"),FALSE,FALSE,FALSE);
				DATA_SET_FULL(global_data,"delimiter",g_strdup("\t"),cleanup);
				break;
			case REALTIME_VIEW:
				set_logviewer_mode(LV_REALTIME);
				break;
			case PLAYBACK_VIEW:
				set_logviewer_mode(LV_PLAYBACK);
				break;
			default:
				if (!common_handler)
				{
					if (get_symbol("common_toggle_button_handler",(void *)&common_handler))
						return common_handler(widget,data);
					else
					{
						MTXDBG(CRITICAL,_("Toggle button handler in common plugin is MISSING, BUG!\n"));
						return FALSE;
					}
				}
				else
					return common_handler(widget,data);
				break;
		}
	}
	else
	{	/* not pressed */
		switch ((ToggleHandler)handler)
		{
			case TOGGLE_NETMODE:
				DATA_SET(global_data,"network_access",GINT_TO_POINTER(FALSE));
				break;
			case COMM_AUTODETECT:
				DATA_SET(global_data,"autodetect_port", GINT_TO_POINTER(FALSE));
				gtk_entry_set_editable(GTK_ENTRY(lookup_widget("active_port_entry")),TRUE);
				gtk_entry_set_text(GTK_ENTRY(lookup_widget("active_port_entry")),DATA_GET(global_data,"override_port"));
				break;
			case TRACKING_FOCUS:
				tmpbuf = (gchar *)OBJ_GET(widget,"table_num");
				tracking_focus[(GINT)strtol(tmpbuf,NULL,10)] = FALSE;
				break;
			case TOOLTIPS_STATE:
				if (!settings)
					settings = gtk_settings_get_default();
				if (gtk_minor_version >= 14)
					g_object_set(settings,"gtk-enable-tooltips",FALSE,NULL);
				DATA_SET(global_data,"tips_in_use",GINT_TO_POINTER(FALSE));
				break;
			case LOG_RAW_DATASTREAM:
				DATA_SET(global_data,"log_raw_datastream",GINT_TO_POINTER(FALSE));
				close_binary_logs();
				break;
			case TOGGLE_FIXED_COLOR_SCALE:
			case TOGGLE_AUTO_COLOR_SCALE:
			case TOGGLE_FAHRENHEIT:
			case TOGGLE_CELSIUS:
			case TOGGLE_KELVIN:
			case COMMA:
			case TAB:
			case OFFLINE_FIRMWARE_CHOICE:
			case REALTIME_VIEW:
			case PLAYBACK_VIEW:
				/* Not pressed, just break */
				break;
			default:
				if (!common_handler)
				{
					if (get_symbol("common_toggle_button_handler",(void *)&common_handler))
						return common_handler(widget,data);
					else
					{
						MTXDBG(CRITICAL,_("Toggle button handler in common plugin is MISSING, BUG!\n"));
						return FALSE;
					}
				}
				else
					return common_handler(widget,data);
				break;
		}
	}
	return TRUE;
}


/*!
  \brief bitmask_button_handler() handles all controls that refer to a group
  of bits in a variable (i.e. a var shared by multiple controls),  most commonly
  used for check/radio buttons referring to features that control single
  bits in the firmware
  \param widget is the widget being changed
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean bitmask_button_handler(GtkWidget *widget, gpointer data)
{
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;
	gint handler = 0;
	gint tmp32 = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	gint bitval = 0;


	if (!GTK_IS_OBJECT(widget))
		return FALSE;

	if ((DATA_GET(global_data,"paused_handlers")) ||
			(!DATA_GET(global_data,"ready")))
		return TRUE;

	handler = (GINT)OBJ_GET(widget,"handler");
	bitval = (GINT)OBJ_GET(widget,"bitval");
	bitmask = (GINT)OBJ_GET(widget,"bitmask");
	bitshift = get_bitshift(bitmask);
	if (!GTK_IS_RADIO_BUTTON(widget))
		bitval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	switch ((StdHandler)handler)
	{
		case DEBUG_LEVEL:
			/* Debugging selection buttons */
			tmp32 = (GINT)DATA_GET(global_data,"dbg_lvl");
			tmp32 = tmp32 & ~bitmask;
			tmp32 = tmp32 | (bitval << bitshift);
			DATA_SET(global_data,"dbg_lvl",GINT_TO_POINTER(tmp32));
			break;

		default:
			if (!common_handler)
			{
				if (get_symbol("common_bitmask_button_handler",(void *)&common_handler))
					return common_handler(widget,data);
				else
				{
					MTXDBG(CRITICAL,_("Bitmask button handler in common plugin is MISSING, BUG!\n"));
					return FALSE;
				}
			}
			else
				return common_handler(widget,data);
			break;
	}
	return TRUE;
}



/*!
  \brief entry_changed_handler() gets called anytime a text entries is changed
  (i.e. during edit) it's main purpose is to turn the entry red to signify
  to the user it's being modified but not yet SENT to the ecu
  \param widget is the the widget being modified
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean entry_changed_handler(GtkWidget *widget, gpointer data)
{
	if ((DATA_GET(global_data,"paused_handlers")) || 
			(!DATA_GET(global_data,"ready")))
		return TRUE;

	gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&red);
	OBJ_SET(widget,"not_sent",GINT_TO_POINTER(TRUE));

	return TRUE;
}


/*!
  \brief focus_out_handler() auto-sends data IF IT IS CHANGED to the ecu thus
  hopefully ending the user confusion about why data isn't sent.
  \param widget is the the widget being modified
  \param event is unused
  \param data is unused
  \returns FALSE
  */
G_MODULE_EXPORT gboolean focus_out_handler(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	if (OBJ_GET(widget,"not_sent"))
	{
		OBJ_SET(widget,"not_sent",NULL);
		if (GTK_IS_SPIN_BUTTON(widget))
			spin_button_handler(widget, data);
		else if (GTK_IS_ENTRY(widget))
			std_entry_handler(widget, data);
		else if (GTK_IS_COMBO_BOX(widget))
			std_combo_handler(widget, data);
	}
	return FALSE;
}


/*!
  \brief slider_value_changed() handles controls based upon a slider
  sort of like spinbutton controls
  \param widget is the pointer to slider widget
  \param data is unused
  \returns if paused/not ready, return TRUE, otherwise call plugin handler
  and returm whatever it returns
  */
G_MODULE_EXPORT gboolean slider_value_changed(GtkWidget *widget, gpointer data)
{
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;

	if ((DATA_GET(global_data,"paused_handlers")) ||
			(!DATA_GET(global_data,"ready")))
	{
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
		return TRUE;
	}
	if (!common_handler)
	{
		if (get_symbol("common_slider_handler",(void *)&common_handler))
			return common_handler(widget,data);
		else
		{
			MTXDBG(CRITICAL,_("Slider handler in common plugin is MISSING, BUG!\n"));
			return FALSE;
		}
	}
	else
		return common_handler(widget,data);

}


/*!
  \brief std_entry_handler() gets called when a text entries is "activated"
  i.e. when the user hits enter. This function extracts the data, converts it
  to a number (checking for base10 or base16) performs the proper conversion
  and send it to the ECU and updates the gui to the closest value if the user
  didn't enter in an exact value.
  \param widget is the widget being modified
  \param data is not used
  \returns TRUE
  */
G_MODULE_EXPORT gboolean std_entry_handler(GtkWidget *widget, gpointer data)
{
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;

	g_return_val_if_fail(GTK_IS_OBJECT(widget),FALSE);

	if (!common_handler)
		get_symbol("common_entry_handler",(void *)&common_handler);
	g_return_val_if_fail(common_handler,FALSE);

	if ((DATA_GET(global_data,"paused_handlers")) ||
			(!DATA_GET(global_data,"ready")))
	{
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
		return TRUE;
	}
	else
	{
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
		return common_handler(widget,data);
	}
}



/*!
  \brief std_button_handler() handles all standard non toggle/check/radio
  buttons. 
  \param widget is the widget being modified
  \param data is not used
  \returns TRUE
  */
G_MODULE_EXPORT gboolean std_button_handler(GtkWidget *widget, gpointer data)
{
	/* get any datastructures attached to the widget */

	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;
	void *obj_data = NULL;
	gint handler = -1;
	Firmware_Details *firmware = NULL;
	void (*revert)(void) = NULL;
	gboolean (*create_2d_table_editor)(gint,GtkWidget *) = NULL;
	gboolean (*create_2d_table_editor_group)(GtkWidget *) = NULL;

	firmware = DATA_GET(global_data,"firmware");

	if (!GTK_IS_OBJECT(widget))
		return FALSE;

	obj_data = (void *)OBJ_GET(widget,"data");
	handler = (StdHandler)OBJ_GET(widget,"handler");

	if (handler == 0)
	{
		MTXDBG(CRITICAL,_("Handler not bound to object, CRITICAL ERROR, aborting\n"));
		return FALSE;
	}

	switch ((StdHandler)handler)
	{
		case PHONE_HOME:
			printf("Phone home (callback TCP) is not yet implemented, ask nicely\n");
			break;
		case EXPORT_SINGLE_TABLE:
			if (OBJ_GET(widget,"table_num"))
				export_single_table((GINT)strtol(OBJ_GET(widget,"table_num"),NULL,10));
			break;
		case IMPORT_SINGLE_TABLE:
			if (OBJ_GET(widget,"table_num"))
				import_single_table((GINT)strtol(OBJ_GET(widget,"table_num"),NULL,10));
			break;
		case RESCALE_TABLE:
			rescale_table(widget);
			break;
		case INTERROGATE_ECU:
			set_title(g_strdup(_("User initiated interrogation...")));
			update_logbar("interr_view","warning",_("USER Initiated ECU interrogation...\n"),FALSE,FALSE,FALSE);
			widget = lookup_widget("interrogate_button");
			if (GTK_IS_WIDGET(widget))
				gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
			io_cmd("interrogation", NULL);
			break;
		case START_PLAYBACK:
			start_tickler(LV_PLAYBACK_TICKLER);
			break;
		case STOP_PLAYBACK:
			stop_tickler(LV_PLAYBACK_TICKLER);
			break;

		case START_REALTIME:
			if (DATA_GET(global_data,"offline"))
				break;
			if (!DATA_GET(global_data,"interrogated"))
				io_cmd("interrogation", NULL);
			start_tickler(RTV_TICKLER);
			DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
			break;
		case STOP_REALTIME:
			if (DATA_GET(global_data,"offline"))
				break;
			stop_tickler(RTV_TICKLER);
			break;
		case READ_VE_CONST:
			set_title(g_strdup(_("Reading VE/Constants...")));
			io_cmd(firmware->get_all_command, NULL);
			break;
		case BURN_FLASH:
			io_cmd(firmware->burn_all_command,NULL);
			break;
		case DLOG_SELECT_ALL:
			dlog_select_all();
			break;
		case DLOG_DESELECT_ALL:
			dlog_deselect_all();
			break;
		case DLOG_SELECT_DEFAULTS:
			dlog_select_defaults();
			break;
		case CLOSE_LOGFILE:
			stop_datalogging();
			break;
		case START_DATALOGGING:
			start_datalogging();
			break;
		case STOP_DATALOGGING:
			stop_datalogging();
			break;
		case REVERT_TO_BACKUP:
			if (get_symbol("revert_to_previous_data",(void*)&revert))
				revert();
			break;
		case SELECT_PARAMS:
			if (!DATA_GET(global_data,"interrogated"))
				break;
			gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
			present_viewer_choices();
			break;
		case OFFLINE_MODE:
			set_title(g_strdup(_("Offline Mode...")));
			g_timeout_add(100,(GSourceFunc)set_offline_mode,NULL);
			break;
		case TE_TABLE:
			if (OBJ_GET(widget,"te_table_num"))
				if (get_symbol("create_2d_table_editor",(void *)&create_2d_table_editor))
					create_2d_table_editor((GINT)strtol(OBJ_GET(widget,"te_table_num"),NULL,10), NULL);
			break;
		case TE_TABLE_GROUP:
			if (get_symbol("create_2d_table_editor_group",(void *)&create_2d_table_editor_group))
				create_2d_table_editor_group(widget);
			break;

		default:
			if (!common_handler)
			{
				if (get_symbol("common_std_button_handler",(void *)&common_handler))
					return common_handler(widget,data);
				else
					MTXDBG(CRITICAL,_("Default case, common handler NOT found in plugins, BUG!\n"));
			}
			else
				return common_handler(widget,data);
			break;
	}		
	return TRUE;
}


/*!
  \brief Generic handler for all combo boxes
  \param widget is the widget being modified
  \param data is not used
  \returns TRUE
  */
G_MODULE_EXPORT gboolean std_combo_handler(GtkWidget *widget, gpointer data)
{
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;

	if (!GTK_IS_OBJECT(widget))
		return FALSE;

	if ((DATA_GET(global_data,"paused_handlers")) ||
			(!DATA_GET(global_data,"ready")))
	{
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
		return TRUE;
	}
	if (!common_handler)
	{
		if (get_symbol("common_combo_handler",(void *)&common_handler))
			return common_handler(widget,data);
		else
		{
			MTXDBG(CRITICAL,_("Combo handler in common plugin is MISSING, BUG!\n"));
			return FALSE;
		}
	}
	else
		return common_handler(widget,data);
	return TRUE;
}


/*!
  \brief Generic handler for ALL spinbuttons in MegaTunix and does
  whatever is needed for the data before sending it to the ECU
  \param widget is the widget being modified
  \param data is not used
  \returns TRUE
  */
G_MODULE_EXPORT gboolean spin_button_handler(GtkWidget *widget, gpointer data)
{
	/* Gets the value from the spinbutton then modifues the 
	 * necessary deta in the the app and calls any handlers 
	 * if necessary.  works well,  one generic function with a 
	 * select/case branch to handle the choices..
	 */
	static gboolean (*common_handler)(GtkWidget *, gpointer) = NULL;
	gint tmpi = 0;
	gint id = 0;
	gint handler = -1;
	gint source = 0;
	gfloat value = 0.0;
	GtkWidget * tmpwidget = NULL;
	Serial_Params *serial_params = NULL;

	serial_params = DATA_GET(global_data,"serial_params");

	if (!GTK_IS_WIDGET(widget))
	{
		MTXDBG(CRITICAL,_("Widget pointer is NOT valid\n"));
		return FALSE;
	}
	if ((DATA_GET(global_data,"paused_handlers")) || 
			(!DATA_GET(global_data,"ready")))
	{
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
		return TRUE;
	}

	handler = (StdHandler)OBJ_GET(widget,"handler");
	value = (float)gtk_spin_button_get_value((GtkSpinButton *)widget);

	/* Why is this here? tmpi = (int)(value+.001); */
	tmpi = (int)(value);


	switch ((StdHandler)handler)
	{
		case SER_INTERVAL_DELAY:
			serial_params->read_wait = (GINT)value;
			break;
		case SER_READ_TIMEOUT:
			DATA_SET(global_data,"read_timeout",GINT_TO_POINTER((GINT)value));
			break;
		case RTSLIDER_FPS:
			DATA_SET(global_data,"rtslider_fps",GINT_TO_POINTER(tmpi));
			source = (GINT)DATA_GET(global_data,"rtslider_id");
			if (source)
			{
				g_source_remove(source);
				id = g_timeout_add_full(175,(GINT)(1000/(float)tmpi),(GSourceFunc)update_rtsliders,NULL,NULL);
				DATA_SET(global_data,"rtslider_id",GINT_TO_POINTER(id));
			}
			break;
		case RTTEXT_FPS:
			DATA_SET(global_data,"rttext_fps",GINT_TO_POINTER(tmpi));
			source = (GINT)DATA_GET(global_data,"rttext_id");
			if (source)
			{
				g_source_remove(source);
				id = g_timeout_add_full(180,(GINT)(1000.0/(float)tmpi),(GSourceFunc)update_rttext,NULL,NULL);
				DATA_SET(global_data,"rttext_id",GINT_TO_POINTER(id));
			}
			source = (GINT)DATA_GET(global_data,"rtstatus_id");
			if (source)
			{
				g_source_remove(source);
				id = g_timeout_add_full(220,(GINT)(2000.0/(float)tmpi),(GSourceFunc)update_rtstatus,NULL,NULL);
				DATA_SET(global_data,"rtstatus_id",GINT_TO_POINTER(id));
			}
			break;
		case DASHBOARD_FPS:
			DATA_SET(global_data,"dashboard_fps",GINT_TO_POINTER(tmpi));
			source = (GINT)DATA_GET(global_data,"dashboard_id");
			if (source)
			{
				g_source_remove(source);
				id = g_timeout_add_full(135,(GINT)(1000.0/(float)tmpi),(GSourceFunc)update_dashboards,NULL,NULL);
				DATA_SET(global_data,"dashboard_id",GINT_TO_POINTER(id));
			}
			break;
		case VE3D_FPS:
			DATA_SET(global_data,"ve3d_fps",GINT_TO_POINTER(tmpi));
			break;
		case LOGVIEW_ZOOM:
			DATA_SET(global_data,"lv_zoom",GINT_TO_POINTER(tmpi));
			tmpwidget = lookup_widget("logviewer_trace_darea");	
			if (tmpwidget)
				lv_configure_event(lookup_widget("logviewer_trace_darea"),NULL,NULL);
			/*	g_signal_emit_by_name(tmpwidget,"configure_event",NULL);*/
			break;
		default:
			if (!common_handler)
			{
				if (get_symbol("common_spin_button_handler",(void *)&common_handler))
					return common_handler(widget,data);
				else
				{
					MTXDBG(CRITICAL,_("Default case, common handler NOT found in plugins, BUG!\n"));
					return TRUE;
				}
			}
			else
				return common_handler(widget,data);
			break;
	}
	return TRUE;
}



/*!
  \brief Handler for key button presses is triggered when a key is pressed 
  on a widget that has a key_press/release_event handler registered.
  \param widget is the widget where the event occurred
  \param event is the pointer to GdkEventKey event struct, which 
  contains the key that was pressed
  \param data is unused
  \returns FALSE if not handled, TRUE otherwise
  */
G_MODULE_EXPORT gboolean key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	static gint (*get_ecu_data_f)(gpointer) = NULL;
	static void (*send_to_ecu_f)(gpointer, gint, gboolean) = NULL;
	static void (*update_widget_f)(gpointer, gpointer);
	static GList ***ecu_widgets = NULL;
	DataSize size = 0;
	gint value = 0;
	gint dl_type = 0;
	gint active_table = -1;
	gint smallstep = 0;
	gint bigstep = 0;
	gint precision = 0;
	gfloat tmpf = 0.0;
	glong lower = 0;
	glong upper = 0;
	glong hardlower = 0;
	glong hardupper = 0;
	gfloat *multiplier = NULL;
	gfloat *adder = NULL;
	gint dload_val = 0;
	gboolean send = FALSE;
	gboolean retval = FALSE;
	gboolean reverse_keys = FALSE;
	gboolean *tracking_focus = NULL;

	if (!ecu_widgets)
		ecu_widgets = DATA_GET(global_data,"ecu_widgets");
	if (!get_ecu_data_f)
		get_symbol("get_ecu_data",(void *)&get_ecu_data_f);
	if (!send_to_ecu_f)
		get_symbol("send_to_ecu",(void *)&send_to_ecu_f);
	if (!update_widget_f)
		get_symbol("update_widget",(void *)&update_widget_f);
	g_return_val_if_fail(ecu_widgets,FALSE);
	g_return_val_if_fail(get_ecu_data_f,FALSE);
	g_return_val_if_fail(send_to_ecu_f,FALSE);
	g_return_val_if_fail(update_widget_f,FALSE);

	tracking_focus = (gboolean *)DATA_GET(global_data,"tracking_focus");

	dl_type = (GINT)OBJ_GET(widget,"dl_type");
	size = (DataSize) OBJ_GET(widget,"size");
	reverse_keys = (GBOOLEAN) OBJ_GET(widget,"reverse_keys");
	if (OBJ_GET(widget,"table_num"))
		active_table = (GINT)strtol(OBJ_GET(widget,"table_num"),NULL,10);
	if (OBJ_GET(widget,"raw_lower"))
		lower = (GINT)strtol(OBJ_GET(widget,"raw_lower"),NULL,10);
	else
		lower = get_extreme_from_size(size,LOWER);
	if (OBJ_GET(widget,"raw_upper"))
		upper = (GINT)strtol(OBJ_GET(widget,"raw_upper"),NULL,10);
	else
		upper = get_extreme_from_size(size,UPPER);
	precision = (GINT)OBJ_GET(widget,"precision");
	multiplier = OBJ_GET(widget,"fromecu_mult");
	adder = OBJ_GET(widget,"fromecu_add");
	smallstep = (GINT)OBJ_GET(widget,"smallstep");
	bigstep = (GINT)OBJ_GET(widget,"bigstep");
	/* Get factor to give sane small/bigstep no matter what */
	if (smallstep == 0)
	{
		tmpf = pow(10.0,(double)-precision);
		if (multiplier)
			smallstep = (GINT)(tmpf/(*multiplier));
		else
			smallstep = (GINT)tmpf;
		smallstep = smallstep < 1 ? 1:smallstep;
		OBJ_SET(widget,"smallstep",GINT_TO_POINTER(smallstep));
	}
	if (bigstep == 0)
	{
		tmpf = pow(10.0,(double)-(precision-1));
		if (multiplier)
			bigstep = (GINT)(tmpf/(*multiplier));
		else
			bigstep = (GINT)tmpf;
		bigstep = bigstep < 10 ? 10:bigstep;
		OBJ_SET(widget,"bigstep",GINT_TO_POINTER(bigstep));
	}
	hardlower = get_extreme_from_size(size,LOWER);
	hardupper = get_extreme_from_size(size,UPPER);

	upper = upper > hardupper ? hardupper:upper;
	lower = lower < hardlower ? hardlower:lower;
	/* In the rare case where bigstep exceeds the limit, reset to more
	   sane values.  Only should happen on extreme conversions
	 */
	if (bigstep > upper)
	{
		bigstep = upper/10;
		smallstep = bigstep/10;
	}
	value = get_ecu_data_f(widget);
	DATA_SET(global_data,"active_table",GINT_TO_POINTER(active_table));

	if (event->keyval == GDK_Control_L)
	{
		if (event->type == GDK_KEY_PRESS)
			grab_single_cell = TRUE;
		else
			grab_single_cell = FALSE;
		return FALSE;
	}
	if (event->keyval == GDK_Shift_L)
	{
		if (event->type == GDK_KEY_PRESS)
			grab_multi_cell = TRUE;
		else
			grab_multi_cell = FALSE;
		return FALSE;
	}

	if (event->type == GDK_KEY_RELEASE)
	{
		/*		grab_single_cell = FALSE;
				grab_multi_cell = FALSE;
		 */
		return FALSE;
	}
	switch (event->keyval)
	{
		case GDK_Page_Up:
			if (reverse_keys)
			{
				if (value >= (lower+bigstep))
					dload_val = value - bigstep;
				else
					dload_val = lower;
			}
			else 
			{
				if (value <= (upper-bigstep))
					dload_val = value + bigstep;
				else
					dload_val = upper;
			}
			send = TRUE;
			retval = TRUE;
			break;
		case GDK_Page_Down:
			if (reverse_keys)
			{
				if (value <= (upper-bigstep))
					dload_val = value + bigstep;
				else
					dload_val = upper;
			}
			else 
			{
				if (value >= (lower+bigstep))
					dload_val = value - bigstep;
				else
					dload_val = lower;
			}
			send = TRUE;
			retval = TRUE;
			break;
		case GDK_plus:
		case GDK_KP_Add:
		case GDK_KP_Equal:
		case GDK_equal:
		case GDK_Q:
		case GDK_q:
			if (reverse_keys)
			{
				if (value >= (lower+smallstep))
					dload_val = value - smallstep;
				else
					dload_val = lower;
			}
			else 
			{
				if (value <= (upper-smallstep))
					dload_val = value + smallstep;
				else
					dload_val = upper;
			}
			send = TRUE;
			retval = TRUE;
			break;
		case GDK_W:
		case GDK_w:
			if (reverse_keys)
			{
				if (value <= (upper-smallstep))
					dload_val = value + smallstep;
				else
					dload_val = upper;
			}
			else 
			{
				if (value >= (lower+smallstep))
					dload_val = value - smallstep;
				else
					dload_val = lower;
			}
			send = TRUE;
			retval = TRUE;
			break;
		case GDK_minus:
		case GDK_KP_Subtract:
			if (lower >= 0)
			{
				if (reverse_keys)
				{
					if (value <= (upper-smallstep))
						dload_val = value + smallstep;
					else
						dload_val = upper;
				}
				else 
				{
					if (value >= (lower+smallstep))
						dload_val = value - smallstep;
					else
						dload_val = lower;
				}
				send = TRUE;
				retval = TRUE;
			}
			break;
		case GDK_H:
		case GDK_h:
		case GDK_KP_Left:
		case GDK_Left:
			if (active_table >= 0)
			{
				refocus_cell(widget,GO_LEFT);
				if (grab_single_cell)
					widget_grab(widget,(GdkEventButton *)event,GINT_TO_POINTER(TRUE));
			}
			retval = TRUE;
			break;
		case GDK_L:
		case GDK_l:
		case GDK_KP_Right:
		case GDK_Right:
			if (active_table >= 0)
			{
				refocus_cell(widget,GO_RIGHT);
				if (grab_single_cell)
					widget_grab(widget,(GdkEventButton *)event,GINT_TO_POINTER(TRUE));
			}
			retval = TRUE;
			break;
		case GDK_K:
		case GDK_k:
		case GDK_KP_Up:
		case GDK_Up:
			if (active_table >= 0)
			{
				refocus_cell(widget,GO_UP);
				if (grab_single_cell)
					widget_grab(widget,(GdkEventButton *)event,GINT_TO_POINTER(TRUE));
			}
			retval = TRUE;
			break;
		case GDK_J:
		case GDK_j:
		case GDK_KP_Down:
		case GDK_Down:
			if (active_table >= 0)
			{
				refocus_cell(widget,GO_DOWN);
				if (grab_single_cell)
					widget_grab(widget,(GdkEventButton *)event,GINT_TO_POINTER(TRUE));
			}
			retval = TRUE;
			break;
		case GDK_F:
		case GDK_f:
			if (tracking_focus[active_table])
				tracking_focus[active_table] = FALSE;
			else
				tracking_focus[active_table] = TRUE;
			retval = TRUE;
			break;
		case GDK_Escape:
			if (dl_type != IGNORED)
				update_widget_f(widget,NULL);
			retval = FALSE;
			break;
		case GDK_Return:
		case GDK_KP_Enter:
			if (GTK_IS_SPIN_BUTTON(widget))
				spin_button_handler(widget, NULL);
			else
				std_entry_handler(widget,NULL);
			retval = FALSE;
			break;
		default:	
			retval = FALSE;
			break;
	}
	if ((send) && (dl_type == IMMEDIATE))
		send_to_ecu_f(widget,dload_val,TRUE);
	return retval;
}


/*!
  \brief Validates the input is numeric/puntuation (numbers) and filters
  as necessary
  \param entry is the pointer to GtkEntry
  \param text is the new text typed into the entry
  \param len is the length of new text entered
  \param pos is the pointer to the position of the cursor within the entry
  \param data is unused
  */
G_MODULE_EXPORT void insert_text_handler(GtkEntry *entry, const gchar *text, gint len, gint *pos, gpointer data)
{
	gint count = 0;
	gint i = 0;
	GtkEditable *editable = GTK_EDITABLE(entry);
	gchar *result = NULL;

	if ((DATA_GET(global_data,"paused_handlers")) ||
			(!DATA_GET(global_data,"ready")))
		return;

	result = g_new (gchar, len);
	for (i=0; i < len; i++) 
	{
		if ((g_ascii_isdigit(text[i])) || g_ascii_ispunct(text[i]))
			result[count++] = text[i];
	}
	if (count > 0)
	{
		g_signal_handlers_block_by_func (G_OBJECT (editable),
				G_CALLBACK (insert_text_handler),
				data);
		gtk_editable_insert_text (editable, result, count, pos);
		g_signal_handlers_unblock_by_func (G_OBJECT (editable),
				G_CALLBACK (insert_text_handler),
				data);
	}
	g_signal_stop_emission_by_name (G_OBJECT (editable), "insert_text");

	g_free (result);
}


/*!
  \brief used on Tables only to "select" the widget or 
  group of widgets for rescaling . Requires shift key to be held down and click
  on each spinner/entry that you want to select for rescaling
  \param widget is the widget being selected
  \param event is the struct containing details on the event
  \param data is unused
  \returns FALSE to allow other handlers to run
  */
G_MODULE_EXPORT gboolean widget_grab(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gboolean marked = FALSE;
	extern GdkColor red;
	static gint total_marked = 0;
	GtkWidget *frame = NULL;
	GtkWidget *parent = NULL;
	gchar * frame_name = NULL;

	if ((GBOOLEAN)data == TRUE)
		goto testit;

	if (event->button != 1) /* Left button click  */
		return FALSE;

	if (!grab_single_cell)
		return FALSE;

testit:
	marked = (GBOOLEAN)OBJ_GET(widget,"marked");

	if (marked)
	{
		total_marked--;
		OBJ_SET(widget,"marked",GINT_TO_POINTER(FALSE));
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&black);
	}
	else
	{
		total_marked++;
		OBJ_SET(widget,"marked",GINT_TO_POINTER(TRUE));
		gtk_widget_modify_text(widget,GTK_STATE_NORMAL,&red);
	}

	parent = gtk_widget_get_parent(GTK_WIDGET(widget));
	frame_name = (gchar *)OBJ_GET(parent,"rescaler_frame");
	if (!frame_name)
	{
		MTXDBG(CRITICAL,_("\"rescaler_frame\" key could NOT be found\n"));
		return FALSE;
	}

	frame = lookup_widget( frame_name);
	if ((total_marked > 0) && (frame != NULL))
		gtk_widget_set_sensitive(GTK_WIDGET(frame),TRUE);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(frame),FALSE);

	return FALSE;	/* Allow other handles to run...  */

}


/*
  \brief Handler that is fired off whenever a new notebook page is chosen.
  This function just sets a variable marking the current page.  this is to
  prevent the runtime sliders from being updated if they aren't visible
  \param notebook is the notebook that emitted the event
  \param page is the page
  \param page_no is the page number that's now active
  \param data is unused
  */
G_MODULE_EXPORT void notebook_page_changed(GtkNotebook *notebook, GtkWidget *page, guint page_no, gpointer data)
{
	static void (*update_widget_f)(gpointer, gpointer);
	static gint last_notebook_page = -1;
	gint tab_ident = 0;
	gint sub_page = 0;
	gint active_table = -1;
	GList *tab_widgets = NULL;
	GList *func_list = NULL;
	GList *func_fps_list = NULL;
	gint i = 0;
	gint id = 0;
	gint fps = 0;
	GSourceFunc func = NULL;
	GtkWidget *sub = NULL;
	GtkWidget *topframe = NULL;
	GtkWidget *widget = gtk_notebook_get_nth_page(notebook,page_no);

	if (!update_widget_f)
		get_symbol("update_widget",(void *)&update_widget_f);
	g_return_if_fail(update_widget_f);

	if (OBJ_GET(gtk_notebook_get_tab_label(notebook,widget),"not_rendered"))
	{
		g_signal_handlers_block_by_func (G_OBJECT (notebook),
				G_CALLBACK (notebook_page_changed),
				data);
		set_title(g_strdup(_("Rendering Tab...")));
		load_actual_tab(notebook,page_no);
		g_signal_handlers_unblock_by_func (G_OBJECT (notebook),
				G_CALLBACK (notebook_page_changed),
				data);
		set_title(g_strdup(_("Ready")));
	}
	topframe = OBJ_GET(widget,"topframe");
	if (!topframe)
		topframe = widget;
	tab_widgets = OBJ_GET(topframe,"tab_widgets");
	if (tab_widgets)
	{
		set_title(g_strdup(_("Updating Tab with current data...")));
		g_list_foreach(tab_widgets,update_widget_f,NULL);
	}
	set_title(g_strdup(_("Ready...")));
	tab_ident = (TabIdent)OBJ_GET(topframe,"tab_ident");
	DATA_SET(global_data,"active_page",GINT_TO_POINTER(tab_ident));

	if (tab_ident == RUNTIME_TAB)
		DATA_SET(global_data,"rt_forced_update",GINT_TO_POINTER(TRUE));

#if GTK_MINOR_VERSION >= 18
	if ((OBJ_GET(topframe,"table_num")) && (gtk_widget_get_state(topframe) != GTK_STATE_INSENSITIVE))
#else
	if ((OBJ_GET(topframe,"table_num")) && (GTK_WIDGET_STATE(topframe) != GTK_STATE_INSENSITIVE))
#endif
	{
		active_table = (GINT)strtol(OBJ_GET(topframe,"table_num"),NULL,10);
		func_list = OBJ_GET(topframe,"func_list");
		func_fps_list = OBJ_GET(topframe,"func_fps_list");
		if (func_list)
		{
			for (i=0;i<g_list_length(func_list);i++)
			{
				func = (GSourceFunc)g_list_nth_data(func_list,i);
				fps = (GINT)g_list_nth_data(func_fps_list,i);
				id = g_timeout_add_full(110,1000.0/fps,func,NULL,NULL);
				g_signal_connect(G_OBJECT(notebook), "switch_page",
						G_CALLBACK(cancel_visible_function),
						GINT_TO_POINTER(id));
			}
		}
	}
	else
		active_table = -1;

	if (OBJ_GET(topframe,"sub-notebook"))
	{
		sub = lookup_widget( (OBJ_GET(topframe,"sub-notebook")));
		if (GTK_IS_WIDGET(sub))
		{
			sub_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(sub));
			widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(sub),sub_page);
#if GTK_MINOR_VERSION >= 18
			if ((OBJ_GET(widget,"table_num")) && (gtk_widget_get_state(widget) != GTK_STATE_INSENSITIVE))
#else
			if ((OBJ_GET(widget,"table_num")) && (GTK_WIDGET_SENSITIVE(widget) != GTK_STATE_INSENSITIVE))
#endif
			{
				func_list = OBJ_GET(widget,"func_list");
				func_fps_list = OBJ_GET(widget,"func_fps_list");
				if (func_list)
				{
					for (i=0;i<g_list_length(func_list);i++)
					{
						func = (GSourceFunc)g_list_nth_data(func_list,i);
						fps = (GINT)g_list_nth_data(func_fps_list,i);
						id = g_timeout_add_full(110,1000.0/fps,func,NULL,NULL);
						g_signal_connect(G_OBJECT(notebook), "switch_page",
								G_CALLBACK(cancel_visible_function),
								GINT_TO_POINTER(id));
						g_signal_connect(G_OBJECT(sub), "switch_page",
								G_CALLBACK(cancel_visible_function),
								GINT_TO_POINTER(id));
					}
				}

				active_table = (GINT)strtol((gchar *)OBJ_GET(widget,"table_num"),NULL,10);
			}
			else
				active_table = -1;
			 
		}
	}
	DATA_SET(global_data,"active_table",GINT_TO_POINTER(active_table));
	DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
	last_notebook_page = page_no;
	return;
}


/*
  \brief Handler that is fired off whenever a new sub-notebook page is chosen.
  This function just sets a variable marking the current page.  this is to
  prevent the runtime sliders from being updated if they aren't visible
  \param notebook is the nbotebook that emitted the event
  \param page is the page
  \param page_no is the page number that's now active
  \param data is unused
  */
G_MODULE_EXPORT void subtab_changed(GtkNotebook *notebook, GtkWidget *page, guint page_no, gpointer data)
{
	gint active_table = -1;
	gint id = 0;
	GtkWidget *widget = gtk_notebook_get_nth_page(notebook,page_no);
	GtkWidget *parent = lookup_widget("toplevel_notebook");
	GList *func_list = NULL;
	GList *func_fps_list = NULL;
	gint i = 0;
	GSourceFunc func = NULL;
	gint fps = 0;

	if (OBJ_GET(widget,"table_num"))
	{
		active_table = (GINT)strtol((gchar *)OBJ_GET(widget,"table_num"),NULL,10);
		DATA_SET(global_data,"active_table",GINT_TO_POINTER(active_table));
		DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
	}

#if GTK_MINOR_VERSION >= 18
	if ((OBJ_GET(widget,"table_num")) && (gtk_widget_get_state(widget) != GTK_STATE_INSENSITIVE))
#else
	if ((OBJ_GET(widget,"table_num")) && (GTK_WIDGET_SENSITIVE(widget) != GTK_STATE_INSENSITIVE))
#endif
	{
		func_list = OBJ_GET(widget,"func_list");
		func_fps_list = OBJ_GET(widget,"func_fps_list");
		if (func_list)
		{
			for (i=0;i<g_list_length(func_list);i++)
			{
				func = (GSourceFunc)g_list_nth_data(func_list,i);
				fps = (GINT)g_list_nth_data(func_fps_list,i);
				id = g_timeout_add_full(110,1000.0/fps,func,NULL,NULL);
				g_signal_connect(G_OBJECT(notebook), "switch_page",
						G_CALLBACK(cancel_visible_function),
						GINT_TO_POINTER(id));
				g_signal_connect(G_OBJECT(parent), "switch_page",
						G_CALLBACK(cancel_visible_function),
						GINT_TO_POINTER(id));
			}
		}
	}
	return;
}



/*!
  \brief Handles the buttons that deal with the Fueling
  algorithm, as special things need to be taken care of to enable proper
  display of data.
  \param widget is the the toggle button that changes
  \param data is unused in most cases
  \returns TRUE if it handles
  */
G_MODULE_EXPORT gboolean set_algorithm(GtkWidget *widget, gpointer data)
{
	gint algo = 0; 
	gint tmpi = 0;
	gint i = 0;
	gint *algorithm = NULL;
	gchar *tmpbuf = NULL;
	gchar **vector = NULL;
	extern gconstpointer *global_data;

	algorithm = (gint *)DATA_GET(global_data,"algorithm");

	if (GTK_IS_OBJECT(widget))
	{
		algo = (Algorithm)OBJ_GET(widget,"algorithm");
		tmpbuf = (gchar *)OBJ_GET(widget,"applicable_tables");
	}
	if (gtk_toggle_button_get_inconsistent(GTK_TOGGLE_BUTTON(widget)))
		gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(widget),FALSE);
	/* Split string to pieces to grab the list of tables this algorithm
	 * applies to
	 */
	vector = g_strsplit(tmpbuf,",",-1);
	if (!vector)
		return FALSE;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
	{	/* It's pressed (or checked) */
		i = 0;
		while (vector[i])
		{
			tmpi = (GINT)strtol(vector[i],NULL,10);
			algorithm[tmpi]=(Algorithm)algo;
			i++;
		}
		DATA_SET(global_data,"forced_update",GINT_TO_POINTER(TRUE));
	}
	g_strfreev(vector);
	return TRUE;
}



/*!
 * \brief dummy handler to prevent window closing
 */
G_MODULE_EXPORT gboolean prevent_close(GtkWidget *widget, gpointer data)
{
	return TRUE;
}


/*!
 * \brief prompts user to save internal datalog and ecu backup
 */
G_MODULE_EXPORT void prompt_to_save(void)
{
	gint result = 0;
	GtkWidget *dialog = NULL;
	GtkWidget *label = NULL;
	GtkWidget *hbox = NULL;
	GdkPixbuf *pixbuf = NULL;
	GtkWidget *image = NULL;
	void (*do_ecu_backup)(GtkWidget *, gpointer) = NULL;


	if (DATA_GET(global_data,"offline"))
		return;
		dialog = gtk_dialog_new_with_buttons(_("Save internal log, yes/no ?"),
				GTK_WINDOW(lookup_widget("main_window")),GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_YES,GTK_RESPONSE_YES,
				GTK_STOCK_NO,GTK_RESPONSE_NO,
				NULL);
		hbox = gtk_hbox_new(FALSE,0);
		//gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hbox,TRUE,TRUE,10);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),hbox,TRUE,TRUE,10);
		pixbuf = gtk_widget_render_icon (hbox,GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG,NULL);
		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_box_pack_start(GTK_BOX(hbox),image,TRUE,TRUE,10);
		label = gtk_label_new(_("Would you like to save the internal datalog for this session to disk?  It is a complete log and useful for playback/analysis at a future point in time"));
		gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
		gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,10);
		gtk_widget_show_all(hbox);

		gtk_window_set_transient_for(GTK_WINDOW(gtk_widget_get_toplevel(dialog)),GTK_WINDOW(lookup_widget("main_window")));

		result = gtk_dialog_run(GTK_DIALOG(dialog));
		g_object_unref(pixbuf);
		if (result == GTK_RESPONSE_YES)
			internal_datalog_dump(NULL,NULL);
		gtk_widget_destroy (dialog);


	dialog = gtk_dialog_new_with_buttons(_("Save ECU settings to file?"),
			GTK_WINDOW(lookup_widget("main_window")),GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_YES,GTK_RESPONSE_YES,
			GTK_STOCK_NO,GTK_RESPONSE_NO,
			NULL);
	hbox = gtk_hbox_new(FALSE,0);
	//gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hbox,TRUE,TRUE,10);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),hbox,TRUE,TRUE,10);
	pixbuf = gtk_widget_render_icon (hbox,GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG,NULL);
	image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_box_pack_start(GTK_BOX(hbox),image,TRUE,TRUE,10);
	label = gtk_label_new(_("Would you like to save the ECU settings to a file so that they can be restored at a future time?"));
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,10);
	gtk_widget_show_all(hbox);

	gtk_window_set_transient_for(GTK_WINDOW(gtk_widget_get_toplevel(dialog)),GTK_WINDOW(lookup_widget("main_window")));
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_YES)
	{
		get_symbol("select_file_for_ecu_backup",(void *)&do_ecu_backup);
		do_ecu_backup(NULL,NULL);
	}
	gtk_widget_destroy (dialog);

}


/*!
 * \brief prompts user for yes/no to quit
 */
G_MODULE_EXPORT gboolean prompt_r_u_sure(void)
{
	gint result = 0;
	GtkWidget *dialog = NULL;
	GtkWidget *label = NULL;
	GtkWidget *hbox = NULL;
	GdkPixbuf *pixbuf = NULL;
	GtkWidget *image = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Quit MegaTunix, yes/no ?"),
			GTK_WINDOW(lookup_widget("main_window")),GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_YES,GTK_RESPONSE_YES,
			GTK_STOCK_NO,GTK_RESPONSE_NO,
			NULL);
	hbox = gtk_hbox_new(FALSE,0);
	//gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hbox,TRUE,TRUE,10);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),hbox,TRUE,TRUE,10);
	pixbuf = gtk_widget_render_icon (hbox,GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG,NULL);
	image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_box_pack_start(GTK_BOX(hbox),image,TRUE,TRUE,10);
	label = gtk_label_new(_("Are you sure you want to quit?"));
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,10);
	gtk_widget_show_all(hbox);

	gtk_window_set_transient_for(GTK_WINDOW(gtk_widget_get_toplevel(dialog)),GTK_WINDOW(lookup_widget("main_window")));

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	g_object_unref(pixbuf);
	gtk_widget_destroy (dialog);

	if (result == GTK_RESPONSE_YES)
		return TRUE;
	else 
		return FALSE;
	return FALSE;
}


/*!
  \brief gets the bitshift from the provided mask 
  */
G_MODULE_EXPORT guint get_bitshift(guint mask)
{
	gint i = 0;
	for (i=0;i<32;i++)
		if (mask & (1 << i))
			return i;
	return 0;
}


/*
  \brief sets the value of a miscelaneous gauge
  */
G_MODULE_EXPORT void update_misc_gauge(RtvWatch *watch)
{
	if (GTK_IS_WIDGET(watch->user_data))
		mtx_gauge_face_set_value(MTX_GAUGE_FACE(watch->user_data),watch->val);
	else
		remove_rtv_watch(watch->id);
}


/*!
  \brief Gets the min/max based on the size enumeration passed 
  */
G_MODULE_EXPORT glong get_extreme_from_size(DataSize size,Extreme limit)
{
	glong lower_limit = 0;
	glong upper_limit = 0;

	switch (size)
	{
		case MTX_CHAR:
		case MTX_S08:
			lower_limit = G_MININT8;
			upper_limit = G_MAXINT8;
			break;
		case MTX_U08:
			lower_limit = 0;
			upper_limit = G_MAXUINT8;
			break;
		case MTX_S16:
			lower_limit = G_MININT16;
			upper_limit = G_MAXINT16;
			break;
		case MTX_U16:
			lower_limit = 0;
			upper_limit = G_MAXUINT16;
			break;
		case MTX_S32:
			lower_limit = G_MININT32;
			upper_limit = G_MAXINT32;
			break;
		case MTX_U32:
			lower_limit = 0;
			upper_limit = G_MAXUINT32;
			break;
		case MTX_UNDEF:
			break;
	}
	switch (limit)
	{
		case LOWER:
			return lower_limit;
			break;
		case UPPER:
			return upper_limit;
			break;
	}
	return 0;
}


/*!
  \brief Clamps a value to it's limits and updates if needed 
 */
G_MODULE_EXPORT gboolean clamp_value(GtkWidget *widget, gpointer data)
{
	gint lower = 0;
	gint upper = 0;
	gint precision = 0;
	gfloat val = 0.0;
	gboolean clamped = FALSE;

	if (OBJ_GET(widget,"raw_lower"))
		lower = (GINT)strtol(OBJ_GET(widget,"raw_lower"),NULL,10);
	else
		lower = get_extreme_from_size((DataSize)OBJ_GET(widget,"size"),LOWER);
	if (OBJ_GET(widget,"raw_upper"))
		upper = (GINT)strtol(OBJ_GET(widget,"raw_upper"),NULL,10);
	else
		upper = get_extreme_from_size((DataSize)OBJ_GET(widget,"size"),UPPER);
	precision = (GINT)OBJ_GET(widget,"precision");

	val = g_ascii_strtod(gtk_entry_get_text(GTK_ENTRY(widget)),NULL);

	if (val > upper)
	{
		val = upper;
		clamped = TRUE;
	}
	if (val < lower)
	{
		val = lower;
		clamped = TRUE;
	}
	if (clamped)
		gtk_entry_set_text(GTK_ENTRY(widget),g_strdup_printf("%1$.*2$f",val,precision));
	return TRUE;
}


/*!
  \brief refocuses a cell based on a direction to go within a table. 
  This is hacky and I don't like it at all
  */
G_MODULE_EXPORT void refocus_cell(GtkWidget *widget, Direction dir)
{
	gchar *widget_name = NULL;
	GtkWidget *widget_2_focus = NULL;
	gchar *ptr = NULL;
	gchar *tmpbuf = NULL;
	gchar *prefix = NULL;
	gboolean return_now = FALSE;
	gint table_num = -1;
	gint row = -1;
	gint col = -1;
	gint index = -1;
	gint count = -1;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	widget_name = OBJ_GET(widget,"fullname");
	if (!widget_name)
		return;
	if (OBJ_GET(widget,"table_num"))
		table_num = (GINT) strtol(OBJ_GET(widget,"table_num"),NULL,10);
	else
		return;

	ptr = g_strrstr_len(widget_name,strlen(widget_name),"_of_");
	ptr = g_strrstr_len(widget_name,ptr-widget_name,"_");
	tmpbuf = g_strdelimit(g_strdup(ptr),"_",' ');
	prefix = g_strndup(widget_name,ptr-widget_name);
	sscanf(tmpbuf,"%d of %d",&index, &count);
	g_free(tmpbuf);
	row = index/firmware->table_params[table_num]->x_bincount;
	col = index%firmware->table_params[table_num]->x_bincount;

	switch (dir)
	{
		case GO_LEFT:
			if (col == 0)
				return_now = TRUE;
			else
				col--;
			break;
		case GO_RIGHT:
			if (col > firmware->table_params[table_num]->x_bincount-2)
				return_now = TRUE;
			else
				col++;
			break;
		case GO_UP:
			if (row > firmware->table_params[table_num]->y_bincount-2)
				return_now = TRUE;
			else
				row++;
			break;
		case GO_DOWN:
			if (row == 0)
				return_now = TRUE;
			else
				row--;
			break;
	}
	if (return_now)
		return;
	tmpbuf = g_strdup_printf("%s_%i_of_%i",prefix,col+(row*firmware->table_params[table_num]->x_bincount),count);

	widget_2_focus = lookup_widget(tmpbuf);
	if (GTK_IS_WIDGET(widget_2_focus))
		gtk_widget_grab_focus(widget_2_focus);

	g_free(tmpbuf);
}


/*!
  \brief set_widget_labels takes a passed string which is a comma 
  separated string of name/value pairs, first being the widget name 
  (global name) and the second being the string to set on this widget
  */
G_MODULE_EXPORT void set_widget_labels(const gchar *input)
{
	gchar ** vector = NULL;
	gint count = 0;
	gint i = 0;
	GtkWidget * widget = NULL;

	if (!input)
		return;

	vector = parse_keys(input,&count,",");
	if (count%2 != 0)
	{
		MTXDBG(CRITICAL,_("String passed was not properly formatted, should be an even number of elements, Total elements %i, string itself \"%s\""),count,input);

		return;
	}
	for(i=0;i<count;i+=2)
	{
		widget = lookup_widget(vector[i]);
		if ((widget) && (GTK_IS_LABEL(widget)))
			gtk_label_set_text(GTK_LABEL(widget),vector[i+1]);
		else
			MTXDBG(CRITICAL,_("Widget \"%s\" could NOT be located or is NOT a label\n"),vector[i]);

	}
	g_strfreev(vector);

}


/*!
  \brief swap_labels() is a utility function that extracts the list passed to 
  it, and kicks off a subfunction to do the swapping on each widget in the list
  \param widget is the name of list to enumeration and run the subfunction on
  \param state is the state passed on to the subfunction
  */
G_MODULE_EXPORT void swap_labels(GtkWidget *widget, gboolean state)
{
	GList *list = NULL;
	GtkWidget *tmpwidget = NULL;
	gchar **fields = NULL;
	gint i = 0;
	gint num_widgets = 0;

	fields = parse_keys(OBJ_GET(widget,"swap_labels"),&num_widgets,",");

	for (i=0;i<num_widgets;i++)
	{
		tmpwidget = NULL;
		tmpwidget = lookup_widget(fields[i]);
		if (GTK_IS_WIDGET(tmpwidget))
			switch_labels((gpointer)tmpwidget,GINT_TO_POINTER(state));
		else if ((list = get_list(fields[i])) != NULL)
			g_list_foreach(list,switch_labels,GINT_TO_POINTER(state));
	}
	g_strfreev(fields);
}


/*!
  \brief switch_labels() swaps labels that depend on the state of another 
  control. Handles temp dependant labels as well..
  \param key (gpointer) gpointer encapsulation of the widget
  \param data (gpointer)  gpointer encapsulation of the target state if TRUE 
  we use the alternate label, if FALSE we use
  the default label
  */
G_MODULE_EXPORT void switch_labels(gpointer key, gpointer data)
{
	GtkWidget * widget = (GtkWidget *) key;
	gboolean state = (GBOOLEAN) data;
	gint mtx_temp_units;

	mtx_temp_units = (GINT)DATA_GET(global_data,"mtx_temp_units");
	if (GTK_IS_WIDGET(widget))
	{
		if ((GBOOLEAN)OBJ_GET(widget,"temp_dep") == TRUE)
		{
			if (state)
			{
				if (mtx_temp_units == FAHRENHEIT)
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"alt_f_label"));
				else if (mtx_temp_units == KELVIN)
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"alt_k_label"));
				else
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"alt_c_label"));
			}
			else
			{
				if (mtx_temp_units == FAHRENHEIT)
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"f_label"));
				else if (mtx_temp_units == KELVIN)
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"k_label"));
				else
					gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"c_label"));
			}
		}
		else
		{
			if (state)
				gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"alt_label"));
			else
				gtk_label_set_text(GTK_LABEL(widget),OBJ_GET(widget,"label"));
		}
	}
}



/*!
 * \brief combo_toggle_groups_linked is used to change the state of 
 * controls that
 * are "linked" to various other controls for the purpose of making the 
 * UI more intuitive.  i.e. if u uncheck a feature, this can be used to 
 * grey out a group of related controls.
 * \param widget is the combo button
 * \param active is the entry in list was selected
 */
G_MODULE_EXPORT void combo_toggle_groups_linked(GtkWidget *widget,gint active)
{
	gint num_groups = 0;
	gint num_choices = 0;
	gint i = 0;
	gint j = 0;
	gboolean state = FALSE;
	gint page = 0;
	gint offset = 0;

	gchar **choices = NULL;
	gchar **groups = NULL;
	gchar * toggle_groups = NULL;
	gchar * tmpbuf = NULL;
	const gchar *name = NULL;
	GHashTable *widget_group_states = NULL;

	if (!DATA_GET(global_data,"ready"))
		return;
	widget_group_states = DATA_GET(global_data,"widget_group_states");
	toggle_groups = (gchar *)OBJ_GET(widget,"toggle_groups");
	page = (GINT)OBJ_GET(widget,"page");
	offset = (GINT)OBJ_GET(widget,"offset");

	/*printf("toggling combobox groups\n");*/
	choices = parse_keys(toggle_groups,&num_choices,",");
	if (active >= num_choices)
	{
		name = glade_get_widget_name(widget);
		printf(_("active is %i, num_choices is %i\n"),active,num_choices);
		printf(_("active is out of bounds for widget %s\n"),(name == NULL ? "undefined" : name));
	}
	/*printf("toggle groups defined for combo box %p at page %i, offset %i\n",widget,page,offset);*/

	/* First TURN OFF all non active groups */
	for (i=0;i<num_choices;i++)
	{
		if (i == active)
			continue;
		groups = parse_keys(choices[i],&num_groups,":");
		/*printf("Choice %i, has %i groups\n",i,num_groups);*/
		state = FALSE;
		for (j=0;j<num_groups;j++)
		{
			/*printf("setting all widgets in group %s to state %i\n\n",groups[j],state);*/
			tmpbuf = g_strdup_printf("!%s",groups[j]);
			g_hash_table_replace(widget_group_states,g_strdup(tmpbuf),GINT_TO_POINTER(!state));
			g_list_foreach(get_list(tmpbuf),alter_widget_state,NULL);
			g_free(tmpbuf);
			g_hash_table_replace(widget_group_states,g_strdup(groups[j]),GINT_TO_POINTER(state));
			g_list_foreach(get_list(groups[j]),alter_widget_state,NULL);
		}
		g_strfreev(groups);
	}

	/* Then TURN ON all active groups */
	groups = parse_keys(choices[active],&num_groups,":");
	state = TRUE;
	for (j=0;j<num_groups;j++)
	{
		tmpbuf = g_strdup_printf("!%s",groups[j]);
		g_hash_table_replace(widget_group_states,g_strdup(tmpbuf),GINT_TO_POINTER(!state));
		g_list_foreach(get_list(tmpbuf),alter_widget_state,NULL);
		g_free(tmpbuf);
		g_hash_table_replace(widget_group_states,g_strdup(groups[j]),GINT_TO_POINTER(state));
		g_list_foreach(get_list(groups[j]),alter_widget_state,NULL);
	}
	g_strfreev(groups);

	/*printf ("DONE!\n\n\n");*/
	g_strfreev(choices);
}


/*!
  \brief If a comboboxentry has a "set_labels" attribute handle it
  */
G_MODULE_EXPORT void combo_set_labels(GtkWidget *widget, GtkTreeModel *model)
{
	gint total = 0;
	gint tmpi = 0;
	gint i = 0;
	gchar *tmpbuf = NULL;
	gchar **vector = NULL;
	gchar * set_labels = NULL;

	set_labels = (gchar *)OBJ_GET(widget,"set_widgets_label");

	total = get_choice_count(model);
	tmpi = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	vector = g_strsplit(set_labels,",",-1);
	if ((g_strv_length(vector)%(total+1)) != 0)
	{
		MTXDBG(CRITICAL,_("Problem with set_labels, counts don't match up\n"));
		return;
	}
	for (i=0;i<(g_strv_length(vector)/(total+1));i++)
	{
		tmpbuf = g_strconcat(vector[i*(total+1)],",",vector[(i*(total+1))+1+tmpi],NULL);
		set_widget_labels(tmpbuf);
		g_free(tmpbuf);
	}
	g_strfreev(vector);
}


/*!
  \brief Get the total number of choices for a treemodel
  */
G_MODULE_EXPORT gint get_choice_count(GtkTreeModel *model)
{
	gboolean valid = TRUE;
	GtkTreeIter iter;
	gint i = 0;

	valid = gtk_tree_model_get_iter_first(model,&iter);
	while (valid)
	{
		gtk_tree_model_get(model,&iter,-1);
		valid = gtk_tree_model_iter_next (model, &iter);
		i++;
	}
	return i;
}


/*!
 * \brief combo_toggle_labels_linked is used to change the state of 
 * controls that
 * are "linked" to various other controls for the purpose of making the 
 * UI more intuitive.  i.e. if u uncheck a feature, this can be used to 
 * grey out a group of related controls.
 * \param widget is the combo button
 * \param active is the entry in list was selected
 */
G_MODULE_EXPORT void combo_toggle_labels_linked(GtkWidget *widget,gint active)
{
	gint num_groups = 0;
	gint i = 0;
	gchar **groups = NULL;
	gchar * toggle_labels = NULL;

	if (!DATA_GET(global_data,"ready"))
		return;
	toggle_labels = (gchar *)OBJ_GET(widget,"toggle_labels");

	groups = parse_keys(toggle_labels,&num_groups,",");
	/* toggle_labels is a list of groups of widgets that need to have their 
	 * label reset to a specific one from an array bound to that widget.
	 * So we get the names of those groups, and call "set_widget_label_from_array"
	 * passing in the index of the one in the array we want
	 */
	for (i=0;i<num_groups;i++)
		g_list_foreach(get_list(groups[i]),set_widget_label_from_array,GINT_TO_POINTER(active));

	g_strfreev(groups);
}



/*!
  \brief Sets the text in a widget based on the passed index (data)
  */
G_MODULE_EXPORT void set_widget_label_from_array(gpointer key, gpointer data)
{
	gchar *labels = NULL;
	gchar **vector = NULL;
	GtkWidget *label = (GtkWidget *)key;
	gint index = (GINT)data;

	if (!GTK_IS_LABEL(label))
		return;
	labels = OBJ_GET(label,"labels");
	if (!labels)
		return;
	vector = g_strsplit(labels,",",-1);
	if (index > g_strv_length(vector))
		return;
	gtk_label_set_text(GTK_LABEL(label),vector[index]);
	g_strfreev(vector);
	return;
}


/*!
  \brief recalc_table_limits() Finds the minimum and maximum values for a 
  2D table (this will be deprecated when thevetables are a custom widget)
  */
G_MODULE_EXPORT void recalc_table_limits(gint canID, gint table_num)
{
	static Firmware_Details *firmware = NULL;
	static gint (*get_ecu_data_f)(gpointer) = NULL;
	gint i = 0;
	gint x_count = 0;
	gint y_count = 0;
	gint z_base = 0;
	gint z_page = 0;
	gint z_size = 0;
	gint z_mult = 0;
	GObject *container = NULL;
	gint tmpi = 0;
	gint max = 0;
	gint min = 0;

	if (!firmware)
		firmware = DATA_GET(global_data,"firmware");
	if (!get_ecu_data_f)
		get_symbol("get_ecu_data",(void*)&get_ecu_data_f);

	g_return_if_fail(firmware);
	g_return_if_fail(get_ecu_data_f);

	container = g_object_new(GTK_TYPE_INVISIBLE,NULL);
        g_object_ref_sink(container);

	/* Limit check */
	if ((table_num < 0 ) || (table_num > firmware->total_tables-1))
		return;
	firmware->table_params[table_num]->last_z_maxval = firmware->table_params[table_num]->z_maxval;
	firmware->table_params[table_num]->last_z_minval = firmware->table_params[table_num]->z_minval;
	x_count = firmware->table_params[table_num]->x_bincount;
	y_count = firmware->table_params[table_num]->y_bincount;
	z_base = firmware->table_params[table_num]->z_base;
	z_page = firmware->table_params[table_num]->z_page;
	z_size = firmware->table_params[table_num]->z_size;
	OBJ_SET(container,"page",GINT_TO_POINTER(z_page));
	OBJ_SET(container,"size",GINT_TO_POINTER(z_size));
	OBJ_SET(container,"canID",GINT_TO_POINTER(canID));
	z_mult = get_multiplier(z_size);
	min = get_extreme_from_size(z_size,UPPER);
	max = get_extreme_from_size(z_size,LOWER);

	for (i=0;i<x_count*y_count;i++)
	{
		OBJ_SET(container,"offset",GINT_TO_POINTER(z_base+(i*z_mult)));
		tmpi = get_ecu_data_f(container);
		if (tmpi > max)
			max = tmpi;
		if (tmpi < min)
			min = tmpi;
	}
	if (min == max) /* FLAT table, causes black screen */
	{
		min -= 10;
		max += 10;
	}
	firmware->table_params[table_num]->z_maxval = max;
	firmware->table_params[table_num]->z_minval = min;
	/*printf("table %i min %i, max %i\n",table_num,min,max); */
	g_object_unref(container);
	return;
}


/*!
  \brief Calls the process_group function for each item in the 
  global toggle_group_list List
  */
G_MODULE_EXPORT void update_groups_pf()
{
	GList *list = NULL;
	list = DATA_GET(global_data,"toggle_group_list");
	if (!list)
		return;
	g_list_foreach(list,process_group,NULL);
}


/*!
  \brief Calls the process_source function for each item in the 
  global souce_list List
  */
G_MODULE_EXPORT void update_sources_pf()
{
	GList *list = NULL;
	list = DATA_GET(global_data,"source_list");
	if (!list)
		return;
	g_list_foreach(list,process_source,NULL);
}


/*!
  \brief deals with interdepenant controls with "toggle_groups" keys 
  loaded after ECU state is loaded.i.e. deferred tabs
  */
void process_group(gpointer data, gpointer nothing)
{
	GObject *object = (GObject *)data;
	gint i = 0;
	gint bitval = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	gint value = 0;
	gchar * bitvals = NULL;
	gchar **vector = NULL;

	bitmask = (GINT)OBJ_GET(object,"bitmask");

	bitshift = get_bitshift(bitmask);
	value = (GINT)convert_after_upload((GtkWidget *)object);
	if (OBJ_GET(object,"bitval"))
	{
		bitval = (GINT)OBJ_GET(object,"bitval");
		if (((value & bitmask) >> bitshift) == bitval)
			combo_toggle_groups_linked((GtkWidget *)object,1);
		else
			combo_toggle_groups_linked((GtkWidget *)object,0);
	}
	else /* Combo button, multiple choices */
	{
		bitvals = OBJ_GET(object,"bitvals");
		vector = g_strsplit(bitvals,",",-1);
		for (i=0;i<g_strv_length(vector);i++)
		{
			bitval = strtol(vector[i],NULL,10);
			/*printf("bitval str %s, bitval %i, rawvalue %i, bitmask %i, bitshift %i\n",vector[i],bitval,value,bitmask,bitshift);*/
			if (((value & bitmask) >> bitshift) == bitval)
			{
				/*printf("It was choice %i\n",i);*/
				combo_toggle_groups_linked((GtkWidget *)object,i);
				break;
			}
		}
		g_strfreev(vector);
	}
}


/*!
  \brief deals with multi_source controls loaded after ECU state is loaded
  i.e. deferred tabs
  */
void process_source(gpointer data, gpointer nothing)
{
	static GHashTable *sources_hash = NULL;
	GObject *object = (GObject *)data;
	gint i = 0;
	gint bitval = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	gint value = 0;
	gchar * bitvals = NULL;
	gchar * source_values = NULL;
	gchar **vector = NULL;
	gchar **vector2 = NULL;

	if (!sources_hash)
		sources_hash = DATA_GET(global_data,"sources_hash");
	g_return_if_fail(sources_hash);

	bitmask = (GINT)OBJ_GET(object,"bitmask");
	source_values = OBJ_GET(object,"source_values");

	bitshift = get_bitshift(bitmask);
	value = (GINT)convert_after_upload((GtkWidget *)object);
	bitvals = OBJ_GET(object,"bitvals");
	vector = g_strsplit(bitvals,",",-1);
	vector2 = g_strsplit(source_values,",",-1);
	for (i=0;i<g_strv_length(vector);i++)
	{
		bitval = strtol(vector[i],NULL,10);
		/*printf("bitval str %s, bitval %i, rawvalue %i, bitmask %i, bitshift %i\n",vector[i],bitval,value,bitmask,bitshift);*/
		if (((value & bitmask) >> bitshift) == bitval)
		{
			g_hash_table_replace(sources_hash,g_strdup(OBJ_GET(object,"source_key")),g_strdup(vector2[i]));
			break;
		}
	}
	g_strfreev(vector);
	g_strfreev(vector2);
}


/*!
  \brief updates all mtx widgets on the current visible notebook page
  */
G_MODULE_EXPORT void update_current_notebook_page()
{
	static void (*update_widget_f)(gpointer, gpointer);
	GtkWidget *notebook = NULL;
	GtkWidget *topframe = NULL;
	GtkWidget *widget = NULL;
	GList *tab_widgets = NULL;

	if (!update_widget_f)
		get_symbol("update_widget",(void *)&update_widget_f);
	g_return_if_fail(update_widget_f);

	notebook = lookup_widget("toplevel_notebook");
	g_return_if_fail(notebook);

	DATA_SET(global_data,"force_update",GINT_TO_POINTER(1));
	widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
	topframe = OBJ_GET(widget,"topframe");
	if (!topframe)
		topframe = widget;
	tab_widgets = OBJ_GET(topframe,"tab_widgets");
	if (tab_widgets)
	{
		set_title(g_strdup(_("Updating Tab with current data...")));
		g_list_foreach(tab_widgets,update_widget_f,NULL);
	}
	/*
	else
		printf("WARNING: this tab has no tab_widgets list assigned\n");
	*/
	set_title(g_strdup(_("Ready...")));
	DATA_SET(global_data,"force_update",NULL);
}


G_MODULE_EXPORT void cancel_visible_function(GtkNotebook *notebook, GtkWidget *page, guint page_no, gpointer data)
{
	g_source_remove((GINT)data);
	return;
}


G_MODULE_EXPORT void update_entry_color(GtkWidget *widget, gint table_num, gboolean in_table, gboolean force)
{
	static gint (*get_ecu_data_f)(gpointer) = NULL;
	static Firmware_Details *firmware = NULL;
	gfloat scaler = 0.0;
	GdkColor color;
	gint raw_lower = 0;
	gint raw_upper = 0;
	gint size = 0;
	gint low = 0;
	gint color_scale;

	if (!firmware)
		firmware = DATA_GET(global_data,"firmware");
	if (!get_ecu_data_f)
		get_symbol("get_ecu_data",(void *)&get_ecu_data_f);
	g_return_if_fail(firmware);
	g_return_if_fail(get_ecu_data_f);

	if (in_table)
	{
		color_scale = (GINT)DATA_GET(global_data,"mtx_color_scale");
		if (color_scale == FIXED_COLOR_SCALE)
		{
			scaler = (firmware->table_params[table_num]->z_raw_upper - firmware->table_params[table_num]->z_raw_lower)+0.1;
			low = firmware->table_params[table_num]->z_raw_lower;
		}
		else if (color_scale == AUTO_COLOR_SCALE)
		{
			scaler = (firmware->table_params[table_num]->z_maxval-firmware->table_params[table_num]->z_minval)+0.1;
			scaler = (firmware->table_params[table_num]->z_maxval - firmware->table_params[table_num]->z_minval)+0.1;
			low = firmware->table_params[table_num]->z_minval;
		}
		else
			MTXDBG(CRITICAL,_("mtx_color_scale value is undefined (%i)\n"),color_scale);
		color = get_colors_from_hue(-(220*((get_ecu_data_f(widget)-low)/scaler)+135), 0.50, 1.0);
		gtk_widget_modify_base(GTK_WIDGET(widget),GTK_STATE_NORMAL,&color);
	}
	else 
	{
		if (OBJ_GET(widget,"raw_lower"))
			raw_lower = (GINT)strtol(OBJ_GET(widget,"raw_lower"),NULL,10);
		else
			raw_lower = get_extreme_from_size(size,LOWER);
		if (OBJ_GET(widget,"raw_upper"))
			raw_upper = (GINT)strtol(OBJ_GET(widget,"raw_upper"),NULL,10);
		else
			raw_upper = get_extreme_from_size(size,UPPER);
		scaler = (raw_upper - raw_lower)+0.1;
		color = get_colors_from_hue(-(220*((get_ecu_data_f(widget)-raw_lower)/scaler)+135), 0.50, 1.0);
		gtk_widget_modify_base(GTK_WIDGET(widget),GTK_STATE_NORMAL,&color);
	}
}


G_MODULE_EXPORT gboolean table_color_refresh(gpointer data)
{
	static Firmware_Details *firmware = NULL;
	static GList ***ecu_widgets = NULL;
	gint table_num = 0;
	gint base = 0;
	gint page = 0;
	gint offset = 0;
	gint length = 0;
	gint mult = 0;
	DataSize size;
	gchar * tmpbuf = NULL;

	if (!firmware)
		firmware = DATA_GET(global_data,"firmware");
	if (!ecu_widgets)
		ecu_widgets = DATA_GET(global_data,"ecu_widgets");
	g_return_val_if_fail(firmware,FALSE);
	g_return_val_if_fail(ecu_widgets,FALSE);

	table_num = (GINT)data;
	tmpbuf = g_strdup_printf("table%i_color_id",table_num);
	DATA_SET(global_data,tmpbuf,NULL);
	g_free(tmpbuf);

	base = firmware->table_params[table_num]->z_base;
	size = firmware->table_params[table_num]->z_size;
	mult = get_multiplier(size);
	length = firmware->table_params[table_num]->x_bincount *
		firmware->table_params[table_num]->y_bincount * mult;
	page =  firmware->table_params[table_num]->z_page;
	for (offset=base;offset<base+length;offset++)
	{
		if (DATA_GET(global_data,"leaving"))
			return FALSE;
		if (ecu_widgets[page][offset] != NULL)
			g_list_foreach(ecu_widgets[page][offset],update_entry_color_wrapper,GINT_TO_POINTER(table_num));
	}
	return FALSE;
}


void update_entry_color_wrapper(gpointer object, gpointer data)
{
	GtkWidget *widget = (GtkWidget *)object;
	gint table_num = (GINT)data;
	update_entry_color(widget,table_num,TRUE,FALSE);
}
