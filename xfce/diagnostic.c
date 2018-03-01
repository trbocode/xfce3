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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "constant.h"
#include "my_intl.h"
#include "xfce-common.h"
#include "configfile.h"
#include "diagicon.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GtkWidget *dialog_show_diag = NULL;
static GtkWidget *text;

static void
on_ok_show_diag (GtkWidget * widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (dialog_show_diag));
  gdk_window_withdraw ((GTK_WIDGET (dialog_show_diag))->window);
}

static void
on_clear_show_diag (GtkWidget * widget, gpointer data)
{
  guint lg;

  lg = gtk_text_get_length (GTK_TEXT (text));
  gtk_text_backward_delete (GTK_TEXT (text), lg);
}

static gboolean
delete_event_show_diag (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (dialog_show_diag));
  gdk_window_withdraw ((GTK_WIDGET (dialog_show_diag))->window);
  return (TRUE);
}

void
show_diag (gchar * message)
{
  GtkWidget *scrolled_window, *bbox, *button, *clear;

  if ((!message) || (!strlen (message)))
    return;

  if (dialog_show_diag != NULL)
  {
    if (current_config.show_diagnostic)
    {
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
      if (!GTK_WIDGET_VISIBLE (dialog_show_diag))
      {
	gtk_widget_show (dialog_show_diag);
      }
    }
    return;
  }

  dialog_show_diag = gtk_dialog_new ();
  gtk_container_border_width (GTK_CONTAINER (dialog_show_diag), 5);

  gtk_window_position (GTK_WINDOW (dialog_show_diag), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (dialog_show_diag), _("Subprocess diagnostic"));
  gtk_widget_realize (dialog_show_diag);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_show_diag)->vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_show (scrolled_window);

  text = gtk_text_new (NULL, NULL);
  gtk_widget_set_usize (text, 300, 100);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_show_diag)->action_area), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);

  clear = gtk_button_new_with_label (_("Clear"));
  gtk_container_add (GTK_CONTAINER (bbox), clear);
  GTK_WIDGET_SET_FLAGS (clear, GTK_CAN_DEFAULT);
  gtk_widget_show (clear);

  button = gtk_button_new_with_label (_("Dismiss"));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_signal_connect (GTK_OBJECT (clear), "clicked", GTK_SIGNAL_FUNC (on_clear_show_diag), GTK_WIDGET (dialog_show_diag));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok_show_diag), GTK_WIDGET (dialog_show_diag));
  gtk_signal_connect (GTK_OBJECT (dialog_show_diag), "delete_event", GTK_SIGNAL_FUNC (delete_event_show_diag), GTK_WIDGET (dialog_show_diag));
  set_icon (dialog_show_diag, _("Subprocess diagnostic"), diagicon);

  if (current_config.show_diagnostic)
  {
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
    gtk_widget_show (dialog_show_diag);
  }
}

gboolean
process_diag_messages (gpointer client_data, gint source, GdkInputCondition condition)
{
  char buffer[MAXSTRLEN];
  size_t elts;

  elts = read (source, buffer, MAXSTRLEN - 1);
  if ((int) elts > 0)
  {
    buffer[elts] = '\0';
    show_diag ((gchar *) buffer);
    /* printf ("Diag : %s\n",  buffer); */
  }
  return (1);
}
