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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GtkWidget *fontselectiondialog;
static char *result = NULL;

static void
ok_cb (GtkWidget * twidget, gpointer data)
{
  char *fontname = (char *) data;
  gchar *s;
  s = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (fontselectiondialog));
  if ((s) && strlen (s))
    strcpy (fontname, s);
  else
    strcpy (fontname, DEFAULTFONT);
  result = fontname;
  gtk_widget_destroy (fontselectiondialog);
  gtk_main_quit ();
}

static void
cancel_cb (GtkWidget * twidget)
{
  result = NULL;
  gtk_widget_destroy (fontselectiondialog);
  gtk_main_quit ();
}

static gboolean
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_widget_destroy (fontselectiondialog);
  gtk_main_quit ();
  return (TRUE);
}

char *
open_fontselection (char *init)
{
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *apply_button;
  GtkAccelGroup *accel_group;

  static char fontname[MAXSTRLEN];

  fontselectiondialog = gtk_font_selection_dialog_new (_("Font Selection"));
  gtk_widget_set_name (fontselectiondialog, "fontselectiondialog");
  gtk_object_set_data (GTK_OBJECT (fontselectiondialog), "fontselectiondialog", fontselectiondialog);
  gtk_container_border_width (GTK_CONTAINER (fontselectiondialog), 4);
  gtk_window_set_position (GTK_WINDOW (fontselectiondialog), GTK_WIN_POS_MOUSE);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (fontselectiondialog), accel_group);

  ok_button = GTK_FONT_SELECTION_DIALOG (fontselectiondialog)->ok_button;
  gtk_widget_set_name (ok_button, "ok_button");
  gtk_object_set_data (GTK_OBJECT (fontselectiondialog), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

  cancel_button = GTK_FONT_SELECTION_DIALOG (fontselectiondialog)->cancel_button;
  gtk_widget_set_name (cancel_button, "cancel_button");
  gtk_object_set_data (GTK_OBJECT (fontselectiondialog), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

  apply_button = GTK_FONT_SELECTION_DIALOG (fontselectiondialog)->apply_button;
  gtk_object_set_data (GTK_OBJECT (fontselectiondialog), "apply_button", apply_button);
  gtk_widget_hide (apply_button);

  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), (gpointer) fontname);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (fontselectiondialog), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (fontselectiondialog), "destroy", GTK_SIGNAL_FUNC (delete_event), NULL);

  if (init)
    gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (fontselectiondialog), init);
  gtk_window_set_modal (GTK_WINDOW (fontselectiondialog), TRUE);
  gtk_widget_show (fontselectiondialog);
  result = NULL;
  gtk_main ();
  return result;
}
