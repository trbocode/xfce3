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
#include "screen.h"
#include "screen_cb.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "configfile.h"
#include "xfce.h"
#include "gnome_protocol.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

GtkWidget *
create_screen (void)
{
  GtkWidget *screen;
  GtkWidget *screen_mainframe;
  GtkWidget *screen_vbox;
  GtkWidget *screen_hbox;
  GtkWidget *screen_topframe;
  GtkWidget *screen_table;
  GtkWidget *screen_displayed_label;
  GtkWidget *screen_bottomframe;
  GtkWidget *screen_hbuttonbox;
  GtkWidget *screen_cancel_button;
  GtkAccelGroup *accel_group;

  screen = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_widget_set_name (screen, "screen");
  gtk_object_set_data (GTK_OBJECT (screen), "screen", screen);
  gtk_window_set_title (GTK_WINDOW (screen), _("Screen label ..."));
  gtk_window_position (GTK_WINDOW (screen), GTK_WIN_POS_CENTER);
  gtk_widget_realize (screen);
  /*
     gnome_layer (screen->window, MAX_LAYERS);
   */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (screen), accel_group);

  screen_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (screen_mainframe, "screen_mainframe");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_mainframe", screen_mainframe);
  gtk_widget_show (screen_mainframe);
  gtk_container_add (GTK_CONTAINER (screen), screen_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (screen_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (screen_mainframe), GTK_SHADOW_OUT);
#endif

  screen_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (screen_vbox, "screen_vbox");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_vbox", screen_vbox);
  gtk_widget_show (screen_vbox);
  gtk_container_add (GTK_CONTAINER (screen_mainframe), screen_vbox);

  screen_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (screen_hbox, "screen_hbox");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_hbox", screen_hbox);
  gtk_widget_show (screen_hbox);
  gtk_box_pack_start (GTK_BOX (screen_vbox), screen_hbox, TRUE, TRUE, 0);

  screen_topframe = gtk_frame_new (NULL);
  gtk_widget_set_name (screen_topframe, "screen_topframe");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_topframe", screen_topframe);
  gtk_widget_show (screen_topframe);
  gtk_box_pack_start (GTK_BOX (screen_hbox), screen_topframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (screen_topframe), 5);

  screen_table = gtk_table_new (1, 2, FALSE);
  gtk_widget_set_name (screen_table, "screen_table");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_table", screen_table);
  gtk_widget_show (screen_table);
  gtk_container_add (GTK_CONTAINER (screen_topframe), screen_table);

  screen_displayed_label = gtk_label_new (_("Define screen label :"));
  gtk_widget_set_name (screen_displayed_label, "screen_displayed_label");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_displayed_label", screen_displayed_label);
  gtk_widget_show (screen_displayed_label);
  gtk_table_attach (GTK_TABLE (screen_table), screen_displayed_label, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (screen_displayed_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (screen_displayed_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (screen_displayed_label), 10, 0);

  screen_displayed_entry = gtk_entry_new ();
  gtk_widget_set_name (screen_displayed_entry, "screen_displayed_entry");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_displayed_entry", screen_displayed_entry);
  gtk_widget_show (screen_displayed_entry);
  gtk_table_attach (GTK_TABLE (screen_table), screen_displayed_entry, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  screen_bottomframe = gtk_frame_new (NULL);
  gtk_widget_set_name (screen_bottomframe, "screen_bottomframe");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_bottomframe", screen_bottomframe);
  gtk_widget_show (screen_bottomframe);
  gtk_box_pack_start (GTK_BOX (screen_vbox), screen_bottomframe, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (screen_bottomframe), 5);

  screen_hbuttonbox = gtk_hbutton_box_new ();
  gtk_widget_set_name (screen_hbuttonbox, "screen_hbuttonbox");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_hbuttonbox", screen_hbuttonbox);
  gtk_widget_show (screen_hbuttonbox);
  gtk_container_add (GTK_CONTAINER (screen_bottomframe), screen_hbuttonbox);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (screen_hbuttonbox), GTK_BUTTONBOX_SPREAD);

  screen_ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (screen_ok_button, "screen_ok_button");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_ok_button", screen_ok_button);
  gtk_widget_show (screen_ok_button);
  gtk_container_add (GTK_CONTAINER (screen_hbuttonbox), screen_ok_button);
  gtk_container_border_width (GTK_CONTAINER (screen_ok_button), 5);
  GTK_WIDGET_SET_FLAGS (screen_ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (screen_ok_button);


  screen_cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (screen_cancel_button, "screen_cancel_button");
  gtk_object_set_data (GTK_OBJECT (screen), "screen_cancel_button", screen_cancel_button);
  gtk_widget_show (screen_cancel_button);
  gtk_container_add (GTK_CONTAINER (screen_hbuttonbox), screen_cancel_button);
  gtk_container_border_width (GTK_CONTAINER (screen_cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (screen_cancel_button, GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator (screen_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (screen_ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (screen_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (screen_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (screen_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (screen), "delete_event", GTK_SIGNAL_FUNC (screen_delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (screen_cancel_button), "clicked", GTK_SIGNAL_FUNC (screen_cancel_cb), NULL);

  return screen;
}

void
open_screen (GtkWidget * screen, gint scr_number)
{
  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
    return;
    
  screen_signal_id = gtk_signal_connect (GTK_OBJECT (screen_ok_button), "clicked", GTK_SIGNAL_FUNC (screen_ok_cb), (gpointer) ((long) scr_number));

  gtk_entry_set_text (GTK_ENTRY (screen_displayed_entry), get_gxfce_screen_label (scr_number));
  gtk_entry_set_position (GTK_ENTRY (screen_displayed_entry), 0);

  gtk_window_set_modal (GTK_WINDOW (screen), TRUE);
  gnome_sticky (screen->window);
  gtk_widget_show (screen);
  gtk_main ();
}
