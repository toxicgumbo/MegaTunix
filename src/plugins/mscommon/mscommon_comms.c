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
  \file src/plugins/mscommon/mscommon_comms.c
  \ingroup MSCommonPlugin,Plugins
  \brief MS specific communication functions
  \author David Andruczyk
  */

#include <datamgmt.h>
#include <errno.h>
#include <firmware.h>
#include <mscommon_comms.h>
#include <mscommon_plugin.h>
#include <debugging.h>
#include <mtxsocket.h>
#include <serialio.h>
#include <stdio.h>
#include <string.h>


extern gconstpointer *global_data;

/*!
 \brief queue_burn_ecu_flash() issues the commands to the ECU to 
 burn the contents of RAM to flash.
 \param page is the MTX page (not  to be confused with the ECU physical page)
 to be burnt
 */
G_MODULE_EXPORT void queue_burn_ecu_flash(gint page)
{
	OutputData *output = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	if (DATA_GET(global_data,"offline"))
		return;

	output = initialize_outputdata_f();
	DATA_SET(output->data,"canID", GINT_TO_POINTER(firmware->canID));
	DATA_SET(output->data,"page", GINT_TO_POINTER(page));
	DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
	DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_CMD_WRITE));
	io_cmd_f(firmware->burn_command,output);
	DATA_SET(global_data,"last_page",GINT_TO_POINTER(page));
}


/*!
 \brief queue_ms1_page_change() issues the commands to the ECU to 
 change the ECU page
 \param page is the page to be read into ECU RAM from flash by the ECU
 */
G_MODULE_EXPORT void queue_ms1_page_change(gint page)
{
	OutputData *output = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	if (DATA_GET(global_data,"offline"))
		return;

	output = initialize_outputdata_f();
	DATA_SET(output->data,"page", GINT_TO_POINTER(page));
	DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
	DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_CMD_WRITE));
	io_cmd_f(firmware->page_command,output);
	DATA_SET(global_data,"last_page",GINT_TO_POINTER(page));
	return;
}



/*! 
 \brief comms_test sends the clock_request command to the ECU and
 checks the response.  if nothing comes back, MegaTunix assumes the ecu isn't
 connected or powered down. NO Gui updates are done from this function as it
 gets called from a thread. 
 */
G_MODULE_EXPORT gint comms_test(void)
{
	static gint errcount = 0;
	gboolean result = FALSE;
	gchar * err_text = NULL;
	gint len = 0;
	Serial_Params *serial_params = NULL;
	extern gconstpointer *global_data;

	if (DATA_GET(global_data,"leaving"))
		return TRUE;
	serial_params = DATA_GET(global_data,"serial_params");

	MTXDBG(SERIAL_RD,_("Entered\n"));
	g_return_val_if_fail(serial_params, FALSE);

	if ((GINT)DATA_GET(global_data,"ecu_baud") < 115200)
	{
		MTXDBG(SERIAL_RD,_("Requesting MS-1 ECU Clock\n"));
		/*printf("MS-1 comms test\n");*/
		if (!write_wrapper_f(serial_params->fd,"C",1,&len))
		{
			err_text = (gchar *)g_strerror(errno);
			MTXDBG(SERIAL_WR|CRITICAL,_("Error writing \"C\" to the ecu, ERROR \"%s\" in comms_test()\n"),err_text);
			thread_update_logbar_f("comms_view","warning",g_strdup_printf(_("Error writing \"C\" to the ecu, ERROR \"%s\" in comms_test()\n"),err_text),FALSE,FALSE);
			DATA_SET(global_data,"connected",GINT_TO_POINTER(FALSE));
			return FALSE;
		}
		result = read_data_f(1,NULL,FALSE);
	}
	else
	{
		MTXDBG(SERIAL_RD,_("Requesting MS-2 ECU Clock\n"));
		/*printf("MS-2 comms test\n");*/
		if (!write_wrapper_f(serial_params->fd,"c",1,&len))
		{
			err_text = (gchar *)g_strerror(errno);
			MTXDBG(SERIAL_WR|CRITICAL,_("Error writing \"c\" (MS-II clock test) to the ecu, ERROR \"%s\" in comms_test()\n"),err_text);
			thread_update_logbar_f("comms_view","warning",g_strdup_printf(_("Error writing \"c\" (MS-II clock test) to the ecu, ERROR \"%s\" in comms_test()\n"),err_text),FALSE,FALSE);
			DATA_SET(global_data,"connected",GINT_TO_POINTER(FALSE));
			return FALSE;
		}
		result = read_data_f(2,NULL,FALSE);
	}
	if (result)     /* Success */
	{
		DATA_SET(global_data,"connected",GINT_TO_POINTER(TRUE));
		errcount=0;
		MTXDBG(SERIAL_RD,_("ECU Comms Test Successful\n"));
		queue_function_f("kill_conn_warning");
		thread_update_widget_f("titlebar",MTX_TITLE,g_strdup(_("ECU Connected...")));
		thread_update_logbar_f("comms_view","info",g_strdup_printf(_("ECU Comms Test Successful\n")),FALSE,FALSE);
		return TRUE;

	}
	else
	{
		/* An I/O Error occurred with the MegaSquirt ECU  */
		DATA_SET(global_data,"connected",GINT_TO_POINTER(FALSE));
		errcount++;
		if (errcount > 5 )
			queue_function_f("conn_warning");
		thread_update_widget_f("titlebar",MTX_TITLE,g_strdup_printf(_("COMMS ISSUES: Check COMMS tab")));
		MTXDBG(SERIAL_RD|IO_PROCESS,_("I/O with ECU Timeout\n"));
		thread_update_logbar_f("comms_view","warning",g_strdup_printf(_("I/O with ECU Timeout\n")),FALSE,FALSE);
		return FALSE;
	}
	return FALSE;
}


