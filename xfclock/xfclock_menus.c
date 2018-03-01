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
#include "my_intl.h"
#include "mygtkclock.h"
#include "xfce-common.h"
#include "xfclock_cb.h"
#include "xfclock_menus.h"
#include "xfclock_config.h"
#include "xfclock_opt.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

GtkWidget *
clock_make_popup (MyGtkClock * clockw)
{
  GtkWidget *menuitem;

  if (!clock_popup_menu)
  {
    clock_popup_menu = gtk_menu_new ();
    if (!clock_popup_menu)
      return (GtkWidget *) NULL;
  }

  menuitem = gtk_menu_item_new_with_label (_("Hide/show menubar"));

  gtk_menu_append (GTK_MENU (clock_popup_menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_menubar_cb), (gpointer) clockw);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Hide/show calendar"));

  gtk_menu_append (GTK_MENU (clock_popup_menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_calendar_cb), (gpointer) clockw);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();

  gtk_menu_append (GTK_MENU (clock_popup_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Quit"));
  gtk_menu_append (GTK_MENU (clock_popup_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (quit_cb), (gpointer) clockw);
  return clock_popup_menu;
}

/* clock widget *has to* be created and properly configured when create_menu is called */
GtkWidget *
create_menu (MyGtkClock * clockw)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  menubar = gtk_menu_bar_new ();
  gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
  gtk_widget_show (menubar);

  /* Create "File" menu */
  menuitem = gtk_menu_item_new_with_label (_("File"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  menuitem = gtk_menu_item_new_with_label (_("Quit"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (quit_cb), (gpointer) clockw);
  /* Create "Settings" menu */
  menuitem = gtk_menu_item_new_with_label (_("Settings"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("Hide/show calendar"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_calendar_cb), (gpointer) clockw);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Digital mode"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (my_gtk_clock_get_mode (MY_GTK_CLOCK (clockw)) == MY_GTK_CLOCK_DIGITAL);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_digital_cb), (gpointer) clockw);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("24 hour mode"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = my_gtk_clock_military_shown (MY_GTK_CLOCK (clockw));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_military_cb), (gpointer) clockw);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Display AM/PM"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = my_gtk_clock_ampm_shown (MY_GTK_CLOCK (clockw));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_ampm_cb), (gpointer) clockw);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Display seconds"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = my_gtk_clock_secs_shown (MY_GTK_CLOCK (clockw));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_secs_cb), (gpointer) clockw);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Background color..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (background_cb), (gpointer) clockw);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Foreground color..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (foreground_cb), (gpointer) clockw);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Font..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (font_cb), (gpointer) clockw);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Hide menubar"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_menubar_cb), (gpointer) clockw);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Create "Calendar options" menu */
  menuitem = gtk_menu_item_new_with_label (_("Calendar Options"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_label (_("Mark Fridays"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.holy_days & 0x04) == 0x04);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_friday_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Mark Saturdays"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.holy_days & 0x02) == 0x02);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_saturday_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Mark Sundays"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.holy_days & 0x01) == 0x01);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (toggle_sunday_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Show week numbers"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.calendar_opt & GTK_CALENDAR_SHOW_WEEK_NUMBERS) == GTK_CALENDAR_SHOW_WEEK_NUMBERS);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (show_weeks_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Show day names"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.calendar_opt & GTK_CALENDAR_SHOW_DAY_NAMES) == GTK_CALENDAR_SHOW_DAY_NAMES);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (show_days_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Show heading"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.calendar_opt & GTK_CALENDAR_SHOW_HEADING) == GTK_CALENDAR_SHOW_HEADING);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (show_heading_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_label (_("Week starts on Monday"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = ((current_config.calendar_opt & GTK_CALENDAR_WEEK_START_MONDAY) == GTK_CALENDAR_WEEK_START_MONDAY);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (start_monday_cb), NULL);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Create "Help" menu */
  menuitem = gtk_menu_item_new_with_label (_("Help"));
  gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  menuitem = gtk_menu_item_new_with_label (_("About..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (about_cb), NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  return menubar;
}
