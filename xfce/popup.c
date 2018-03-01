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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "my_intl.h"
#include "constant.h"
#include "xfce_main.h"
#include "xfce.h"
#include "xfce_cb.h"
#include "xfce-common.h"
#include "popup.h"
#include "xpmext.h"
#include "popup_cb.h"
#include "configfile.h"
#include "fileutil.h"
#include "my_tooltips.h"
#include "gnome_protocol.h"

#include "menu_icon.h"
#include "addiconpix.h"
#include "empty.h"
#include "defaulticon.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define POPUP_ICON_SIZE ((current_config.popup_icon_size == 0) ? SMALL_POPUP_ICONS : \
                           ((current_config.popup_icon_size == 2) ? LARGE_POPUP_ICONS : MEDIUM_POPUP_ICONS))

enum
{
  TARGET_STRING,
  TARGET_ROOTWIN,
  TARGET_URL
};

static GtkTargetEntry popup_target_table[] = {
  {"STRING", 0, TARGET_STRING},
  {"text/uri-list", 0, TARGET_URL}
};

static guint n_popup_targets = sizeof (popup_target_table) / sizeof (popup_target_table[0]);

/* Added by Jason Litowitz */
static gint Current_Popup_Window = -1;

GtkWidget *
create_popup_item (GtkWidget * toplevel, gint nbr_menu, gint nbr_item)
{
  GtkWidget *hbox;
  GtkWidget *label_frame;

  popup_menus[nbr_menu].popup_buttons[nbr_item].label = NULL;
  popup_menus[nbr_menu].popup_buttons[nbr_item].pixfile = NULL;
  popup_menus[nbr_menu].popup_buttons[nbr_item].command = NULL;

  popup_menus[nbr_menu].popup_buttons[nbr_item].item_button = gtk_button_new ();
  gtk_widget_set_name (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button, "menuitem");
  gtk_object_set_data (GTK_OBJECT (toplevel), "menuitem", popup_menus[nbr_menu].popup_buttons[nbr_item].item_button);
  gtk_button_set_relief ((GtkButton *) popup_menus[nbr_menu].popup_buttons[nbr_item].item_button, GTK_RELIEF_NONE);
  gtk_widget_show (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button);

  popup_menus[nbr_menu].popup_buttons[nbr_item].item_tooltip = my_tooltips_new (current_config.tooltipsdelay);
  gtk_tooltips_set_tip (popup_menus[nbr_menu].popup_buttons[nbr_item].item_tooltip, popup_menus[nbr_menu].popup_buttons[nbr_item].item_button, popup_menus[nbr_menu].popup_buttons[nbr_item].command, "ContextHelp/buttons/?");
  gtk_object_set_data (GTK_OBJECT (toplevel), "tooltips", popup_menus[nbr_menu].popup_buttons[nbr_item].item_tooltip);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox, "hbox");
  gtk_object_set_data (GTK_OBJECT (toplevel), "hbox", hbox);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button), hbox);

  popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame, "frame");
  gtk_object_set_data (GTK_OBJECT (toplevel), "frame", popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame);
  gtk_widget_show (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame);
  gtk_box_pack_start (GTK_BOX (hbox), popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame, POPUP_ICON_SIZE, POPUP_ICON_SIZE);
