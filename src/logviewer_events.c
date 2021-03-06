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
  \file src/logviewer_events.c
  \ingroup CoreMtx
  \brief the Low Level logviewer functions which should be scrapped
  \author David Andruczyk
  */

#include <logviewer_events.h>
#include <logviewer_gui.h>
#include <math.h>
#include <stdio.h>
#include <timeout_handlers.h>
#include <widgetmgmt.h>

extern Logview_Data *lv_data;

/*! 
  \brief lv_configure_event() is the logviewer configure event that gets called
  whenever the display is resized or created
  \param widget is the pointer to widget receiving event
  \param event is the pointer to Config event structure
  \param data is unused)
  \returns FALSE
  */
G_MODULE_EXPORT gboolean lv_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	GdkPixmap *pixmap = NULL;
	GdkPixmap *pmap = NULL;
	gint w = 0;
	gint h = 0;
	GtkAllocation allocation;
	GdkWindow *window = gtk_widget_get_window(widget);

	gtk_widget_get_allocation(widget,&allocation);
	/* Get pointer to backing pixmap ... */
	if (!lv_data)
	{
		lv_data = g_new0(Logview_Data,1);
		lv_data->traces = g_hash_table_new(g_str_hash,g_str_equal);
	}
	pixmap = lv_data->pixmap;
	pmap = lv_data->pmap;
			
	if (window)
	{
		if (pixmap)
			g_object_unref(pixmap);

		if (pmap)
			g_object_unref(pmap);

		w=allocation.width;
		h=allocation.height;
		pixmap=gdk_pixmap_new(window,
				w,h,
				gtk_widget_get_visual(widget)->depth);
		gdk_draw_rectangle(pixmap,
				gtk_widget_get_style(widget)->black_gc,
				TRUE, 0,0,
				w,h);
		pmap=gdk_pixmap_new(window,
				w,h,
				gtk_widget_get_visual(widget)->depth);
		gdk_draw_rectangle(pmap,
				gtk_widget_get_style(widget)->black_gc,
				TRUE, 0,0,
				w,h);
		gdk_window_set_back_pixmap(window,pixmap,0);
		lv_data->pixmap = pixmap;
		lv_data->pmap = pmap;

		if ((lv_data->traces) && (g_list_length(lv_data->tlist) > 0))
		{
			draw_infotext();
			trace_update(TRUE);
		}
		gdk_window_clear(window);
	}

	return FALSE;
}


/*!
  \brief lv_expose_event() is called whenever part of the display is uncovered
  so that the screen can be redraw from the backing pixmap
  \param widget is the pointer to widget receiving the event
  \param event is the pointer to Expose event structure
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean lv_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GdkPixmap *pixmap = NULL;
	pixmap = lv_data->pixmap;

	/* Expose event handler... */
	gdk_draw_drawable(gtk_widget_get_window(widget),
                        gtk_widget_get_style(widget)->fg_gc[gtk_widget_get_state (widget)],
                        pixmap,
                        event->area.x, event->area.y,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);

	return TRUE;
}


/*!
  \brief lv_mouse_motion_event() is called whenever there is pointer 
  motion on the logviewer.  We use this to context highlight things 
  and provide for popup menus...
  \param widget is the widget receiving the event
  \param event is the pointer to motion event structure
  \param data is unused
  \returns TRUE on handled, FALSE otherwise
  */
G_MODULE_EXPORT gboolean lv_mouse_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	gint x = 0;
	gint y = 0;
	guint tnum = 0;
	GdkModifierType state;
	extern Logview_Data *lv_data;
	Viewable_Value *v_value = NULL;

	x = event->x;
	y = event->y;
	state = event->state;

	if (lv_data->active_traces == 0)
		return FALSE;
	if (x > lv_data->info_width) /* If out of bounds just return... */
	{
		highlight_tinfo(lv_data->tselect,FALSE);
		lv_data->tselect = -1;
		return FALSE;
	}

	tnum = (guint)ceil(y/lv_data->spread);
	if (tnum >= g_list_length(lv_data->tlist))
		return FALSE;
	v_value = g_list_nth_data(lv_data->tlist,tnum);
	if (lv_data->tselect != tnum)
	{
		highlight_tinfo(lv_data->tselect,FALSE);
		lv_data->tselect = tnum;
		highlight_tinfo(tnum,TRUE);
	}

	return TRUE;

}


