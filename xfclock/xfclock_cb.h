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

#ifndef __XFCLOCK_CB_H__
#define __XFCLOCK_CB_H__

void quit_cb (GtkWidget *, GdkEvent *, gpointer);
gint delete_event (GtkWidget *, GdkEvent *, gpointer);
void destroy_cb (GtkWidget *);
void font_cb (GtkWidget *, gpointer);
void foreground_cb (GtkWidget *, gpointer);
void background_cb (GtkWidget *, gpointer);
gint show_popup_cb (GtkWidget *, GdkEventButton *, gpointer);
void toggle_menubar_cb (GtkWidget *, gpointer);
void toggle_calendar_cb (GtkWidget *, gpointer);
void toggle_digital_cb (GtkWidget *, gpointer);
void toggle_secs_cb (GtkWidget *, gpointer);
void toggle_ampm_cb (GtkWidget *, gpointer);
void toggle_military_cb (GtkWidget *, gpointer);
void about_cb (GtkWidget *, gpointer data);

#endif
