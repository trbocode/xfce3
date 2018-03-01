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

#ifndef __SETUP_CB_H__
#define __SETUP_CB_H__

#include "xfcolor.h"
#include <gtk/gtk.h>

void color_button_cb (GtkWidget * widget, gpointer data);

void setup_ok_cb (GtkWidget * widget, gpointer data);

void setup_apply_cb (GtkWidget * widget, gpointer data);

void setup_cancel_cb (GtkWidget * widget, gpointer data);

gboolean setup_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data);

void setup_default_cb (GtkWidget * widget, gpointer data);

void setup_loadpal_cb (GtkWidget * widget, gpointer data);

void setup_savepal_cb (GtkWidget * widget, gpointer data);

void xfce_font_cb (GtkWidget * widget, gpointer data);

void toggle_repaint_checkbutton_cb (GtkWidget * widget, gpointer data);

void toggle_focusmode_checkbutton_cb (GtkWidget * widget, gpointer data);

void toggle_digital_clock_checkbutton_cb (GtkWidget * widget, gpointer data);

void xfwm_titlefont_cb (GtkWidget * widget, gpointer data);

void xfwm_menufont_cb (GtkWidget * widget, gpointer data);

void xfwm_iconfont_cb (GtkWidget * widget, gpointer data);
#endif
