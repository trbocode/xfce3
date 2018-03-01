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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "constant.h"
#include "xfcolor.h"
#include "xfce_main.h"
#include "xfce.h"
#include "xfce_cb.h"
#include "xfce-common.h"
#include "configfile.h"
#include "popup.h"
#include "fileutil.h"
#include "modify.h"
#include "my_string.h"

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
private_close_popup_button (gint menu)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (popup_buttons.popup_button[menu]), FALSE);
  toggle_popup_button (GTK_WIDGET (popup_buttons.popup_button[menu]), GTK_PIXMAP (popup_buttons.popup_pixmap[menu]));
}

gboolean
delete_popup_cb (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  private_close_popup_button ((gint) ((long) data));
  return (TRUE);
}

gboolean
popup_entry_modify_cb (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  gint menu, item;

  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
    return FALSE;
    
  menu = ((gint) ((long) data)) / NBMAXITEMS;
  item = ((gint) ((long) data)) % NBMAXITEMS;

  if (event->button == 3)
  {
    open_modify (modify, menu, item);
    if (!popup_menus[menu].detach)
    {
      hide_popup_menu (menu, FALSE);
      show_popup_menu (menu, -1, -1, FALSE);
    }
    writeconfig ();
    return TRUE;
  }
  return FALSE;
}

void
popup_entry_cb (GtkWidget * widget, gpointer data)
{
  gint menu, item;

  menu = ((gint) ((long) data)) / NBMAXITEMS;
  item = ((gint) ((long) data)) % NBMAXITEMS;

  if (!popup_menus[menu].detach)
  {
    private_close_popup_button (menu);
    gtk_button_released (GTK_BUTTON (widget));
    gtk_button_leave (GTK_BUTTON (widget));
  }
  exec_comm (popup_menus[menu].popup_buttons[item].command, current_config.wm);
}

void
popup_addicon_cb (GtkWidget * widget, gpointer data)
{
  gint menu;

  menu = (gint) ((long) data);
  open_modify (modify, menu, -1);
  hide_popup_menu (menu, FALSE);
  show_popup_menu (menu, -1, -1, FALSE);
  writeconfig ();
}

void
detach_cb (GtkWidget * widget, gpointer data)
{
  hide_popup_menu ((gint) ((long) data), FALSE);
  popup_menus[(gint) ((long) data)].detach = TRUE;
  show_popup_menu ((gint) ((long) data), -1, -1, FALSE);
}

void
popup_entry_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata)
{
  GList *fnames, *fnp;
  guint count;
  char *execute, *cmd;
  gint menu, item;

  menu = ((gint) ((long) cbdata)) / NBMAXITEMS;
  item = ((gint) ((long) cbdata)) % NBMAXITEMS;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);
  if (count > 0)
  {
    if (!popup_menus[menu].detach)
      private_close_popup_button (menu);
    execute = (char *) g_malloc (MAXSTRLEN * sizeof (char) + 1);
    cmd = popup_menus[menu].popup_buttons[item].command;
    for (fnp = fnames; fnp; fnp = fnp->next, count--)
    {
      snprintf (execute, MAXSTRLEN - 1, "%s %s", cmd, (char *) (fnp->data));
      exec_comm (execute, current_config.wm);
    }
    g_free (execute);
    gtk_drag_finish (context, TRUE, TRUE, time);
  }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, FALSE, TRUE, time);
}

void
popup_addicon_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata)
{
  GList *fnames;
  guint count;
  char *cmd;
  char *label;
  char *name;
  gint menu;

  menu = (gint) ((long) cbdata);

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);
  if (count > 0)
  {
    hide_popup_menu (menu, FALSE);
    label = (char *) g_malloc (MAXSTRLEN * sizeof (char) + 1);
    cmd = (char *) fnames->data;
    if (existfile (cmd))
    {
      name = my_strrchr (cmd, '/');
      if (name)
      {
	snprintf (label, MAXSTRLEN - 1, "%s", name);
	*label = toupper (*label);
	add_popup_entry (menu, label, "Default icon", cmd, 0);
	writeconfig ();
      }
    }
    else
      my_show_message (_("Cannot find the file you dropped !"));
    g_free (label);
    show_popup_menu (menu, -1, -1, FALSE);
    gtk_drag_finish (context, TRUE, TRUE, time);
  }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, FALSE, TRUE, time);
}
