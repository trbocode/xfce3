/******************************************************************************
 * xfbdmgr - a Random Backdrop list manager for XFCE
 * xfbdmgr (c) 2000 by Alex Lambert (alex@penwing.uklinux.net)
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
 *****************************************************************************/

/******************************************************************************
 * xfbdmgr - a Random Backdrop List Manager for XFCE.
 * This program manages lists for xfbd when operating in random mode. 
 *****************************************************************************/

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<stdio.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#include "my_intl.h"
#include "xfbdmgr_cb.h"
#include "xfbdmgr.h"
#include "constant.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

enum
{
  TARGET_STRING,
  TARGET_ROOTWIN,
  TARGET_URL
};

static GtkTargetEntry xfbdmgr_target_table[] = {
  {"STRING", 0, TARGET_STRING},
  {"text/plain", 0, TARGET_STRING},
  {"text/uri-list", 0, TARGET_URL},
};

static guint n_xfbdmgr_targets = sizeof (xfbdmgr_target_table) / sizeof (xfbdmgr_target_table[0]);

GtkWidget *
create_xfbdmgr ()
{
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *hbox1;
  GtkWidget *hbuttonbox1;
  GtkWidget *scroll_window;

  GtkWidget *label_name;
  GtkWidget *save_button;
  GtkWidget *load_button;
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *exit_button;

  xfbdmgr = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (xfbdmgr, "xfbdmgr");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "xfbdmgr", xfbdmgr);
  gtk_window_set_title (GTK_WINDOW (xfbdmgr), _("xfbdmgr - XFCE List Manager"));
  /*  gtk_window_posistion(GTK_WINDOW(xfbdmgr), GTK_WIN_POS_CENTER); */

  frame1 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame1, "frame1");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "frame1", frame1);
  gtk_widget_show (frame1);
  gtk_container_add (GTK_CONTAINER (xfbdmgr), frame1);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
#endif

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (frame1), vbox1);

  scroll_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scroll_window, "scroll_window");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "scroll_window", scroll_window);
  gtk_widget_show (scroll_window);
  gtk_box_pack_start (GTK_BOX (vbox1), scroll_window, TRUE, TRUE, 0);
  gtk_widget_set_usize (scroll_window, 300, 300);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, TRUE, 0);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 5);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame2, "frame2");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "frame2", frame2);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox2), frame2, TRUE, TRUE, 3);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame2), 5);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "hbuttonbox1", hbuttonbox1);
  gtk_widget_show (hbuttonbox1);
  gtk_container_add (GTK_CONTAINER (frame2), hbuttonbox1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 0, 3);
  gtk_container_border_width (GTK_CONTAINER (hbuttonbox1), 5);

  label_name = gtk_label_new (_("Filename : "));
  gtk_widget_set_name (label_name, "label_name");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "label_name", label_name);
  gtk_widget_show (label_name);
  gtk_box_pack_start (GTK_BOX (hbox1), label_name, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label_name), 1, 0.5);

  edit_name = gtk_entry_new ();
  gtk_widget_set_name (edit_name, "edit_name");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "edit_name", edit_name);
  gtk_widget_show (edit_name);
  gtk_box_pack_start (GTK_BOX (hbox1), edit_name, TRUE, TRUE, 5);

  save_button = gtk_button_new_with_label (_("Save ..."));
  gtk_widget_set_name (save_button, "save_button");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "save_button", save_button);
  gtk_widget_show (save_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), save_button);

  load_button = gtk_button_new_with_label (_("Load ..."));
  gtk_widget_set_name (load_button, "load_button");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "load_button", load_button);
  gtk_widget_show (load_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), load_button);

  add_button = gtk_button_new_with_label (_("Add Item ..."));
  gtk_widget_set_name (add_button, "add_button");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "add_button", add_button);
  gtk_widget_show (add_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), add_button);

  remove_button = gtk_button_new_with_label (_("Remove"));
  gtk_widget_set_name (remove_button, "remove_button");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "remove_button", remove_button);
  gtk_widget_show (remove_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), remove_button);

  exit_button = gtk_button_new_with_label (_("Quit"));
  gtk_widget_set_name (remove_button, "exit_button");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "exit_button", exit_button);
  gtk_widget_show (exit_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), exit_button);

  clist_fileslist = gtk_clist_new (1);
  gtk_widget_set_name (clist_fileslist, "clist_fileslist");
  gtk_object_set_data (GTK_OBJECT (xfbdmgr), "clist_fileslist", clist_fileslist);
  gtk_widget_show (clist_fileslist);
  gtk_clist_set_column_width (GTK_CLIST (clist_fileslist), 0, 300);
  gtk_container_add (GTK_CONTAINER (scroll_window), clist_fileslist);

  gtk_drag_dest_set (clist_fileslist, GTK_DEST_DEFAULT_ALL, xfbdmgr_target_table, n_xfbdmgr_targets, GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gtk_signal_connect (GTK_OBJECT (clist_fileslist), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (edit_name), "changed", GTK_SIGNAL_FUNC (edit_changed), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (xfbdmgr), "destroy", GTK_SIGNAL_FUNC (Myquit), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (save_button), "clicked", GTK_SIGNAL_FUNC (save_clicked), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (load_button), "clicked", GTK_SIGNAL_FUNC (load_clicked), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (add_button), "clicked", GTK_SIGNAL_FUNC (add_clicked), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (remove_button), "clicked", GTK_SIGNAL_FUNC (remove_clicked), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (exit_button), "clicked", GTK_SIGNAL_FUNC (Myquit), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (clist_fileslist), "select_row", GTK_SIGNAL_FUNC (selectionMade), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (clist_fileslist), "unselect_row", GTK_SIGNAL_FUNC (selectionLost), (gpointer) NULL);
  return xfbdmgr;
}