/*!
 \brief ms_table_write() gets called to send a block of lookuptable values to the ECU
 \param page is the MTX page in which the value refers to.
 \param num_bytes is the length of block to sent
 \param data is the block of data to be sent which better damn well be
 in ECU byte order if there is an endianness thing..
 */
G_MODULE_EXPORT void ms_table_write(gint page, gint num_bytes, guint8 * data)
{
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	OutputData *output = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");
	g_static_mutex_lock(&mutex);

	MTXDBG(SERIAL_WR,_("Sending page %i, num_bytes %i, data %p\n"),page,num_bytes,data);

	output = initialize_outputdata_f();
	DATA_SET(output->data,"page", GINT_TO_POINTER(page));
	DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
	DATA_SET(output->data,"num_bytes", GINT_TO_POINTER(num_bytes));
	DATA_SET(output->data,"data", (gpointer)data);
	DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_CHUNK_WRITE));

	/* save it otherwise the burn checker can miss it due to a potential
	 * race condition
	 */
	ms_store_new_block(0,page,0,data,num_bytes);

	if (firmware->multi_page)
		ms_handle_page_change(page,(GINT)DATA_GET(global_data,"last_page"));
	output->queue_update = TRUE;
	io_cmd_f(firmware->table_write_command,output);

	g_static_mutex_unlock(&mutex);
	return;
}


/*!
  \brief cis passed the current/last pages and  determines if the ECU
  requires a  burn to flash or not, if so, it injects the command to do so
  \param page is the current page
  \param last is the page  used  for the last command
  */
G_MODULE_EXPORT void ms_handle_page_change(gint page, gint last)
{
	guint8 **ecu_data = NULL;
	guint8 **ecu_data_last = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");
	ecu_data = firmware->ecu_data;
	ecu_data_last = firmware->ecu_data_last;

	/*printf("handle page change!, page %i, last %i\n",page,last);
	 */

	if (last == -1)  /* First Write of the day, burn not needed... */
	{
		queue_ms1_page_change(page);
		return;
	}
	if ((page == last) && (!DATA_GET(global_data,"force_page_change")))
	{
		/*printf("page == last and force_page_change is not set\n");
		 */
		return;
	}
	/* If current page is NOT a dl_by_default page, but the last one WAS
	 * then a burn is required otherwise settings will be lost in the
	 * last
	 */
	if ((!firmware->page_params[page]->dl_by_default) && (firmware->page_params[last]->dl_by_default))
	{
		/*printf("current was not dl by default  but last was,  burning\n");
		 */
		queue_burn_ecu_flash(last);
		if (firmware->capabilities & MS1)
			queue_ms1_page_change(page);
		return;
	}
	/* If current page is NOT a dl_by_default page, OR the last one was
	 * not then a burn is NOT required.
	 */
	if ((!firmware->page_params[page]->dl_by_default) || (!firmware->page_params[last]->dl_by_default))
	{
		/*printf("current is not dl by default or last was not as well\n");
		 */
		if ((page != last) && (firmware->capabilities & MS1))
		{
			/*printf("page diff and MS1, changing page\n");
			 */
			queue_ms1_page_change(page);
		}
		return;
	}
	/* If current and last pages are DIFFERENT,  do a memory buffer scan
	 * to see if previous and last match,  if so return, otherwise burn
	 * then change page
	 */
	if (((page != last) && (((memcmp(ecu_data_last[last],ecu_data[last],firmware->page_params[last]->length) != 0)) || ((memcmp(ecu_data_last[page],ecu_data[page],firmware->page_params[page]->length) != 0)))))
	{
		/*printf("page and last don't match AND there's a ram difference, burning, before changing\n");
		 */
		queue_burn_ecu_flash(last);
		if (firmware->capabilities & MS1)
			queue_ms1_page_change(page);
	}
	else if ((page != last) && (firmware->capabilities & MS1))
	{
		/*printf("page and last don't match AND there's a NOT a RAM difference, changing page\n");
		 */
		queue_ms1_page_change(page);
	}
}


/*!
 \brief chunk_write() gets called to send a block of data to the ECU.  
 This function has an ECU agnostic interface and is for sending 
 arbritrary blocks of data to the ECU. This function extracts the 
 important things from the passed ptr and sends to the real function 
 which is ecu specific.
 \param data is the pointer to the object containing the really important 
 things like thecanID, page  and offset
 \param num_bytes is how many bytes in the next var to send
 \param block is a pointer to the buffer to write
 */
G_MODULE_EXPORT void chunk_write(gpointer data, gint num_bytes, guint8 * block)
{
	gint canID = 0;
	gint page = 0;
	gint offset = 0;
	GtkWidget *widget = (GtkWidget *)data;
	gconstpointer *gptr = (gconstpointer *)data;

	if (GTK_IS_WIDGET(widget))
	{
		canID = (GINT)OBJ_GET(widget,"canID");
		page = (GINT)OBJ_GET(widget,"page");
		offset = (GINT)OBJ_GET(widget,"offset");
	}
	else
	{
		canID = (GINT)DATA_GET(gptr,"canID");
		page = (GINT)DATA_GET(gptr,"page");
		offset = (GINT)DATA_GET(gptr,"offset");
	}
	ms_chunk_write(canID, page, offset, num_bytes, block);
}

