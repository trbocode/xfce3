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

#include "screen.h"
#include "my_string.h"
#include "xfce.h"
#include "xfwm.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "configfile.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
screen_cancel_cb (GtkWidget * widget, gpointer data)
{
  gtk_signal_disconnect (GTK_OBJECT (screen_ok_button), screen_signal_id);
  gtk_main_quit ();
  gtk_widget_hide (screen);
  gdk_window_withdraw ((GTK_WIDGET (screen))->window);
}

gboolean
screen_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  screen_cancel_cb (widget, data);
  return (TRUE);
}

void
screen_ok_cb (GtkWidget * widget, gpointer data)
{
  char *s;
  gtk_signal_disconnect (GTK_OBJECT (screen_ok_button), screen_signal_id);

  s = cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (screen_displayed_entry)));
  set_gxfce_screen_label ((gint) ((long) data), s);
  apply_wm_desk_names (current_config.visible_screen);
  gtk_main_quit ();
  gtk_widget_hide (screen);
  gdk_window_withdraw ((GTK_WIDGET (screen))->window);
  writeconfig ();
}
