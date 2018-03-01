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
#include "my_tooltips.h"
#include "xfcolor.h"
#include "setup.h"
#include "setup_cb.h"
#include "colorselect.h"
#include "fileselect.h"
#include "xfce.h"
#include "xfce_cb.h"
#include "popup.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "xpmext.h"
#include "fontselection.h"
#include "configfile.h"
#include "xfwm.h"
#include "gnome_protocol.h"
#include "constant.h"

#ifdef XFCE_TASKBAR
  #include "taskbar.h"
#endif


#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
color_button_cb (GtkWidget * widget, gpointer data)
{
  open_colorselect (temp_pal, (gint) ((long) data));
  apply_pal_colortable (temp_pal);
  pal_changed = TRUE;
}

void
setup_ok_cb (GtkWidget * widget, gpointer data)
{
  gchar *engine;
  cursor_wait (setup);
  get_setup_values ();
  engine = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_palette_engine_combo)->entry));
  if (strcmp (engine, temp_pal->engine))
  {
    set_engine (temp_pal, engine);
    pal_changed = TRUE;
  }
  if (pal_changed)
  {
    copyvaluepal (pal, temp_pal);
    savepal (pal);
    create_gtkrc_file (pal, NULL);
    /* applypal (pal, gxfce); */
    applypal_to_all ();
    apply_xpalette (pal, current_config.apply_xcolors);
  }
  else if (prev_apply_xcolors != current_config.apply_xcolors)
  {
    apply_xpalette (pal, current_config.apply_xcolors);
  }
  update_delay_tooltips (current_config.tooltipsdelay);

  update_gxfce_clock ();
  gnome_layer (gxfce->window, current_config.panel_layer);
  if (prev_visible_screen != current_config.visible_screen)
  {
    update_gxfce_screen_buttons (current_config.visible_screen);
    gnome_set_desk_count (current_config.visible_screen ? current_config.visible_screen : 1);
  }
  if (current_config.colorize_root)
    ApplyRootColor (pal, (current_config.gradient_root != 0), get_screen_color (get_current_screen ()));
  if (prev_visible_popup != current_config.visible_popup)
  {
    hide_current_popup_menu ();
    update_gxfce_popup_buttons (current_config.visible_popup);
  }
  if (prev_panel_icon_size != current_config.select_icon_size)
  {
    hide_current_popup_menu ();
    update_gxfce_size ();
  }
  if (prev_popup_icon_size != current_config.popup_icon_size)
  {
    hide_current_popup_menu ();
    update_popup_size ();
  }
  if (current_config.wm == XFWM)
  {
    apply_wm_colors (pal);
    apply_wm_fonts ();
    apply_wm_iconpos ();
    apply_wm_options ();
    apply_wm_snapsize ();
    if (prev_xfwm_engine != current_config.xfwm_engine)
    {
      apply_wm_engine ();
    }
  }
  writeconfig ();
  cursor_reset (setup);
/*
  gtk_main_quit ();
 */
  gtk_widget_hide (setup);
  gdk_window_withdraw ((GTK_WIDGET (setup))->window);
#ifdef XFCE_TASKBAR
  taskbar_applay_xfce_config(&current_config);
#endif

}