#ifndef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame), GTK_SHADOW_IN);
#endif

  popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap = MyCreateFromPixmapData (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame, empty);
  if (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap, "popup_pixmap");
  gtk_object_set_data (GTK_OBJECT (toplevel), "popup_pixmap", popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap);
  gtk_widget_show (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap_frame), popup_menus[nbr_menu].popup_buttons[nbr_item].item_pixmap);

  label_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (label_frame, "frame");
  gtk_object_set_data (GTK_OBJECT (toplevel), "frame", label_frame);
  gtk_widget_show (label_frame);
  gtk_container_set_border_width (GTK_CONTAINER (label_frame), 2);
  gtk_box_pack_start (GTK_BOX (hbox), label_frame, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (label_frame), GTK_SHADOW_NONE);

  popup_menus[nbr_menu].popup_buttons[nbr_item].item_label = gtk_label_new (popup_menus[nbr_menu].popup_buttons[nbr_item].label);
  gtk_widget_set_name (popup_menus[nbr_menu].popup_buttons[nbr_item].item_label, "popup_label");
  gtk_object_set_data (GTK_OBJECT (toplevel), "popup_label", popup_menus[nbr_menu].popup_buttons[nbr_item].item_label);
  gtk_widget_show (popup_menus[nbr_menu].popup_buttons[nbr_item].item_label);
  gtk_container_add (GTK_CONTAINER (label_frame), popup_menus[nbr_menu].popup_buttons[nbr_item].item_label);
  gtk_widget_set_usize (popup_menus[nbr_menu].popup_buttons[nbr_item].item_label, 130, 0);
  gtk_label_set_justify (GTK_LABEL (popup_menus[nbr_menu].popup_buttons[nbr_item].item_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (popup_menus[nbr_menu].popup_buttons[nbr_item].item_label), 0.0, 0.5);

  gtk_drag_dest_set (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button, GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, popup_target_table, n_popup_targets, GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button), "drag_data_received", GTK_SIGNAL_FUNC (popup_entry_drag_data_received), (gpointer) ((long) (nbr_menu * NBMAXITEMS + nbr_item)));

  gtk_widget_set_events (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button, gtk_widget_get_events (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button) | GDK_BUTTON3_MASK);
  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button), "button_press_event", GTK_SIGNAL_FUNC (popup_entry_modify_cb), (gpointer) ((long) (nbr_menu * NBMAXITEMS + nbr_item)));
  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr_menu].popup_buttons[nbr_item].item_button), "clicked", GTK_SIGNAL_FUNC (popup_entry_cb), (gpointer) ((long) (nbr_menu * NBMAXITEMS + nbr_item)));

  return popup_menus[nbr_menu].popup_buttons[nbr_item].item_button;
}

