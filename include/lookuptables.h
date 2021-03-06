/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file include/lookuptables.h
  \ingroup Headers
  \brief Header for lookuptable related functions
  \author David Andruczyk
  */

/* MegaSquirt Linux tuning lookup tables for converting incoming values
 * to real world numbers. From Bruce Bowlings PCC tuning wsoftware.
 */

#ifndef __LOOKUPTABLES_H__
#define __LOOKUPTABLES_H__

#include <enums.h>


typedef struct _LookupTable LookupTable;
/*!
 \brief _LookupTable is a mini-container holding hte filename and table
 info for each lookuptable stored in the LookupTables hashtable
 */
struct _LookupTable
{
	gint *array;		/*!< the table itself */
	gchar *filename;	/*!< The relative filename where 
				    this table came from */
};

/* Prototypes */
gboolean load_table(gchar *, gchar *);
void get_table(gpointer, gpointer, gpointer );
gint reverse_lookup(gconstpointer *, gint );
gint reverse_lookup_obj(GObject *, gint );
gint direct_reverse_lookup(gchar *, gint );
gfloat lookup_data(gconstpointer *, gint );
gfloat lookup_data_obj(GObject *, gint );
gfloat direct_lookup_data(gchar *, gint );
gboolean lookuptables_configurator(GtkWidget *, gpointer );
gboolean lookuptables_configurator_hide(GtkWidget *, gpointer );
gboolean lookuptable_changed(GtkCellRendererCombo *, gchar  *, GtkTreeIter  *, gpointer );
void row_activated(GtkTreeView *, GtkTreePath *, GtkTreeViewColumn *, gpointer);
void update_lt_config(gpointer , gpointer , gpointer );
void dump_lookuptables(gpointer , gpointer , gpointer);


#endif
