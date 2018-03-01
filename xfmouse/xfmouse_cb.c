/*  xfmouse
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include "my_intl.h"
#include "xfmouse.h"
#include "xfmouse_cb.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
ok_cb (GtkWidget * widget, gpointer data)
{
  apply_mouse_values (&mouseval);
  savecfg (&mouseval);
  gtk_main_quit ();
  exit (0);
}

void
apply_cb (GtkWidget * widget, gpointer data)
{
  apply_mouse_values (&mouseval);
}

void
cancel_cb (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
  exit (0);
}

gboolean delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  cancel_cb (widget, data);
  return (TRUE);
}
