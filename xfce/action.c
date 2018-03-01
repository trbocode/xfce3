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
#include "xfce_main.h"
#include "xfce-common.h"
#include "configfile.h"
#include "selects.h"
#include "action.h"
#include "action_cb.h"
#include "gnome_protocol.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static long private_action_menu_selected = -1;

GtkWidget *
create_action (void)
{
  GtkWidget *action;
  GtkWidget *action_mainframe;
  GtkWidget *action_vbox;
  GtkWidget *action_hbox;
  GtkWidget *actopn_topframe;
  GtkWidget *action_table;
  GtkWidget *action_command_label;
  GtkWidget *action_command_browse_button;
  GtkWidget *action_bottom_frame;
  GtkWidget *action_hbuttonbox;
  GtkWidget *action_cancel_button;
  GtkAccelGroup *accel_group;

  action = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_widget_set_name (action, "action");
  gtk_object_set_data (GTK_OBJECT (action), "action", action);
  gtk_window_set_title (GTK_WINDOW (action), _("Define action ..."));
  gtk_window_position (GTK_WINDOW (action), GTK_WIN_POS_CENTER);
  gtk_widget_realize (action);
  /*
     gnome_layer (action->window, MAX_LAYERS);
   */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (action), accel_group);

  action_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (action_mainframe, "action_mainframe");
  gtk_object_set_data (GTK_OBJECT (action), "action_mainframe", action_mainframe);
  gtk_widget_show (action_mainframe);
  gtk_container_add (GTK_CONTAINER (action), action_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (action_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (action_mainframe), GTK_SHADOW_OUT);
#endif

  action_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (action_vbox, "action_vbox");
  gtk_object_set_data (GTK_OBJECT (action), "action_vbox", action_vbox);
  gtk_widget_show (action_vbox);
  gtk_container_add (GTK_CONTAINER (action_mainframe), action_vbox);

  action_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (action_hbox, "action_hbox");
  gtk_object_set_data (GTK_OBJECT (action), "action_hbox", action_hbox);
  gtk_widget_show (action_hbox);
  gtk_box_pack_start (GTK_BOX (action_vbox), action_hbox, TRUE, TRUE, 0);

  actopn_topframe = gtk_frame_new (NULL);
  gtk_widget_set_name (actopn_topframe, "actopn_topframe");
  gtk_object_set_data (GTK_OBJECT (action), "actopn_topframe", actopn_topframe);
  gtk_widget_show (actopn_topframe);
  gtk_box_pack_start (GTK_BOX (action_hbox), actopn_topframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (actopn_topframe), 5);

  action_table = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (action_table, "action_table");
  gtk_object_set_data (GTK_OBJECT (action), "action_table", action_table);
  gtk_widget_show (action_table);
  gtk_container_add (GTK_CONTAINER (actopn_topframe), action_table);

  action_command_label = gtk_label_new (_("Command Line :"));
  gtk_widget_set_name (action_command_label, "action_command_label");
  gtk_object_set_data (GTK_OBJECT (action), "action_command_label", action_command_label);
  gtk_widget_show (action_command_label);
  gtk_table_attach (GTK_TABLE (action_table), action_command_label, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (action_command_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (action_command_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (action_command_label), 10, 0);

  action_icon_label = gtk_label_new (_("Icon Style :"));
  gtk_widget_set_name (action_icon_label, "action_icon_label");
  gtk_object_set_data (GTK_OBJECT (action), "action_icon_label", action_icon_label);
  gtk_widget_show (action_icon_label);
  gtk_table_attach (GTK_TABLE (action_table), action_icon_label, 0, 1, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (action_icon_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (action_icon_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (action_icon_label), 10, 0);

  action_command_entry = gtk_entry_new ();
  gtk_widget_set_name (action_command_entry, "action_command_entry");
  gtk_object_set_data (GTK_OBJECT (action), "action_command_entry", action_command_entry);
  gtk_widget_show (action_command_entry);
  gtk_table_attach (GTK_TABLE (action_table), action_command_entry, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  action_icon_optionmenu = gtk_option_menu_new ();
  gtk_widget_set_name (action_icon_optionmenu, "action_icon_optionmenu");
  gtk_object_set_data (GTK_OBJECT (action), "action_icon_optionmenu", action_icon_optionmenu);
  gtk_widget_show (action_icon_optionmenu);
  gtk_table_attach (GTK_TABLE (action_table), action_icon_optionmenu, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  action_icon_optionmenu_menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (action_icon_optionmenu), action_icon_optionmenu_menu);

  action_icon_browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (action_icon_browse_button, "action_icon_browse_button");
  gtk_object_set_data (GTK_OBJECT (action), "action_icon_browse_button", action_icon_browse_button);
  gtk_widget_show (action_icon_browse_button);
  gtk_table_attach (GTK_TABLE (action_table), action_icon_browse_button, 2, 3, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (action_icon_browse_button, 90, -2);
  gtk_container_border_width (GTK_CONTAINER (action_icon_browse_button), 5);
  GTK_WIDGET_SET_FLAGS (action_icon_browse_button, GTK_CAN_DEFAULT);

  action_command_browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (action_command_browse_button, "action_command_browse_button");
  gtk_object_set_data (GTK_OBJECT (action), "action_command_browse_button", action_command_browse_button);
  gtk_widget_show (action_command_browse_button);
  gtk_table_attach (GTK_TABLE (action_table), action_command_browse_button, 2, 3, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (action_command_browse_button, 90, -2);
  gtk_container_border_width (GTK_CONTAINER (action_command_browse_button), 5);
  GTK_WIDGET_SET_FLAGS (action_command_browse_button, GTK_CAN_DEFAULT);

  action_bottom_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (action_bottom_frame, "action_bottom_frame");
  gtk_object_set_data (GTK_OBJECT (action), "action_bottom_frame", action_bottom_frame);
  gtk_widget_show (action_bottom_frame);
  gtk_box_pack_start (GTK_BOX (action_vbox), action_bottom_frame, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (action_bottom_frame), 5);

  action_hbuttonbox = gtk_hbutton_box_new ();
  gtk_widget_set_name (action_hbuttonbox, "action_hbuttonbox");
  gtk_object_set_data (GTK_OBJECT (action), "action_hbuttonbox", action_hbuttonbox);
  gtk_widget_show (action_hbuttonbox);
  gtk_container_add (GTK_CONTAINER (action_bottom_frame), action_hbuttonbox);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_hbuttonbox), GTK_BUTTONBOX_SPREAD);

  action_ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (action_ok_button, "action_ok_button");
  gtk_object_set_data (GTK_OBJECT (action), "action_ok_button", action_ok_button);
  gtk_widget_show (action_ok_button);
  gtk_container_add (GTK_CONTAINER (action_hbuttonbox), action_ok_button);
  gtk_container_border_width (GTK_CONTAINER (action_ok_button), 5);
  GTK_WIDGET_SET_FLAGS (action_ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (action_ok_button);

  action_cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (action_cancel_button, "action_cancel_button");
  gtk_object_set_data (GTK_OBJECT (action), "action_cancel_button", action_cancel_button);
  gtk_widget_show (action_cancel_button);
  gtk_container_add (GTK_CONTAINER (action_hbuttonbox), action_cancel_button);
  gtk_container_border_width (GTK_CONTAINER (action_cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (action_cancel_button, GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator (action_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (action_ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (action_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (action_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (action_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (action), "delete_event", GTK_SIGNAL_FUNC (action_delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (action_cancel_button), "clicked", GTK_SIGNAL_FUNC (action_cancel_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (action_command_browse_button), "clicked", GTK_SIGNAL_FUNC (action_browse_command_cb), NULL);

  init_choice_str ();

  return action;
}

void
private_action_update_menuentry_cb (GtkWidget * widget, gpointer data)
{
  private_action_menu_selected = (gint) ((long) data);
}

GtkWidget *
action_addto_choice (char *name)
{
  static gint position = 0;
  GtkWidget *menuitem = NULL;

  if (name && strlen (name))
  {
    menuitem = gtk_menu_item_new_with_label (name);
    gtk_menu_append (GTK_MENU (action_icon_optionmenu_menu), menuitem);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (private_action_update_menuentry_cb), (gpointer) ((long) position++));
    gtk_widget_show (menuitem);
    gtk_widget_realize (menuitem);
  }
  return menuitem;
}

gint
action_get_choice_selected (void)
{
  return private_action_menu_selected;
}

void
action_set_choice_selected (gint index)
{
  gtk_option_menu_set_history (GTK_OPTION_MENU (action_icon_optionmenu), index);
  private_action_menu_selected = index;
}

void
open_action (GtkWidget * action, gint nbr)
{
  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
    return;
    
  if (!GTK_IS_WIDGET (action))
    action = create_action ();

  gtk_entry_set_text (GTK_ENTRY (action_command_entry), get_command (nbr));
  gtk_entry_set_position (GTK_ENTRY (action_command_entry), 0);
  if (nbr < NBSELECTS)
  {
    set_choice_value (nbr);
    gtk_widget_show (action_icon_label);
    gtk_widget_show (action_icon_optionmenu);
    gtk_widget_show (action_icon_browse_button);
  }
  else
  {
    gtk_widget_hide (action_icon_label);
    gtk_widget_hide (action_icon_optionmenu);
    gtk_widget_hide (action_icon_browse_button);
  }
  action_signal_id1 = gtk_signal_connect (GTK_OBJECT (action_ok_button), "clicked", GTK_SIGNAL_FUNC (action_ok_cb), (gpointer) ((long) nbr));
  action_signal_id2 = gtk_signal_connect (GTK_OBJECT (action_icon_browse_button), "clicked", GTK_SIGNAL_FUNC (action_browse_icon_cb), (gpointer) ((long) nbr));
  gtk_window_set_modal (GTK_WINDOW (action), TRUE);
  gnome_sticky (action->window);
  gtk_widget_show (action);
  gtk_main ();
}