/*!
 \brief ecu_chunk_write() is an abstraction wrapper
 \param canID is the CAN identifier (0-14)
 \param page is the MTX page in which the value refers to.
 \param offset is the offset from the beginning of the page that this data
 refers to.
 \param num_bytes is the length of block to sent
 \param block is the the block of data to be sent which better damn well be
 in ECU byte order if there is an endianness thing..
 */
G_MODULE_EXPORT void ecu_chunk_write(gint canID, gint page, gint offset, gint num_bytes, guint8 * block)
{
	/* Should check if firmware is chunk capable first though and fallback
	 * as needed
	 */
	ms_chunk_write(canID,page,offset,num_bytes,block);
}


/*!
 \brief ms_chunk_write() gets called to send a block of values to the ECU.
 \param canID is the CAN identifier (0-14)
 \param page is the MTX page in which the value refers to.
 \param offset is the offset from the beginning of the page that this data
 refers to.
 \param num_bytes is the length of block to sent
 \param block is the the block of data to be sent which better damn well be
 in ECU byte order if there is an endianness thing..
 */
G_MODULE_EXPORT void ms_chunk_write(gint canID, gint page, gint offset, gint num_bytes, guint8 * block)
{
	OutputData *output = NULL;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	MTXDBG(SERIAL_WR,_("Sending canID %i, page %i, offset %i, num_bytes %i, data %p\n"),canID,page,offset,num_bytes,block);
	output = initialize_outputdata_f();
	DATA_SET(output->data,"canID", GINT_TO_POINTER(canID));
	DATA_SET(output->data,"page", GINT_TO_POINTER(page));
	DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
	DATA_SET(output->data,"offset", GINT_TO_POINTER(offset));
	DATA_SET(output->data,"num_bytes", GINT_TO_POINTER(num_bytes));
	DATA_SET_FULL(output->data,"data", (gpointer)block, g_free);
	DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_CHUNK_WRITE));

	/* save it otherwise the burn checker can miss it due to a potential
	 * race condition
	 */
	ms_store_new_block(canID,page,offset,block,num_bytes);

	if (firmware->multi_page)
		ms_handle_page_change(page,(GINT)DATA_GET(global_data,"last_page"));
	output->queue_update = TRUE;
	io_cmd_f(firmware->chunk_write_command,output);
	DATA_SET(global_data,"last_page",GINT_TO_POINTER(page));
	return;
}


/*!
 \brief send_to_ecu() gets called to send a value to the ECU.  This function
 is has an ECU agnostic interface and is for sending single 8-32 bit bits of 
 data to the ECU. This one extracts the important things from the passed ptr
 and sends to the real function which is ecu specific
 \param data is a pointer to the  object holding the important bits
 \param value is the new value to send
 \param queue_update is a flag to trigger other wisgets on this address to
 update
 */
G_MODULE_EXPORT void send_to_ecu(gpointer data, gint value, gboolean queue_update)
{
	gint canID = 0;
	gint page = 0;
	gint offset = 0;
	DataSize size = MTX_U08;
	GtkWidget *widget = (GtkWidget *)data;
	gconstpointer *gptr = (gconstpointer *)data;

	if (GTK_IS_WIDGET(widget))
	{
		canID = (GINT)OBJ_GET(widget,"canID");
		page = (GINT)OBJ_GET(widget,"page");
		offset = (GINT)OBJ_GET(widget,"offset");
		size = (DataSize)OBJ_GET(widget,"size");
	}
	else
	{
		canID = (GINT)DATA_GET(gptr,"canID");
		page = (GINT)DATA_GET(gptr,"page");
		offset = (GINT)DATA_GET(gptr,"offset");
		size = (DataSize)DATA_GET(gptr,"size");
	}
	ms_send_to_ecu(canID,page,offset,size,value,queue_update);
}


/*!
 \brief ms_send_to_ecu() gets called to send a value to the ECU.  This function
 will check if the value sent is NOT the reqfuel_offset (that has special
 interdependancy issues) and then will check if there are more than 1 widgets
 that are associated with this page/offset and update those widgets before
 sending the value to the ECU.
 \param canID is the CAN identifier
 \param page is the page in which the value refers to.
 \param offset is the offset from the beginning of the page that this data
 refers to.
 \param size is the size enumeration for this value
 \param value is the the value that should be sent to the ECU At page.offset
 \param queue_update if true queues a gui update, used to prevent
 a horrible stall when doing an ECU restore or batch load...
 */