GtkWidget *
create_popup_menu (gint nbr)
{
  GtkWidget *popup_mainframe;
  GtkWidget *popup_vbox;
  GtkTooltips *popup_addicon_tooltip;
  GtkWidget *popup_addicon_hbox;
  GtkWidget *popup_addicon_icon_pixmap;
  GtkWidget *popup_addicon_icon_label;
  GtkWidget *popup_tearoff_hseparator;
  GtkTooltips *popup_tearoff_tooltip;
  gint i;

  popup_menus[nbr].detach = FALSE;
  popup_menus[nbr].entries = 0;
  popup_menus[nbr].uposx = 0;
  popup_menus[nbr].uposy = 0;
  popup_menus[nbr].lastpx = 0;
  popup_menus[nbr].lastpy = 0;
  popup_menus[nbr].animatewin = NULL;

  popup_menus[nbr].popup_toplevel = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_policy (GTK_WINDOW (popup_menus[nbr].popup_toplevel), FALSE, FALSE, TRUE);
  gtk_widget_set_name (popup_menus[nbr].popup_toplevel, "popup");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup", popup_menus[nbr].popup_toplevel);
  gtk_window_set_title (GTK_WINDOW (popup_menus[nbr].popup_toplevel), "Popup Menu");
  gnome_sticky (popup_menus[nbr].popup_toplevel->window);

  popup_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (popup_mainframe, "popup_mainframe");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_mainframe", popup_mainframe);
  gtk_widget_show (popup_mainframe);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr].popup_toplevel), popup_mainframe);
  gtk_frame_set_shadow_type (GTK_FRAME (popup_mainframe), GTK_SHADOW_OUT);

  popup_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (popup_vbox, "popup_vbox");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_vbox", popup_vbox);
  gtk_widget_show (popup_vbox);
  gtk_container_add (GTK_CONTAINER (popup_mainframe), popup_vbox);

  popup_menus[nbr].popup_addicon_button = gtk_button_new ();
  gtk_widget_set_name (popup_menus[nbr].popup_addicon_button, "popup_addicon_button");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_button", popup_menus[nbr].popup_addicon_button);
  gtk_button_set_relief ((GtkButton *) popup_menus[nbr].popup_addicon_button, GTK_RELIEF_NONE);
  gtk_widget_show (popup_menus[nbr].popup_addicon_button);
  gtk_box_pack_start (GTK_BOX (popup_vbox), popup_menus[nbr].popup_addicon_button, FALSE, TRUE, 0);

  popup_addicon_tooltip = my_tooltips_new (current_config.tooltipsdelay);
  gtk_tooltips_set_tip (popup_addicon_tooltip, popup_menus[nbr].popup_addicon_button, _("Add a new entry..."), "ContextHelp/buttons/?");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "tooltips", popup_addicon_tooltip);

  popup_addicon_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (popup_addicon_hbox, "popup_addicon_hbox");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_hbox", popup_addicon_hbox);
  gtk_widget_show (popup_addicon_hbox);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr].popup_addicon_button), popup_addicon_hbox);

  popup_menus[nbr].popup_addicon_icon_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (popup_menus[nbr].popup_addicon_icon_frame, "popup_addicon_icon_frame");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_icon_frame", popup_menus[nbr].popup_addicon_icon_frame);
  gtk_widget_show (popup_menus[nbr].popup_addicon_icon_frame);
  gtk_box_pack_start (GTK_BOX (popup_addicon_hbox), popup_menus[nbr].popup_addicon_icon_frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (popup_menus[nbr].popup_addicon_icon_frame, POPUP_ICON_SIZE, POPUP_ICON_SIZE);
#ifndef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (popup_menus[nbr].popup_addicon_icon_frame), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (popup_menus[nbr].popup_addicon_icon_frame), GTK_SHADOW_IN);
#endif

  popup_addicon_icon_pixmap = MyCreateFromPixmapData (popup_menus[nbr].popup_addicon_icon_frame, addiconpix);
  if (popup_addicon_icon_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (popup_addicon_icon_pixmap, "popup_addicon_icon_pixmap");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_icon_pixmap", popup_addicon_icon_pixmap);
  gtk_widget_show (popup_addicon_icon_pixmap);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr].popup_addicon_icon_frame), popup_addicon_icon_pixmap);

  popup_addicon_icon_label = gtk_label_new (_("Add icon..."));
  gtk_widget_set_name (popup_addicon_icon_label, "popup_addicon_icon_label");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_icon_label", popup_addicon_icon_label);
  gtk_widget_show (popup_addicon_icon_label);
  gtk_box_pack_start (GTK_BOX (popup_addicon_hbox), popup_addicon_icon_label, TRUE, TRUE, 0);
  gtk_widget_set_usize (popup_addicon_icon_label, 120, -2);
  gtk_label_set_justify (GTK_LABEL (popup_addicon_icon_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (popup_addicon_icon_label), 0.1, 0.5);

  popup_menus[nbr].popup_addicon_separator = gtk_hseparator_new ();
  gtk_widget_set_name (popup_menus[nbr].popup_addicon_separator, "popup_addicon_separator");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_addicon_separator", popup_menus[nbr].popup_addicon_separator);
  gtk_widget_show (popup_menus[nbr].popup_addicon_separator);
  gtk_box_pack_start (GTK_BOX (popup_vbox), popup_menus[nbr].popup_addicon_separator, FALSE, TRUE, 0);

  for (i = 0; i < NBMAXITEMS; i++)
    gtk_box_pack_start (GTK_BOX (popup_vbox), create_popup_item (popup_menus[nbr].popup_toplevel, nbr, i), FALSE, TRUE, 0);

  popup_menus[nbr].popup_detach = gtk_button_new ();
  gtk_widget_set_name (popup_menus[nbr].popup_detach, "popup_tearoff_button");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_tearoff_button", popup_menus[nbr].popup_detach);
  gtk_button_set_relief ((GtkButton *) popup_menus[nbr].popup_detach, GTK_RELIEF_NONE);
  gtk_widget_show (popup_menus[nbr].popup_detach);
  gtk_box_pack_start (GTK_BOX (popup_vbox), popup_menus[nbr].popup_detach, FALSE, TRUE, 0);

  popup_tearoff_tooltip = my_tooltips_new (current_config.tooltipsdelay);
  gtk_tooltips_set_tip (popup_tearoff_tooltip, popup_menus[nbr].popup_detach, _("Click to tear off popup menu"), "ContextHelp/buttons/?");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "tooltips", popup_tearoff_tooltip);

  popup_tearoff_hseparator = gtk_hseparator_new ();
  gtk_widget_set_name (popup_tearoff_hseparator, "popup_tearoff_hseparator");
  gtk_object_set_data (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "popup_tearoff_hseparator", popup_tearoff_hseparator);
  gtk_widget_show (popup_tearoff_hseparator);
  gtk_container_add (GTK_CONTAINER (popup_menus[nbr].popup_detach), popup_tearoff_hseparator);

  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr].popup_toplevel), "delete_event", GTK_SIGNAL_FUNC (delete_popup_cb), (gpointer) ((long) nbr));

  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr].popup_detach), "clicked", GTK_SIGNAL_FUNC (detach_cb), (gpointer) ((long) nbr));

  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr].popup_addicon_button), "clicked", GTK_SIGNAL_FUNC (popup_addicon_cb), (gpointer) ((long) nbr));

  gtk_drag_dest_set (popup_menus[nbr].popup_addicon_button, GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, popup_target_table, n_popup_targets, GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (popup_menus[nbr].popup_addicon_button), "drag_data_received", GTK_SIGNAL_FUNC (popup_addicon_drag_data_received), (gpointer) ((long) nbr));

  set_icon (popup_menus[nbr].popup_toplevel, "Popup Menu", menu_icon);

  update_popup_entries (nbr);

  return popup_menus[nbr].popup_toplevel;
}

