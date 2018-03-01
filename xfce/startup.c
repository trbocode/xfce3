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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "xfce_main.h"
#include "xpmext.h"
#include "gnome_protocol.h"
#include "xfce-common.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

GtkWidget *
create_startup ()
{
  GtkWidget *startup;
  GtkWidget *startup_mainframe;
  GtkWidget *startup_vbox;
  GtkWidget *startup_topframe;
  GtkWidget *startup_logo_pixmap;
  GtkWidget *startup_wait_label;

  startup = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (startup, "startup");
  gtk_object_set_data (GTK_OBJECT (startup), "startup", startup);
  gtk_widget_set_usize (startup, 280, 160);
  gtk_window_set_title (GTK_WINDOW (startup), _("Startup"));
  gtk_window_position (GTK_WINDOW (startup), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (startup), FALSE, FALSE, FALSE);
  gtk_widget_realize (startup);
  gdk_window_set_decorations (startup->window, (GdkWMDecoration) 0);
  gnome_layer (startup->window, MAX_LAYERS);

  startup_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (startup_mainframe, "startup_mainframe");
  gtk_object_set_data (GTK_OBJECT (startup), "startup_mainframe", startup_mainframe);
  gtk_widget_show (startup_mainframe);
  gtk_container_add (GTK_CONTAINER (startup), startup_mainframe);
  gtk_frame_set_shadow_type (GTK_FRAME (startup_mainframe), GTK_SHADOW_OUT);

  startup_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (startup_vbox, "startup_vbox");
  gtk_object_set_data (GTK_OBJECT (startup), "startup_vbox", startup_vbox);
  gtk_widget_show (startup_vbox);
  gtk_container_add (GTK_CONTAINER (startup_mainframe), startup_vbox);

  startup_topframe = gtk_frame_new (NULL);
  gtk_widget_set_name (startup_topframe, "startup_topframe");
  gtk_object_set_data (GTK_OBJECT (startup), "startup_topframe", startup_topframe);
  gtk_widget_show (startup_topframe);
  gtk_box_pack_start (GTK_BOX (startup_vbox), startup_topframe, TRUE, TRUE, 0);
  gtk_widget_set_usize (startup_topframe, 260, 100);
  gtk_container_border_width (GTK_CONTAINER (startup_topframe), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (startup_topframe), GTK_SHADOW_IN);

  startup_logo_pixmap = MyCreateFromPixmapFile (startup, build_path (XFCE_LOGO));
  if (startup_logo_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (startup_logo_pixmap, "startup_logo_pixmap");
  gtk_object_set_data (GTK_OBJECT (startup), "startup_logo_pixmap", startup_logo_pixmap);
  gtk_widget_show (startup_logo_pixmap);
  gtk_container_add (GTK_CONTAINER (startup_topframe), startup_logo_pixmap);

  startup_wait_label = gtk_label_new (_("Please wait, XFce is loading..."));
  gtk_widget_set_name (startup_wait_label, "startup_wait_label");
  gtk_object_set_data (GTK_OBJECT (startup), "startup_wait_label", startup_wait_label);
  gtk_widget_show (startup_wait_label);
  gtk_box_pack_start (GTK_BOX (startup_vbox), startup_wait_label, TRUE, TRUE, 0);

  return startup;
}

void
open_startup (GtkWidget * startup)
{

  gtk_widget_show_all (startup);
  my_flush_events ();
}

void
close_startup (GtkWidget * startup)
{
  gtk_widget_destroy (startup);
}
