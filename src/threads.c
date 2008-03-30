/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

#include <comms.h>
#include <comms_gui.h>
#include <config.h>
#include <conversions.h>
#include <dataio.h>
#include <datalogging_gui.h>
#include <defines.h>
#include <debugging.h>
#include <enums.h>
#include <errno.h>
#include <gui_handlers.h>
#include <init.h>
#include <interrogate.h>
#include <listmgmt.h>
#include <logviewer_gui.h>
#include <mode_select.h>
#include <notifications.h>
#include <serialio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <tabloader.h>
#include <xmlcomm.h>
#include <unistd.h>


extern gboolean connected;			/* valid connection with MS */
extern volatile gboolean offline;		/* ofline mode with MS */
extern gboolean interrogated;			/* valid connection with MS */
extern gint dbg_lvl;
extern GObject *global_data;
gchar *handler_types[]={"Realtime Vars","VE-Block","Raw Memory Dump","Comms Test","Get ECU Error", "NULL Handler"};


/*!
 \brief io_cmd() is called from all over the gui to kick off a threaded I/O
 command.  A command enumeration and an option block of data is passed and
 this function allocates a Io_Message and shoves it down an GAsyncQueue
 to the main thread dispatcher which runs things and then passes any 
 needed information back to the gui via another GAsyncQueue which takes care
 of any post thread GUI updates. (which can NOT be done in a thread context
 due to reentrancy and deadlock conditions)
 \param cmd (gchar *) and enumerated representation of a command to execute
 \param data (void *) additional data for fringe cases..
 */