G_MODULE_EXPORT void ms_send_to_ecu(gint canID, gint page, gint offset, DataSize size, gint value, gboolean queue_update)
{
	OutputData *output = NULL;
	guint8 *data = NULL;
	guint16 u16 = 0;
	gint16 s16 = 0;
	guint32 u32 = 0;
	gint32 s32 = 0;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");

	MTXDBG(SERIAL_WR,_("Sending canID %i, page %i, offset %i, value %i\n"),canID,page,offset,value);

	switch (size)
	{
		case MTX_CHAR:
		case MTX_S08:
		case MTX_U08:
			/*              printf("8 bit var %i at offset %i\n",value,offset);*/
			break;
		case MTX_S16:
		case MTX_U16:
			/*              printf("16 bit var %i at offset %i\n",value,offset);*/
			break;
		case MTX_S32:
		case MTX_U32:
			/*              printf("32 bit var %i at offset %i\n",value,offset);*/
			break;
		default:
			printf(_("ms_send_to_ecu() ERROR!!! Size undefined for variable at canID %i, page %i, offset %i\n"),canID,page,offset);
	}
	/* If the ecu is multi-page, run the handler to take care of queing
	 * burns and/or page changing
	 */
	if (firmware->multi_page)
		ms_handle_page_change(page,(GINT)DATA_GET(global_data,"last_page"));
	/* VERY special case for busted as MS-1 which can only accept 8 bit
	   writes
	   */
	if ((firmware->capabilities & MS1) && ((size == MTX_U16) || (size == MTX_S16)))
	{
		/* First byte */
		output = initialize_outputdata_f();
		output->queue_update = queue_update;
		DATA_SET(output->data,"canID", GINT_TO_POINTER(canID));
		DATA_SET(output->data,"page", GINT_TO_POINTER(page));
		DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
		DATA_SET(output->data,"offset", GINT_TO_POINTER(offset));
		DATA_SET(output->data,"value", GINT_TO_POINTER((value & 0xff00)>> 8));
		DATA_SET(output->data,"size", GINT_TO_POINTER(MTX_U08));
		DATA_SET(output->data,"num_bytes", GINT_TO_POINTER(get_multiplier_f(MTX_U08)));
		DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_SIMPLE_WRITE));
		io_cmd_f(firmware->write_command,output);
		/* Second byte */
		output = initialize_outputdata_f();
		output->queue_update = queue_update;
		DATA_SET(output->data,"canID", GINT_TO_POINTER(canID));
		DATA_SET(output->data,"page", GINT_TO_POINTER(page));
		DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
		DATA_SET(output->data,"offset", GINT_TO_POINTER(offset+1));
		DATA_SET(output->data,"value", GINT_TO_POINTER(value & 0xff));
		DATA_SET(output->data,"size", GINT_TO_POINTER(MTX_U08));
		DATA_SET(output->data,"num_bytes", GINT_TO_POINTER(get_multiplier_f(MTX_U08)));
		DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_SIMPLE_WRITE));
		io_cmd_f(firmware->write_command,output);
	}
	else
	{
		/* Normal 8 bit stuff */
		output = initialize_outputdata_f();
		output->queue_update = queue_update;
		DATA_SET(output->data,"canID", GINT_TO_POINTER(canID));
		DATA_SET(output->data,"page", GINT_TO_POINTER(page));
		DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->page_params[page]->phys_ecu_page));
		DATA_SET(output->data,"offset", GINT_TO_POINTER(offset));
		DATA_SET(output->data,"value", GINT_TO_POINTER(value));
		DATA_SET(output->data,"size", GINT_TO_POINTER(size));
		DATA_SET(output->data,"num_bytes", GINT_TO_POINTER(get_multiplier_f(size)));
		DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_SIMPLE_WRITE));
		/* SPECIAL case for MS2, as it's write always assume a "datablock"
		 * and it doesn't have a simple easy write api due to it's use of 
		 * different sized vars,  hence the extra complexity.
		 */
		if (firmware->capabilities & MS2)
		{
			/* Get memory */
			data = g_new0(guint8,get_multiplier_f(size));
			switch (size)
			{
				case MTX_CHAR:
				case MTX_U08:
					data[0] = (guint8)value;
					break;
				case MTX_S08:
					data[0] = (gint8)value;
					break;
				case MTX_U16:
					if (firmware->bigendian)
						u16 = GUINT16_TO_BE((guint16)value);
					else
						u16 = GUINT16_TO_LE((guint16)value);
					data[0] = (guint8)u16;
					data[1] = (guint8)((guint16)u16 >> 8);
					break;
				case MTX_S16:
					if (firmware->bigendian)
						s16 = GINT16_TO_BE((gint16)value);
					else
						s16 = GINT16_TO_LE((gint16)value);
					data[0] = (guint8)s16;
					data[1] = (guint8)((gint16)s16 >> 8);
					break;
				case MTX_S32:
					if (firmware->bigendian)
						s32 = GINT32_TO_BE((gint32)value);
					else
						s32 = GINT32_TO_LE((gint32)value);
					data[0] = (guint8)s32;
					data[1] = (guint8)((gint32)s32 >> 8);
					data[2] = (guint8)((gint32)s32 >> 16);
					data[3] = (guint8)((gint32)s32 >> 24);
					break;
				case MTX_U32:
					if (firmware->bigendian)
						u32 = GUINT32_TO_BE((guint32)value);
					else
						u32 = GUINT32_TO_LE((guint32)value);
					data[0] = (guint8)u32;
					data[1] = (guint8)((guint32)u32 >> 8);
					data[2] = (guint8)((guint32)u32 >> 16);
					data[3] = (guint8)((guint32)u32 >> 24);
					break;
				default:
					break;
			}
			DATA_SET_FULL(output->data,"data", (gpointer)data,g_free);
		}
		io_cmd_f(firmware->write_command,output);
	}

	/* Set it here otherwise there's a risk of a missed burn due to 
	 * a potential race condition in the burn checker
	 */
	ms_set_ecu_data(canID,page,offset,size,value);

	DATA_SET(global_data,"last_page",GINT_TO_POINTER(page));
	return;
}


/*!
 \brief send_to_slaves() sends messages to a thread talking to the slave
 clients to trigger them to update their GUI with appropriate changes
 \param data is a pointer to an OutputData structure which contains the
 necessary info to pass to the slave 
 */
