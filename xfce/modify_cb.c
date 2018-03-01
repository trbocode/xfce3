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
#include "modify.h"
#include "modify_cb.h"
#include "my_string.h"
#include "popup.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "constant.h"
#include "fileutil.h"
#include "xpmext.h"
#include "fileselect.h"
#include "configfile.h"
#include "defaulticon.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
modify_cancel_cb (GtkWidget * widget, gpointer data)
{
  gtk_signal_disconnect (GTK_OBJECT (modify_ok_button), signal_id1);
  gtk_signal_disconnect (GTK_OBJECT (modify_remove_button), signal_id2);
  gtk_main_quit ();
  gtk_widget_hide (modify);
  gdk_window_withdraw ((GTK_WIDGET (modify))->window);
}

gboolean
modify_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  modify_cancel_cb (widget, data);
  return (TRUE);
}

void
modify_browse_command_cb (GtkWidget * widget, gpointer data)
{
  char *command;

  command = gtk_entry_get_text (GTK_ENTRY (modify_command_entry));
  if (strlen (command) && existfile (command))
    command = open_fileselect (command);
  else
    command = open_fileselect (XBINDIR);
  if (command)
  {
    gtk_entry_set_text (GTK_ENTRY (modify_command_entry), command);
  }
}

void
modify_browse_icon_cb (GtkWidget * widget, gpointer data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  char *pixfile;

  pixfile = gtk_entry_get_text (GTK_ENTRY (modify_icon_entry));

  if (strlen (pixfile) && existfile (pixfile))
    pixfile = open_fileselect (pixfile);
  else
    pixfile = open_fileselect (build_path (XFCE_ICONS));
  if (pixfile)
  {
    if (existfile (pixfile))
    {
      gtk_entry_set_text (GTK_ENTRY (modify_icon_entry), pixfile);
      pixmap = MyCreateGdkPixmapFromFile (pixfile, modify_preview_frame, &mask, TRUE);
      gtk_pixmap_set (GTK_PIXMAP (modify_preview_pixmap), pixmap, mask);
    }
    else
    {
      gtk_entry_set_text (GTK_ENTRY (modify_icon_entry), "Default icon");
      pixmap = MyCreateGdkPixmapFromData (defaulticon, modify_preview_frame, &mask, TRUE);
      gtk_pixmap_set (GTK_PIXMAP (modify_preview_pixmap), pixmap, mask);
    }
    gtk_entry_set_text (GTK_ENTRY (modify_icon_entry), pixfile);
  }
}

void
modify_remove_cb (GtkWidget * widget, gpointer data)
{
  if (my_yesno_dialog (_("Are you sure you want to remove this entry ?")))
    remove_popup_entry (((gint) ((long) data)) / NBMAXITEMS, ((gint) ((long) data)) % NBMAXITEMS);
  gtk_signal_disconnect (GTK_OBJECT (modify_ok_button), signal_id1);
  gtk_signal_disconnect (GTK_OBJECT (modify_remove_button), signal_id2);
  gtk_main_quit ();
  gtk_widget_hide (modify);
  gdk_window_withdraw ((GTK_WIDGET (modify))->window);
  writeconfig ();
}

void
modify_change_cb (GtkWidget * widget, gpointer data)
{
  char *s1, *s2, *s3;
  gint x1, x3;
  gint menu, item, position;

  menu = ((gint) ((long) data)) / NBMAXITEMS;
  item = ((gint) ((long) data)) % NBMAXITEMS;
  position = my_get_adjustment_as_int (GTK_ADJUSTMENT (modify_pos_adj)) - 1;

  s1 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_command_entry)));
  s2 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_icon_entry)));
  s3 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_displayed_entry)));
  x1 = strlen (s1);
  x3 = strlen (s3);
  if (x1 && x3)
  {
    set_entry (menu, item, s3, s2, s1);
    if (item != position)
    {
      move_popup_entry (menu, item, position);
    }
    gtk_signal_disconnect (GTK_OBJECT (modify_ok_button), signal_id1);
    gtk_signal_disconnect (GTK_OBJECT (modify_remove_button), signal_id2);
    gtk_main_quit ();
    gtk_widget_hide (modify);
    gdk_window_withdraw ((GTK_WIDGET (modify))->window);
    writeconfig ();
  }
  else
    my_show_message (_("You must provide the command\nand the label, at least !"));
}

void
modify_add_cb (GtkWidget * widget, gpointer data)
{
  char *s1, *s2, *s3;
  gint x1, x3;
  gint menu, position;

  menu = ((gint) ((long) data)) / NBMAXITEMS;
  position = my_get_adjustment_as_int (GTK_ADJUSTMENT (modify_pos_adj)) - 1;

  s1 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_command_entry)));
  s2 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_icon_entry)));
  s3 = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (modify_displayed_entry)));
  x1 = strlen (s1);
  x3 = strlen (s3);
  if (x1 && x3)
  {
    add_popup_entry (menu, s3, s2, s1, position);
    gtk_signal_disconnect (GTK_OBJECT (modify_ok_button), signal_id1);
    gtk_signal_disconnect (GTK_OBJECT (modify_remove_button), signal_id2);
    gtk_main_quit ();
    gtk_widget_hide (modify);
    gdk_window_withdraw ((GTK_WIDGET (modify))->window);
    writeconfig ();
  }
  else
    my_show_message (_("You must provide the command\nand the label, at least !"));
}
