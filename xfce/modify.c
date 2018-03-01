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

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "configfile.h"
#include "fileutil.h"
#include "modify.h"
#include "popup.h"
#include "xpmext.h"
#include "modify_cb.h"
#include "gnome_protocol.h"
#include "empty.h"
#include "defaulticon.h"

GtkWidget *
create_modify ()
{
  GtkWidget *modify;
  GtkWidget *modify_mainframe;
  GtkWidget *modify_vbox;
  GtkWidget *modify_hbox;
  GtkWidget *modify_leftframe;
  GtkWidget *modify_vbox2;
  GtkWidget *modify_preview_label;
  GtkWidget *modify_rightframe;
  GtkWidget *modify_table;
  GtkWidget *modify_command_label;
  GtkWidget *modify_icon_label;
  GtkWidget *modify_pos_frame;
  GtkWidget *modify_displayed_label;
  GtkWidget *modify_position_label;
  GtkWidget *modify_command_browse_button;
  GtkWidget *modify_icon_browse_button;
  GtkWidget *modify_packer;
  GtkWidget *modify_packer2;
  GtkWidget *modify_bottomframe;
  GtkWidget *modify_hbuttonbox;
  GtkAccelGroup *accel_group;

  modify = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_widget_set_name (modify, "modify");
  gtk_object_set_data (GTK_OBJECT (modify), "modify", modify);
  gtk_window_set_title (GTK_WINDOW (modify), _("Modify Item ..."));
  gtk_window_position (GTK_WINDOW (modify), GTK_WIN_POS_CENTER);
  gtk_widget_realize (modify);
  /*
     gnome_layer (modify->window, MAX_LAYERS);
   */

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (modify), accel_group);

  modify_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_mainframe, "modify_mainframe");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_mainframe", modify_mainframe);
  gtk_widget_show (modify_mainframe);
  gtk_container_add (GTK_CONTAINER (modify), modify_mainframe);