G_MODULE_EXPORT void send_to_slaves(void *data)
{
	static GAsyncQueue *slave_msg_queue = NULL;
	Io_Message *message = (Io_Message *)data;
	OutputData *output = (OutputData *)message->payload;
	SlaveMessage *msg = NULL;

	if (!slave_msg_queue)
		slave_msg_queue = DATA_GET(global_data,"slave_msg_queue");
	if (!(GBOOLEAN)DATA_GET(global_data,"network_access"))
		return;
	if (!output) /* If no data, don't bother the slaves */
		return;
	if (!slave_msg_queue)
		return;
	msg = g_new0(SlaveMessage, 1);
	msg->page = (guint8)(GINT)DATA_GET(output->data,"page");
	msg->offset = (guint16)(GINT)DATA_GET(output->data,"offset");
	msg->length = (guint16)(GINT)DATA_GET(output->data,"num_bytes");
	msg->size = (DataSize)DATA_GET(output->data,"size");
	msg->mode = (WriteMode)DATA_GET(output->data,"mode");
	msg->type = MTX_DATA_CHANGED;
	if (msg->mode == MTX_CHUNK_WRITE)
		msg->data = g_memdup(DATA_GET(output->data,"data"), msg->length);
	else if (msg->mode == MTX_SIMPLE_WRITE)
		msg->value = (GINT)DATA_GET(output->data,"value");
	else
	{
		printf(_("Non simple/chunk write command, not notifying peers\n"));
		g_free(msg);
		return;
	}

	/*      printf("Sending message to peer(s)\n");*/
	g_async_queue_ref(slave_msg_queue);
	g_async_queue_push(slave_msg_queue,(gpointer)msg);
	g_async_queue_unref(slave_msg_queue);
	return;
}


/*!
  \brief sends a message to connected slaves to change the color of the
  widgets in the passed group name
  \param clr is an enumeration representing the color
  \param groupname is the group to manipulate
  \see GuiColor
  */
G_MODULE_EXPORT void slaves_set_color(GuiColor clr, const gchar *groupname)
{
	static GAsyncQueue *slave_msg_queue = NULL;
	SlaveMessage *msg = NULL;

	if (!slave_msg_queue)
		slave_msg_queue = DATA_GET(global_data,"slave_msg_queue");
	if (!(GBOOLEAN)DATA_GET(global_data,"network_access"))
		return;

	msg = g_new0(SlaveMessage, 1);
	msg->type = MTX_STATUS_CHANGED;
	msg->action = GROUP_SET_COLOR;
	msg->value = (guint8)clr;
	msg->data = g_strdup(groupname);
	msg->length = (guint16)(GINT)strlen(groupname);

	/*      printf("Sending message to peer(s)\n");*/
	g_async_queue_ref(slave_msg_queue);
	g_async_queue_push(slave_msg_queue,(gpointer)msg);
	g_async_queue_unref(slave_msg_queue);
	return;
}


/*!
 \brief update_write_status() checks the differences between the current ECU
 data snapshot and the last one, if there are any differences (things need to
 be burnt) then it turns all the widgets in the "burners" group to RED
 \param data is a pointer to data sent to ECU used to
 update other widgets that refer to that Page/Offset
 */
G_MODULE_EXPORT void update_write_status(void *data)
{
	Io_Message *message = (Io_Message *)data;
	OutputData *output = (OutputData *)message->payload;
	guint8 **ecu_data = NULL;
	guint8 **ecu_data_last = NULL;
	gint i = 0;
	gint canID = 0;
	gint page = 0;
	gint offset = 0;
	gint length = 0;
	gint id = 0;
	gchar * tmpbuf = NULL;
	WriteMode mode = MTX_CMD_WRITE;
	gint z = 0;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");
	ecu_data = firmware->ecu_data;
	ecu_data_last = firmware->ecu_data_last;

	if (!output)
		goto red_or_black;
	else
	{
		canID = (GINT)DATA_GET(output->data,"canID");
		page = (GINT)DATA_GET(output->data,"page");
		offset = (GINT)DATA_GET(output->data,"offset");
		length = (GINT)DATA_GET(output->data,"num_bytes");
		mode = (WriteMode)DATA_GET(output->data,"mode");

		if (!message->status) /* Bad write! */
		{
			MTXDBG(SERIAL_WR,_("WRITE failed, rolling back!\n"));
			memcpy(ecu_data[page]+offset, ecu_data_last[page]+offset,length);
		}
	}
	if (output->queue_update)
	{
		if ((GINT)DATA_GET(global_data,"mtx_color_scale") == AUTO_COLOR_SCALE)
		{
			for (i=0;i<firmware->total_tables;i++)
			{
				// This at least only recalcs the limits on one... 
				if (firmware->table_params[i]->z_page == page)
				{
					gdk_threads_enter();
					recalc_table_limits_f(canID,i);
					gdk_threads_leave();
					if ((firmware->table_params[i]->last_z_maxval != firmware->table_params[i]->z_maxval) || (firmware->table_params[i]->last_z_minval != firmware->table_params[i]->z_minval))
					{
						tmpbuf = g_strdup_printf("table%i_color_id",i);
						if (!DATA_GET(global_data,tmpbuf))
						{
							id = gdk_threads_add_timeout(2000,(GSourceFunc)table_color_refresh_f,GINT_TO_POINTER(i));
							DATA_SET(global_data,tmpbuf,GINT_TO_POINTER(id));
						}
						g_free(tmpbuf);
					}
				}
			}
		}

		if (mode == MTX_CHUNK_WRITE)
			thread_refresh_widget_range_f(page,offset,length);
		else
		{
			for (z=offset;z<offset+length;z++)
			{
				/*printf("refreshing widgets at page %i, offset %i\n",page,z);*/
				thread_refresh_widgets_at_offset_f(page,z);
			}
		}
		DATA_SET(global_data,"paused_handlers",GINT_TO_POINTER(FALSE));
	}
	/* We check to see if the last burn copy of the VE/constants matches 
	 * the currently set, if so take away the "burn now" notification.
	 * avoid unnecessary burns to the FLASH 
	 */

	if (DATA_GET(global_data,"offline"))
		return;
red_or_black:
	for (i=0;i<firmware->total_pages;i++)
	{
		if (!firmware->page_params[i]->dl_by_default)
			continue;

		if(memcmp(ecu_data_last[i],ecu_data[i],firmware->page_params[i]->length) != 0)
		{
			firmware->page_params[i]->needs_burn = TRUE;
			gdk_threads_enter();
			thread_set_group_color_f(RED,"burners");
			slaves_set_color(RED,"burners");
			gdk_threads_leave();
			return;
		}
		else
			firmware->page_params[i]->needs_burn = FALSE;
	}
	gdk_threads_enter();
	thread_set_group_color_f(BLACK,"burners");
	slaves_set_color(BLACK,"burners");
	gdk_threads_leave();
	return;
}