void io_cmd(gchar *io_cmd_name, void *data)
{
	Io_Message *message = NULL;
	GHashTable *commands_hash = NULL;
	GHashTable *args_hash = NULL;
	Command *command = NULL;
	PotentialArg *arg = NULL;
	extern gboolean tabs_loaded;
	extern Firmware_Details * firmware;
	extern GHashTable *dynamic_widgets;
	extern GAsyncQueue *io_queue;
	static gboolean just_starting = TRUE;
	gint tmp = -1;
	gint i = 0;

	commands_hash = OBJ_GET(global_data,"commands_hash");
	args_hash = OBJ_GET(global_data,"arguments_hash");

	/* Fringe case for FUNC_CALL helpers that need to trigger 
	 * post_functions AFTER all their subhandlers have ran.  We
	 * call io_cmd with no cmd name and pack in the post functions into
	 * the void pointer part.
	 */
	if (!io_cmd_name)
	{
		message = initialize_io_message();
		message->command = g_new0(Command, 1);
		message->command->post_functions = (GArray *)data;
		message->command->type = NULL_CMD;
	}
	/* Std io_message passed by string name */
	else
	{
		command = g_hash_table_lookup(commands_hash,io_cmd_name);
		message = initialize_io_message();
		message->command = command;
		message->payload = data;
		if (command->type != FUNC_CALL)
			build_output_string(message,command,data);
	}

	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);

	/* This function is the bridge from the main GTK thread (gui) to
	 * the Serial I/O Handler (GThread). Communication is achieved through
	 * Asynchronous Queues. (preferred method in GLib and should be more 
	 * portable to more arch's)
	 */
	/*
	   switch (command->type)
	   {
	   case IO_REALTIME_READ:
	   message = initialize_io_message();
	   message->need_page_change = FALSE;
	   message->cmd_type = READ_CMD;
	   message->out_str = g_strdup(cmds->realtime_cmd);
	   message->out_len = cmds->rt_cmd_len;
	   message->handler = REALTIME_VARS;
	   message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	   tmp = UPD_REALTIME;
	   g_array_append_val(message->funcs,tmp);
	   tmp = UPD_LOGVIEWER;
	   g_array_append_val(message->funcs,tmp);
	   tmp = UPD_DATALOGGER;
	   g_array_append_val(message->funcs,tmp);
	   g_async_queue_ref(io_queue);
	   g_async_queue_push(io_queue,(gpointer)message);
	   g_async_queue_unref(io_queue);
	   break;
	   case IO_READ_TRIGMON_PAGE:
	   message = initialize_io_message();
	   message->need_page_change = TRUE;
	   message->page = firmware->trigmon_page;
	   message->truepgnum = firmware->page_params[firmware->trigmon_page]->truepgnum;
	   message->cmd_type = READ_CMD;
	   message->out_str = g_strdup(cmds->veconst_cmd);
	   message->out_len = cmds->ve_cmd_len;
	   message->handler = VE_BLOCK;
	   message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	   tmp = UPD_TRIGTOOTHMON;
	   g_array_append_val(message->funcs,tmp);
	   g_async_queue_ref(io_queue);
	   g_async_queue_push(io_queue,(gpointer)message);
	   g_async_queue_unref(io_queue);
	   break;
	   case IO_READ_TOOTHMON_PAGE:
	   message = initialize_io_message();
	   message->need_page_change = TRUE;
	   message->page = firmware->toothmon_page;
	   message->truepgnum = firmware->page_params[firmware->toothmon_page]->truepgnum;
	   message->cmd_type = READ_CMD;
	   message->out_str = g_strdup(cmds->veconst_cmd);
	   message->out_len = cmds->ve_cmd_len;
	   message->handler = VE_BLOCK;
	   message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	   tmp = UPD_TRIGTOOTHMON;
	   g_array_append_val(message->funcs,tmp);
	   g_async_queue_ref(io_queue);
	   g_async_queue_push(io_queue,(gpointer)message);
	   g_async_queue_unref(io_queue);
	   break;
	   case IO_CLEAN_REBOOT:
	   message = initialize_io_message();
	   message->cmd_type = NULL_CMD;
	   message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	   tmp = UPD_GET_BOOT_PROMPT;
	   g_array_append_val(message->funcs,tmp);
	   tmp = UPD_JUST_BOOT;
	   g_array_append_val(message->funcs,tmp);
	   g_async_queue_ref(io_queue);
	   g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_REBOOT_GET_ERROR:
	message = initialize_io_message();
	message->cmd_type = NULL_CMD;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_BURN_MS_FLASH;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_GET_BOOT_PROMPT;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_REBOOT_GET_ERROR;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_GET_BOOT_PROMPT:
	message = initialize_io_message();
	message->need_page_change = FALSE;
	message->cmd_type = READ_CMD;
	message->out_str = g_strdup("!!");
	message->out_len = 2;
	message->handler = NULL_HANDLER;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_BOOT_READ_ERROR:
	message = initialize_io_message();
	message->need_page_change = FALSE;
	message->cmd_type = READ_CMD;
	message->out_str = g_strdup("X");
	message->out_len = 1;
	message->handler = GET_ERROR;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_FORCE_UPDATE;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_FORCE_PAGE_CHANGE;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_JUST_BOOT:
	message = initialize_io_message();
	message->need_page_change = FALSE;
	message->cmd_type = READ_CMD;
	message->out_str = g_strdup("X");
	message->out_len = 1;
	message->handler = NULL_HANDLER;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;

	case IO_INTERROGATE_ECU:
	gtk_widget_set_sensitive(GTK_WIDGET(g_hash_table_lookup(dynamic_widgets, "interrogate_button")),FALSE);
	message = initialize_io_message();
	message->cmd_type = INTERROGATION;
	message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
	tmp = UPD_COMMS_STATUS;
	g_array_append_val(message->funcs,tmp);
	if (!tabs_loaded)
	{
		tmp = UPD_LOAD_REALTIME_MAP;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_INITIALIZE_DASH;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_LOAD_RT_STATUS;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_LOAD_RT_TEXT;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_LOAD_GUI_TABS;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_LOAD_RT_SLIDERS;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_POPULATE_DLOGGER;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_REENABLE_INTERROGATE_BUTTON;
		g_array_append_val(message->funcs,tmp);
		tmp = UPD_START_STATUSCOUNTS;
		g_array_append_val(message->funcs,tmp);
	}
	tmp = UPD_READ_VE_CONST;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_UPDATE_VE_CONST:
	message = initialize_io_message();
	message->cmd_type = NULL_CMD;
	message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
	tmp = UPD_VE_CONST;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_ENABLE_THREE_D_BUTTONS;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_SET_STORE_BLACK;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_REENABLE_GET_DATA_BUTTONS;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_LOAD_REALTIME_MAP:
	message = initialize_io_message();
	message->cmd_type = NULL_CMD;
	message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
	tmp = UPD_LOAD_REALTIME_MAP;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_LOAD_GUI_TABS:
	message = initialize_io_message();
	message->cmd_type = NULL_CMD;
	message->funcs = g_array_new(TRUE,TRUE,sizeof(gint));
	tmp = UPD_LOAD_GUI_TABS;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;

	case IO_READ_VE_CONST:
	if (!firmware)
		break;
	g_list_foreach(get_list("get_data_buttons"),set_widget_sensitive,GINT_TO_POINTER(FALSE));
	if (!offline)
	{
		for (i=0;i<=firmware->ro_above;i++)
		{
			message = initialize_io_message();
			message->cmd_type = READ_CMD;
			message->truepgnum = firmware->page_params[i]->truepgnum;
			message->page = i;
			message->need_page_change = TRUE;
			message->out_str = g_strdup(cmds->veconst_cmd);
			message->out_len = cmds->ve_cmd_len;
			message->handler = VE_BLOCK;
			g_async_queue_ref(io_queue);
			g_async_queue_push(io_queue,(gpointer)message);
			g_async_queue_unref(io_queue);
		}
	}
	message = initialize_io_message();
	message->cmd_type = NULL_CMD;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_VE_CONST;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_ENABLE_THREE_D_BUTTONS;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_SET_STORE_BLACK;
	g_array_append_val(message->funcs,tmp);
	tmp = UPD_REENABLE_GET_DATA_BUTTONS;
	g_array_append_val(message->funcs,tmp);
	if (just_starting)
	{
		tmp = UPD_START_REALTIME;
		g_array_append_val(message->funcs,tmp);
		just_starting = FALSE;
	}
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_READ_RAW_MEMORY:
	message = initialize_io_message();
	message->cmd_type = READ_CMD;
	message->need_page_change = FALSE;
	message->offset = (gint)data;
	message->out_str = g_strdup(cmds->raw_mem_cmd);
	message->out_len = cmds->raw_mem_cmd_len;
	message->handler = RAW_MEMORY_DUMP;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_RAW_MEMORY;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_BURN_MS_FLASH:
	message = initialize_io_message();
	message->need_page_change = FALSE;
	message->cmd_type = BURN_CMD;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_SET_STORE_BLACK;
	g_array_append_val(message->funcs,tmp);
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
	case IO_WRITE_DATA:
	message = initialize_io_message();
	message->cmd_type = WRITE_CMD;
	message->page=((Output_Data *)data)->page;
	message->truepgnum=firmware->page_params[((Output_Data *)data)->page]->truepgnum;
	message->payload = data;
	message->need_page_change = TRUE;
	message->funcs = g_array_new(FALSE,TRUE,sizeof(gint));
	if (((Output_Data *)data)->queue_update)
	{
		tmp = UPD_WRITE_STATUS;
		g_array_append_val(message->funcs,tmp);
	}
	g_async_queue_ref(io_queue);
	g_async_queue_push(io_queue,(gpointer)message);
	g_async_queue_unref(io_queue);
	break;
}
*/
}


