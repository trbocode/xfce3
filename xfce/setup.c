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
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "xfcolor.h"
#include "setup.h"
#include "setup_cb.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "xpmext.h"
#include "configfile.h"
#include "xfwm.h"
#include "popup.h"
#include "gnome_protocol.h"
#include "constant.h"

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GList *list_engines = NULL;
static GList *xfwm_engines = NULL;

void
apply_pal_colortable (XFCE_palette * pal)
{
  if ((pal->fnt) && strlen (pal->fnt))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_xfce_entry), pal->fnt);
  else
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_xfce_entry), DEFAULTFONT);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_palette_engine_combo)->entry), temp_pal->engine);
  gtk_entry_set_position (GTK_ENTRY (setup_options.setup_font_xfce_entry), 0);
  apply_temp_gtkrc_file (pal, setup);
}

GtkWidget *
create_setup_colortable (GtkWidget * toplevel, XFCE_palette * pal)
{
  gint i;

  GtkWidget *setup_table;
  char temp_name[16];

  setup_table = gtk_table_new (2, 4, FALSE);
  gtk_widget_set_name (setup_table, "setup_table");
  gtk_object_set_data (GTK_OBJECT (toplevel), "setup_table", setup_table);
  gtk_widget_show (setup_table);
  gtk_container_border_width (GTK_CONTAINER (setup_table), 5);


  for (i = 0; i < NB_XFCE_COLORS; i++)
  {
    snprintf (temp_name, 15, "temp_color%i", i);
    color_buttons.setup_color_button_frame[i] = gtk_frame_new (NULL);
    gtk_widget_set_name (color_buttons.setup_color_button_frame[i], "setup_color_button_frame");
    gtk_object_set_data (GTK_OBJECT (toplevel), "setup_color_button_frame", color_buttons.setup_color_button_frame[i]);
    gtk_container_border_width (GTK_CONTAINER (color_buttons.setup_color_button_frame[i]), 5);
    gtk_frame_set_shadow_type (GTK_FRAME (color_buttons.setup_color_button_frame[i]), GTK_SHADOW_IN);
    /* gtk_widget_set_style (color_buttons.setup_color_button_frame[i],
       pal->cm[i]); */
    gtk_widget_show (color_buttons.setup_color_button_frame[i]);
    gtk_table_attach (GTK_TABLE (setup_table), color_buttons.setup_color_button_frame[i], (i % 4), (i % 4) + 1, (i / 4), (i / 4) + 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

    color_buttons.setup_color_button[i] = gtk_button_new ();
    gtk_widget_set_name (color_buttons.setup_color_button[i], temp_name);
    gtk_object_set_data (GTK_OBJECT (toplevel), temp_name, color_buttons.setup_color_button[i]);
    /* gtk_widget_set_style (color_buttons.setup_color_button[i], pal->cm[i]); */
    gtk_widget_set_usize (color_buttons.setup_color_button[i], 0, 16);
    gtk_widget_show (color_buttons.setup_color_button[i]);
    gtk_container_add (GTK_CONTAINER (color_buttons.setup_color_button_frame[i]), color_buttons.setup_color_button[i]);

    gtk_signal_connect (GTK_OBJECT (color_buttons.setup_color_button[i]), "clicked", GTK_SIGNAL_FUNC (color_button_cb), (gpointer) ((long) i));

  }
  return setup_table;
}

GList *
find_gtk_engines (void)
{
  GList *list = NULL;
  FILE *pdata;
  char buffer[MAXSTRLEN];

  /* always add these */
  list = g_list_append (list, "");
  list = g_list_append (list, "xfce");

  /* Try to find the location of gtk engines */
  if ((pdata = popen ("gtk-config --prefix", "r")) != NULL && fgets (buffer, sizeof (buffer), pdata) != NULL)
  {
    char *path;
    DIR *gtk_engines_dir;
    struct dirent *dir_entry;
    struct stat stat_buffer;

    pclose (pdata);

    /* strip final '\n' */
    buffer[strlen (buffer) - 1] = '\0';
    path = g_strconcat (buffer, "/lib/gtk/themes/engines", NULL);

    if ((gtk_engines_dir = opendir (path)) != NULL)
    {
      list = g_list_append (list, "gtk");
      while ((dir_entry = (struct dirent *) readdir (gtk_engines_dir)) != NULL)
      {
	char *name, *start, *end;

	if ((strcmp (dir_entry->d_name, ".") == 0) || (strcmp (dir_entry->d_name, "..") == 0))
	  continue;

	if (lstat (dir_entry->d_name, &stat_buffer) == 0)
	  continue;

	if (S_ISDIR (stat_buffer.st_mode))
	  continue;

	if ((end = strstr (dir_entry->d_name, ".so")) != NULL && strncmp (dir_entry->d_name, "lib", 3) == 0)
	{
	  start = dir_entry->d_name + 3;
	  name = g_strndup (start, end - start);
	  if (g_list_find_custom (list, name, (GCompareFunc) strcmp) == NULL)
	    list = g_list_append (list, name);
	  else
	    g_free (name);
	}

      }

      closedir (gtk_engines_dir);
      g_free (path);
      return list;
    }
  }

  /* something went wrong:
   * Initialization of well known engines */
  list = g_list_append (list, "gtk");
  list = g_list_append (list, "xfce");
  list = g_list_append (list, "notif");
  list = g_list_append (list, "redmond95");
  return list;
}

void
free_internals_setup (void)
{
  if (list_engines)
  {
    g_list_free (list_engines);
  }
  if (xfwm_engines)
  {
    g_list_free (xfwm_engines);
  }
  freepal (temp_pal);
}


GtkWidget *
create_setup (XFCE_palette * pal)
{
  GtkWidget *setup;
  GtkWidget *setup_mainframe;
  GtkWidget *setup_borderframe;
  GtkWidget *setup_vbox1;
  GtkWidget *setup_notebook;
  GtkWidget *setup_vbox2;
  GtkWidget *setup_upframe;
  GtkWidget *setup_loadframe;
  GtkWidget *setup_hbuttonbox;
  GtkWidget *setup_engine_frame;
  GtkWidget *setup_xcolors_frame;
  GtkWidget *setup_engine_hbox;
  GtkWidget *setup_engine_label;
  GtkWidget *setup_vbox3;
  GtkWidget *setup_tooltipsdelay_frame;
  GtkWidget *setup_tooltipsdelay_hbox;
  GtkWidget *setup_tooltipsdelay_label;
  GtkWidget *setup_layer_frame;
  GtkWidget *setup_layer_hbox;
  GtkWidget *setup_layerbot_label;
  GtkWidget *setup_layer_hscale;
  GtkWidget *setup_layertop_label;
  GtkWidget *setup_clock_frame;
  GtkWidget *setup_clock_vbox;
  GtkWidget *setup_numscreens_frame;
  GtkWidget *setup_numscreens_vbox;
  GtkWidget *setup_numscreens_hbox;
  GtkWidget *setup_numscreens_label;
  GtkWidget *setup_switchdeskfactor_hbox;
  GtkWidget *setup_switchdeskfactor_label;
  GtkWidget *setup_numpopups_frame;
  GtkWidget *setup_numpopups_hbox;
  GtkWidget *setup_numpopups_label;
  GtkWidget *setup_panelicons_frame;
  GtkWidget *setup_panelicons_hbox;
  GSList *setup_panelicons_hbox_group = NULL;
  GtkWidget *setup_popupicons_frame;
  GtkWidget *setup_popupicons_hbox;
  GSList *setup_popupicons_hbox_group = NULL;
  GtkWidget *setup_font_xfce_frame;
  GtkWidget *setup_font_xfce_hbox;
  GtkWidget *setup_vbox4;
  GtkWidget *setup_iconpos_frame;
  GtkWidget *setup_iconpos_hbox;
  GtkWidget *setup_iconpos_label;
  GtkWidget *setup_iconpos_table;
  GSList *setup_iconpos_table_group = NULL;
  GtkWidget *setup_packer1;
  GtkWidget *setup_packer2;
  GtkWidget *setup_packer3;
  GtkWidget *setup_packer4;
  GtkWidget *setup_packer5;
  GtkWidget *setup_font_frame;
  GtkWidget *setup_font_table;
  GtkWidget *setup_font_title_label;
  GtkWidget *setup_font_icon_label;
  GtkWidget *setup_font_menu_label;
  GtkWidget *setup_xfwm_engine_frame;
  GtkWidget *setup_xfwm_engine_hbox;
  GtkWidget *setup_xfwm_engine_label;
  GtkWidget *setup_snapsize_frame;
  GtkWidget *setup_snapsize_hbox;
  GtkWidget *setup_snapsize_label;
  GtkWidget *setup_vbox5;
  GtkWidget *setup_notebook_palette_label;
  GtkWidget *setup_notebook_xfce_label;
  GtkWidget *setup_notebook_windows_label;
  GtkWidget *setup_notebook_startup_label;
  GtkWidget *setup_bottomframe;
  GtkWidget *setup_hbuttonbox2;
  GtkWidget *scrolled_window1;
  GtkWidget *scrolled_window2;
  GtkWidget *scrolled_window3;
  GtkWidget *scrolled_window4;

  GtkAccelGroup *accel_group;

  temp_pal = newpal ();
  copyvaluepal (temp_pal, pal);

  if (!list_engines)
  {
    list_engines = find_gtk_engines ();
  }

  setup = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_widget_set_name (setup, "setup");
  gtk_object_set_data (GTK_OBJECT (setup), "setup", setup);
  gtk_window_set_title (GTK_WINDOW (setup), _("Setup"));
  gtk_window_position (GTK_WINDOW (setup), GTK_WIN_POS_CENTER);
  gtk_widget_set_usize (setup, -2, 380);
  gtk_window_set_policy (GTK_WINDOW (setup), TRUE, TRUE, FALSE);
  gtk_widget_realize (setup);
  /*
     gnome_layer (setup->window, MAX_LAYERS);
   */

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (setup), accel_group);

  setup_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_mainframe, "setup_mainframe");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_mainframe", setup_mainframe);
  gtk_widget_show (setup_mainframe);
  gtk_container_add (GTK_CONTAINER (setup), setup_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (setup_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (setup_mainframe), GTK_SHADOW_OUT);
#endif

  setup_borderframe = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_borderframe, "setup_borderframe");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_borderframe", setup_borderframe);
  gtk_widget_show (setup_borderframe);
  gtk_container_add (GTK_CONTAINER (setup_mainframe), setup_borderframe);
  gtk_container_border_width (GTK_CONTAINER (setup_borderframe), 10);

  setup_vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_vbox1, "setup_vbox1");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_vbox1", setup_vbox1);
  gtk_widget_show (setup_vbox1);
  gtk_container_add (GTK_CONTAINER (setup_borderframe), setup_vbox1);

  setup_notebook = gtk_notebook_new ();
  gtk_widget_set_name (setup_notebook, "setup_notebook");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_notebook", setup_notebook);
  gtk_widget_show (setup_notebook);
  gtk_box_pack_start (GTK_BOX (setup_vbox1), setup_notebook, TRUE, TRUE, 0);

  scrolled_window1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window1), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window1), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled_window1);
  gtk_container_add (GTK_CONTAINER (setup_notebook), scrolled_window1);

  setup_vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_vbox2, "setup_vbox2");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_vbox2", setup_vbox2);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window1), setup_vbox2);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (setup_vbox2), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window1)));
  gtk_widget_show (setup_vbox2);
  gtk_container_border_width (GTK_CONTAINER (setup_vbox2), 5);

  setup_upframe = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_upframe, "setup_upframe");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_upframe", setup_upframe);
  gtk_box_pack_start (GTK_BOX (setup_vbox2), setup_upframe, FALSE, TRUE, 0);
  gtk_widget_show (setup_upframe);
  gtk_container_border_width (GTK_CONTAINER (setup_upframe), 5);

  gtk_container_add (GTK_CONTAINER (setup_upframe), create_setup_colortable (setup, temp_pal));

  setup_engine_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_engine_frame, "setup_engine_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_engine_frame", setup_engine_frame);
  gtk_widget_show (setup_engine_frame);
  gtk_container_border_width (GTK_CONTAINER (setup_engine_frame), 5);
  gtk_box_pack_start (GTK_BOX (setup_vbox2), setup_engine_frame, FALSE, TRUE, 0);

  setup_engine_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_engine_hbox, "setup_engine_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_engine_hbox", setup_engine_hbox);
  gtk_widget_show (setup_engine_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_engine_hbox), 5);
  gtk_container_add (GTK_CONTAINER (setup_engine_frame), setup_engine_hbox);

  setup_engine_label = gtk_label_new (_("GTK theme engine : "));
  gtk_widget_set_name (setup_engine_label, "setup_engine_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_engine_label", setup_engine_label);
  gtk_widget_show (setup_engine_label);
  gtk_box_pack_start (GTK_BOX (setup_engine_hbox), setup_engine_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_engine_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_palette_engine_combo = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (setup_options.setup_palette_engine_combo), list_engines);
  gtk_widget_set_name (setup_options.setup_palette_engine_combo, "setup_palette_engine_combo");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_palette_engine_combo", setup_options.setup_palette_engine_combo);
  gtk_widget_show (setup_options.setup_palette_engine_combo);
  gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO (setup_options.setup_palette_engine_combo)->entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (setup_engine_hbox), setup_options.setup_palette_engine_combo, FALSE, FALSE, 0);

  setup_xcolors_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_xcolors_frame, "setup_xcolors_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_xcolors_frame", setup_xcolors_frame);
  gtk_widget_show (setup_xcolors_frame);
  gtk_container_border_width (GTK_CONTAINER (setup_xcolors_frame), 5);
  gtk_box_pack_start (GTK_BOX (setup_vbox2), setup_xcolors_frame, FALSE, TRUE, 0);

  setup_options.setup_apply_xcolors_checkbutton = gtk_check_button_new_with_label (_("Apply colors to all applications"));
  gtk_widget_set_name (setup_options.setup_apply_xcolors_checkbutton, "setup_apply_xcolors_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_apply_xcolors_checkbutton", setup_options.setup_apply_xcolors_checkbutton);
  gtk_widget_show (setup_options.setup_apply_xcolors_checkbutton);
  gtk_container_add (GTK_CONTAINER (setup_xcolors_frame), setup_options.setup_apply_xcolors_checkbutton);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_apply_xcolors_checkbutton), 2);

  setup_loadframe = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_loadframe, "setup_loadframe");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_loadframe", setup_loadframe);
  gtk_widget_show (setup_loadframe);
  gtk_box_pack_start (GTK_BOX (setup_vbox2), setup_loadframe, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_loadframe), 5);

  setup_hbuttonbox = gtk_hbutton_box_new ();
  gtk_widget_set_name (setup_hbuttonbox, "setup_hbuttonbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_hbuttonbox", setup_hbuttonbox);
  gtk_widget_show (setup_hbuttonbox);
  gtk_container_add (GTK_CONTAINER (setup_loadframe), setup_hbuttonbox);

  setup_options.setup_palette_load_button = gtk_button_new_with_label (_("Load ..."));
  gtk_widget_set_name (setup_options.setup_palette_load_button, "setup_palette_load_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_palette_load_button", setup_options.setup_palette_load_button);
  gtk_widget_show (setup_options.setup_palette_load_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox), setup_options.setup_palette_load_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_palette_load_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_palette_load_button, GTK_CAN_DEFAULT);

  setup_options.setup_palette_save_button = gtk_button_new_with_label (_("Save ..."));
  gtk_widget_set_name (setup_options.setup_palette_save_button, "setup_palette_save_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_palette_save_button", setup_options.setup_palette_save_button);
  gtk_widget_show (setup_options.setup_palette_save_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox), setup_options.setup_palette_save_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_palette_save_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_palette_save_button, GTK_CAN_DEFAULT);

  setup_options.setup_palette_default_button = gtk_button_new_with_label (_("Default"));
  gtk_widget_set_name (setup_options.setup_palette_default_button, "setup_palette_default_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_palette_default_button", setup_options.setup_palette_default_button);
  gtk_widget_show (setup_options.setup_palette_default_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox), setup_options.setup_palette_default_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_palette_default_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_palette_default_button, GTK_CAN_DEFAULT);

  scrolled_window2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window2), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window2), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled_window2);
  gtk_container_add (GTK_CONTAINER (setup_notebook), scrolled_window2);

  setup_vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_vbox3, "setup_vbox3");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_vbox3", setup_vbox3);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window2), setup_vbox3);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (setup_vbox3), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window2)));
  gtk_widget_show (setup_vbox3);
  gtk_container_border_width (GTK_CONTAINER (setup_vbox3), 5);

  setup_options.setup_repaint_checkbutton = gtk_check_button_new_with_label (_("Repaint root window of workspace"));
  gtk_widget_set_name (setup_options.setup_repaint_checkbutton, "setup_repaint_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_repaint_checkbutton", setup_options.setup_repaint_checkbutton);
  gtk_widget_show (setup_options.setup_repaint_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_options.setup_repaint_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_repaint_checkbutton), 2);

  setup_options.setup_gradient_checkbutton = gtk_check_button_new_with_label (_("Use gradient color as wallpaper"));
  gtk_widget_set_name (setup_options.setup_gradient_checkbutton, "setup_gradient_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_gradient_checkbutton", setup_options.setup_gradient_checkbutton);
  gtk_widget_show (setup_options.setup_gradient_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_options.setup_gradient_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_gradient_checkbutton), 2);

  setup_options.setup_tearoff_checkbutton = gtk_check_button_new_with_label (_("Use tear-off menus"));
  gtk_widget_set_name (setup_options.setup_tearoff_checkbutton, "setup_tearoff_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_tearoff_checkbutton", setup_options.setup_tearoff_checkbutton);
  gtk_widget_show (setup_options.setup_tearoff_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_options.setup_tearoff_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_tearoff_checkbutton), 2);

  setup_options.setup_show_diagnostic_checkbutton = gtk_check_button_new_with_label (_("Show subprocess diagnostic dialog"));
  gtk_widget_set_name (setup_options.setup_show_diagnostic_checkbutton, "setup_show_diagnostic_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_show_diagnostic_checkbutton", setup_options.setup_show_diagnostic_checkbutton);
  gtk_widget_show (setup_options.setup_show_diagnostic_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_options.setup_show_diagnostic_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_show_diagnostic_checkbutton), 2);

  setup_layer_frame = gtk_frame_new (_("Panel layer"));
  gtk_widget_set_name (setup_layer_frame, "setup_layer_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_layer_frame", setup_layer_frame);
  gtk_widget_show (setup_layer_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_layer_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_layer_frame), 5);

  setup_layer_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_layer_hbox, "setup_layer_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_layer_hbox", setup_layer_hbox);
  gtk_widget_show (setup_layer_hbox);
  gtk_container_add (GTK_CONTAINER (setup_layer_frame), setup_layer_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_layer_hbox), 5);

  setup_layerbot_label = gtk_label_new (_("Bottom"));
  gtk_widget_set_name (setup_layerbot_label, "setup_layerbot_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_layerbot_label", setup_layerbot_label);
  gtk_widget_show (setup_layerbot_label);
  gtk_box_pack_start (GTK_BOX (setup_layer_hbox), setup_layerbot_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_layerbot_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_panel_layer_adj = gtk_adjustment_new (0, 1, 13, 1, 1, 1);
  setup_layer_hscale = gtk_hscale_new (GTK_ADJUSTMENT (setup_options.setup_panel_layer_adj));
  gtk_scale_set_digits (GTK_SCALE (setup_layer_hscale), 0);
  /* gtk_scale_set_draw_value (GTK_SCALE (setup_layer_hscale), FALSE); */
  gtk_scale_set_value_pos (GTK_SCALE (setup_layer_hscale), GTK_POS_BOTTOM);

  gtk_widget_set_name (setup_layer_hscale, "setup_layer_hscale");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_layer_hscale", setup_layer_hscale);
  gtk_widget_show (setup_layer_hscale);
  gtk_box_pack_start (GTK_BOX (setup_layer_hbox), setup_layer_hscale, TRUE, TRUE, 5);

  setup_layertop_label = gtk_label_new (_("Top"));
  gtk_widget_set_name (setup_layertop_label, "setup_layertop_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_layertop_label", setup_layertop_label);
  gtk_widget_show (setup_layertop_label);
  gtk_box_pack_start (GTK_BOX (setup_layer_hbox), setup_layertop_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_layertop_label), GTK_JUSTIFY_LEFT);

  setup_clock_frame = gtk_frame_new (_("Clock"));
  gtk_widget_set_name (setup_clock_frame, "setup_clock_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_clock_frame", setup_clock_frame);
  gtk_widget_show (setup_clock_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_clock_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_clock_frame), 5);

  setup_clock_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_clock_vbox, "setup_clock_vbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_clock_vbox", setup_clock_vbox);
  gtk_widget_show (setup_clock_vbox);
  gtk_container_add (GTK_CONTAINER (setup_clock_frame), setup_clock_vbox);
  gtk_container_border_width (GTK_CONTAINER (setup_clock_vbox), 5);

  setup_options.setup_digital_clock_checkbutton = gtk_check_button_new_with_label (_("Use digital clock on panel"));
  gtk_widget_set_name (setup_options.setup_digital_clock_checkbutton, "setup_digital_clock_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_digital_clock_checkbutton", setup_options.setup_digital_clock_checkbutton);
  gtk_widget_show (setup_options.setup_digital_clock_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_clock_vbox), setup_options.setup_digital_clock_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_digital_clock_checkbutton), 2);

  setup_options.setup_hrs_mode_checkbutton = gtk_check_button_new_with_label (_("Use military time (24 hours)"));
  gtk_widget_set_name (setup_options.setup_hrs_mode_checkbutton, "setup_hrs_mode_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_hrs_mode_checkbutton", setup_options.setup_hrs_mode_checkbutton);
  gtk_widget_show (setup_options.setup_hrs_mode_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_clock_vbox), setup_options.setup_hrs_mode_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_hrs_mode_checkbutton), 2);

  setup_tooltipsdelay_frame = gtk_frame_new (_("Tooltips"));
  gtk_widget_set_name (setup_tooltipsdelay_frame, "setup_tooltipsdelay_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_tooltipsdelay_frame", setup_tooltipsdelay_frame);
  gtk_widget_show (setup_tooltipsdelay_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_tooltipsdelay_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_tooltipsdelay_frame), 5);

  setup_tooltipsdelay_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_tooltipsdelay_hbox, "setup_tooltipsdelay_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_tooltipsdelay_hbox", setup_tooltipsdelay_hbox);
  gtk_widget_show (setup_tooltipsdelay_hbox);
  gtk_container_add (GTK_CONTAINER (setup_tooltipsdelay_frame), setup_tooltipsdelay_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_tooltipsdelay_hbox), 5);

  setup_tooltipsdelay_label = gtk_label_new (_("Tooltips delay (ms.) : "));
  gtk_widget_set_name (setup_tooltipsdelay_label, "setup_tooltipsdelay_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_tooltipsdelay_label", setup_tooltipsdelay_label);
  gtk_widget_show (setup_tooltipsdelay_label);
  gtk_box_pack_start (GTK_BOX (setup_tooltipsdelay_hbox), setup_tooltipsdelay_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_tooltipsdelay_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_tooltipsdelay_spinbutton_adj = gtk_adjustment_new (250, 0, 50000, 1, 100, 1);
  setup_options.setup_tooltipsdelay_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (setup_options.setup_tooltipsdelay_spinbutton_adj), 5, 0);
  gtk_widget_set_name (setup_options.setup_tooltipsdelay_spinbutton, "setup_tooltipsdelay_spinbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_tooltipsdelay_spinbutton", setup_options.setup_tooltipsdelay_spinbutton);
  gtk_widget_set_usize (setup_options.setup_tooltipsdelay_spinbutton, 100, -2);
  gtk_widget_show (setup_options.setup_tooltipsdelay_spinbutton);
  gtk_box_pack_start (GTK_BOX (setup_tooltipsdelay_hbox), setup_options.setup_tooltipsdelay_spinbutton, FALSE, FALSE, 0);

  setup_numscreens_frame = gtk_frame_new (_("Virtual Screens"));
  gtk_widget_set_name (setup_numscreens_frame, "setup_numscreens_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numscreens_frame", setup_numscreens_frame);
  gtk_widget_show (setup_numscreens_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_numscreens_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_numscreens_frame), 5);

  setup_numscreens_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_numscreens_vbox, "setup_numscreens_vbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numscreens_vbox", setup_numscreens_vbox);
  gtk_widget_show (setup_numscreens_vbox);
  gtk_container_add (GTK_CONTAINER (setup_numscreens_frame), setup_numscreens_vbox);
  gtk_container_border_width (GTK_CONTAINER (setup_numscreens_vbox), 5);

  setup_numscreens_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_numscreens_hbox, "setup_numscreens_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numscreens_hbox", setup_numscreens_hbox);
  gtk_widget_show (setup_numscreens_hbox);
  gtk_container_add (GTK_CONTAINER (setup_numscreens_vbox), setup_numscreens_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_numscreens_hbox), 5);

  setup_numscreens_label = gtk_label_new (_("Number of virtual desktops : "));
  gtk_widget_set_name (setup_numscreens_label, "setup_numscreens_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numscreens_label", setup_numscreens_label);
  gtk_widget_show (setup_numscreens_label);
  gtk_box_pack_start (GTK_BOX (setup_numscreens_hbox), setup_numscreens_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_numscreens_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_numscreens_spinbutton_adj = gtk_adjustment_new (4, 0, 10, 2, 2, 1);
  setup_options.setup_numscreens_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (setup_options.setup_numscreens_spinbutton_adj), 1, 0);
  gtk_widget_set_name (setup_options.setup_numscreens_spinbutton, "setup_numscreens_spinbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numscreens_spinbutton", setup_options.setup_numscreens_spinbutton);
  gtk_widget_show (setup_options.setup_numscreens_spinbutton);
  gtk_box_pack_end (GTK_BOX (setup_numscreens_hbox), setup_options.setup_numscreens_spinbutton, FALSE, FALSE, 0);

  setup_switchdeskfactor_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_switchdeskfactor_hbox, "setup_switchdeskfactor_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_switchdeskfactor_hbox", setup_switchdeskfactor_hbox);
  gtk_widget_show (setup_switchdeskfactor_hbox);
  gtk_container_add (GTK_CONTAINER (setup_numscreens_vbox), setup_switchdeskfactor_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_switchdeskfactor_hbox), 5);

  setup_switchdeskfactor_label = gtk_label_new (_("switch desktop at screen border, fixed factor 4: "));
  gtk_widget_set_name (setup_switchdeskfactor_label, "setup_switchdeskfactor_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_switchdeskfactor_label", setup_switchdeskfactor_label);
  gtk_widget_show (setup_switchdeskfactor_label);
  gtk_box_pack_start (GTK_BOX (setup_switchdeskfactor_hbox), setup_switchdeskfactor_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_switchdeskfactor_label), GTK_JUSTIFY_RIGHT);