void
writeToList ()
{
  char *filename;
  gchar *listentry;
  FILE *theFile;

  gtk_clist_clear (GTK_CLIST (clist_fileslist));

  Changed = FALSE;

  theNumber = 0;

  filename = malloc (sizeof (char) * MAXSTRLEN);
  listentry = malloc (sizeof (gchar) * MAXSTRLEN);

  strcpy (filename, gtk_entry_get_text (GTK_ENTRY (edit_name)));
  filename[strlen (filename)] = 0;
  gtk_clist_freeze (GTK_CLIST (clist_fileslist));
  if ((theFile = fopen (filename, "r")) != NULL)
  {
    if ((fgets (listentry, 22, theFile)) && (strncmp (listentry, "# xfce backdrop list", 20) == 0))
    {
      while (!feof (theFile))
      {
	char *j;
	theNumber++;
	j = fgets (listentry, MAXSTRLEN, theFile);
	if ((j) && (strlen (listentry)))
	{
	  listentry[strlen (listentry) - 1] = '\0';
	  gtk_clist_append (GTK_CLIST (clist_fileslist), &listentry);
	}
      }
      fclose (theFile);
    }
    else
    {
      gtk_entry_set_text (GTK_ENTRY (edit_name), "");
    }
  }
  else
  {
    fprintf (stderr, _("File not found.\n"));;
  }
  gtk_clist_thaw (GTK_CLIST (clist_fileslist));
  free (filename);		/* Is OK to free because filename has a COPY of what is in 
				 * edit_name - It does not point to the same place */
  free (listentry);

}

void
saveList ()
{
  FILE *theFile;
  char *tempStr = NULL;
  gchar *dummy = NULL;
  int i;

  Changed = FALSE;

  if ((theFile = fopen (gtk_entry_get_text (GTK_ENTRY (edit_name)), "w")) != NULL)
  {
    fputs ("# xfce backdrop list\n", theFile);
    tempStr = malloc (sizeof (gchar) * MAXSTRLEN);
    for (i = 0; i < theNumber; i++)
    {
      gtk_clist_get_text (GTK_CLIST (clist_fileslist), i, 0, (gchar **) & dummy);
      strcpy (tempStr, dummy);
      strcat (tempStr, "\n\0");
      fputs (tempStr, theFile);
    }
    fclose (theFile);
    free (tempStr);
  }
  else
  {
    GtkWidget *dialogWindow;
    GtkWidget *message;
    GtkWidget *ok_button;

    dialogWindow = gtk_dialog_new ();
    message = gtk_label_new ("File Error");
    ok_button = gtk_button_new_with_label ("OK");
    gtk_signal_connect_object (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (dialogWindow));
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialogWindow)->action_area), ok_button);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialogWindow)->vbox), message);
    gtk_widget_show_all (dialogWindow);
  }
}

int
main (int argc, char **argv)
{
  xfce_init (&argc, &argv);

  Changed = FALSE;

  xfbdmgr = create_xfbdmgr ();
  selected = -1;
  if (argc == 2)
  {
    gtk_entry_set_text (GTK_ENTRY (edit_name), argv[1]);
    writeToList ();
  }

  gtk_widget_show (xfbdmgr);
  gtk_main ();
  xfce_end ((gpointer) NULL, 0);
  return 0;
}
