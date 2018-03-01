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

#ifndef __XFCE_CB_H__
#define __XFCE_CB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "mygtkclock.h"

void iconify_cb (GtkWidget * widget, gpointer data);

void quit_cb (GtkWidget * widget, gpointer data);

gboolean delete_event (GtkWidget * widget, GdkEvent * event, gpointer data);

void destroy_cb (GtkWidget * widget, gpointer data);

void popup_cb (GtkWidget * widget, gpointer data);

void select_cb (GtkWidget * widget, gpointer data);

gboolean select_modify_cb (GtkWidget * widget, GdkEventButton * event, gpointer data);

void update_screen (gint);

void screen_cb (GtkWidget * widget, gpointer data);

gboolean screen_modify_cb (GtkWidget * widget, GdkEventButton * event, gpointer data);

void info_cb (GtkWidget * widget, gpointer data);

void clock_digital_cb (GtkWidget * widget, gpointer data);

void clock_hrs_cb (GtkWidget * widget, gpointer data);

gboolean gxfce_clock_show_popup_cb (GtkWidget * widget, GdkEventButton * event, gpointer data);

void toggle_popup_button (GtkWidget * widget, GtkPixmap * pix);

void setup_cb (GtkWidget * widget, gpointer data);


gint get_current_screen (void);

void set_current_screen (gint);

void select_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer i);

gboolean update_gxfce_date_timer (GtkWidget * widget);

gboolean handle_gnome_workspace_event_cb (GtkWidget * widget, GdkEventProperty * event);

void move_event_pressed (GtkWidget * widget, GdkEventButton * event, MyGtkClock * clock_widget);

void move_event_released (GtkWidget * widget, GdkEventButton * event, MyGtkClock * clock_widget);


#endif
