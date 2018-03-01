/*  xfbd
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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "my_intl.h"
#include "xfbd.h"
#include "xfbd_cb.h"
#include "constant.h"
#include "fileselect.h"
#include "fileutil.h"
#include "my_string.h"
#include "xfce-common.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
browse_cb (GtkWidget * widget, gpointer data)
{
  char *fselect;

  fselect = gtk_entry_get_text (GTK_ENTRY (filename_entry));
  if (strlen (fselect) && existfile (fselect))
    fselect = open_fileselect (fselect);
  else
    fselect = open_fileselect (build_path (XFCE_BACKDROPS));
  if (fselect)
    if (strlen (fselect))
    {
      gtk_entry_set_text (GTK_ENTRY (filename_entry), fselect);
      gtk_entry_set_position (GTK_ENTRY (filename_entry), 0);
      strcpy (backdrp, fselect);
      display_back (backdrp);
    }
}

void
apply_cb (GtkWidget * widget, gpointer data)
{
  strncpy (filename, cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (filename_entry))), MAXSTRLEN - 1);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_tiled)) ? 1 : 0)
    strcpy (istiled, "tiled");
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_stretched)) ? 1 : 0)
    strcpy (istiled, "stretched");
  else
    strcpy (istiled, "auto");
  if (!strlen (filename))
    strcpy (filename, NOBACK);
  select_backdrp (filename, backdrp);
  display_back (backdrp);
  setroot (backdrp, istiled);
}

void
cancel_cb (GtkWidget * widget, gpointer data)
{
  free (backdrp);
  free (filename);
  gtk_main_quit ();
  exit (0);
}

void
ok_cb (GtkWidget * widget, gpointer data)
{
  strncpy (filename, cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (filename_entry))), MAXSTRLEN - 1);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_tiled)) ? 1 : 0)
    strcpy (istiled, "tiled");
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_stretched)) ? 1 : 0)
    strcpy (istiled, "stretched");
  else
    strcpy (istiled, "auto");
  if (!strlen (filename))
    strcpy (filename, NOBACK);
  select_backdrp (filename, backdrp);
  writestr (filename, istiled);
  setroot (filename, istiled);
  free (backdrp);
  free (filename);
  gtk_main_quit ();
  exit (0);
}

void
clear_cb (GtkWidget * widget, gpointer data)
{
  gtk_entry_set_text (GTK_ENTRY (filename_entry), NOBACK);
  gtk_entry_set_position (GTK_ENTRY (filename_entry), 0);
  strcpy (backdrp, NOBACK);
  strcpy (filename, NOBACK);
  display_back (filename);
}

void
on_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata)
{
  GList *fnames;
  guint count;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);
  if (count > 0)
  {
    gtk_entry_set_text (GTK_ENTRY (filename_entry), (char *) fnames->data);
    gtk_entry_set_position (GTK_ENTRY (filename_entry), 0);
    strcpy (backdrp, (char *) fnames->data);
    display_back (backdrp);
  }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE), time);
}

gboolean delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  cancel_cb (widget, data);
  return (TRUE);
}
