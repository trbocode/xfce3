/*  gxfce
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or action
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
#include "action.h"
#include "action_cb.h"
#include "my_string.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "constant.h"
#include "fileutil.h"
#include "fileselect.h"
#include "selects.h"
#include "configfile.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
action_cancel_cb (GtkWidget * widget, gpointer data)
{
  gtk_signal_disconnect (GTK_OBJECT (action_ok_button), action_signal_id1);
  gtk_signal_disconnect (GTK_OBJECT (action_icon_browse_button), action_signal_id2);
  gtk_main_quit ();
  gtk_widget_hide (action);
  gdk_window_withdraw ((GTK_WIDGET (action))->window);
}

void
action_ok_cb (GtkWidget * widget, gpointer data)
{
  char *s;
  gint x;

  s = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (action_command_entry)));
  x = strlen (s);
  if (x)
  {
    gtk_signal_disconnect (GTK_OBJECT (action_ok_button), action_signal_id1);
    gtk_signal_disconnect (GTK_OBJECT (action_icon_browse_button), action_signal_id2);
    set_command ((gint) ((long) data), s);
    if (((gint) ((long) data)) < NBSELECTS)
      set_icon_nbr ((gint) ((long) data), action_get_choice_selected ());
    gtk_main_quit ();
    gtk_widget_hide (action);
    gdk_window_withdraw ((GTK_WIDGET (action))->window);
    writeconfig ();
  }
  else
    my_show_message (_("You must provide the command to execute !"));
}

gboolean
action_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  action_cancel_cb (widget, data);
  return (TRUE);
}

void
action_browse_command_cb (GtkWidget * widget, gpointer data)
{
  char *command;

  command = get_command ((gint) ((long) data));
  if (strlen (command) && existfile (command))
    command = open_fileselect (command);
  else
    command = open_fileselect (XBINDIR);
  if (command)
  {
    gtk_entry_set_text (GTK_ENTRY (action_command_entry), command);
  }
}

void
action_browse_icon_cb (GtkWidget * widget, gpointer data)
{
  char *pixfile;

  pixfile = get_exticon_str ((gint) ((long) data));
  if (strlen (pixfile) && existfile (pixfile))
    pixfile = open_fileselect (pixfile);
  else
    pixfile = open_fileselect (build_path (XFCE_ICONS));
  if (pixfile)
  {
    action_set_choice_selected (NB_PANEL_ICONS);
    if (existfile (pixfile))
      set_exticon_str ((gint) ((long) data), pixfile);
    else
      set_exticon_str ((gint) ((long) data), "None");
  }
}