# ifdef HAVE_LIBXML2
  setup_options.setup_switchdeskfactor_spinbutton_adj = gtk_adjustment_new (4, 1, 99, 1, 4, 1);
# else
  setup_options.setup_switchdeskfactor_spinbutton_adj = gtk_adjustment_new (4, 4, 4, 1, 4, 1);
# endif
  setup_options.setup_switchdeskfactor_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (setup_options.setup_switchdeskfactor_spinbutton_adj), 1, 0);
  gtk_widget_set_name (setup_options.setup_switchdeskfactor_spinbutton, "setup_switchdeskfactor_spinbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_switchdeskfactor_spinbutton", setup_options.setup_switchdeskfactor_spinbutton);
  gtk_widget_show (setup_options.setup_switchdeskfactor_spinbutton);
  gtk_box_pack_end (GTK_BOX (setup_switchdeskfactor_hbox), setup_options.setup_switchdeskfactor_spinbutton, FALSE, FALSE, 0);

  setup_numpopups_frame = gtk_frame_new (_("Popups & Icons"));
  gtk_widget_set_name (setup_numscreens_frame, "setup_numpopups_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numpopups_frame", setup_numpopups_frame);
  gtk_widget_show (setup_numpopups_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_numpopups_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_numpopups_frame), 5);

  setup_numpopups_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_numpopups_hbox, "setup_numpopups_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numpopups_hbox", setup_numpopups_hbox);
  gtk_widget_show (setup_numpopups_hbox);
  gtk_container_add (GTK_CONTAINER (setup_numpopups_frame), setup_numpopups_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_numpopups_hbox), 5);

  setup_numpopups_label = gtk_label_new (_("Number of popup menus : "));
  gtk_widget_set_name (setup_numpopups_label, "setup_numpopups_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numpopups_label", setup_numpopups_label);
  gtk_widget_show (setup_numpopups_label);
  gtk_box_pack_start (GTK_BOX (setup_numpopups_hbox), setup_numpopups_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_numpopups_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_numpopups_spinbutton_adj = gtk_adjustment_new (4, 0, NBPOPUPS, 1, 1, 1);
  setup_options.setup_numpopups_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (setup_options.setup_numpopups_spinbutton_adj), 1, 0);
  gtk_widget_set_name (setup_options.setup_numpopups_spinbutton, "setup_numpopups_spinbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_numpopups_spinbutton", setup_options.setup_numpopups_spinbutton);
  gtk_widget_show (setup_options.setup_numpopups_spinbutton);
  gtk_box_pack_start (GTK_BOX (setup_numpopups_hbox), setup_options.setup_numpopups_spinbutton, FALSE, FALSE, 0);

  setup_panelicons_frame = gtk_frame_new (_("Size of icons on panel"));
  gtk_widget_set_name (setup_panelicons_frame, "setup_panelicons_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_panelicons_frame", setup_panelicons_frame);
  gtk_widget_show (setup_panelicons_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_panelicons_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_panelicons_frame), 5);

  setup_panelicons_hbox = gtk_hbox_new (TRUE, 0);
  gtk_widget_set_name (setup_panelicons_hbox, "setup_panelicons_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_panelicons_hbox", setup_panelicons_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_panelicons_hbox), 5);
  gtk_widget_show (setup_panelicons_hbox);
  gtk_container_add (GTK_CONTAINER (setup_panelicons_frame), setup_panelicons_hbox);

  setup_options.setup_panelicons_large = gtk_radio_button_new_with_label (setup_panelicons_hbox_group, _("Large"));
  setup_panelicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_panelicons_large));
  gtk_widget_set_name (setup_options.setup_panelicons_large, "setup_panelicons_large");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_panelicons_large", setup_options.setup_panelicons_large);
  gtk_widget_show (setup_options.setup_panelicons_large);
  gtk_box_pack_start (GTK_BOX (setup_panelicons_hbox), setup_options.setup_panelicons_large, TRUE, TRUE, 0);

  setup_options.setup_panelicons_medium = gtk_radio_button_new_with_label (setup_panelicons_hbox_group, _("Medium"));
  setup_panelicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_panelicons_medium));
  gtk_widget_set_name (setup_options.setup_panelicons_medium, "setup_panelicons_medium");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_panelicons_medium", setup_options.setup_panelicons_medium);
  gtk_widget_show (setup_options.setup_panelicons_medium);
  gtk_box_pack_start (GTK_BOX (setup_panelicons_hbox), setup_options.setup_panelicons_medium, TRUE, TRUE, 0);

  setup_options.setup_panelicons_small = gtk_radio_button_new_with_label (setup_panelicons_hbox_group, _("Small"));
  setup_panelicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_panelicons_small));
  gtk_widget_set_name (setup_options.setup_panelicons_small, "setup_panelicons_small");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_panelicons_small", setup_options.setup_panelicons_small);
  gtk_widget_show (setup_options.setup_panelicons_small);
  gtk_box_pack_start (GTK_BOX (setup_panelicons_hbox), setup_options.setup_panelicons_small, TRUE, TRUE, 0);

  setup_popupicons_frame = gtk_frame_new (_("Size of icons on pop-up menus"));
  gtk_widget_set_name (setup_popupicons_frame, "setup_popupicons_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_popupicons_frame", setup_popupicons_frame);
  gtk_widget_show (setup_popupicons_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_popupicons_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_popupicons_frame), 5);

  setup_popupicons_hbox = gtk_hbox_new (TRUE, 0);
  gtk_widget_set_name (setup_popupicons_hbox, "setup_popupicons_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_popupicons_hbox", setup_popupicons_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_popupicons_hbox), 5);
  gtk_widget_show (setup_popupicons_hbox);
  gtk_container_add (GTK_CONTAINER (setup_popupicons_frame), setup_popupicons_hbox);

  setup_options.setup_popupicons_large = gtk_radio_button_new_with_label (setup_popupicons_hbox_group, _("Large"));
  setup_popupicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_popupicons_large));
  gtk_widget_set_name (setup_options.setup_popupicons_large, "setup_popupicons_large");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_popupicons_large", setup_options.setup_popupicons_large);
  gtk_widget_show (setup_options.setup_popupicons_large);
  gtk_box_pack_start (GTK_BOX (setup_popupicons_hbox), setup_options.setup_popupicons_large, TRUE, TRUE, 0);

  setup_options.setup_popupicons_medium = gtk_radio_button_new_with_label (setup_popupicons_hbox_group, _("Medium"));
  setup_popupicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_popupicons_medium));
  gtk_widget_set_name (setup_options.setup_popupicons_medium, "setup_popupicons_medium");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_popupicons_medium", setup_options.setup_popupicons_medium);
  gtk_widget_show (setup_options.setup_popupicons_medium);
  gtk_box_pack_start (GTK_BOX (setup_popupicons_hbox), setup_options.setup_popupicons_medium, TRUE, TRUE, 0);

  setup_options.setup_popupicons_small = gtk_radio_button_new_with_label (setup_popupicons_hbox_group, _("Small"));
  setup_popupicons_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_popupicons_small));
  gtk_widget_set_name (setup_options.setup_popupicons_small, "setup_popupicons_small");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_popupicons_small", setup_options.setup_popupicons_small);
  gtk_widget_show (setup_options.setup_popupicons_small);
  gtk_box_pack_start (GTK_BOX (setup_popupicons_hbox), setup_options.setup_popupicons_small, TRUE, TRUE, 0);

  setup_font_xfce_frame = gtk_frame_new (_("Font style"));
  gtk_widget_set_name (setup_font_xfce_frame, "setup_font_xfce_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_xfce_frame", setup_font_xfce_frame);
  gtk_widget_show (setup_font_xfce_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox3), setup_font_xfce_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_font_xfce_frame), 5);

  setup_font_xfce_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_font_xfce_hbox, "setup_font_xfce_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_xfce_hbox", setup_font_xfce_hbox);
  gtk_widget_show (setup_font_xfce_hbox);
  gtk_container_add (GTK_CONTAINER (setup_font_xfce_frame), setup_font_xfce_hbox);

  setup_options.setup_font_xfce_entry = gtk_entry_new ();
  gtk_widget_set_name (setup_options.setup_font_xfce_entry, "setup_font_xfce_entry");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_xfce_entry", setup_options.setup_font_xfce_entry);
  /* gtk_widget_set_style(setup_options.setup_font_xfce_entry, pal->cm[4]); */
  gtk_entry_set_editable (GTK_ENTRY (setup_options.setup_font_xfce_entry), TRUE);
  gtk_widget_show (setup_options.setup_font_xfce_entry);
  gtk_box_pack_start (GTK_BOX (setup_font_xfce_hbox), setup_options.setup_font_xfce_entry, TRUE, TRUE, 5);

  setup_options.setup_font_xfce_font_button = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_set_name (setup_options.setup_font_xfce_font_button, "setup_font_xfce_font_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_xfce_font_button", setup_options.setup_font_xfce_font_button);
  gtk_widget_show (setup_options.setup_font_xfce_font_button);
  gtk_box_pack_start (GTK_BOX (setup_font_xfce_hbox), setup_options.setup_font_xfce_font_button, FALSE, FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_font_xfce_font_button), 5);

  scrolled_window3 = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window3), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window3), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  if (current_config.wm == XFWM)
  {
    gtk_widget_show (scrolled_window3);
  }
  gtk_container_add (GTK_CONTAINER (setup_notebook), scrolled_window3);

  setup_vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_vbox4, "setup_vbox4");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_vbox4", setup_vbox4);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window3), setup_vbox4);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (setup_vbox4), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window3)));
  gtk_widget_show (setup_vbox4);
  gtk_container_border_width (GTK_CONTAINER (setup_vbox4), 5);

  setup_options.setup_focusmode_checkbutton = gtk_check_button_new_with_label (_("Click to focus windows"));
  gtk_widget_set_name (setup_options.setup_focusmode_checkbutton, "setup_focusmode_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_focusmode_checkbutton", setup_options.setup_focusmode_checkbutton);
  gtk_widget_show (setup_options.setup_focusmode_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_focusmode_checkbutton, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_focusmode_checkbutton), 2);

  setup_options.setup_mapfocus_checkbutton = gtk_check_button_new_with_label (_("Automatically focus on new window"));
  gtk_widget_set_name (setup_options.setup_mapfocus_checkbutton, "setup_mapfocus_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_mapfocus_checkbutton", setup_options.setup_mapfocus_checkbutton);
  gtk_widget_show (setup_options.setup_mapfocus_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_mapfocus_checkbutton, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_mapfocus_checkbutton), 2);

  setup_options.setup_autoraise_checkbutton = gtk_check_button_new_with_label (_("Auto raise windows"));
  gtk_widget_set_name (setup_options.setup_autoraise_checkbutton, "setup_autoraise_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_autoraise_checkbutton", setup_options.setup_autoraise_checkbutton);
  gtk_widget_show (setup_options.setup_autoraise_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_autoraise_checkbutton, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_autoraise_checkbutton), 2);

  setup_options.setup_opaquemove_checkbutton = gtk_check_button_new_with_label (_("Show contents of window while moving"));
  gtk_widget_set_name (setup_options.setup_opaquemove_checkbutton, "setup_opaquemove_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_opaquemove_checkbutton", setup_options.setup_opaquemove_checkbutton);
  gtk_widget_show (setup_options.setup_opaquemove_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_opaquemove_checkbutton, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_opaquemove_checkbutton), 2);

  setup_options.setup_opaqueresize_checkbutton = gtk_check_button_new_with_label (_("Show contents of window while resizing"));
  gtk_widget_set_name (setup_options.setup_opaqueresize_checkbutton, "setup_opaqueresize_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_opaqueresize_checkbutton", setup_options.setup_opaqueresize_checkbutton);
  gtk_widget_show (setup_options.setup_opaqueresize_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_opaqueresize_checkbutton, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_opaqueresize_checkbutton), 2);

  setup_options.setup_gradient_activetitle = gtk_check_button_new_with_label (_("Use gradient color for the active window"));
  gtk_widget_set_name (setup_options.setup_gradient_activetitle, "setup_gradient_activetitle");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_gradient_activetitle", setup_options.setup_gradient_activetitle);
  gtk_widget_show (setup_options.setup_gradient_activetitle);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_gradient_activetitle, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_gradient_activetitle), 2);

  setup_options.setup_gradient_inactivetitle = gtk_check_button_new_with_label (_("Use gradient color for inactive windows"));
  gtk_widget_set_name (setup_options.setup_gradient_inactivetitle, "setup_gradient_inactivetitle");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_gradient_inactivetitle", setup_options.setup_gradient_inactivetitle);

  gtk_widget_show (setup_options.setup_gradient_inactivetitle);

  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_options.setup_gradient_inactivetitle, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_gradient_inactivetitle), 2);

  setup_xfwm_engine_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_xfwm_engine_frame, "setup_xfwm_engine_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_xfwm_engine_frame", setup_xfwm_engine_frame);
  gtk_widget_show (setup_xfwm_engine_frame);
  gtk_container_border_width (GTK_CONTAINER (setup_xfwm_engine_frame), 5);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_xfwm_engine_frame, FALSE, TRUE, 0);

  setup_xfwm_engine_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_xfwm_engine_hbox, "setup_xfwm_engine_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_xfwmengine_hbox", setup_xfwm_engine_hbox);
  gtk_widget_show (setup_xfwm_engine_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_xfwm_engine_hbox), 5);
  gtk_container_add (GTK_CONTAINER (setup_xfwm_engine_frame), setup_xfwm_engine_hbox);

  setup_xfwm_engine_label = gtk_label_new (_("Window border style : "));
  gtk_widget_set_name (setup_xfwm_engine_label, "setup_xfwm_engine_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_xfwm_engine_label", setup_xfwm_engine_label);
  gtk_widget_show (setup_xfwm_engine_label);
  gtk_box_pack_start (GTK_BOX (setup_xfwm_engine_hbox), setup_xfwm_engine_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_xfwm_engine_label), GTK_JUSTIFY_RIGHT);

  if (!xfwm_engines)
  {
    xfwm_engines = g_list_append (xfwm_engines, "Gtk");
    xfwm_engines = g_list_append (xfwm_engines, "Xfce");
    xfwm_engines = g_list_append (xfwm_engines, "Mofit");
    xfwm_engines = g_list_append (xfwm_engines, "Trench");
    xfwm_engines = g_list_append (xfwm_engines, "Linea");
  }
  setup_options.setup_xfwm_engine_combo = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (setup_options.setup_xfwm_engine_combo), xfwm_engines);
  gtk_widget_set_name (setup_options.setup_xfwm_engine_combo, "setup_xfwm_engine_combo");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_xfwm_engine_combo", setup_options.setup_xfwm_engine_combo);
  gtk_widget_show (setup_options.setup_xfwm_engine_combo);
  gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), 0, 0);
  gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), FALSE);
  gtk_box_pack_start (GTK_BOX (setup_xfwm_engine_hbox), setup_options.setup_xfwm_engine_combo, FALSE, FALSE, 0);

  setup_snapsize_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_snapsize_frame, "setup_snapsize_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_snapsize_frame", setup_snapsize_frame);
  gtk_widget_show (setup_snapsize_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_snapsize_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_snapsize_frame), 5);

  setup_snapsize_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_snapsize_hbox, "setup_snapsize_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_snapsize_hbox", setup_snapsize_hbox);
  gtk_widget_show (setup_snapsize_hbox);
  gtk_container_add (GTK_CONTAINER (setup_snapsize_frame), setup_snapsize_hbox);
  gtk_container_border_width (GTK_CONTAINER (setup_snapsize_hbox), 5);

  setup_snapsize_label = gtk_label_new (_("Window snapping size (0 disable) : "));
  gtk_widget_set_name (setup_snapsize_label, "setup_snapsize_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_snapsize_label", setup_snapsize_label);
  gtk_widget_show (setup_snapsize_label);
  gtk_box_pack_start (GTK_BOX (setup_snapsize_hbox), setup_snapsize_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (setup_snapsize_label), GTK_JUSTIFY_RIGHT);

  setup_options.setup_snapsize_spinbutton_adj = gtk_adjustment_new (10, 0, 150, 1, 5, 1);
  setup_options.setup_snapsize_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (setup_options.setup_snapsize_spinbutton_adj), 1, 0);
  gtk_widget_set_name (setup_options.setup_snapsize_spinbutton, "setup_snapsize_spinbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_snapsize_spinbutton", setup_options.setup_snapsize_spinbutton);
  gtk_widget_show (setup_options.setup_snapsize_spinbutton);
  gtk_box_pack_start (GTK_BOX (setup_snapsize_hbox), setup_options.setup_snapsize_spinbutton, FALSE, FALSE, 0);

  setup_iconpos_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_iconpos_frame, "setup_iconpos_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_frame", setup_iconpos_frame);
  gtk_widget_show (setup_iconpos_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_iconpos_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_iconpos_frame), 5);

  setup_iconpos_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (setup_iconpos_hbox, "setup_iconpos_hbox");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_hbox", setup_iconpos_hbox);
  gtk_widget_show (setup_iconpos_hbox);
  gtk_container_add (GTK_CONTAINER (setup_iconpos_frame), setup_iconpos_hbox);

  setup_iconpos_label = gtk_label_new (_("Icon position"));
  gtk_widget_set_name (setup_iconpos_label, "setup_iconpos_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_label", setup_iconpos_label);
  gtk_widget_show (setup_iconpos_label);
  gtk_box_pack_start (GTK_BOX (setup_iconpos_hbox), setup_iconpos_label, TRUE, TRUE, 0);

  setup_iconpos_table = gtk_table_new (3, 3, FALSE);
  gtk_widget_set_name (setup_iconpos_table, "setup_iconpos_table");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_table", setup_iconpos_table);
  gtk_widget_show (setup_iconpos_table);
  gtk_box_pack_start (GTK_BOX (setup_iconpos_hbox), setup_iconpos_table, TRUE, TRUE, 0);

  setup_options.setup_iconpos_topbutton = gtk_radio_button_new_with_label (setup_iconpos_table_group, _("Top"));
  setup_iconpos_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_iconpos_topbutton));
  gtk_widget_set_name (setup_options.setup_iconpos_topbutton, "setup_iconpos_topbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_topbutton", setup_options.setup_iconpos_topbutton);
  gtk_widget_show (setup_options.setup_iconpos_topbutton);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_options.setup_iconpos_topbutton, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_options.setup_iconpos_leftbutton = gtk_radio_button_new_with_label (setup_iconpos_table_group, _("Left"));
  setup_iconpos_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_iconpos_leftbutton));
  gtk_widget_set_name (setup_options.setup_iconpos_leftbutton, "setup_iconpos_leftbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_leftbutton", setup_options.setup_iconpos_leftbutton);
  gtk_widget_show (setup_options.setup_iconpos_leftbutton);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_options.setup_iconpos_leftbutton, 0, 1, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_options.setup_iconpos_botbutton = gtk_radio_button_new_with_label (setup_iconpos_table_group, _("bottom"));
  setup_iconpos_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_iconpos_botbutton));
  gtk_widget_set_name (setup_options.setup_iconpos_botbutton, "setup_iconpos_botbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_botbutton", setup_options.setup_iconpos_botbutton);
  gtk_widget_show (setup_options.setup_iconpos_botbutton);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_options.setup_iconpos_botbutton, 1, 2, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_options.setup_iconpos_rightbutton = gtk_radio_button_new_with_label (setup_iconpos_table_group, _("Right"));
  setup_iconpos_table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_options.setup_iconpos_rightbutton));
  gtk_widget_set_name (setup_options.setup_iconpos_rightbutton, "setup_iconpos_rightbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_iconpos_rightbutton", setup_options.setup_iconpos_rightbutton);
  gtk_widget_show (setup_options.setup_iconpos_rightbutton);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_options.setup_iconpos_rightbutton, 2, 3, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_packer1 = gtk_packer_new ();
  gtk_widget_set_name (setup_packer1, "setup_packer1");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_packer1", setup_packer1);
  gtk_widget_show (setup_packer1);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_packer1, 0, 1, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_packer2 = gtk_packer_new ();
  gtk_widget_set_name (setup_packer2, "setup_packer2");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_packer2", setup_packer2);
  gtk_widget_show (setup_packer2);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_packer2, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_packer3 = gtk_packer_new ();
  gtk_widget_set_name (setup_packer3, "setup_packer3");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_packer3", setup_packer3);
  gtk_widget_show (setup_packer3);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_packer3, 2, 3, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_packer4 = gtk_packer_new ();
  gtk_widget_set_name (setup_packer4, "setup_packer4");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_packer4", setup_packer4);
  gtk_widget_show (setup_packer4);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_packer4, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_packer5 = gtk_packer_new ();
  gtk_widget_set_name (setup_packer5, "setup_packer5");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_packer5", setup_packer5);
  gtk_widget_show (setup_packer5);
  gtk_table_attach (GTK_TABLE (setup_iconpos_table), setup_packer5, 2, 3, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_font_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_font_frame, "setup_font_frame");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_frame", setup_font_frame);
  gtk_widget_show (setup_font_frame);
  gtk_box_pack_start (GTK_BOX (setup_vbox4), setup_font_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_font_frame), 5);

  setup_font_table = gtk_table_new (3, 3, FALSE);
  gtk_widget_set_name (setup_font_table, "setup_font_table");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_table", setup_font_table);
  gtk_widget_show (setup_font_table);
  gtk_container_add (GTK_CONTAINER (setup_font_frame), setup_font_table);
  gtk_container_border_width (GTK_CONTAINER (setup_font_table), 6);

  setup_options.setup_font_title_entry = gtk_entry_new ();
  gtk_widget_set_name (setup_options.setup_font_title_entry, "setup_font_title_entry");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_title_entry", setup_options.setup_font_title_entry);
  /* gtk_widget_set_style(setup_options.setup_font_title_entry, pal->cm[4]); */
  gtk_entry_set_editable (GTK_ENTRY (setup_options.setup_font_title_entry), TRUE);
  gtk_widget_show (setup_options.setup_font_title_entry);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_title_entry, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_options.setup_font_icon_entry = gtk_entry_new ();
  gtk_widget_set_name (setup_options.setup_font_icon_entry, "setup_font_icon_entry");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_icon_entry", setup_options.setup_font_icon_entry);
  /* gtk_widget_set_style(setup_options.setup_font_icon_entry, pal->cm[4]); */
  gtk_entry_set_editable (GTK_ENTRY (setup_options.setup_font_icon_entry), TRUE);
  gtk_widget_show (setup_options.setup_font_icon_entry);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_icon_entry, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_options.setup_font_menu_entry = gtk_entry_new ();
  gtk_widget_set_name (setup_options.setup_font_menu_entry, "setup_font_menu_entry");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_menu_entry", setup_options.setup_font_menu_entry);
  /* gtk_widget_set_style(setup_options.setup_font_menu_entry, pal->cm[4]); */
  /* 
   * Since GTKFontSelection doesn't support FontSet selection, it's
   * better make fontname editable.
   */
  gtk_entry_set_editable (GTK_ENTRY (setup_options.setup_font_menu_entry), TRUE);
  gtk_widget_show (setup_options.setup_font_menu_entry);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_menu_entry, 1, 2, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  setup_font_title_label = gtk_label_new (_("Title font :"));
  gtk_widget_set_name (setup_font_title_label, "setup_font_title_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_title_label", setup_font_title_label);
  gtk_widget_show (setup_font_title_label);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_font_title_label, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (setup_font_title_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (setup_font_title_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (setup_font_title_label), 5, 5);

  setup_font_icon_label = gtk_label_new (_("Icon font :"));
  gtk_widget_set_name (setup_font_icon_label, "setup_font_icon_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_icon_label", setup_font_icon_label);
  gtk_widget_show (setup_font_icon_label);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_font_icon_label, 0, 1, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (setup_font_icon_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (setup_font_icon_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (setup_font_icon_label), 5, 5);

  setup_font_menu_label = gtk_label_new (_("Menu font :"));
  gtk_widget_set_name (setup_font_menu_label, "setup_font_menu_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_menu_label", setup_font_menu_label);
  gtk_widget_show (setup_font_menu_label);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_font_menu_label, 0, 1, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (setup_font_menu_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (setup_font_menu_label), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (setup_font_menu_label), 5, 5);

  setup_options.setup_font_title_button = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_set_name (setup_options.setup_font_title_button, "setup_font_title_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_title_button", setup_options.setup_font_title_button);
  gtk_widget_show (setup_options.setup_font_title_button);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_title_button, 2, 3, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_font_title_button), 5);

  setup_options.setup_font_icon_button = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_set_name (setup_options.setup_font_icon_button, "setup_font_icon_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_icon_button", setup_options.setup_font_icon_button);
  gtk_widget_show (setup_options.setup_font_icon_button);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_icon_button, 2, 3, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_font_icon_button), 5);

  setup_options.setup_font_menu_button = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_set_name (setup_options.setup_font_menu_button, "setup_font_menu_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_font_menu_button", setup_options.setup_font_menu_button);
  gtk_widget_show (setup_options.setup_font_menu_button);
  gtk_table_attach (GTK_TABLE (setup_font_table), setup_options.setup_font_menu_button, 2, 3, 2, 3, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_font_menu_button), 5);

  scrolled_window4 = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window4), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window4), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  if (current_config.wm == XFWM)
  {
    gtk_widget_show (scrolled_window4);
  }
  gtk_container_add (GTK_CONTAINER (setup_notebook), scrolled_window4);

  setup_vbox5 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (setup_vbox5, "setup_vbox5");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_vbox5", setup_vbox5);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window4), setup_vbox5);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (setup_vbox5), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window4)));
  gtk_widget_show (setup_vbox5);
  gtk_container_border_width (GTK_CONTAINER (setup_vbox5), 5);

  setup_options.setup_soundmodule_checkbutton = gtk_check_button_new_with_label (_("Sound module (xfsound)"));
  gtk_widget_set_name (setup_options.setup_soundmodule_checkbutton, "setup_soundmodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_soundmodule_checkbutton", setup_options.setup_soundmodule_checkbutton);
  gtk_widget_show (setup_options.setup_soundmodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_soundmodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_soundmodule_checkbutton), 2);

  setup_options.setup_mousemodule_checkbutton = gtk_check_button_new_with_label (_("Mouse settings (xfmouse)"));
  gtk_widget_set_name (setup_options.setup_mousemodule_checkbutton, "setup_mousemodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_mousemodule_checkbutton", setup_options.setup_mousemodule_checkbutton);
  gtk_widget_show (setup_options.setup_mousemodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_mousemodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_mousemodule_checkbutton), 2);

  setup_options.setup_backdropmodule_checkbutton = gtk_check_button_new_with_label (_("Backdrop settings (xfbd)"));
  gtk_widget_set_name (setup_options.setup_backdropmodule_checkbutton, "setup_backdropmodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_backdropmodule_checkbutton", setup_options.setup_backdropmodule_checkbutton);
  gtk_widget_show (setup_options.setup_backdropmodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_backdropmodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_backdropmodule_checkbutton), 2);

  setup_options.setup_pagermodule_checkbutton = gtk_check_button_new_with_label (_("Pager (xfpager)"));
  gtk_widget_set_name (setup_options.setup_pagermodule_checkbutton, "setup_pagermodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_pagermodule_checkbutton", setup_options.setup_pagermodule_checkbutton);
  gtk_widget_show (setup_options.setup_pagermodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_pagermodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_pagermodule_checkbutton), 2);

  setup_options.setup_gnomemodule_checkbutton = gtk_check_button_new_with_label (_("GNOME (xfgnome)"));
  gtk_widget_set_name (setup_options.setup_gnomemodule_checkbutton, "setup_gnomemodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_gnomemodule_checkbutton", setup_options.setup_gnomemodule_checkbutton);
  gtk_widget_show (setup_options.setup_gnomemodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_gnomemodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_gnomemodule_checkbutton), 2);

  setup_options.setup_gnomemenumodule_checkbutton = gtk_check_button_new_with_label (_("GNOME menu (xfmenu)"));
  gtk_widget_set_name (setup_options.setup_gnomemenumodule_checkbutton, "setup_gnomemenumodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_gnomemenumodule_checkbutton", setup_options.setup_gnomemenumodule_checkbutton);
  gtk_widget_show (setup_options.setup_gnomemenumodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_gnomemenumodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_gnomemenumodule_checkbutton), 2);

  setup_options.setup_kdemenumodule_checkbutton = gtk_check_button_new_with_label (_("KDE menu (xfmenu)"));
  gtk_widget_set_name (setup_options.setup_kdemenumodule_checkbutton, "setup_kdemenumodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_kdemenumodule_checkbutton", setup_options.setup_kdemenumodule_checkbutton);
  gtk_widget_show (setup_options.setup_kdemenumodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5), setup_options.setup_kdemenumodule_checkbutton, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_kdemenumodule_checkbutton), 2);

  setup_options.setup_debianmenumodule_checkbutton =
    gtk_check_button_new_with_label (_("Debian menu (xfmenu)"));
  gtk_widget_set_name (setup_options.setup_debianmenumodule_checkbutton,
		       "setup_debianmenumodule_checkbutton");
  gtk_object_set_data (GTK_OBJECT (setup), 
		       "setup_debianmenumodule_checkbutton", 
		       setup_options.setup_debianmenumodule_checkbutton);
  gtk_widget_show (setup_options.setup_debianmenumodule_checkbutton);
  gtk_box_pack_start (GTK_BOX (setup_vbox5),
		      setup_options.setup_debianmenumodule_checkbutton, 
		      FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_debianmenumodule_checkbutton), 2);

  setup_notebook_palette_label = gtk_label_new (_("Palette"));
  gtk_widget_set_name (setup_notebook_palette_label, "setup_notebook_palette_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_notebook_palette_label", setup_notebook_palette_label);
  gtk_widget_show (setup_notebook_palette_label);
  set_notebook_tab (setup_notebook, 0, setup_notebook_palette_label);

  setup_notebook_xfce_label = gtk_label_new (_("XFce"));
  gtk_widget_set_name (setup_notebook_xfce_label, "setup_notebook_xfce_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_notebook_xfce_label", setup_notebook_xfce_label);
  gtk_widget_show (setup_notebook_xfce_label);
  set_notebook_tab (setup_notebook, 1, setup_notebook_xfce_label);

  setup_notebook_windows_label = gtk_label_new (_("Windows"));
  gtk_widget_set_name (setup_notebook_windows_label, "setup_notebook_windows_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_notebook_windows_label", setup_notebook_windows_label);
  gtk_widget_show (setup_notebook_windows_label);
  set_notebook_tab (setup_notebook, 2, setup_notebook_windows_label);

  setup_notebook_startup_label = gtk_label_new (_("Startup"));
  gtk_widget_set_name (setup_notebook_startup_label, "setup_notebook_startup_label");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_notebook_startup_label", setup_notebook_startup_label);
  gtk_widget_show (setup_notebook_startup_label);
  set_notebook_tab (setup_notebook, 3, setup_notebook_startup_label);

  setup_bottomframe = gtk_frame_new (NULL);
  gtk_widget_set_name (setup_bottomframe, "setup_bottomframe");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_bottomframe", setup_bottomframe);
  gtk_widget_show (setup_bottomframe);
  gtk_box_pack_start (GTK_BOX (setup_vbox1), setup_bottomframe, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (setup_bottomframe), 10);

  setup_hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_set_name (setup_hbuttonbox2, "setup_hbuttonbox2");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_hbuttonbox2", setup_hbuttonbox2);
  gtk_widget_show (setup_hbuttonbox2);
  gtk_container_add (GTK_CONTAINER (setup_bottomframe), setup_hbuttonbox2);

  setup_options.setup_ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (setup_options.setup_ok_button, "setup_ok_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_ok_button", setup_options.setup_ok_button);
  gtk_widget_show (setup_options.setup_ok_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox2), setup_options.setup_ok_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_ok_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (setup_options.setup_ok_button);

  setup_options.setup_apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_set_name (setup_options.setup_apply_button, "setup_apply_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_apply_button", setup_options.setup_apply_button);
  gtk_widget_show (setup_options.setup_apply_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox2), setup_options.setup_apply_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_apply_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_apply_button, GTK_CAN_DEFAULT);

  setup_options.setup_cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (setup_options.setup_cancel_button, "setup_cancel_button");
  gtk_object_set_data (GTK_OBJECT (setup), "setup_cancel_button", setup_options.setup_cancel_button);
  gtk_widget_show (setup_options.setup_cancel_button);
  gtk_container_add (GTK_CONTAINER (setup_hbuttonbox2), setup_options.setup_cancel_button);
  gtk_container_border_width (GTK_CONTAINER (setup_options.setup_cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (setup_options.setup_cancel_button, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_ok_button), "clicked", GTK_SIGNAL_FUNC (setup_ok_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_cancel_button), "clicked", GTK_SIGNAL_FUNC (setup_cancel_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_apply_button), "clicked", GTK_SIGNAL_FUNC (setup_apply_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_palette_default_button), "clicked", GTK_SIGNAL_FUNC (setup_default_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_palette_load_button), "clicked", GTK_SIGNAL_FUNC (setup_loadpal_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_palette_save_button), "clicked", GTK_SIGNAL_FUNC (setup_savepal_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_font_xfce_font_button), "clicked", GTK_SIGNAL_FUNC (xfce_font_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_digital_clock_checkbutton), "clicked", GTK_SIGNAL_FUNC (toggle_digital_clock_checkbutton_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_focusmode_checkbutton), "clicked", GTK_SIGNAL_FUNC (toggle_focusmode_checkbutton_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_repaint_checkbutton), "clicked", GTK_SIGNAL_FUNC (toggle_repaint_checkbutton_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup), "delete_event", GTK_SIGNAL_FUNC (setup_delete_event), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_font_title_button), "clicked", GTK_SIGNAL_FUNC (xfwm_titlefont_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_font_menu_button), "clicked", GTK_SIGNAL_FUNC (xfwm_menufont_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (setup_options.setup_font_icon_button), "clicked", GTK_SIGNAL_FUNC (xfwm_iconfont_cb), NULL);

  gtk_widget_add_accelerator (setup_options.setup_ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (setup_options.setup_ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (setup_options.setup_apply_button, "clicked", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (setup_options.setup_cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (setup_options.setup_cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  return setup;
}

void
show_setup (XFCE_palette * pal)
{
  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
    return;

  hide_current_popup_menu ();
  copyvaluepal (temp_pal, pal);
  apply_pal_colortable (temp_pal);
/*
  gtk_window_set_modal (GTK_WINDOW (setup), TRUE);
 */
  prev_panel_icon_size = current_config.select_icon_size;
  prev_popup_icon_size = current_config.popup_icon_size;
  prev_visible_screen = current_config.visible_screen;
  prev_visible_popup = current_config.visible_popup;
  prev_apply_xcolors = current_config.apply_xcolors;
  prev_xfwm_engine = current_config.xfwm_engine;
  pal_changed = FALSE;
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_palette_engine_combo)->entry), temp_pal->engine);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_apply_xcolors_checkbutton), (current_config.apply_xcolors != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_repaint_checkbutton), (current_config.colorize_root != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_checkbutton), (((DEFAULT_DEPTH >= 16) && (current_config.colorize_root != 0)) ? (current_config.gradient_root != 0) : FALSE));
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_gradient_checkbutton), ((DEFAULT_DEPTH >= 16) && (current_config.colorize_root != 0)));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_tearoff_checkbutton), (current_config.detach_menu != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_show_diagnostic_checkbutton), (current_config.show_diagnostic != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_panelicons_large), (current_config.select_icon_size == 2));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_panelicons_medium), (current_config.select_icon_size == 1));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_panelicons_small), (current_config.select_icon_size == 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_popupicons_large), (current_config.popup_icon_size == 2));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_popupicons_medium), (current_config.popup_icon_size == 1));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_popupicons_small), (current_config.popup_icon_size == 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_autoraise_checkbutton), (current_config.autoraise != 0) && (current_config.clicktofocus == 0));
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_autoraise_checkbutton), (current_config.clicktofocus == 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_focusmode_checkbutton), (current_config.clicktofocus != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_mapfocus_checkbutton), (current_config.mapfocus != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_opaquemove_checkbutton), (current_config.opaquemove != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_opaqueresize_checkbutton), (current_config.opaqueresize != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_activetitle), ((DEFAULT_DEPTH >= 16) ? (current_config.gradient_active_title != 0) : FALSE));
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_gradient_activetitle), (DEFAULT_DEPTH >= 16));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_inactivetitle), ((DEFAULT_DEPTH >= 16) ? (current_config.gradient_inactive_title != 0) : FALSE));
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_gradient_inactivetitle), (DEFAULT_DEPTH >= 16));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_topbutton), (current_config.iconpos == 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_leftbutton), (current_config.iconpos == 1));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_botbutton), (current_config.iconpos == 2));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_rightbutton), (current_config.iconpos == 3));
  if ((current_config.fonts[0]) && strlen (current_config.fonts[0]))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_title_entry), current_config.fonts[0]);
  else
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_title_entry), XFWM_TITLEFONT);
  if ((current_config.fonts[1]) && strlen (current_config.fonts[1]))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_menu_entry), current_config.fonts[1]);
  else
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_menu_entry), XFWM_MENUFONT);
  if ((current_config.fonts[2]) && strlen (current_config.fonts[2]))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_icon_entry), current_config.fonts[2]);
  else
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_icon_entry), XFWM_ICONFONT);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_panel_layer_adj), current_config.panel_layer + 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_digital_clock_checkbutton), (current_config.digital_clock != 0));
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_hrs_mode_checkbutton), (current_config.digital_clock != 0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_hrs_mode_checkbutton), (current_config.hrs_mode != 0));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_tooltipsdelay_spinbutton_adj), current_config.tooltipsdelay);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_numscreens_spinbutton_adj), current_config.visible_screen);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_numpopups_spinbutton_adj), current_config.visible_popup);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_snapsize_spinbutton_adj), current_config.snapsize);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (setup_options.setup_switchdeskfactor_spinbutton_adj), current_config.switchdeskfactor);
  switch (current_config.xfwm_engine)
  {
  case MOFIT_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Mofit");
    break;
  case TRENCH_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Trench");
    break;
  case GTK_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Gtk");
    break;
  case LINEA_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Linea");
    break;
  default:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Xfce");
    break;
  }
  switch (current_config.xfwm_engine)
  {
  case MOFIT_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Mofit");
    break;
  case TRENCH_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Trench");
    break;
  case GTK_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Gtk");
    break;
  case LINEA_ENGINE:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Linea");
    break;
  default:
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry), "Xfce");
    break;
  }
  gtk_entry_set_position (GTK_ENTRY (setup_options.setup_font_title_entry), 0);
  gtk_entry_set_position (GTK_ENTRY (setup_options.setup_font_menu_entry), 0);
  gtk_entry_set_position (GTK_ENTRY (setup_options.setup_font_icon_entry), 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_soundmodule_checkbutton), (current_config.startup_flags & F_SOUNDMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_mousemodule_checkbutton), (current_config.startup_flags & F_MOUSEMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_backdropmodule_checkbutton), (current_config.startup_flags & F_BACKDROPMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_pagermodule_checkbutton), (current_config.startup_flags & F_PAGERMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gnomemodule_checkbutton), (current_config.startup_flags & F_GNOMEMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gnomemenumodule_checkbutton), (current_config.startup_flags & F_GNOMEMENUMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_kdemenumodule_checkbutton), (current_config.startup_flags & F_KDEMENUMODULE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_debianmenumodule_checkbutton), (current_config.startup_flags & F_DEBIANMENUMODULE));
  gnome_sticky (setup->window);
  gtk_widget_show (setup);
