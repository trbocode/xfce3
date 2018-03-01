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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_GDK_IMLIB
#include <fcntl.h>
#include <gdk_imlib.h>
#endif

#include <gtk/gtk.h>
#include <time.h>
#include <string.h>
#include "my_intl.h"
#include "mygtkclock.h"
#include "my_string.h"
#include "xfcolor.h"
#include "lightdark.h"
#include "fontselection.h"
#include "xfclock_colorsel.h"
#include "xfce-common.h"
#include "gnome_protocol.h"
#include "xfclock_menus.h"
#include "xfclock_config.h"
#include "xfclock_style.h"
#include "xfclock_cb.h"
#include "xfclock.h"
#include "xfclock_opt.h"

#include "appointments.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfclock_icon.xpm"

gint
update_xfclock_date_timer (GtkWidget * widget)
{
  time_t ticks;
  struct tm *tm;
  static gint mday = -1;
  static gint mon = -1;
  static gint year = -1;
  guint cal_day, cal_mon, cal_year;

  if (!timeout)
  {
    timeout = TRUE;
    return TRUE;
  }

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);

  ticks = time (0);
  tm = localtime (&ticks);
  gtk_calendar_get_date (GTK_CALENDAR (widget), &cal_year, &cal_mon, &cal_day);
  if ((mday != tm->tm_mday) || (mon != tm->tm_mon) || (year != tm->tm_year) || (cal_day != tm->tm_mday) || (cal_mon != tm->tm_mon) || (cal_year != tm->tm_year))
  {
    mday = tm->tm_mday;
    mon = tm->tm_mon;
    year = tm->tm_year;

    gtk_calendar_select_day (GTK_CALENDAR (widget), mday);
    gtk_calendar_select_month (GTK_CALENDAR (widget), mon, year + 1900);
    mark_holy_days (widget);

  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *main_frame;
  GtkWidget *menu;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *eventbox;
  gdouble colors[4];
  GtkStyle *style, *newstyle;

  xfce_init (&argc, &argv);

  create_gnome_atoms ();

  readconfig (&current_config);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
  if ((current_config.x >= 0) || (current_config.y >= 0))
    gtk_widget_set_uposition (window, current_config.x, current_config.y);

  if ((current_config.w >= 0) && (current_config.h >= 0))
    gtk_widget_set_usize (window, current_config.w, current_config.h);

  gtk_window_set_title (GTK_WINDOW (window), _("XFClock - XFce Clock & Calendar"));

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handle_box, FALSE, FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (handle_box), 2);

  if (current_config.menubar)
    gtk_widget_show (handle_box);

  main_frame = gtk_frame_new (NULL);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (main_frame), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (main_frame), GTK_SHADOW_OUT);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), main_frame, TRUE, TRUE, 0);
  gtk_widget_show (main_frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_frame), hbox);
  gtk_widget_show (hbox);

  eventbox = gtk_event_box_new ();
  gtk_container_border_width (GTK_CONTAINER (eventbox), 0);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, TRUE, TRUE, 0);
  gtk_widget_show (eventbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (eventbox), frame);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show (frame);

  clock_widget = my_gtk_clock_new ();
  gtk_container_add (GTK_CONTAINER (frame), clock_widget);
  my_gtk_clock_set_relief (MY_GTK_CLOCK (clock_widget), FALSE);
  style = gtk_widget_get_style (clock_widget);
  newstyle = gtk_style_copy (style);
  if (my_strncasecmp (current_config.font, "*", strlen (current_config.font)))
    setfontstyle (newstyle, current_config.font);
  if ((current_config.fore_red >= 0) && (current_config.fore_green >= 0) && (current_config.fore_blue >= 0))
  {
    colors[0] = ((gdouble) current_config.fore_red) / COLOR_GDK;
    colors[1] = ((gdouble) current_config.fore_green) / COLOR_GDK;
    colors[2] = ((gdouble) current_config.fore_blue) / COLOR_GDK;
    setfgstyle (newstyle, colors);
  }
  if ((current_config.back_red >= 0) && (current_config.back_green >= 0) && (current_config.back_blue >= 0))
  {
    colors[0] = ((gdouble) current_config.back_red) / COLOR_GDK;
    colors[1] = ((gdouble) current_config.back_green) / COLOR_GDK;
    colors[2] = ((gdouble) current_config.back_blue) / COLOR_GDK;
    setbgstyle (newstyle, colors);
  }
  gtk_widget_set_style (clock_widget, newstyle);
  my_gtk_clock_show_ampm (MY_GTK_CLOCK (clock_widget), current_config.ampm);
  my_gtk_clock_show_military (MY_GTK_CLOCK (clock_widget), current_config.military);
  my_gtk_clock_show_secs (MY_GTK_CLOCK (clock_widget), current_config.seconds);
  if (current_config.digital)
    my_gtk_clock_set_mode (MY_GTK_CLOCK (clock_widget), MY_GTK_CLOCK_DIGITAL);
  else
    my_gtk_clock_set_mode (MY_GTK_CLOCK (clock_widget), MY_GTK_CLOCK_ANALOG);
  my_gtk_clock_set_interval (MY_GTK_CLOCK (clock_widget), 100);

  gtk_widget_show (clock_widget);

  calendar = gtk_calendar_new ();
  gtk_object_set_data (GTK_OBJECT (window), "calendar", calendar);
  gtk_box_pack_start (GTK_BOX (hbox), calendar, FALSE, FALSE, 0);
  gtk_calendar_display_options (GTK_CALENDAR (calendar), current_config.calendar_opt);
  if (current_config.calendar)
    gtk_widget_show (calendar);

  menu = create_menu (MY_GTK_CLOCK (clock_widget));
  gtk_container_add (GTK_CONTAINER (handle_box), menu);
  gtk_widget_show (menu);

  gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event", GTK_SIGNAL_FUNC (show_popup_cb), (gpointer) clock_widget);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_signal_connect (GTK_OBJECT (calendar), "month_changed", GTK_SIGNAL_FUNC (monthchanged), NULL);
  gtk_signal_connect (GTK_OBJECT (calendar), "day_selected_double_click", GTK_SIGNAL_FUNC (edit_appointments), NULL);

  timeout = TRUE;
  update_xfclock_date_timer (calendar);
  gtk_timeout_add (30000,	/* 30 secs */
		   (GtkFunction) update_xfclock_date_timer, (gpointer) calendar);

  gtk_widget_show (window);
  set_icon (window, "XFClock", xfclock_icon_xpm);

  gtk_main ();
  xfce_end ((gpointer) NULL, 0);

  return (0);
}