/*!
 \brief thread_dispatcher() runs continuously as a thread listening to the 
 io_queue and running handlers as messages come in. After they are done it
 passes the message back to the gui via the dispatch_queue for further
 gui handling (for things that can't run in a thread context)
 \param data (gpointer) unused
 */
void *thread_dispatcher(gpointer data)
{
	GThread * repair_thread = NULL;
	extern GAsyncQueue *io_queue;
	extern GAsyncQueue *pf_dispatch_queue;
	extern gboolean port_open;
	extern volatile gboolean leaving;
	GTimeVal cur;
	Io_Message *message = NULL;	

	/* Endless Loop, wait for message, processs and repeat... */
	while (1)
	{
		/*printf("thread_dispatch_queue length is %i\n",g_async_queue_length(io_queue));*/
		g_get_current_time(&cur);
		g_time_val_add(&cur,100000); /* 100 ms timeout */
		message = g_async_queue_timed_pop(io_queue,&cur);

		if (leaving)
		{
			/* drain queue and exit thread */
			while (g_async_queue_try_pop(io_queue) != NULL)
			{}
			g_thread_exit(0);
		}
		if (!message) /* NULL message */
			continue;

		if ((!offline) && (((!connected) && (port_open)) || (!port_open)))
		{
			repair_thread = g_thread_create(serial_repair_thread,NULL,TRUE,NULL);
			g_thread_join(repair_thread);
		}
/*		if (!port_open)
		{
			if (dbg_lvl & (THREADS|CRITICAL))
				dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tLINK DOWN, Can't process requested command, aborting call\n"));
			thread_update_logbar("comm_view","warning",g_strdup("Disconnected Serial Link. Check Communications link/cable...\n"),FALSE,FALSE);
			thread_update_widget(g_strdup("titlebar"),MTX_TITLE,g_strdup("Disconnected link, check Communications tab..."));
			continue;
		}
		*/

		switch ((CmdType)message->command->type)
		{
			case FUNC_CALL:
				if (!message->command->function)
					printf("CRITICAL ERROR, function \"%s()\" is not found!!\n",
							message->command->func_call_name);
				else
				{
					/*printf("Calling FUNC_CALL, function \"%s()\" \n",
							message->command->func_call_name);*/
					message->command->function(
							message->command,
							message->command->func_call_arg);
				}
				break;
			case WRITE_CMD:
				write_data(message);
				if (!message->command->helper_function)
					printf("CRITICAL ERROR, helper function \"%s\" is NOT found!!\n",
							message->command->helper_func_name);
				else
					message->command->helper_function(
							message->command->helper_func_arg,
							message);
				break;
			case NULL_CMD:
				printf("NULL_CMD, just passing thru\n");
				break;

				/*
			case INTERROGATION:
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tInterrogation case entered\n"));
				if (!port_open)
				{
					if (dbg_lvl & (THREADS|CRITICAL))
						dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tLINK DOWN, Interrogate_ecu requested, aborting call\n"));
					thread_update_logbar("interr_view","warning",g_strdup("Interrogation failed due to disconnected Serial Link. Check COMMS Tab...\n"),FALSE,FALSE);
					thread_update_widget(g_strdup("titlebar"),MTX_TITLE,g_strdup("Disconnected link, check Communications tab..."));
					if (dbg_lvl & THREADS)
						dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tInterrogation case leaving, link NOT up\n"));
					break;
				}
				if ((connected) && (!offline))
				{
					interrogate_ecu();
				}
				else
				{
					thread_update_widget(g_strdup("titlebar"),MTX_TITLE,g_strdup("Not Connected, Check Serial Parameters..."));
					thread_update_logbar("interr_view","warning",g_strdup("Interrogation failed due to Link Problem. Check COMMS Tab...\n"),FALSE,FALSE);

				}
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tInterrogation case leaving\n"));
				break;
			case READ_CMD:
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tread_cmd case entered\n"));
				if (!port_open)
				{
					if (dbg_lvl & (THREADS|CRITICAL))
						dbg_func(g_strdup_printf(__FILE__": thread_dispatcher()\n\tLINK DOWN, read_command requested, call aborted \n"));
					break;
				}
				if (dbg_lvl & (SERIAL_RD|THREADS))
					dbg_func(g_strdup_printf(__FILE__": thread_dispatcher()\n\tread_command requested (%s)\n",handler_types[message->handler]));
				if (connected)
					readfrom_ecu(message);
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tread_cmd case leaving\n"));
				break;
			case WRITE_CMD:
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\twrite_cmd case entered\n"));
				if ((!port_open) && (!offline))
				{
					if (dbg_lvl & (SERIAL_WR|THREADS|CRITICAL))
						dbg_func(g_strdup_printf(__FILE__": thread_dispatcher()\n\tLINK DOWN, write_command requested, call aborted \n"));
					break;
				}
				if (dbg_lvl & (SERIAL_WR|THREADS))
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\twrite_command requested\n"));
				if ((connected) || (offline))
					writeto_ecu(message);
				else
				{
					if (dbg_lvl & (SERIAL_WR|THREADS|CRITICAL))
						dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\twriteto_ecu skipped, NOT Connected!!\n"));
				}
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\twrite_cmd case leaving\n"));
				break;
			case BURN_CMD:
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tburn_cmd case entered\n"));
				if (!port_open)
				{
					if (dbg_lvl & (SERIAL_WR|THREADS|CRITICAL))
						dbg_func(g_strdup_printf(__FILE__": thread_dispatcher()\n\tLINK DOWN, burn_command requested, call aborted \n"));
					break;
				}
				if (dbg_lvl & (SERIAL_WR|THREADS))
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tburn_command requested\n"));
				if ((connected) || (offline))
					burn_ecu_flash();
				else
				{
					if (dbg_lvl & (SERIAL_WR|THREADS|CRITICAL))
						dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tburn_ecu_flash skipped, NOT Connected!!\n"));
				}

				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tburn_cmd case leaving\n"));
				break;
			case NULL_CMD:
				if (dbg_lvl & THREADS)
					dbg_func(g_strdup(__FILE__": thread_dispatcher()\n\tnull_command requested\n"));
				break;

				*/
			default:
				break;

		}
		/* Send rest of message back up to main context for gui
		 * updates via asyncqueue
		 */
		if (!message->command->defer_post_functions)
		{
			g_async_queue_ref(pf_dispatch_queue);
			g_async_queue_push(pf_dispatch_queue,(gpointer)message);
			g_async_queue_unref(pf_dispatch_queue);
		}
	}
}


