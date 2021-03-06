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
  \file include/menu_handlers.h
  \ingroup Headers
  \brief Header for the global menu handlers
  \author David Andruczyk
  */

#ifndef __MENU_HANDLERS_H__
#define __MENU_HANDLERS_H__

#include <defines.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

/* Enumerations */
typedef enum
{
	ALL_TABLE_IMPORT=0x230,
	ALL_TABLE_EXPORT,
	ECU_BACKUP,
	ECU_RESTORE
}FioAction;

/* Prototypes */
void setup_menu_handlers_pf(void);
gboolean jump_to_tab(GtkWidget *, gpointer );
gboolean settings_transfer(GtkWidget *, gpointer );
gboolean check_tab_existance(TabIdent );
/* Prototypes */

#endif