void
create_all_popup_menus (GtkWidget * main)
{
  gint i;

  for (i = 0; i < NBPOPUPS; i++)
  {
    create_popup_menu (i);
    gtk_window_set_transient_for (GTK_WINDOW (popup_menus[i].popup_toplevel), GTK_WINDOW (main));
  }
}

void
show_popup_menu (gint nbr, gint x, gint y, gboolean animation)
{
  gint uwidth = 0;
  gint uheight = 0;
  gint px = 0;
  gint py = 0;
  gint tempx, tempy;
  gint i;
  GtkRequisition requisition;
#if 0
  gdouble speed = 0.0;
  gboolean anim;
  struct timeval new_tv, old_tv;
  struct timezone tz;

  anim = ((!popup_menus[nbr].detach) && animation && current_config.opaquemove);
#endif

  if ((popup_menus[nbr].entries == 0) || (popup_menus[nbr].detach))
    gtk_widget_hide (popup_menus[nbr].popup_detach);
  else if (current_config.detach_menu)
    gtk_widget_show (popup_menus[nbr].popup_detach);
  else
    gtk_widget_hide (popup_menus[nbr].popup_detach);

  if ((popup_menus[nbr].entries == NBMAXITEMS) || (current_config.disable_user_config))
  {
    gtk_widget_hide (popup_menus[nbr].popup_addicon_button);
    gtk_widget_hide (popup_menus[nbr].popup_addicon_separator);
  }
  else
  {
    gtk_widget_show (popup_menus[nbr].popup_addicon_button);
    gtk_widget_show (popup_menus[nbr].popup_addicon_separator);
  }

  hide_current_popup_menu ();

  if ((x > 0) && (y > 0))
  {
    px = x;
    py = y;
    popup_menus[nbr].lastpx = x;
    popup_menus[nbr].lastpy = y;
  }
  else if (!popup_menus[nbr].detach)
  {
    px = popup_menus[nbr].lastpx;
    py = popup_menus[nbr].lastpy;
  }
  if (!GTK_WIDGET_REALIZED (popup_menus[nbr].popup_toplevel))
  {
    gtk_widget_realize (popup_menus[nbr].popup_toplevel);
    set_icon (popup_menus[nbr].popup_toplevel, "Popup Menu", menu_icon);
  }
  gnome_sticky (popup_menus[nbr].popup_toplevel->window);
  gtk_widget_size_request ((GtkWidget *) popup_menus[nbr].popup_toplevel, &requisition);
  uwidth = requisition.width;
  uheight = requisition.height;
  tempx = (((px - uwidth / 2) > 0) ? (px - uwidth / 2) : 0);
  tempy = (py - uheight);
  if (tempy < 0)
  {
    if ((py + 16 + uheight) < gdk_screen_height ())
      tempy = py + 16;
    else
      tempy = 0;
  }

  if (!popup_menus[nbr].detach)
  {
    Current_Popup_Window = nbr;
    gtk_widget_set_uposition (popup_menus[nbr].popup_toplevel, tempx, tempy);
  }
  else
  {
    Current_Popup_Window = -1;
    gtk_widget_set_uposition (popup_menus[nbr].popup_toplevel, popup_menus[nbr].uposx, popup_menus[nbr].uposy);
  }

#if 0
  if ((!popup_menus[nbr].animatewin) && anim)
  {
    popup_menus[nbr].animatewin = gtk_window_new (GTK_WINDOW_DIALOG);
    gtk_window_set_modal (GTK_WINDOW (popup_menus[nbr].animatewin), TRUE);
    gtk_widget_realize (popup_menus[nbr].animatewin);
    gtk_window_set_transient_for (GTK_WINDOW (popup_menus[nbr].animatewin), GTK_WINDOW (gxfce));
    gdk_window_set_decorations ((popup_menus[nbr].animatewin)->window, (GdkWMDecoration) 0);
  }

  if (anim)
  {
    gtk_window_set_policy (GTK_WINDOW (popup_menus[nbr].animatewin), FALSE, FALSE, FALSE);
    gtk_widget_set_uposition (popup_menus[nbr].animatewin, tempx, tempy + uheight - speed);
    gtk_widget_set_usize (popup_menus[nbr].animatewin, uwidth, 0);
    gtk_widget_show_now (popup_menus[nbr].animatewin);
    gdk_flush ();
    XFlush (GDK_DISPLAY ());
    gettimeofday (&old_tv, &tz);
    for (i = 0; i < (uheight - speed); i += speed)
    {
      gdk_window_move_resize (popup_menus[nbr].animatewin->window, tempx, tempy + uheight - i, uwidth, i);
      gdk_flush ();
      XFlush (GDK_DISPLAY ());
      gettimeofday (&new_tv, &tz);
      /* Kind of smart here : We compute the speed of the animation based on the speed of the system 
         and/or network. The value 400.0 means 400 pixels per second */
      speed = 400.0 / 100000.0 * ((new_tv.tv_sec * 1000000.0 + new_tv.tv_usec) - (old_tv.tv_sec * 1000000.0 + old_tv.tv_usec));
      if (speed < 1.0)
	speed = 1.0;		/* in case we've got a *very* fast system */
      old_tv = new_tv;
    }
    gdk_window_move_resize (popup_menus[nbr].animatewin->window, tempx, tempy, uwidth, uheight);
  }
#endif
  gdk_window_set_decorations (((GtkWidget *) popup_menus[nbr].popup_toplevel)->window, ((popup_menus[nbr].detach) ? (GDK_DECOR_TITLE | GDK_DECOR_MENU | GDK_DECOR_BORDER) : (GdkWMDecoration) 0));
  if (!GTK_WIDGET_VISIBLE (popup_menus[nbr].popup_toplevel))
    gtk_widget_show_now (popup_menus[nbr].popup_toplevel);
  gdk_flush ();

  for (i = 0; i < NBMAXITEMS; i++)
    gtk_button_leave (GTK_BUTTON (popup_menus[nbr].popup_buttons[i].item_button));

#if 0
  if (popup_menus[nbr].animatewin && anim)
    gtk_widget_hide (popup_menus[nbr].animatewin);
#endif
}