/*!
 \brief send_to_ecu() gets called to send a value to the ECU.  This function
 will check if the value sent is NOT the reqfuel_offset (that has special
 interdependancy issues) and then will check if there are more than 1 widgets
 that are associated with this page/offset and update those widgets before
 sending the value to the ECU.
 \param widget (GtkWidget *) pointer to the widget that was modified or NULL
 \param page (gint) page in which the value refers to.
 \param offset (gint) offset from the beginning of the page that this data
 refers to.
 \param value (gint) the value that should be sent to the ECU At page.offset
 \param queue_update (gboolean), if true queues a gui update, used to prevent
 a horrible stall when doing an ECU restore or batch load...
 */
void send_to_ecu(gint canID, gint page, gint offset, DataSize size, gint value, gboolean queue_update)
{
	extern Firmware_Details *firmware;
	Output_Data *output = NULL;

	if (dbg_lvl & SERIAL_WR)
		dbg_func(g_strdup_printf(__FILE__": send_to_ecu()\n\t Sending canID %i, page %i, offset %i, value %i \n",canID,page,offset,value));

	switch (size)
	{
		case MTX_CHAR:
		case MTX_S08:
		case MTX_U08:
		case MTX_S16:
		case MTX_U16:
		case MTX_S32:
		case MTX_U32:
			break;
		default:
			printf("ERROR!!! Size undefined for var at canID %i, page %i, offset %i\n",canID,page,offset);
	}
	output = g_new0(Output_Data, 1);
	output->object = g_object_new(GTK_TYPE_INVISIBLE,NULL);
	g_object_ref(output->object);
	gtk_object_sink(GTK_OBJECT(output->object));
	OBJ_SET(output->object,"canID", GINT_TO_POINTER(canID));
	OBJ_SET(output->object,"page", GINT_TO_POINTER(page));
	OBJ_SET(output->object,"offset", GINT_TO_POINTER(offset));
	OBJ_SET(output->object,"value", GINT_TO_POINTER(value));
	OBJ_SET(output->object,"size", GINT_TO_POINTER(size));
	OBJ_SET(output->object,"mode", GINT_TO_POINTER(MTX_SIMPLE_WRITE));
	output->canID = canID;
	output->page = page;
	output->offset = offset;
	output->value = value;
	output->size = size;
	output->mode = MTX_SIMPLE_WRITE;
	output->queue_update = queue_update;
	io_cmd(firmware->write_command,output);
	return;
}


