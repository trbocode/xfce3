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

#include <gtk/gtk.h>
#include <string.h>
#include "my_intl.h"
#include "mygtkclock.h"
#include "xfcolor.h"
#include "lightdark.h"
#include "fontselection.h"
#include "xfclock_colorsel.h"
#include "xfclock_config.h"
#include "xfce-common.h"
#include "xfclock_menus.h"
#include "xfclock_cb.h"
#include "xfclock_style.h"
#include "xfclock.h"
#include "xfce-common.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
quit_cb (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  writeconfig (window, &current_config);
  gtk_main_quit ();
  exit (0);
}

gint delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  quit_cb (widget, event, data);
  return (TRUE);
}

void
destroy_cb (GtkWidget * widget)
{
  writeconfig (window, &current_config);
  gtk_main_quit ();
}

void
font_cb (GtkWidget * widget, gpointer data)
{
  char *fnt;
  GtkStyle *style, *newstyle;
  GtkWidget *clock_widget;

  clock_widget = GTK_WIDGET (data);

  style = gtk_widget_get_style (clock_widget);
  fnt = open_fontselection (current_config.font);

  if (fnt)
  {
    newstyle = gtk_style_copy (style);
    setfontstyle (newstyle, fnt);
    strncpy (current_config.font, fnt, MAXSTRLEN - 1);
    gtk_widget_set_style (clock_widget, newstyle);
  }
}

void
foreground_cb (GtkWidget * widget, gpointer data)
{
  gdouble colors[4];
  gdouble *newcolor;
  GtkStyle *style, *newstyle;
  GtkWidget *clock_widget;

  clock_widget = GTK_WIDGET (data);

  style = gtk_widget_get_style (clock_widget);
  colors[0] = ((gdouble) style->fg[GTK_STATE_NORMAL].red) / COLOR_GDK;
  colors[1] = ((gdouble) style->fg[GTK_STATE_NORMAL].green) / COLOR_GDK;
  colors[2] = ((gdouble) style->fg[GTK_STATE_NORMAL].blue) / COLOR_GDK;
  newcolor = xfclock_colorselect (colors);
  if (newcolor)
  {
    current_config.fore_red = ((guint) (newcolor[0] * COLOR_GDK));
    current_config.fore_green = ((guint) (newcolor[1] * COLOR_GDK));
    current_config.fore_blue = ((guint) (newcolor[2] * COLOR_GDK));
    newstyle = gtk_style_copy (style);
    setfgstyle (newstyle, newcolor);
    gtk_widget_set_style (clock_widget, newstyle);
  }
}

void
background_cb (GtkWidget * widget, gpointer data)
{
  gdouble colors[4];
  gdouble *newcolor;
  GtkStyle *style, *newstyle;
  GtkWidget *clock_widget;

  clock_widget = GTK_WIDGET (data);

  style = gtk_widget_get_style (clock_widget);
  colors[0] = ((gdouble) style->bg[GTK_STATE_NORMAL].red) / COLOR_GDK;
  colors[1] = ((gdouble) style->bg[GTK_STATE_NORMAL].green) / COLOR_GDK;
  colors[2] = ((gdouble) style->bg[GTK_STATE_NORMAL].blue) / COLOR_GDK;
  newcolor = xfclock_colorselect (colors);
  if (newcolor)
  {
    current_config.back_red = ((guint) (newcolor[0] * COLOR_GDK));
    current_config.back_green = ((guint) (newcolor[1] * COLOR_GDK));
    current_config.back_blue = ((guint) (newcolor[2] * COLOR_GDK));
    newstyle = gtk_style_copy (style);
    setbgstyle (newstyle, newcolor);
    gtk_widget_set_style (clock_widget, newstyle);
  }
}

gint
show_popup_cb (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  if (event->button != 3)
    return (FALSE);

  if (!clock_popup_menu)
    clock_make_popup (MY_GTK_CLOCK (data));

  gtk_menu_popup (GTK_MENU (clock_popup_menu), NULL, NULL, NULL, NULL, event->button, event->time);
  return (TRUE);
}

void
toggle_menubar_cb (GtkWidget * widget, gpointer data)
{
  if (GTK_WIDGET_VISIBLE (handle_box))
  {
    gtk_widget_hide (handle_box);
    current_config.menubar = FALSE;
  }
  else
  {
    gtk_widget_show (handle_box);
    current_config.menubar = TRUE;
  }
}

void
toggle_calendar_cb (GtkWidget * widget, gpointer data)
{
  if (GTK_WIDGET_VISIBLE (calendar))
  {
    gtk_widget_hide (calendar);
    current_config.calendar = FALSE;
  }
  else
  {
    gtk_widget_show (calendar);
    current_config.calendar = TRUE;
  }
}

void
toggle_digital_cb (GtkWidget * widget, gpointer data)
{
  MyGtkClockMode mode;
  
  mode = my_gtk_clock_get_mode (MY_GTK_CLOCK (data));
  if (mode == MY_GTK_CLOCK_ANALOG)
    my_gtk_clock_set_mode (MY_GTK_CLOCK (data), MY_GTK_CLOCK_DIGITAL);
  else
    my_gtk_clock_set_mode (MY_GTK_CLOCK (data), MY_GTK_CLOCK_ANALOG);
  current_config.digital = (mode == MY_GTK_CLOCK_ANALOG);
}

void
toggle_secs_cb (GtkWidget * widget, gpointer data)
{
  my_gtk_clock_secs_toggle (MY_GTK_CLOCK (data));
  current_config.seconds = my_gtk_clock_secs_shown (MY_GTK_CLOCK (data));
}

void
toggle_ampm_cb (GtkWidget * widget, gpointer data)
{
  my_gtk_clock_ampm_toggle (MY_GTK_CLOCK (data));
  current_config.ampm = my_gtk_clock_ampm_shown (MY_GTK_CLOCK (data));
}

void
toggle_military_cb (GtkWidget * widget, gpointer data)
{
  my_gtk_clock_military_toggle (MY_GTK_CLOCK (data));
  current_config.military = my_gtk_clock_military_shown (MY_GTK_CLOCK (data));
}

void
about_cb (GtkWidget * widget, gpointer data)
{
  my_show_message (_("XFClock\n(c) 1997-2002 Olivier Fourdan"));
}
