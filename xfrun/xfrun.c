/*  xfrun
 *  Copyright (C) 2000 Olivier Fourdan (fourdan@xfce.org)
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
#include <stdarg.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"
#include "xfcolor.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

typedef struct
{
  GtkWidget *top;
  GtkWidget *input;
  GtkWidget *check;
  int in_terminal;
}
dlg;

static dlg dl;
static int diag[2] = { -1, -1 };
/* DM: A pair of globals because I didn't want to pass these to the 
   'on_cancel' and 'on_ok' Fns
 */
gchar *historyfile;
GList *cbitems = NULL;

/* Fn prototypes */
gchar *set_history_file ();
GList *get_history (gchar * hfile);
void put_history (char *newest, gchar * hfile, GList * cb);

static void
on_cancel (GtkWidget * btn, gpointer * data)
{
  gtk_main_quit ();
}

/*
 */
static void
on_ok (GtkWidget * ok, gpointer data)
{
  char *cmd = NULL;
  char *temp = NULL;
  GtkWidget *entrytemp;

  diag[0] = -1;
  diag[1] = -1;
  entrytemp = GTK_COMBO (dl.input)->entry;
  temp = gtk_entry_get_text (GTK_ENTRY (entrytemp));
  dl.in_terminal = GTK_TOGGLE_BUTTON (dl.check)->active;

  if (temp && strlen (temp))
  {
    cmd = g_malloc (MAXSTRLEN + 1);
    if (dl.in_terminal)
      snprintf (cmd, MAXSTRLEN, "Term %s", temp);
    else
      snprintf (cmd, MAXSTRLEN, "%s", temp);
    exec_comm (cmd, 0);
    put_history (cmd, historyfile, cbitems);
    g_free (cmd);
  }
  gtk_main_quit ();
}

/*
  Sets history file to ~/.xfrun_history
  The returned value needs to be freed after use
*/
gchar *
set_history_file ()
{
  gchar *a, *result;
  gchar b[] = "/.xfce/xfrun_history";
  a = g_get_home_dir ();
  result = (char *) g_malloc ((strlen (a) + strlen (b) + 1) * sizeof (char));
  strcpy ((char *) result, (char *) a);
  strcat ((char *) result, (char *) b);
  return result;
}

/*
 */
GList *
get_history (gchar * hfile)
{
  gpointer xfrun_history;
  GList *cbtemp;
  xfrun_history = fopen ((char *) hfile, "r");
  cbtemp = NULL;
  if (xfrun_history != NULL)
  {
    int i = 0;
    char *line;
    char *check;
    check = NULL;
    line = g_malloc (MAXSTRLEN);
    while ((i++ < 10) && (fgets (line, MAXSTRLEN, xfrun_history) != NULL))
    {
      /* no more than 10 history items */
      if ((line[0] == '\0') || (line[0] == '\n'))
      {
	g_free (line);
	break;
      }
      else
      {
	/* If there is a newline, remove it from the end of the string */
	check = line;
	while (check[0] != '\0')
	{
	  check++;
	  if (check[0] == '\n')
	    check[0] = '\0';
	}
	/* Add the item to the list */
	cbtemp = g_list_append (cbtemp, line);
	line = NULL;
	check = NULL;
      }
      line = g_malloc (MAXSTRLEN);
    }				/* end for (i<10) */
    fclose (xfrun_history);
  }
  else
  {
    cbtemp = g_list_append (cbtemp, "");
  }
  return cbtemp;
}

/*
 */
void
put_history (char *newest, gchar * hfile, GList * cb)
{
  gpointer xfrun_history;
  GList *node;
  /* print the new ~/.xfap/xfrun_history file here */
  /* Finish print to file */
  xfrun_history = fopen ((char *) hfile, "w");
  if (xfrun_history != NULL)
  {
    fprintf (xfrun_history, "%s\n", newest);
    node = cb;
    while (node)
    {
      if ((strcmp ((char *) node->data, newest) != 0) && (((char *) node->data)[0] != '\0'))
      {
	fprintf (xfrun_history, "%s\n", (char *) node->data);
      }
      node = node->next;
    }				/* endwhile */
    fclose (xfrun_history);
  }				/* endif */
}

int
main (int argc, char *argv[])
{
  GtkWidget *ok = NULL, *cancel = NULL, *box, *bbox, *check;
  GtkAccelGroup *accel_group;

  xfce_init (&argc, &argv);

  dl.top = gtk_dialog_new ();
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dl.top), accel_group);
  gtk_container_border_width (GTK_CONTAINER (dl.top), 0);
  gtk_window_position (GTK_WINDOW (dl.top), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (dl.top), _("Run program ..."));
  gtk_window_set_modal (GTK_WINDOW (dl.top), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dl.top)->vbox), 5);

  ok = gtk_button_new_with_label (_("Ok"));
  cancel = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), bbox, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (bbox), ok);
  gtk_container_add (GTK_CONTAINER (bbox), cancel);

  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), box, TRUE, TRUE, 5);

  /* combo box, created by DM 10/4/00 */
  historyfile = set_history_file ();
  cbitems = get_history (historyfile);
  dl.input = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (dl.input), cbitems);
  gtk_box_pack_start (GTK_BOX (box), dl.input, TRUE, TRUE, 0);
  gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO (dl.input)->entry), 0, -1);
  gtk_combo_disable_activate (GTK_COMBO (dl.input));
  gtk_combo_set_case_sensitive (GTK_COMBO (dl.input), 1);
  gtk_combo_set_use_arrows (GTK_COMBO (dl.input), 1);
  gtk_combo_set_use_arrows_always (GTK_COMBO (dl.input), 1);

  /* check button */
  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), box, TRUE, TRUE, 5);

  dl.check = check = gtk_check_button_new_with_label (_("Open in terminal"));
  gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 5);

  gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (dl.input)->entry), "activate", GTK_SIGNAL_FUNC (on_ok), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (on_cancel), (gpointer) NULL);

  gtk_widget_add_accelerator (ok, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (cancel, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (cancel, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_show_all (dl.top);
  gtk_widget_grab_default (ok);
  gtk_widget_grab_focus (GTK_COMBO (dl.input)->entry);
  gtk_main ();
  g_list_free (cbitems);
  g_free (historyfile);
  xfce_end ((gpointer) NULL, 0);
  return (0);
}