#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (modify_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (modify_mainframe), GTK_SHADOW_OUT);
#endif

  modify_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (modify_vbox, "modify_vbox");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_vbox", modify_vbox);
  gtk_widget_show (modify_vbox);
  gtk_container_add (GTK_CONTAINER (modify_mainframe), modify_vbox);

  modify_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (modify_hbox, "modify_hbox");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_hbox", modify_hbox);
  gtk_widget_show (modify_hbox);
  gtk_box_pack_start (GTK_BOX (modify_vbox), modify_hbox, TRUE, TRUE, 0);

  modify_leftframe = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_leftframe, "modify_leftframe");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_leftframe", modify_leftframe);
  gtk_widget_show (modify_leftframe);
  gtk_box_pack_start (GTK_BOX (modify_hbox), modify_leftframe, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (modify_leftframe), 5);

  modify_vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (modify_vbox2, "modify_vbox2");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_vbox2", modify_vbox2);
  gtk_widget_show (modify_vbox2);
  gtk_container_add (GTK_CONTAINER (modify_leftframe), modify_vbox2);

  modify_preview_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_preview_frame, "popup_addicon_frame");
  gtk_object_set_data (GTK_OBJECT (modify_preview_frame), "modify_preview_frame", modify_preview_frame);
  gtk_frame_set_shadow_type (GTK_FRAME (modify_preview_frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (modify_preview_frame), 5);
  gtk_widget_show (modify_preview_frame);
  gtk_box_pack_start (GTK_BOX (modify_vbox2), modify_preview_frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (modify_preview_frame, 80, 80);

  modify_preview_pixmap = MyCreateFromPixmapData (modify_preview_frame, empty);
  if (modify_preview_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (modify_preview_pixmap, "modify_preview_pixmap");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_preview_pixmap", modify_preview_pixmap);
  gtk_widget_show (modify_preview_pixmap);
  gtk_container_add (GTK_CONTAINER (modify_preview_frame), modify_preview_pixmap);

  modify_preview_label = gtk_label_new (_("Preview Icon"));
  gtk_widget_set_name (modify_preview_label, "modify_preview_label");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_preview_label", modify_preview_label);
  gtk_widget_show (modify_preview_label);
  gtk_box_pack_start (GTK_BOX (modify_vbox2), modify_preview_label, FALSE, FALSE, 0);
  gtk_widget_set_usize (modify_preview_label, 100, -2);
  gtk_misc_set_alignment (GTK_MISC (modify_preview_label), 0.5, 1);
  gtk_misc_set_padding (GTK_MISC (modify_preview_label), 10, 10);

  modify_rightframe = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_rightframe, "modify_rightframe");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_rightframe", modify_rightframe);
  gtk_widget_show (modify_rightframe);
  gtk_box_pack_start (GTK_BOX (modify_hbox), modify_rightframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (modify_rightframe), 5);

  modify_table = gtk_table_new (3, 4, FALSE);
  gtk_widget_set_name (modify_table, "modify_table");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_table", modify_table);
  gtk_widget_show (modify_table);
  gtk_container_add (GTK_CONTAINER (modify_rightframe), modify_table);

  modify_command_label = gtk_label_new (_("Command Line :"));
  gtk_widget_set_name (modify_command_label, "modify_command_label");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_command_label", modify_command_label);
  gtk_widget_show (modify_command_label);
  gtk_table_attach (GTK_TABLE (modify_table), modify_command_label, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (modify_command_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (modify_command_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (modify_command_label), 10, 0);

  modify_icon_label = gtk_label_new (_("Icon File :"));
  gtk_widget_set_name (modify_icon_label, "modify_icon_label");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_icon_label", modify_icon_label);
  gtk_widget_show (modify_icon_label);
  gtk_table_attach (GTK_TABLE (modify_table), modify_icon_label, 0, 1, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (modify_icon_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (modify_icon_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (modify_icon_label), 10, 0);

  modify_displayed_label = gtk_label_new (_("Label :"));
  gtk_widget_set_name (modify_displayed_label, "modify_displayed_label");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_displayed_label", modify_displayed_label);
  gtk_widget_show (modify_displayed_label);
  gtk_table_attach (GTK_TABLE (modify_table), modify_displayed_label, 0, 1, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (modify_displayed_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (modify_displayed_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (modify_displayed_label), 10, 0);

  modify_position_label = gtk_label_new (_("Position :"));
  gtk_widget_set_name (modify_position_label, "modify_position_label");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_position_label", modify_position_label);
  gtk_widget_show (modify_position_label);
  gtk_table_attach (GTK_TABLE (modify_table), modify_position_label, 0, 1, 3, 4, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (modify_position_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (modify_position_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (modify_position_label), 10, 0);

  modify_command_entry = gtk_entry_new ();
  gtk_widget_set_name (modify_command_entry, "modify_command_entry");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_command_entry", modify_command_entry);
  gtk_widget_show (modify_command_entry);
  gtk_table_attach (GTK_TABLE (modify_table), modify_command_entry, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  modify_icon_entry = gtk_entry_new ();
  gtk_widget_set_name (modify_icon_entry, "modify_icon_entry");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_icon_entry", modify_icon_entry);
  gtk_widget_show (modify_icon_entry);
  gtk_table_attach (GTK_TABLE (modify_table), modify_icon_entry, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  modify_displayed_entry = gtk_entry_new ();
  gtk_widget_set_name (modify_displayed_entry, "modify_displayed_entry");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_displayed_entry", modify_displayed_entry);
  gtk_widget_show (modify_displayed_entry);
  gtk_table_attach (GTK_TABLE (modify_table), modify_displayed_entry, 1, 2, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  modify_pos_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_pos_frame, "modify_pos_frame");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_pos_frame", modify_pos_frame);
  gtk_frame_set_shadow_type (GTK_FRAME (modify_pos_frame), GTK_SHADOW_NONE);
  gtk_widget_show (modify_pos_frame);
  gtk_container_border_width (GTK_CONTAINER (modify_pos_frame), 5);
  gtk_table_attach (GTK_TABLE (modify_table), modify_pos_frame, 1, 2, 3, 4, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  modify_pos_adj = gtk_adjustment_new (0, 1, 13, 1, 1, 1);
  modify_pos_hscale = gtk_hscale_new (GTK_ADJUSTMENT (modify_pos_adj));
  gtk_scale_set_digits (GTK_SCALE (modify_pos_hscale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (modify_pos_hscale), GTK_POS_LEFT);

  gtk_widget_set_name (modify_pos_hscale, "modify_pos_hscale");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_pos_hscale", modify_pos_hscale);
  gtk_widget_show (modify_pos_hscale);
  gtk_container_add (GTK_CONTAINER (modify_pos_frame), modify_pos_hscale);

  modify_command_browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (modify_command_browse_button, "modify_command_browse_button");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_command_browse_button", modify_command_browse_button);
  gtk_widget_show (modify_command_browse_button);
  gtk_table_attach (GTK_TABLE (modify_table), modify_command_browse_button, 2, 3, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (modify_command_browse_button, 90, -2);
  gtk_container_border_width (GTK_CONTAINER (modify_command_browse_button), 5);

  modify_icon_browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (modify_icon_browse_button, "modify_icon_browse_button");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_icon_browse_button", modify_icon_browse_button);
  gtk_widget_show (modify_icon_browse_button);
  gtk_table_attach (GTK_TABLE (modify_table), modify_icon_browse_button, 2, 3, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (modify_icon_browse_button, 90, -2);
  gtk_container_border_width (GTK_CONTAINER (modify_icon_browse_button), 5);

  modify_packer = gtk_packer_new ();
  gtk_widget_set_name (modify_packer, "modify_packer");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_packer", modify_packer);
  gtk_table_attach (GTK_TABLE (modify_table), modify_packer, 2, 3, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (modify_packer, -2, 40);
  gtk_widget_set_sensitive (modify_packer, FALSE);

  modify_packer2 = gtk_packer_new ();
  gtk_widget_set_name (modify_packer2, "modify_packer2");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_packer2", modify_packer2);
  gtk_table_attach (GTK_TABLE (modify_table), modify_packer2, 2, 3, 3, 4, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (modify_packer2, -2, 40);
  gtk_widget_set_sensitive (modify_packer2, FALSE);

  modify_bottomframe = gtk_frame_new (NULL);
  gtk_widget_set_name (modify_bottomframe, "modify_bottomframe");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_bottomframe", modify_bottomframe);
  gtk_widget_show (modify_bottomframe);
  gtk_box_pack_start (GTK_BOX (modify_vbox), modify_bottomframe, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (modify_bottomframe), 5);

  modify_hbuttonbox = gtk_hbutton_box_new ();
  gtk_widget_set_name (modify_hbuttonbox, "modify_hbuttonbox");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_hbuttonbox", modify_hbuttonbox);
  gtk_widget_show (modify_hbuttonbox);
  gtk_container_add (GTK_CONTAINER (modify_bottomframe), modify_hbuttonbox);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (modify_hbuttonbox), GTK_BUTTONBOX_SPREAD);

  modify_ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (modify_ok_button, "modify_ok_button");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_ok_button", modify_ok_button);
  gtk_widget_show (modify_ok_button);
  gtk_container_add (GTK_CONTAINER (modify_hbuttonbox), modify_ok_button);
  gtk_container_border_width (GTK_CONTAINER (modify_ok_button), 5);
  GTK_WIDGET_SET_FLAGS (modify_ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (modify_ok_button);

  modify_remove_button = gtk_button_new_with_label (_("Remove"));
  gtk_widget_set_name (modify_remove_button, "modify_remove_button");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_remove_button", modify_remove_button);
  gtk_widget_show (modify_remove_button);
  gtk_container_add (GTK_CONTAINER (modify_hbuttonbox), modify_remove_button);
  gtk_container_border_width (GTK_CONTAINER (modify_remove_button), 5);
  GTK_WIDGET_SET_FLAGS (modify_remove_button, GTK_CAN_DEFAULT);

  modify_cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (modify_cancel_button, "modify_cancel_button");
  gtk_object_set_data (GTK_OBJECT (modify), "modify_cancel_button", modify_cancel_button);
  gtk_widget_show (modify_cancel_button);
  gtk_container_add (GTK_CONTAINER (modify_hbuttonbox), modify_cancel_button);
  gtk_container_border_width (GTK_CONTAINER (modify_cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (modify_cancel_button, GTK_CAN_DEFAULT);

  gtk_widget_add_accelerator (modify_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (modify_ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (modify_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (modify_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (modify_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (modify_remove_button, "clicked", accel_group, GDK_Delete, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (modify), "delete_event", GTK_SIGNAL_FUNC (modify_delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (modify_cancel_button), "clicked", GTK_SIGNAL_FUNC (modify_cancel_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (modify_command_browse_button), "clicked", GTK_SIGNAL_FUNC (modify_browse_command_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (modify_icon_browse_button), "clicked", GTK_SIGNAL_FUNC (modify_browse_icon_cb), NULL);


  return modify;
}

void
open_modify (GtkWidget * modify, gint menu, gint item)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  char *pixfile;

  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
    return;
    
  if (item < 0)
  {
    gtk_window_set_title (GTK_WINDOW (modify), _("Add Item ..."));
    gtk_entry_set_text (GTK_ENTRY (modify_command_entry), "");
    gtk_entry_set_text (GTK_ENTRY (modify_icon_entry), "");
    gtk_entry_set_text (GTK_ENTRY (modify_displayed_entry), "");
    my_set_adjustment_bounds (GTK_ADJUSTMENT (modify_pos_adj), 1.0, (gfloat) (get_popup_menu_entries(menu) + 1));
    gtk_adjustment_set_value (GTK_ADJUSTMENT (modify_pos_adj), 1);
    gtk_range_default_hslider_update(GTK_RANGE (modify_pos_hscale));
    gtk_scale_draw_value (GTK_SCALE (modify_pos_hscale));
    pixmap = MyCreateGdkPixmapFromData (empty, modify_preview_frame, &mask, TRUE);
    gtk_pixmap_set (GTK_PIXMAP (modify_preview_pixmap), pixmap, mask);
    gtk_widget_hide (modify_remove_button);
    signal_id1 = gtk_signal_connect (GTK_OBJECT (modify_ok_button), "clicked", GTK_SIGNAL_FUNC (modify_add_cb), (gpointer) ((long) (menu * NBMAXITEMS)));

    signal_id2 = gtk_signal_connect (GTK_OBJECT (modify_remove_button), "clicked", GTK_SIGNAL_FUNC (modify_cancel_cb), (gpointer) ((long) (menu * NBMAXITEMS)));
  }
  else
  {
    gtk_window_set_title (GTK_WINDOW (modify), _("Modify Item ..."));
    gtk_entry_set_text (GTK_ENTRY (modify_command_entry), get_popup_entry_command (menu, item));
    gtk_entry_set_text (GTK_ENTRY (modify_displayed_entry), get_popup_entry_label (menu, item));
    my_set_adjustment_bounds (GTK_ADJUSTMENT (modify_pos_adj), 1.0, (gfloat) (get_popup_menu_entries(menu)));
    gtk_adjustment_set_value (GTK_ADJUSTMENT (modify_pos_adj), item + 1);
    gtk_range_default_hslider_update(GTK_RANGE (modify_pos_hscale));
    gtk_scale_draw_value (GTK_SCALE (modify_pos_hscale));
    pixfile = get_popup_entry_icon (menu, item);
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
    gtk_widget_show (modify_remove_button);
    signal_id1 = gtk_signal_connect (GTK_OBJECT (modify_ok_button), "clicked", GTK_SIGNAL_FUNC (modify_change_cb), (gpointer) ((long) (menu * NBMAXITEMS + item)));

    signal_id2 = gtk_signal_connect (GTK_OBJECT (modify_remove_button), "clicked", GTK_SIGNAL_FUNC (modify_remove_cb), (gpointer) ((long) (menu * NBMAXITEMS + item)));
  }
  gtk_entry_set_position (GTK_ENTRY (modify_command_entry), 0);
  gtk_entry_set_position (GTK_ENTRY (modify_icon_entry), 0);
  gtk_entry_set_position (GTK_ENTRY (modify_displayed_entry), 0);
  gtk_window_set_modal (GTK_WINDOW (modify), TRUE);
  gnome_sticky (modify->window);
  gtk_widget_show (modify);
  gtk_main ();
}