/*
  gtk_main ();
  while (repaint_in_progress ());
  my_flush_events ();
  gtk_widget_hide (setup);
  gdk_window_withdraw ((GTK_WIDGET (setup))->window);
 */
}

void
get_setup_values (void)
{
  char *s1, *s2, *s3;
  gchar *xfwm_engine;

  current_config.apply_xcolors = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_apply_xcolors_checkbutton));
  current_config.colorize_root = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_repaint_checkbutton));
  current_config.gradient_root = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_checkbutton));
  current_config.detach_menu = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_tearoff_checkbutton));
  current_config.show_diagnostic = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_show_diagnostic_checkbutton));
  current_config.select_icon_size = 2 * (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_panelicons_large)) ? 1 : 0) + (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_panelicons_medium)) ? 1 : 0);
  current_config.popup_icon_size = 2 * (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_popupicons_large)) ? 1 : 0) + (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_popupicons_medium)) ? 1 : 0);
  current_config.autoraise = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_autoraise_checkbutton));
  current_config.clicktofocus = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_focusmode_checkbutton));
  current_config.mapfocus = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_mapfocus_checkbutton));
  current_config.opaquemove = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_opaquemove_checkbutton));
  current_config.opaqueresize = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_opaqueresize_checkbutton));
  current_config.snapsize = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (setup_options.setup_snapsize_spinbutton));
  current_config.switchdeskfactor = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (setup_options.setup_switchdeskfactor_spinbutton));
  current_config.gradient_active_title = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_activetitle));
  current_config.gradient_inactive_title = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_inactivetitle));
  current_config.iconpos = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_leftbutton)) ? 1 : 0) + 2 * (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_botbutton)) ? 1 : 0) + 3 * (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_iconpos_rightbutton)) ? 1 : 0);
  if (current_config.fonts[0])
    g_free (current_config.fonts[0]);
  if (current_config.fonts[1])
    g_free (current_config.fonts[1]);
  if (current_config.fonts[2])
    g_free (current_config.fonts[2]);
  s1 = gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_title_entry));
  s2 = gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_menu_entry));
  s3 = gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_icon_entry));
  current_config.fonts[0] = (char *) g_malloc (sizeof (char) * (strlen (s1) + 1));
  current_config.fonts[1] = (char *) g_malloc (sizeof (char) * (strlen (s2) + 1));
  current_config.fonts[2] = (char *) g_malloc (sizeof (char) * (strlen (s3) + 1));
  strcpy (current_config.fonts[0], s1);
  strcpy (current_config.fonts[1], s2);
  strcpy (current_config.fonts[2], s3);

  current_config.panel_layer = my_get_adjustment_as_int (GTK_ADJUSTMENT (setup_options.setup_panel_layer_adj)) - 1;

  current_config.digital_clock = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_digital_clock_checkbutton));
  current_config.hrs_mode = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_hrs_mode_checkbutton));
  current_config.tooltipsdelay = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (setup_options.setup_tooltipsdelay_spinbutton));
  current_config.visible_screen = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (setup_options.setup_numscreens_spinbutton));
  current_config.visible_popup = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (setup_options.setup_numpopups_spinbutton));
  xfwm_engine = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_xfwm_engine_combo)->entry));
  if (!g_strncasecmp (xfwm_engine, "x", 1))
    current_config.xfwm_engine = XFCE_ENGINE;
  else if (!g_strncasecmp (xfwm_engine, "t", 1))
    current_config.xfwm_engine = TRENCH_ENGINE;
  else if (!g_strncasecmp (xfwm_engine, "g", 1))
    current_config.xfwm_engine = GTK_ENGINE;
  else if (!g_strncasecmp (xfwm_engine, "l", 1))
    current_config.xfwm_engine = LINEA_ENGINE;
  else
    current_config.xfwm_engine = MOFIT_ENGINE;

  current_config.startup_flags = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_soundmodule_checkbutton)) ? F_SOUNDMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_mousemodule_checkbutton)) ? F_MOUSEMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_backdropmodule_checkbutton)) ? F_BACKDROPMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_pagermodule_checkbutton)) ? F_PAGERMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_gnomemodule_checkbutton)) ? F_GNOMEMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_gnomemenumodule_checkbutton)) ? F_GNOMEMENUMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_kdemenumodule_checkbutton)) ? F_KDEMENUMODULE : 0);
  current_config.startup_flags |= (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_debianmenumodule_checkbutton)) ? F_DEBIANMENUMODULE : 0);
}
