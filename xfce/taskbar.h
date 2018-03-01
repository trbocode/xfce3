/*
 * Taskbar add-on to xfce
 * Contributed by Marcin Staszyszyn (marcin_staszyszyn@poczta.onet.pl)
 *
 */
 
#ifndef __TASKBAR_H
#define __TASKBAR_H

#include <gtk/gtk.h>
#include <stdio.h>
#include "configfile.h"                               


void 
taskbar_xfwm_init();

void 
taskbar_check_events(unsigned long type,unsigned long *body);

/* 
 * create taskbar frame (to be added to xfce main window) 
 */
GtkWidget 
*taskbar_create_gxfce_with_taskbar (GtkWidget *gxfce,
                                              GtkWidget *size_model,GtkWidget *top_level_window);      
/* save config WITHOUT header */
void 
taskbar_save_config(FILE *f);
      
/* return next non-taskbar related line */
char*
taskbar_read_config(FILE *f, char *lineread,int max);

void
taskbar_applay_xfce_config(config *config);

#endif
