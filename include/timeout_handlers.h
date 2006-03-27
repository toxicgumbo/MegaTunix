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

#ifndef __TIMEOUT_HANDLERS_H__
#define __TIMEOUT_HANDLERS_H__

#include <gtk/gtk.h>

/* Prototypes */
void start_realtime_tickler(void);
void stop_realtime_tickler(void);
void start_logviewer_playback(void);
void stop_logviewer_playback(void);
void start_cont_read_page(void);
void stop_cont_read_page(void);
gboolean signal_cont_read_page(void);
gboolean signal_read_rtvars(void);
gboolean early_interrogation(void);
/* Prototypes */

#endif