/*!
 \brief start_restore_monitor kicks off a thread to update the tools view
 during an ECU restore to provide user feedback since this is a time
 consuming operation.  if uses message passing over asyncqueues to send the 
 gui update messages.
 */
G_MODULE_EXPORT void start_restore_monitor(void)
{
	GThread * restore_update_thread = NULL;
	restore_update_thread = g_thread_create(restore_update,
			NULL, /* Thread args */
			TRUE, /* Joinable */
			NULL); /*GError Pointer */

}


/*!
  \brief the thread which monitorsthe io_data_queue and updates the GUI with
  feedback, for very long running operrations (restores)
  \param data is unused
  */
G_MODULE_EXPORT void *restore_update(gpointer data)
{
	static GAsyncQueue *io_data_queue = NULL;
	gint max_xfers = 0;
	gint remaining_xfers = 0;
	gint last_xferd = 0;

	if (!io_data_queue)
		io_data_queue = DATA_GET(global_data,"io_data_queue");

	max_xfers = g_async_queue_length(io_data_queue);
	remaining_xfers = max_xfers;
	last_xferd = max_xfers;

	MTXDBG(THREADS,_("restore update thread created!\n"));
	thread_update_logbar_f("tools_view","warning",g_strdup_printf(_("There are %i pending I/O transactions waiting to get to the ECU, please be patient.\n"),max_xfers),FALSE,FALSE);
	while (remaining_xfers > 5)
	{
		remaining_xfers = g_async_queue_length(io_data_queue);
		g_usleep(10000);
		if (remaining_xfers <= (last_xferd-50))
		{
			thread_update_logbar_f("tools_view",NULL,g_strdup_printf(_("Approximately %i Transactions remaining, please wait\n"),remaining_xfers),FALSE,FALSE);
			last_xferd = remaining_xfers;
		}

	}
	thread_update_logbar_f("tools_view","info",g_strdup_printf(_("All Transactions complete\n")),FALSE,FALSE);

	MTXDBG(THREADS,_("restore update thread exiting!\n"));
	return NULL;
}


/*!
  \brief kludgy ugly monster that gets enabled whene hte serial I/O error
  crosses sa magic threshold, This basically closes the port and begins a 
  search for a valid device, and will exit when it does, or is cancelled
  \param data is unused
  */