/*!
 \brief chunk_write() gets called to send a block of values to the ECU.
 \param canID (gint) can identifier (0-14)
 \param page (gint) page in which the value refers to.
 \param offset (gint) offset from the beginning of the page that this data
 refers to.
 \param len (gint) length of block to sent
 \param data (guint8) the block of data to be sent
 a horrible stall when doing an ECU restore or batch load...
 */
void chunk_write(gint canID, gint page, gint offset, gint len, guint8 * data)
{
	extern Firmware_Details *firmware;
	Output_Data *output = NULL;

	if (dbg_lvl & SERIAL_WR)
		dbg_func(g_strdup_printf(__FILE__": chunk_write()\n\t Sending page %i, offset %i, len %i, data %p\n",page,offset,len,data));
	output = g_new0(Output_Data, 1);
	output->object = g_object_new(GTK_TYPE_INVISIBLE,NULL);
	g_object_ref(output->object);
	gtk_object_sink(GTK_OBJECT(output->object));
	OBJ_SET(output->object,"canID", GINT_TO_POINTER(canID));
	OBJ_SET(output->object,"page", GINT_TO_POINTER(page));
	OBJ_SET(output->object,"offset", GINT_TO_POINTER(offset));
	OBJ_SET(output->object,"len", GINT_TO_POINTER(len));
	OBJ_SET(output->object,"data", (gpointer)data);
	OBJ_SET(output->object,"mode", GINT_TO_POINTER(MTX_CHUNK_WRITE));
	output->canID = canID;
	output->page = page;
	output->offset = offset;
	output->len = len;
	output->data = data;
	output->mode = MTX_CHUNK_WRITE;
	output->queue_update = TRUE;
	io_cmd(firmware->chunk_write_command,output);
	return;
}


