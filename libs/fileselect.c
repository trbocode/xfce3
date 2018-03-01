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
#include "xfce-common.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GtkWidget *fileselect;
static char *result = NULL;

static void
ok_cb (GtkWidget * twidget, gpointer data)
{
  char *filename = (char *) data;
  strcpy (filename, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselect)));
  result = filename;
  gtk_widget_destroy (fileselect);
  gtk_main_quit ();
}

static void
cancel_cb (GtkWidget * twidget)
{
  result = NULL;
  gtk_widget_destroy (fileselect);
  gtk_main_quit ();
}

static gboolean
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_widget_destroy (fileselect);
  gtk_main_quit ();
  return (TRUE);
}

char *
open_fileselect (char *pat)
{
  GtkWidget *fileselect_ok_button;
  GtkWidget *fileselect_cancel_button;
  GtkAccelGroup *accel_group;

  static char filename[MAXSTRLEN];
  char pattern[MAXSTRLEN];
  char *slash;



  fileselect = gtk_file_selection_new (_("Select File"));
  gtk_widget_set_name (fileselect, "fileselect");
  gtk_object_set_data (GTK_OBJECT (fileselect), "fileselect", fileselect);
  gtk_container_border_width (GTK_CONTAINER (fileselect), 10);
  GTK_WINDOW (fileselect)->type = GTK_WINDOW_DIALOG;
  gtk_window_position (GTK_WINDOW (fileselect), GTK_WIN_POS_MOUSE);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (fileselect), accel_group);

  fileselect_ok_button = GTK_FILE_SELECTION (fileselect)->ok_button;
  gtk_widget_set_name (fileselect_ok_button, "fileselect_ok_button");
  gtk_object_set_data (GTK_OBJECT (fileselect), "fileselect_ok_button", fileselect_ok_button);
  gtk_widget_show (fileselect_ok_button);
  GTK_WIDGET_SET_FLAGS (fileselect_ok_button, GTK_CAN_DEFAULT);

  fileselect_cancel_button = GTK_FILE_SELECTION (fileselect)->cancel_button;
  gtk_widget_set_name (fileselect_cancel_button, "fileselect_cancel_button");
  gtk_object_set_data (GTK_OBJECT (fileselect), "fileselect_cancel_button", fileselect_cancel_button);
  gtk_widget_show (fileselect_cancel_button);
  GTK_WIDGET_SET_FLAGS (fileselect_cancel_button, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (fileselect_ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), (gpointer) filename);
  gtk_signal_connect (GTK_OBJECT (fileselect_cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (fileselect), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (fileselect), "destroy", GTK_SIGNAL_FUNC (delete_event), NULL);

  gtk_widget_add_accelerator (fileselect_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (fileselect_ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (fileselect_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (fileselect_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (fileselect_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


  /* gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION (fileselect)); */
  if ((pat) && strlen (pat))
  {
    strcpy (pattern, pat);
    slash = strrchr (pattern, '/');
    if (slash)
      *(++slash) = '\0';
    gtk_file_selection_complete (GTK_FILE_SELECTION (fileselect), pattern);
  }
  gtk_window_set_modal (GTK_WINDOW (fileselect), TRUE);
  result = NULL;
  gtk_widget_show (fileselect);
  gtk_main ();

  return result;
}
