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


#ifndef __XFCE_COMMON_H__
#define __XFCE_COMMON_H__

#include <gtk/gtk.h>
#include <gtk/gtkfeatures.h>

#define MY_GDK_SCREEN()	DefaultScreen(GDK_DISPLAY())
#define my_alert(s) my_show_message(s)

/*
 * This function returns a widget in a component created by my.
 * Call it with the toplevel widget in the component (i.e. a window/dialog),
 * or alternatively any widget in the component, and the name of the widget
 * you want returned.
 */
GtkWidget *get_widget (GtkWidget * widget, gchar * widget_name);


 /*
  * This is an internally used function for setting notebook tabs. It is only
  * included in this header file so you don't get compilation warnings
  */
void set_notebook_tab (GtkWidget * notebook, gint page_num, GtkWidget * widget);

void lock_size (GtkWidget * toplevel);

void set_icon (GtkWidget *, gchar *, gchar **);

/* This shows a simple dialog box with a label and an 'OK' button.
   Example usage:
    my_show_message ("Error saving file");
 */
void my_show_message (gchar * message);

/* This creates a dialog box with a message and a number of buttons.
 * Signal handlers can be supplied for any of the buttons.
 * NOTE: The dialog is automatically destroyed when any button is clicked.
 * default_button specifies the default button, numbered from 1..
 * data is passed to the signal handler.

   Example usage:
     gchar *buttons[] = { "Yes", "No", "Cancel" };
     GtkSignalFunc signal_handlers[] = { on_yes, on_no, NULL };
     my_show_dialog ("Do you want to save the current project?",
			     3, buttons, 3, signal_handlers, NULL);
 */
GtkWidget *my_show_dialog (gchar * message, gint nbuttons, gchar * buttons[], gint default_button, GtkSignalFunc signal_handlers[], gpointer data);


gint my_yesno_dialog (gchar * message);

void refresh_screen (void);

char *build_path (char *);

void update_events (void);

void gnome_uri_list_free_strings (GList * list);

GList *gnome_uri_list_extract_uris (const gchar * uri_list);

GList *gnome_uri_list_extract_filenames (const gchar * uri_list);

void my_flush_events (void);

void cursor_wait (GtkWidget * widget);

void cursor_reset (GtkWidget * widget);

void cursor_hide(GtkWidget * widget);

gint my_get_adjustment_as_int (GtkAdjustment *);

void my_set_adjustment_bounds (GtkAdjustment *, gfloat, gfloat);

void terminate (char *fmt, ...);

void on_signal (int sig_num);

void signal_setup (void);

void xfce_init (int *, char **argv[]);

void xfce_end (gpointer, int);

#endif