/*!
 \brief thread_update_logbar() is a function to be called from within threads
 to update a logbar (textview). It's not safe to update a widget from a 
 threaded context in win32, hence this fucntion is created to pass the 
 information to the main thread via an GAsyncQueue to a dispatcher that will
 take care of the message. Since the functions that call this ALWAYS send
 dynamically allocated test in the msg field we DEALLOCATE it HERE...
 \param view_name (gchar *) textual name fothe textview to update (required)
 \param tagname (gchar *) textual name ofthe tag to be applied to the text 
 sent.  This can be NULL is no tag is desired
 \param msg (gchar *) the message to be sent (required)
 \param count (gboolean) Flag to display a counter
 \param clear (gboolean) Flag to clear the display before writing the text
 */
void  thread_update_logbar(
		gchar * view_name, 
		gchar * tagname, 
		gchar * msg,
		gboolean count,
		gboolean clear)
{

	Io_Message *message = NULL;
	Text_Message *t_message = NULL;
	extern GAsyncQueue *gui_dispatch_queue;
	gint tmp = 0;

	message = initialize_io_message();

	t_message = g_new0(Text_Message, 1);
	t_message->view_name = view_name;
	t_message->tagname = tagname;
	t_message->msg = msg;
	t_message->count = count;
	t_message->clear = clear;

	message->payload = t_message;
	message->functions = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_LOGBAR;
	g_array_append_val(message->functions,tmp);

	g_async_queue_ref(gui_dispatch_queue);
	g_async_queue_push(gui_dispatch_queue,(gpointer)message);
	g_async_queue_unref(gui_dispatch_queue);
	return;
}


/*!
 \brief queue_function() is used to run ANY global function in the context
 of the main GUI from within any thread.  It does it by passing a message 
 up an AsyncQueue to the gui process which will lookup the function name
 and run it with no arguments (currently inflexible and can only run "void"
 functions (ones that take no params)
 \param name name of function to lookup and run in the main gui context..
 */
gboolean queue_function(gchar * name)
{
	Io_Message *message = NULL;
	QFunction *qfunc = NULL;
	extern GAsyncQueue *gui_dispatch_queue;
	gint tmp = 0;

	message = initialize_io_message();

	qfunc = g_new0(QFunction, 1);
	qfunc->func_name = name;

	message->payload = qfunc;
	message->functions = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_RUN_FUNCTION;
	g_array_append_val(message->functions,tmp);

	g_async_queue_ref(gui_dispatch_queue);
	g_async_queue_push(gui_dispatch_queue,(gpointer)message);
	g_async_queue_unref(gui_dispatch_queue);
	return FALSE;
}


/*!
 \brief thread_update_widget() is a function to be called from within threads
 to update a widget (spinner/entry/label). It's not safe to update a 
 widget from a threaded context in win32, hence this fucntion is created 
 to pass the information to the main thread via an GAsyncQueue to a 
 dispatcher that will take care of the message. 
 \param widget_name (gchar *) textual name of the widget to update
 \param type (WidgetType enumeration) type of widget to update
 \param msg (gchar *) the message to be sent (required)
 */
void  thread_update_widget(
		gchar * widget_name,
		WidgetType type,
		gchar * msg)
{

	Io_Message *message = NULL;
	Widget_Update *w_update = NULL;
	extern GAsyncQueue *gui_dispatch_queue;
	gint tmp = 0;

	message = initialize_io_message();

	w_update = g_new0(Widget_Update, 1);
	w_update->widget_name = widget_name;
	w_update->type = type;
	w_update->msg = msg;

	message->payload = w_update;
	message->functions = g_array_new(FALSE,TRUE,sizeof(gint));
	tmp = UPD_WIDGET;
	g_array_append_val(message->functions,tmp);

	g_async_queue_ref(gui_dispatch_queue);
	g_async_queue_push(gui_dispatch_queue,(gpointer)message);
	g_async_queue_unref(gui_dispatch_queue);
	return;
}


/*!
 \brief start_restore_monitor kicks off a thread to update the tools view
 during an ECU restore to provide user feedback since this is a time
 consuming operation.  if uses message passing over asyncqueues to send the 
 gui update messages.
 */
