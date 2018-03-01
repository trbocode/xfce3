/*  gxfce
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __POPUP_CB_H__
#define __POPUP_CB_H__

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "popup.h"

gboolean delete_popup_cb (GtkWidget * widget, GdkEvent * event, gpointer data);

gboolean popup_entry_modify_cb (GtkWidget * widget, GdkEventButton * event, gpointer data);

void popup_entry_cb (GtkWidget * widget, gpointer data);

void popup_addicon_cb (GtkWidget * widget, gpointer data);

void detach_cb (GtkWidget * widget, gpointer data);

void popup_entry_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata);

void popup_addicon_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata);

/* Added by Jason Litowitz */
void private_close_popup_button (gint menu);

#endif