void
setup_apply_cb (GtkWidget * widget, gpointer data)
{
  gchar *engine;
  cursor_wait (setup);
  get_setup_values ();
  engine = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (setup_options.setup_palette_engine_combo)->entry));
  if (strcmp (engine, temp_pal->engine))
  {
    set_engine (temp_pal, engine);
    pal_changed = TRUE;
  }
  if (pal_changed)
  {
    copyvaluepal (pal, temp_pal);
    savepal (pal);
    create_gtkrc_file (pal, NULL);
    applypal_to_all ();
    apply_xpalette (pal, current_config.apply_xcolors);
  }
  else if (prev_apply_xcolors != current_config.apply_xcolors)
  {
    apply_xpalette (pal, current_config.apply_xcolors);
    prev_apply_xcolors = current_config.apply_xcolors;
  }
  update_delay_tooltips (current_config.tooltipsdelay);
  update_gxfce_clock ();
  gnome_layer (gxfce->window, current_config.panel_layer);
  if (prev_visible_screen != current_config.visible_screen)
  {
    update_gxfce_screen_buttons (current_config.visible_screen);
    gnome_set_desk_count (current_config.visible_screen ? current_config.visible_screen : 1);
  }
  if (current_config.colorize_root)
    ApplyRootColor (pal, (current_config.gradient_root != 0), get_screen_color (get_current_screen ()));
  if (prev_visible_popup != current_config.visible_popup)
  {
    hide_current_popup_menu ();
    update_gxfce_popup_buttons (current_config.visible_popup);
  }
  if (prev_panel_icon_size != current_config.select_icon_size)
  {
    hide_current_popup_menu ();
    update_gxfce_size ();
  }
  if (prev_popup_icon_size != current_config.popup_icon_size)
  {
    hide_current_popup_menu ();
    update_popup_size ();
  }
  if (current_config.wm == XFWM)
  {
    apply_wm_colors (pal);
    apply_wm_fonts ();
    apply_wm_iconpos ();
    apply_wm_options ();
    apply_wm_snapsize ();
    if (prev_xfwm_engine != current_config.xfwm_engine)
    {
      apply_wm_engine ();
    }
  }
  pal_changed = FALSE;
  prev_panel_icon_size = current_config.select_icon_size;
  prev_popup_icon_size = current_config.popup_icon_size;
  prev_visible_screen = current_config.visible_screen;
  prev_visible_popup = current_config.visible_popup;
  prev_xfwm_engine = current_config.xfwm_engine;
  writeconfig ();
  cursor_reset (setup);
#ifdef XFCE_TASKBAR
  taskbar_applay_xfce_config(&current_config);
#endif
}

void
setup_cancel_cb (GtkWidget * widget, gpointer data)
{
/*
  gtk_main_quit ();
 */
  gtk_widget_hide (setup);
  gdk_window_withdraw ((GTK_WIDGET (setup))->window);
}

gboolean
setup_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  setup_cancel_cb (widget, data);
  return (TRUE);
}

void
setup_default_cb (GtkWidget * widget, gpointer data)
{
  defpal (temp_pal);
  apply_pal_colortable (temp_pal);
  pal_changed = TRUE;
}

void
setup_loadpal_cb (GtkWidget * widget, gpointer data)
{
  char *t;

  t = open_fileselect (build_path (XFCE_PAL));
  if (t)
    loadnamepal (temp_pal, t);
  apply_pal_colortable (temp_pal);
  pal_changed = TRUE;
}

void
setup_savepal_cb (GtkWidget * widget, gpointer data)
{
  char *t;

  t = open_fileselect (build_path (XFCE_PAL));
  if (t)
    savenamepal (temp_pal, t);
}

void
xfce_font_cb (GtkWidget * widget, gpointer data)
{
  char *s = NULL;
  s = open_fontselection (gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_xfce_entry)));
  if ((s) && strlen (s))
  {
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_xfce_entry), s);
    set_font (temp_pal, s);
    pal_changed = TRUE;
  }
}

void
toggle_repaint_checkbutton_cb (GtkWidget * widget, gpointer data)
{
  gboolean status;
  status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_repaint_checkbutton));
  if (!status)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_gradient_checkbutton), FALSE);
  }
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_gradient_checkbutton), (DEFAULT_DEPTH >= 16) && (status));
}

void
toggle_focusmode_checkbutton_cb (GtkWidget * widget, gpointer data)
{
  if ((gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_focusmode_checkbutton))))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_options.setup_autoraise_checkbutton), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_autoraise_checkbutton), (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_focusmode_checkbutton))));

}

void
toggle_digital_clock_checkbutton_cb (GtkWidget * widget, gpointer data)
{
  gtk_widget_set_sensitive (GTK_WIDGET (setup_options.setup_hrs_mode_checkbutton), (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setup_options.setup_digital_clock_checkbutton))));

}

void
xfwm_titlefont_cb (GtkWidget * widget, gpointer data)
{
  char *s = NULL;
  s = open_fontselection (gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_title_entry)));
  if ((s) && strlen (s))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_title_entry), s);
}

void
xfwm_menufont_cb (GtkWidget * widget, gpointer data)
{
  char *s = NULL;
  s = open_fontselection (gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_menu_entry)));
  if ((s) && strlen (s))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_menu_entry), s);
}

void
xfwm_iconfont_cb (GtkWidget * widget, gpointer data)
{
  char *s = NULL;
  s = open_fontselection (gtk_entry_get_text (GTK_ENTRY (setup_options.setup_font_icon_entry)));
  if ((s) && strlen (s))
    gtk_entry_set_text (GTK_ENTRY (setup_options.setup_font_icon_entry), s);
}