void start_restore_monitor(void)
{
	GThread * restore_update_thread = NULL;
	restore_update_thread = g_thread_create(restore_update,
			NULL, /* Thread args */
			TRUE, /* Joinable */
			NULL); /*GError Pointer */

}

void *restore_update(gpointer data)
{
	extern GAsyncQueue *io_queue;
	gint max_xfers = g_async_queue_length(io_queue);
	gint remaining_xfers = max_xfers;
	gint last_xferd = max_xfers;

	thread_update_logbar("tools_view","warning",g_strdup_printf("There are %i pending I/O transactions waiting to get to the ECU, please be patient.\n",max_xfers),FALSE,FALSE);
	while (remaining_xfers > 5)
	{
		remaining_xfers = g_async_queue_length(io_queue);
		g_usleep(5000);
		if (remaining_xfers <= (last_xferd-50))
		{
			thread_update_logbar("tools_view",NULL,g_strdup_printf("Approximately %i Transactions remaining, please wait\n",remaining_xfers),FALSE,FALSE);
			last_xferd = remaining_xfers;
		}

	}
	thread_update_logbar("tools_view","warning",g_strdup_printf("All Transactions complete\n"),FALSE,FALSE);

	return NULL;
}



/*! 
 \brief build_output_string() is called when doing output to the ECU, to 
 append the needed data together into one nice string for sending
 */
void build_output_string(Io_Message *message, Command *command, gpointer data)
{
	gint i = 0;
	gint total = 0;
	guint8 tmp8 = 0;
	guint16 tmp16 = 0;
	guint32 tmp32 = 0;
	Output_Data *output = NULL;
	PotentialArg * arg = NULL;
	DBlock *block = NULL;

	output = (Output_Data *)data;

	total = total_arg_length(command);
	message->sequence = g_array_new(FALSE,TRUE,sizeof(DBlock *));

	/* Base command */
	block = g_new0(DBlock, 1);
	block->type = DATA;
	block->str = g_strdup(command->base);
	block->len = strlen(command->base);
	g_array_append_val(message->sequence,block);

	/* Arguments */
	for (i=0;i<command->args->len;i++)
	{
		arg = g_array_index(command->args,PotentialArg *, i);
		block = g_new0(DBlock, 1);
		switch (arg->size)
		{
			case MTX_U08:
			case MTX_S08:
			case MTX_CHAR:
				block->type = DATA;
				tmp8 = (guint8)OBJ_GET(output->object,arg->internal_name);
				block->str = g_memdup(&tmp8,1);
				block->len = 1;
				break;
			case MTX_U16:
			case MTX_S16:
				block->type = DATA;
				tmp16 = (guint16)OBJ_GET(output->object,arg->internal_name);
				block->str = g_new0(gchar,2);
				block->str[0] = (tmp16 & 0xff00) >> 8;
				block->str[1] = (tmp16 & 0x00ff);
				block->len = 2;
			case MTX_U32:
			case MTX_S32:
				block->type = DATA;
				tmp32 = (guint32)OBJ_GET(output->object,arg->internal_name);
				block->str = g_new0(gchar,4);
				block->str[0] = (tmp32 & 0xff000000) >> 24;
				block->str[1] = (tmp32 & 0xff0000) >> 16;
				block->str[2] = (tmp32 & 0xff00) >> 8;
				block->str[3] = (tmp32 & 0x00ff);
				block->len = 4;
				break;
			case MTX_UNDEF:
				if (!arg->internal_name)
					printf("ERROR, MTX_UNDEF, donno what to do!!\n");
				block->str = g_memdup(output->data,output->len);
				block->len = output->len;
		}
		g_array_append_val(message->sequence,block);
	}
}


gint total_arg_length(Command *cmd)
{
	gint i = 0;
	gint total = 0;
	PotentialArg * arg = NULL;
	
	if (cmd->base)
		total = strlen(cmd->base);
	for (i=0;i<cmd->args->len;i++)
	{
		arg = g_array_index(cmd->args,PotentialArg *, i);
		switch (arg->size)
		{
			case MTX_U08:
			case MTX_S08:
			case MTX_CHAR:
				total++;
				break;
			case MTX_U16:
			case MTX_S16:
				total+=2;
				break;
			case MTX_U32:
			case MTX_S32:
				total+=4;
				break;
			case MTX_UNDEF:
			       break;	
		}
	}
	return total;
}

