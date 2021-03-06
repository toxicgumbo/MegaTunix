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
  \file src/plugins/pis/pis_plugin.h
  \ingroup PisPlugin,Headers
  \brief PIS Plugin specific plugin init/shutdown code
  \author David Andruczyk
  */

#ifndef __PIS_PLUGIN_H__
#define __PIS_PLUGIN_H__

#include <defines.h>
#include <configfile.h>

#ifdef __PIS_PLUGIN_C__
#define EXTERN
#else
#define EXTERN extern
#endif

/* Function Pointers */
EXTERN void (*dbg_func_f)(int,const gchar *, const gchar *, gint ,const gchar *, ...);
/* Function Pointers */

/* Prototypes */
void plugin_init(gconstpointer *);
void plugin_shutdown(void);
void register_ecu_enums(void);
void deregister_ecu_enums(void);
/* Prototypes */

#endif