void
hide_popup_menu (gint nbr, gboolean animation)
{
#if 0
  gint uwidth = 0;
  gint uheight = 0;
  gint uposx = 0;
  gint uposy = 0;
  gint i;
  GtkRequisition requisition;
  gdouble speed = 0.0;
  gboolean anim;
  struct timeval new_tv, old_tv;
  struct timezone tz;

  anim = ((!popup_menus[nbr].detach) && animation && current_config.opaquemove);

#endif

#if 0
  if (anim)
  {
    gdk_window_get_root_origin (((GtkWidget *) popup_menus[nbr].popup_toplevel)->window, &uposx, &uposy);
    gtk_widget_size_request ((GtkWidget *) popup_menus[nbr].popup_toplevel, &requisition);
    uwidth = requisition.width;
    uheight = requisition.height;
  }
#endif

  if ((popup_menus[nbr].popup_toplevel)->window)
    gdk_window_get_root_origin (((GtkWidget *) popup_menus[nbr].popup_toplevel)->window, &popup_menus[nbr].uposx, &popup_menus[nbr].uposy);
  /* Added by Jason Litowitz */
  if (!popup_menus[nbr].detach)
    Current_Popup_Window = -1;

#if 0
  if ((!popup_menus[nbr].animatewin) && anim)
  {
    popup_menus[nbr].animatewin = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_modal (GTK_WINDOW (popup_menus[nbr].animatewin), TRUE);
  }

  if (anim)
  {
    gtk_window_set_policy (GTK_WINDOW (popup_menus[nbr].animatewin), FALSE, FALSE, FALSE);
    gtk_widget_set_uposition (popup_menus[nbr].animatewin, uposx, uposy);
    gtk_widget_set_usize (popup_menus[nbr].animatewin, uwidth, uheight);
    gtk_widget_show_now (popup_menus[nbr].animatewin);
  }
#endif
  gtk_widget_hide (popup_menus[nbr].popup_toplevel);
  gdk_window_withdraw ((popup_menus[nbr].popup_toplevel)->window);

#if 0
  gdk_flush ();
  XFlush (GDK_DISPLAY ());
  gettimeofday (&old_tv, &tz);

  if (anim)
  {
    for (i = uheight; i > 0; i -= speed)
    {
      gdk_window_move_resize (popup_menus[nbr].animatewin->window, uposx, uposy + uheight - i, uwidth, i);
      gdk_flush ();
      XFlush (GDK_DISPLAY ());
      gettimeofday (&new_tv, &tz);
      /* Kind of smart here : We compute the speed of the animation based on the speed of the system 
         and/or network. The value 400.0 means 400 pixels per second */
      speed = 400.0 / 100000.0 * ((new_tv.tv_sec * 1000000.0 + new_tv.tv_usec) - (old_tv.tv_sec * 1000000.0 + old_tv.tv_usec));
      if (speed < 1.0)
	speed = 1.0;		/* in case we've got a *very* fast system */
      old_tv = new_tv;
    }
  }

  if (popup_menus[nbr].animatewin && anim)
    gtk_widget_hide (popup_menus[nbr].animatewin);
#endif
}

