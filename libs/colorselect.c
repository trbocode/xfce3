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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "colorselect.h"
#include "xfcolor.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

typedef struct
{
  GtkWidget *colorselect;
  gdouble selcolor[4];
  XFCE_palette *p_tmp;
  int color_index_tmp;
}
colorselect_obj;

static void
ok_cb (GtkWidget * twidget, gpointer data)
{
  colorselect_obj *colsel = (colorselect_obj *) data;
  g_assert (colsel != NULL);
  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->colorsel), colsel->selcolor);
  set_palcolor (colsel->p_tmp, colsel->color_index_tmp, colsel->selcolor);
  gtk_widget_destroy (colsel->colorselect);
  gtk_main_quit ();
}

static void
cancel_cb (GtkWidget * twidget, gpointer data)
{
  colorselect_obj *colsel = (colorselect_obj *) data;
  g_assert (colsel != NULL);
  gtk_widget_destroy (colsel->colorselect);
  gtk_main_quit ();
}

static gboolean
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  colorselect_obj *colsel = (colorselect_obj *) data;
  g_assert (colsel != NULL);
  gtk_widget_destroy (colsel->colorselect);
  gtk_main_quit ();
  return (TRUE);
}

void
open_colorselect (XFCE_palette * p, int color_index)
{
  GtkWidget *colorselect_ok_button;
  GtkWidget *colorselect_cancel_button;
  GtkWidget *colorselect_help_button;
  GtkAccelGroup *accel_group;
  colorselect_obj *colsel;

  colsel = g_new (colorselect_obj, 1);

  colsel->p_tmp = p;
  colsel->color_index_tmp = color_index;

  colsel->colorselect = gtk_color_selection_dialog_new (_("Select Color"));
  gtk_widget_set_name (colsel->colorselect, "colorselect");
  gtk_object_set_data (GTK_OBJECT (colsel->colorselect), "colorselect", colsel->colorselect);
  gtk_container_border_width (GTK_CONTAINER (colsel->colorselect), 10);
  GTK_WINDOW (colsel->colorselect)->type = GTK_WINDOW_DIALOG;
  gtk_color_selection_set_update_policy (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->colorsel), GTK_UPDATE_CONTINUOUS);
  gtk_window_position (GTK_WINDOW (colsel->colorselect), GTK_WIN_POS_MOUSE);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (colsel->colorselect), accel_group);


  colorselect_ok_button = GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->ok_button;
  gtk_widget_set_name (colorselect_ok_button, "colorselect_ok_button");
  gtk_object_set_data (GTK_OBJECT (colsel->colorselect), "colorselect_ok_button", colorselect_ok_button);
  gtk_widget_show (colorselect_ok_button);
  GTK_WIDGET_SET_FLAGS (colorselect_ok_button, GTK_CAN_DEFAULT);

  colorselect_cancel_button = GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->cancel_button;
  gtk_widget_set_name (colorselect_cancel_button, "colorselect_cancel_button");
  gtk_object_set_data (GTK_OBJECT (colsel->colorselect), "colorselect_cancel_button", colorselect_cancel_button);
  gtk_widget_show (colorselect_cancel_button);
  GTK_WIDGET_SET_FLAGS (colorselect_cancel_button, GTK_CAN_DEFAULT);

  colorselect_help_button = GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->help_button;
  gtk_widget_set_name (colorselect_help_button, "colorselect_help_button");
  gtk_object_set_data (GTK_OBJECT (colsel->colorselect), "colorselect_help_button", colorselect_help_button);
  /* gtk_widget_show (colorselect_help_button); */
  GTK_WIDGET_UNSET_FLAGS (colorselect_help_button, GTK_CAN_FOCUS);
  GTK_WIDGET_SET_FLAGS (colorselect_help_button, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive (colorselect_help_button, FALSE);
  gtk_widget_hide (colorselect_help_button);

  gtk_widget_add_accelerator (colorselect_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (colorselect_ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (colorselect_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (colorselect_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (colorselect_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (colorselect_ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), (gpointer) colsel);
  gtk_signal_connect (GTK_OBJECT (colorselect_cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), (gpointer) colsel);

  gtk_signal_connect (GTK_OBJECT (colsel->colorselect), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) colsel);

  set_selcolor (p, color_index, colsel->selcolor);
  gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (colsel->colorselect)->colorsel), colsel->selcolor);
  gtk_window_set_modal (GTK_WINDOW (colsel->colorselect), TRUE);
  gtk_widget_show (colsel->colorselect);
  gtk_main ();
}
