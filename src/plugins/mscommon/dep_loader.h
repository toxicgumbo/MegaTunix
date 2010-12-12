/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

#ifndef __DEP_LOADER_H__
#define __DEP_LOADER_H__

#include <configfile.h>
#include <enums.h>
#include <gtk/gtk.h>

/* Externs */
extern void (*dbg_func_f)(gint, gchar *);
extern gchar **(*parse_keys_f)(const gchar *, gint *, const gchar * );
extern gint (*translate_string_f)(const gchar *);
/* Externs */

/* Prototypes */
void load_dependancies(gconstpointer *,ConfigFile * ,gchar *, gchar *);
void load_dependancies_obj(GObject *,ConfigFile * ,gchar *, gchar *);
gboolean check_size(DataSize );
/* Prototypes */

#endif