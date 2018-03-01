/******************************************************************************
 * xfbdmgr - a Random Backdrop list manager for XFCE
 * xfbdmgr (c) 2000 by Alex Lambert (alex@penwing.uklinux.net)
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
 *****************************************************************************/

/******************************************************************************
 * xfbdmgr - a Random Backdrop List Manager for XFCE.
 * This program manages lists for xfbd when operating in random mode. 
 *****************************************************************************/

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<dirent.h>
#include<stdio.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#include "my_intl.h"
#include "xfbdmgr_cb.h"
#include "xfbdmgr.h"
#include "constant.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"

#ifndef HAVE_SCANDIR
#include "my_scandir.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


void
Myquit (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
  xfce_end ((gpointer) NULL, 0);
}

void
get_save_filename (GtkFileSelection * selector, gpointer user_data)
{
  gchar *fselect;

  fselect = malloc (sizeof (gchar) * MAXSTRLEN);
  strcpy (fselect, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselector2)));
  fselect[strlen (fselect)] = 0;

  gtk_entry_set_text (GTK_ENTRY (edit_name), fselect);

  saveList ();

  Changed = FALSE;

  free (fselect);

}

void
save_clicked (GtkWidget * widget, gpointer data)
{
  if (strcmp (gtk_entry_get_text (GTK_ENTRY (edit_name)), "") != 0)
  {
    saveList ();
  }
  else
  {
    fileselector2 = gtk_file_selection_new ("Choose List to Save to...");
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileselector2)->ok_button), "clicked", GTK_SIGNAL_FUNC (get_save_filename), (gpointer) NULL);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector2)->ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector2);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector2)->cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector2);
    gtk_widget_show (fileselector2);
  }
}

void
get_load_filename (GtkFileSelection * selector, gpointer user_data)
{
  gchar *fselect;

  fselect = malloc (sizeof (gchar) * MAXSTRLEN);
  strcpy (fselect, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselector1)));
  fselect[strlen (fselect)] = 0;

  gtk_entry_set_text (GTK_ENTRY (edit_name), fselect);

  writeToList ();

  Changed = FALSE;

  free (fselect);

}

void
edit_changed (GtkWidget * widget, gpointer data)
{
  Changed = TRUE;
}

void
load_clicked (GtkWidget * widget, gpointer data)
{
  if (Changed == FALSE)
  {
    fileselector1 = gtk_file_selection_new ("Choose List to Load...");
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileselector1)->ok_button), "clicked", GTK_SIGNAL_FUNC (get_load_filename), (gpointer) NULL);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector1)->ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector1);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector1)->cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector1);
    gtk_widget_show (fileselector1);
  }
  else
  {
    writeToList ();
  }
}

void
add_filename (gchar * fselect)
{
  gchar *indFile;
  struct stat stat_buf;
  struct dirent **namelist;
  int n;

  stat (fselect, &stat_buf);

  if (S_ISDIR (stat_buf.st_mode))
  {
    indFile = malloc (sizeof (gchar) * MAXSTRLEN);
    n = scandir (fselect, &namelist, 0, alphasort);
    if (n > 0)
    {
      while (--n > 1)
      {
	strcpy (indFile, fselect);
	strcat (indFile, "/");
	strcat (indFile, namelist[n]->d_name);
	add_filename (indFile);
      }
      free (indFile);
    }
  }
  else
  {
    theNumber++;
    gtk_clist_append (GTK_CLIST (clist_fileslist), &fselect);
  }

}

void
get_add_filename (GtkFileSelection * selector, gpointer user_data)
{
  gchar *fselect;

  fselect = malloc (sizeof (gchar) * MAXSTRLEN);
  strcpy (fselect, gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselector)));
  if (fselect[strlen (fselect) - 1] == '/')
  {
    fselect[strlen (fselect) - 1] = 0;
  }
  else
  {
    fselect[strlen (fselect)] = 0;
  }

  add_filename (fselect);

  free (fselect);

}

void
add_clicked (GtkWidget * widget, gpointer data)
{
  fileselector = gtk_file_selection_new ("Choose Image to Add");
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileselector)->ok_button), "clicked", GTK_SIGNAL_FUNC (get_add_filename), (gpointer) NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector)->ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileselector)->cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), (gpointer) fileselector);
  gtk_widget_show (fileselector);
}

void
remove_clicked (GtkWidget * widget, gpointer data)
{
  if (selected != -1)
  {
    theNumber--;
    gtk_clist_remove (GTK_CLIST (clist_fileslist), selected);
  }
}

void
selectionMade (GtkWidget * widget, gint row, gint column, GdkEventButton * event, gpointer data)
{
  selected = row;
}

void
selectionLost (GtkWidget * widget, gint row, gint column, GdkEventButton * event, gpointer data)
{
  selected = -1;
}

void
on_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata)
{
  GList *fnames, *fnp;
  guint count;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);
  if (count > 0)
  {
    gtk_clist_freeze (GTK_CLIST (clist_fileslist));
    for (fnp = fnames; fnp; fnp = fnp->next, count--)
    {
      add_filename ((gchar *) fnp->data);
    }
    gtk_clist_thaw (GTK_CLIST (clist_fileslist));
  }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE), time);
}