G_MODULE_EXPORT void *serial_repair_thread(gpointer data)
{
	/* We got sent here because of one of the following occurred:
	 * Serial port isn't opened yet (app just fired up)
	 * Serial I/O errors (missing data, or failures reading/writing)
	 *  - This includes things like pulling the RS232 cable out of the ECU
	 * Serial port disappeared (i.e. device hot unplugged)
	 *  - This includes unplugging the USB side of a USB->Serial adapter
	 *    or going out of bluetooth range, for a BT serial device
	 *
	 * Thus we need to handle all possible conditions cleanly
	 */
	static gboolean serial_is_open = FALSE; /* Assume never opened */
	static GAsyncQueue *io_repair_queue = NULL;
	gchar * active = NULL;
	gchar * potential_ports = NULL;
	gint len = 0;
	gboolean autodetect = FALSE;
	guchar buf [1024];
	gchar ** vector = NULL;
	guint i = 0;
	Serial_Params *serial_params = NULL;
	void (*unlock_serial_f)(void) = NULL;
	void (*close_serial_f)(void) = NULL;
	gboolean (*open_serial_f)(const gchar *,gboolean) = NULL;
	gboolean (*lock_serial_f)(const gchar *) = NULL;
	void (*setup_serial_params_f)(void) = NULL;

	serial_params = DATA_GET(global_data,"serial_params");

	get_symbol_f("setup_serial_params",(void *)&setup_serial_params_f);
	get_symbol_f("open_serial",(void *)&open_serial_f);
	get_symbol_f("close_serial",(void *)&close_serial_f);
	get_symbol_f("lock_serial",(void *)&lock_serial_f);
	get_symbol_f("unlock_serial",(void *)&unlock_serial_f);

	g_return_val_if_fail(setup_serial_params_f,NULL);
	g_return_val_if_fail(open_serial_f,NULL);
	g_return_val_if_fail(close_serial_f,NULL);
	g_return_val_if_fail(lock_serial_f,NULL);
	g_return_val_if_fail(unlock_serial_f,NULL);

	MTXDBG(THREADS,_("serial repair thread created!\n"));

	if (DATA_GET(global_data,"offline"))
	{
		g_timeout_add(100,(GSourceFunc)queue_function_f,"kill_conn_warning");
		MTXDBG(THREADS,_("Thread exiting, offline mode!\n"));
		g_thread_exit(0);
	}

	if (!io_repair_queue)
		io_repair_queue = DATA_GET(global_data,"io_repair_queue");
	/* IF serial_is_open is true, then the port was ALREADY opened 
	 * previously but some error occurred that sent us down here. Thus
	 * first do a simple comms test, if that succeeds, then just cleanup 
	 * and return,  if not, close the port and essentially start over.
	 */
	if (serial_is_open == TRUE)
	{
		MTXDBG(SERIAL_RD|SERIAL_WR,_("Port considered open, but throwing errors\n"));
		i = 0;
		while (i <= 2)
		{
			MTXDBG(SERIAL_RD|SERIAL_WR,_("Calling comms_test, attempt %i\n"),i);
			if (comms_test())
			{
				MTXDBG(THREADS,_("Thread exiting, successfull comms test!\n"));
				g_thread_exit(0);
			}
			i++;
		}
		close_serial_f();
		unlock_serial_f();
		serial_is_open = FALSE;
		/* Fall through */
	}
	/* App just started, no connection yet*/
	while (!serial_is_open) 	
	{
		/* If "leaving" flag set, EXIT now */
		if (DATA_GET(global_data,"leaving"))
			g_thread_exit(0);
		MTXDBG(SERIAL_RD|SERIAL_WR,_("Port NOT considered open yet.\n"));
		autodetect = (GBOOLEAN) DATA_GET(global_data,"autodetect_port");
		if (!autodetect) /* User thinks he/she is S M A R T */
		{
			potential_ports = (gchar *)DATA_GET(global_data, "override_port");
			if (potential_ports == NULL)
				potential_ports = (gchar *)DATA_GET(global_data,"potential_ports");
		}
		else	/* Auto mode */
			potential_ports = (gchar *)DATA_GET(global_data,"potential_ports");
		vector = g_strsplit(potential_ports,",",-1);
		for (i=0;i<g_strv_length(vector);i++)
		{
			if (DATA_GET(global_data,"leaving"))
			{
				g_strfreev(vector);
				g_thread_exit(0);
			}
			/* Message queue used to exit immediately */
			if (g_async_queue_try_pop(io_repair_queue))
			{
				g_timeout_add(300,(GSourceFunc)queue_function_f,"kill_conn_warning");
				MTXDBG(THREADS,_("Thread exiting, told to!\n"));
				g_thread_exit(0);
			}
			if (!g_file_test(vector[i],G_FILE_TEST_EXISTS))
			{
				MTXDBG(SERIAL_RD|SERIAL_WR,_("Port %s does NOT exist\n"),vector[i]);

				/* Wait 100 ms to avoid deadlocking */
				g_usleep(100000);
				continue;
			}
			/* Wait 100 ms to avoid deadlocking */
			g_usleep(100000);
			MTXDBG(SERIAL_RD|SERIAL_WR,_("Attempting to open port %s\n"),vector[i]);
			thread_update_logbar_f("comms_view",NULL,g_strdup_printf(_("Attempting to open port %s\n"),vector[i]),FALSE,FALSE);
			if (lock_serial_f(vector[i]))
			{
				if (open_serial_f(vector[i],FALSE))
				{
					if (autodetect)
						thread_update_widget_f("active_port_entry",MTX_ENTRY,g_strdup(vector[i]));
					MTXDBG(SERIAL_RD|SERIAL_WR,_("Port %s opened\n"),vector[i]);
					setup_serial_params_f();
					/* read out any junk in buffer and toss it */
					read_wrapper_f(serial_params->fd,&buf,1024,&len);

					thread_update_logbar_f("comms_view",NULL,g_strdup_printf(_("Searching for ECU\n")),FALSE,FALSE);
					MTXDBG(SERIAL_RD|SERIAL_WR,_("Performing ECU comms test via port %s.\n"),vector[i]);
					if (comms_test())
					{       /* We have a winner !!  Abort loop */
						thread_update_logbar_f("comms_view",NULL,g_strdup_printf(_("Search successfull\n")),FALSE,FALSE);
						serial_is_open = TRUE;
						active = g_strdup(vector[i]);
						break;
					}
					else
					{  
						MTXDBG(SERIAL_RD|SERIAL_WR,_("COMMS test failed, no ECU found, closing port %s.\n"),vector[i]);
						thread_update_logbar_f("comms_view",NULL,g_strdup_printf(_("No ECU found...\n")),FALSE,FALSE);
						close_serial_f();
						unlock_serial_f();
						/*g_usleep(100000);*/
					}
				}
				g_usleep(100000);
			}
			else
			{
				MTXDBG(SERIAL_RD|SERIAL_WR,_("Port %s is open by another application\n"),vector[i]);
				thread_update_logbar_f("comms_view","warning",g_strdup_printf(_("Port %s is open by another application\n"),vector[i]),FALSE,FALSE);
			}
		}
		queue_function_f("conn_warning");
		g_strfreev(vector);
	}
	if (serial_is_open)
	{
		queue_function_f("kill_conn_warning");
		thread_update_widget_f("active_port_entry",MTX_ENTRY,g_strdup(active));
		g_free(active);
	}
	MTXDBG(THREADS,_("Thread exiting, device found!\n"));
	g_thread_exit(0);
	return NULL;
}