void
close_popup_menu (gint nbr)
{
  hide_popup_menu (nbr, TRUE);
  popup_menus[nbr].detach = FALSE;
}

void
update_popup_entries (gint nbr)
{
  gint i;

  for (i = 0; i < popup_menus[nbr].entries; i++)
  {
    gtk_widget_show (popup_menus[nbr].popup_buttons[i].item_button);
  }
  for (i = popup_menus[nbr].entries; i < NBMAXITEMS; i++)
  {
    gtk_widget_hide (popup_menus[nbr].popup_buttons[i].item_button);
  }
}

void
update_popup_size (void)
{
  gint i, j;

  for (i = 0; i < NBPOPUPS; i++)
  {
    gtk_widget_set_usize (popup_menus[i].popup_addicon_icon_frame, POPUP_ICON_SIZE, POPUP_ICON_SIZE);
    for (j = 0; j < popup_menus[i].entries; j++)
    {
      gtk_widget_set_usize (popup_menus[i].popup_buttons[j].item_pixmap_frame, POPUP_ICON_SIZE, POPUP_ICON_SIZE);
      if (j < popup_menus[i].entries)
      {
	if (existfile (popup_menus[i].popup_buttons[j].pixfile))
	{
	  MySetPixmapFile (popup_menus[i].popup_buttons[j].item_pixmap, popup_menus[i].popup_buttons[j].item_pixmap_frame, popup_menus[i].popup_buttons[j].pixfile);
	}
	else
	{
	  MySetPixmapData (popup_menus[i].popup_buttons[j].item_pixmap, popup_menus[i].popup_buttons[j].item_pixmap_frame, defaulticon);
	}
      }
    }
    for (j = popup_menus[i].entries; j < NBMAXITEMS; j++)
      gtk_widget_set_usize (popup_menus[i].popup_buttons[j].item_pixmap_frame, POPUP_ICON_SIZE, POPUP_ICON_SIZE);
  }
}