/*!
  \brief highlight_tinfo() highlights the trace info box on the left side 
  of the logviewer when the mouse goes in there..
  \param tnum is the trace number starting from 0
  \param state if set we highlight the target trace info box
  */
G_MODULE_EXPORT void highlight_tinfo(gint tnum, gboolean state)
{
	GdkRectangle rect;
	extern Logview_Data *lv_data;
	GdkWindow *window = gtk_widget_get_window(lv_data->darea);

	rect.x = 0;
	rect.y = lv_data->spread*tnum;
	rect.width =  lv_data->info_width-1;
	rect.height = lv_data->spread;

	if (state)
		gdk_draw_rectangle(lv_data->pixmap,
				lv_data->highlight_gc,
				FALSE, rect.x,rect.y,
				rect.width,rect.height);
	else
		gdk_draw_rectangle(lv_data->pixmap,
				gtk_widget_get_style(lv_data->darea)->white_gc,
				FALSE, rect.x,rect.y,
				rect.width,rect.height);

	rect.width+=1;
	rect.height+=1;

	gdk_window_clear(window);

	return;

}


/*!
  \brief logviewer generic button event handler
  \param widget is the pointer to button
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean logviewer_button_event(GtkWidget *widget, gpointer data)
{
	Lv_Handler handler;
	GtkWidget *tmpwidget = NULL;
	handler = (Lv_Handler)OBJ_GET(widget,"handler");
	switch(handler)
	{
		case LV_GOTO_START:
			tmpwidget = lookup_widget("logviewer_log_position_hscale");
			if (GTK_IS_RANGE(tmpwidget))
				gtk_range_set_value(GTK_RANGE(tmpwidget),0.0);
			break;
		case LV_GOTO_END:
			tmpwidget = lookup_widget("logviewer_log_position_hscale");
			if (GTK_IS_RANGE(tmpwidget))
				gtk_range_set_value(GTK_RANGE(tmpwidget),100.0);
			break;
		case LV_PLAY:
			start_tickler(LV_PLAYBACK_TICKLER);
			break;
		case LV_STOP:
			stop_tickler(LV_PLAYBACK_TICKLER);
			break;
		case LV_REWIND:
			printf("rewind\n");
			break;
		case LV_FAST_FORWARD:
			printf("fast forward\n");
			break;
		default:
			break;

	}
	return TRUE;
}


/*!
  \brief logview mouse button event handler
  \param widget is the pointer to logviewer drawingarea
  \param event is the pointer to EventButton structure
  \param data is unused
  \returns TRUE if handled, FALSE otherwise
  */
G_MODULE_EXPORT gboolean lv_mouse_button_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gint x = 0;
	gint y = 0;
	gint w = 0;
	gint h = 0;
	guint tnum = 0;
	GdkModifierType state;
	extern Logview_Data *lv_data;
	Viewable_Value *v_value = NULL;
	GtkAllocation allocation;

	x = event->x;
	y = event->y;
	gtk_widget_get_allocation(widget,&allocation);
	w=allocation.width;
	h=allocation.height;
	state = event->state;

	return FALSE;

	/*printf("button with event is %i\n",event->button);
	 *printf("state of event is %i\n",state);
	 */
	if (x > lv_data->info_width) /* If out of bounds just return... */
		return TRUE;

	if (lv_data->active_traces == 0)
		return TRUE;
	tnum = (guint)ceil(y/lv_data->spread);
	if (tnum >= g_list_length(lv_data->tlist))
		return TRUE;
	v_value = g_list_nth_data(lv_data->tlist,tnum);
	if (event->state & (GDK_BUTTON3_MASK))
	{
		/*printf("right button released... \n");*/
		v_value->highlight = FALSE;
		gdk_draw_rectangle(lv_data->pixmap,
				gtk_widget_get_style(widget)->black_gc,
				TRUE, lv_data->info_width,0,
				w-lv_data->info_width,h);
		trace_update(TRUE);
		highlight_tinfo(tnum,TRUE);
		return TRUE;

	}
	else if (event->button == 3) /* right mouse button */
	{
		/*printf("right button pushed... \n");*/
		v_value->highlight = TRUE;
		gdk_draw_rectangle(lv_data->pixmap,
				gtk_widget_get_style(widget)->black_gc,
				TRUE, lv_data->info_width,0,
				w-lv_data->info_width,h);
		trace_update(TRUE);
		highlight_tinfo(tnum,TRUE);
		return TRUE;
	}
	return TRUE;
}