/*!
 \brief signal_read_rtvars() sends io message to I/O core to tell ms to send 
 back runtime vars
 */
G_MODULE_EXPORT void signal_read_rtvars(void)
{
	OutputData *output = NULL;
	extern gconstpointer *global_data;
	Firmware_Details *firmware = NULL;

	firmware = DATA_GET(global_data,"firmware");
	g_return_if_fail(firmware);

	/* MS2 */
	if (firmware->capabilities & MS2)
	{
		output = initialize_outputdata_f();
		DATA_SET(output->data,"canID", GINT_TO_POINTER(firmware->canID));
		DATA_SET(output->data,"page", GINT_TO_POINTER(firmware->ms2_rt_page));
		DATA_SET(output->data,"phys_ecu_page", GINT_TO_POINTER(firmware->ms2_rt_page));
		DATA_SET(output->data,"mode", GINT_TO_POINTER(MTX_CMD_WRITE));
		io_cmd_f(firmware->rt_command,output);
	}
	else /* MS1 */
		io_cmd_f(firmware->rt_command,NULL);
	return;
}


/*! 
 \brief build_output_message() is called when doing output to the ECU, to 
 append the needed data together into one nice blob for sending
 \param message is apointer to the Io_Message structure to finish up
 \param command is a pointer to the Command strucutre which hasthe info on how
 to assemble the message properly
 \param data is a poiinter to the  OutputData structure which contains the 
 source info needed by this function to make the proper output stream
 */
G_MODULE_EXPORT void build_output_message(Io_Message *message, Command *command, gpointer data)
{
	guint i = 0;
	gint v = 0;
	gint len = 0;
	OutputData *output = NULL;
	PotentialArg * arg = NULL;
	guint8 *sent_data = NULL;
	DBlock *block = NULL;

	if (data)
		output = (OutputData *)data;

	message->sequence = g_array_new(FALSE,TRUE,sizeof(DBlock *));

	/* Base command */
	block = g_new0(DBlock, 1);
	block->type = DATA;
	block->data = (guint8 *)g_strdup(command->base);
	if (!command->base)
		block->len = 0;
	else
		block->len = strlen(command->base);
	g_array_append_val(message->sequence,block);

	/* Arguments */
	for (i=0;i<command->args->len;i++)
	{
		arg = g_array_index(command->args,PotentialArg *, i);
		block = g_new0(DBlock, 1);
		if (arg->type == ACTION)
		{
			/*printf("build_output_string(): ACTION being created!\n");*/
			block->type = ACTION;
			block->action = arg->action;
			block->arg = arg->action_arg;
			g_array_append_val(message->sequence,block);
			continue;
		}
		if (arg->type == STATIC_STRING)
		{
			block->type = DATA;
			block->data = (guint8 *)g_strdup(arg->static_string);
			block->len = strlen(arg->static_string);
			g_array_append_val(message->sequence,block);
			continue;
		}
		if (!output)
			continue;
		switch (arg->size)
		{
			case MTX_U08:
			case MTX_S08:
			case MTX_CHAR:
				/*printf("8 bit arg %i, name \"%s\"\n",i,arg->internal_name);*/
				block->type = DATA;
				v = (GINT)DATA_GET(output->data,arg->internal_name);
				/*printf("value %i\n",v);*/
				block->data = g_new0(guint8,1);
				block->data[0] = (guint8)v;
				block->len = 1;
				break;
			case MTX_U16:
			case MTX_S16:
				/*printf("16 bit arg %i, name \"%s\"\n",i,arg->internal_name);*/
				block->type = DATA;
				v = (GINT)DATA_GET(output->data,arg->internal_name);
				/*printf("value %i\n",v);*/
				block->data = g_new0(guint8,2);
				block->data[0] = (v & 0xff00) >> 8;
				block->data[1] = (v & 0x00ff);
				block->len = 2;
				break;
			case MTX_U32:
			case MTX_S32:
				/*                              printf("32 bit arg %i, name \"%s\"\n",i,arg->internal_name);*/
				block->type = DATA;
				v = (GINT)DATA_GET(output->data,arg->internal_name);
				/*                              printf("value %i\n",v); */
				block->data = g_new0(guint8,4);
				block->data[0] = (v & 0xff000000) >> 24;
				block->data[1] = (v & 0xff0000) >> 16;
				block->data[2] = (v & 0xff00) >> 8;
				block->data[3] = (v & 0x00ff);
				block->len = 4;
				break;
			case MTX_UNDEF:
				/*printf("arg %i, name \"%s\"\n",i,arg->internal_name);*/
				block->type = DATA;
				if (!arg->internal_name)
					printf(_("ERROR, MTX_UNDEF, donno what to do!!\n"));
				sent_data = (guint8 *)DATA_GET(output->data,arg->internal_name);
				len = (GINT)DATA_GET(output->data,"num_bytes");
				block->data = g_memdup(sent_data,len);
				block->len = len;
				/*
				   for (j=0;j<len;j++)
				   {
				   printf("sent_data[%i] is %i\n",j,sent_data[j]);
				   printf("block->data[%i] is %i\n",j,block->data[j]);
				   }
				 */
		}
		g_array_append_val(message->sequence,block);
	}
}

/*! 
  \brief stub function for the plugin
  */
G_MODULE_EXPORT gboolean setup_rtv(void)
{
	return TRUE;
}


/*! 
  \brief stub function for the plugin
  */
G_MODULE_EXPORT gboolean teardown_rtv(void)
{
	return TRUE;
}