void
set_entry (gint menu, gint entry, char *label, char *pixfile, char *command)
{
  int l1, l2, l3;
  char defpix[] = "Default Icon";
  gboolean nullpix = False;

  l1 = strlen (label);
  l2 = strlen (pixfile);
  l3 = strlen (command);

  if (l2 == 0)
  {
    l2 = strlen (defpix);
    nullpix = True;
  }

  if (popup_menus[menu].popup_buttons[entry].label)
    g_free (popup_menus[menu].popup_buttons[entry].label);
  if (popup_menus[menu].popup_buttons[entry].pixfile)
    g_free (popup_menus[menu].popup_buttons[entry].pixfile);
  if (popup_menus[menu].popup_buttons[entry].command)
    g_free (popup_menus[menu].popup_buttons[entry].command);

  popup_menus[menu].popup_buttons[entry].label = (char *) g_malloc (sizeof (char) * (l1 + 1));
  popup_menus[menu].popup_buttons[entry].pixfile = (char *) g_malloc (sizeof (char) * (l2 + 1));
  popup_menus[menu].popup_buttons[entry].command = (char *) g_malloc (sizeof (char) * (l3 + 1));

  strcpy (popup_menus[menu].popup_buttons[entry].label, label);
  strcpy (popup_menus[menu].popup_buttons[entry].pixfile, (nullpix ? defpix : pixfile));
  strcpy (popup_menus[menu].popup_buttons[entry].command, command);

  if (existfile (popup_menus[menu].popup_buttons[entry].pixfile))
  {
    MySetPixmapFile (popup_menus[menu].popup_buttons[entry].item_pixmap, popup_menus[menu].popup_buttons[entry].item_pixmap_frame, popup_menus[menu].popup_buttons[entry].pixfile);
  }
  else
  {
    MySetPixmapData (popup_menus[menu].popup_buttons[entry].item_pixmap, popup_menus[menu].popup_buttons[entry].item_pixmap_frame, defaulticon);
  }

  gtk_label_set_text (GTK_LABEL (popup_menus[menu].popup_buttons[entry].item_label), label);

  gtk_tooltips_set_tip (popup_menus[menu].popup_buttons[entry].item_tooltip, popup_menus[menu].popup_buttons[entry].item_button, popup_menus[menu].popup_buttons[entry].command, "ContextHelp/buttons/?");
}

void
set_like_entry (gint menu_s, gint entry_s, gint menu_d, gint entry_d)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  char defpix[] = "Default Icon";
  gboolean nullpix = False;

  if ((!popup_menus[menu_s].popup_buttons[entry_s].label) || (!popup_menus[menu_s].popup_buttons[entry_s].pixfile) || (!popup_menus[menu_s].popup_buttons[entry_s].command))
    return;

  if (strlen (popup_menus[menu_s].popup_buttons[entry_s].pixfile) == 0)
  {
    nullpix = True;
  }

  if (popup_menus[menu_d].popup_buttons[entry_d].label)
    g_free (popup_menus[menu_d].popup_buttons[entry_d].label);
  if (popup_menus[menu_d].popup_buttons[entry_d].pixfile)
    g_free (popup_menus[menu_d].popup_buttons[entry_d].pixfile);
  if (popup_menus[menu_d].popup_buttons[entry_d].command)
    g_free (popup_menus[menu_d].popup_buttons[entry_d].command);

  popup_menus[menu_d].popup_buttons[entry_d].label   = g_strdup (popup_menus[menu_s].popup_buttons[entry_s].label);
  popup_menus[menu_d].popup_buttons[entry_d].pixfile = g_strdup ((nullpix ? defpix : popup_menus[menu_s].popup_buttons[entry_s].pixfile));
  popup_menus[menu_d].popup_buttons[entry_d].command = g_strdup (popup_menus[menu_s].popup_buttons[entry_s].command);

  gtk_pixmap_get (GTK_PIXMAP (popup_menus[menu_s].popup_buttons[entry_s].item_pixmap), &pixmap, &mask);
  gtk_pixmap_set (GTK_PIXMAP (popup_menus[menu_d].popup_buttons[entry_d].item_pixmap), pixmap, mask);

  gtk_label_set_text (GTK_LABEL (popup_menus[menu_d].popup_buttons[entry_d].item_label), popup_menus[menu_s].popup_buttons[entry_s].label);

  gtk_tooltips_set_tip (popup_menus[menu_d].popup_buttons[entry_d].item_tooltip, popup_menus[menu_d].popup_buttons[entry_d].item_button, popup_menus[menu_d].popup_buttons[entry_d].command, "ContextHelp/buttons/?");
}

void
add_popup_entry (gint nbr, char *label, char *pixfile, char *command, int position)
{
  if ((popup_menus[nbr].entries < NBMAXITEMS) && label && pixfile && command)
  {
    int where = position;
    int i;

    if ((where > popup_menus[nbr].entries) || (where < 0))
    {
      where = popup_menus[nbr].entries;
    }
    for (i = popup_menus[nbr].entries; i >= where ; i--)
    {
      set_like_entry (nbr, i - 1, nbr, i);
    }
    set_entry (nbr, where, label, pixfile, command);
    popup_menus[nbr].entries++;
  }
  update_popup_entries (nbr);
}

void
move_popup_entry (gint menu, gint entry_s, gint entry_d)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  gchar *label, *pixfile, *command;
  gint where = entry_d;
  int i;
  
  if (entry_d > popup_menus[menu].entries - 1)
  {
    where = popup_menus[menu].entries - 1;
  }
  if (where == entry_s)
  {
    return;
  }

  label   = g_strdup (popup_menus[menu].popup_buttons[entry_s].label);
  pixfile = g_strdup (popup_menus[menu].popup_buttons[entry_s].pixfile);
  command = g_strdup (popup_menus[menu].popup_buttons[entry_s].command);
  gtk_pixmap_get (GTK_PIXMAP (popup_menus[menu].popup_buttons[entry_s].item_pixmap), &pixmap, &mask);
  
  if (where < entry_s)
  {
    for (i = entry_s; i > where; i--)
    {
      set_like_entry (menu, i - 1, menu, i);
    }
  }
  else
  {
    for (i = entry_s; i < where; i++)
    {
      set_like_entry (menu, i + 1, menu, i);
    }
  }
  
  
  if (popup_menus[menu].popup_buttons[where].label)
    g_free (popup_menus[menu].popup_buttons[where].label);
  if (popup_menus[menu].popup_buttons[where].pixfile)
    g_free (popup_menus[menu].popup_buttons[where].pixfile);
  if (popup_menus[menu].popup_buttons[where].command)
    g_free (popup_menus[menu].popup_buttons[where].command);

  popup_menus[menu].popup_buttons[where].label   = label;
  popup_menus[menu].popup_buttons[where].pixfile = pixfile;
  popup_menus[menu].popup_buttons[where].command = command;
  
  gtk_pixmap_set (GTK_PIXMAP (popup_menus[menu].popup_buttons[where].item_pixmap), pixmap, mask);
  gtk_label_set_text (GTK_LABEL (popup_menus[menu].popup_buttons[where].item_label), label);
  gtk_tooltips_set_tip (popup_menus[menu].popup_buttons[where].item_tooltip, popup_menus[menu].popup_buttons[entry_d].item_button, command, "ContextHelp/buttons/?");
  update_popup_entries (menu);
}

void
remove_popup_entry (gint nbr_menu, gint entry)
{
  gint i;
  gint j;

  j = --popup_menus[nbr_menu].entries;
  for (i = entry; i < j; i++)
  {
    set_like_entry (nbr_menu, i + 1, nbr_menu, i);
  }
  update_popup_entries (nbr_menu);
  if (popup_menus[nbr_menu].popup_buttons[j].label)
  {
    g_free (popup_menus[nbr_menu].popup_buttons[j].label);
    popup_menus[nbr_menu].popup_buttons[j].label = NULL;
  }
  if (popup_menus[nbr_menu].popup_buttons[j].pixfile)
  {
    g_free (popup_menus[nbr_menu].popup_buttons[j].pixfile);
    popup_menus[nbr_menu].popup_buttons[j].pixfile = NULL;
  }
  if (popup_menus[nbr_menu].popup_buttons[j].command)
  {
    g_free (popup_menus[nbr_menu].popup_buttons[j].command);
    popup_menus[nbr_menu].popup_buttons[j].command = NULL;
  }
  MySetPixmapData (popup_menus[nbr_menu].popup_buttons[j].item_pixmap, popup_menus[nbr_menu].popup_buttons[j].item_pixmap_frame, empty);
}

char *
get_popup_entry_label (gint menu, gint entry)
{
  return popup_menus[menu].popup_buttons[entry].label;
}

char *
get_popup_entry_command (gint menu, gint entry)
{
  return popup_menus[menu].popup_buttons[entry].command;
}

char *
get_popup_entry_icon (gint menu, gint entry)
{
  return popup_menus[menu].popup_buttons[entry].pixfile;
}

gint
get_popup_menu_entries (gint menu)
{
  return popup_menus[menu].entries;
}

/* Added by Jason Litowitz */
void
hide_current_popup_menu ()
{
  if ((Current_Popup_Window != -1) && !(popup_menus[Current_Popup_Window].detach))
  {
    private_close_popup_button (Current_Popup_Window);
  }
}
