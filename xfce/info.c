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
#include "xfce_main.h"
#include "xfce-common.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "gnome_protocol.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
info_private_ok_cb (GtkWidget * twidget)
{
  gtk_widget_hide (info_scr);
  gdk_window_withdraw ((GTK_WIDGET (info_scr))->window);
}

gboolean
info_private_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_widget_hide (info_scr);
  gdk_window_withdraw ((GTK_WIDGET (info_scr))->window);
  return (TRUE);
}

GtkWidget *
create_info (void)
{
  GtkWidget *info;
  GtkWidget *info_mainframe;
  GtkWidget *info_vbox;
  GtkWidget *info_topframe;
  GtkWidget *info_logo_pixmap;
  GtkWidget *info_centralframe;
  GtkWidget *info_copyright_label;
  GtkWidget *info_bottomframe;
  GtkWidget *info_scrolled_window;
  GtkWidget *info_license_text;
  GtkWidget *info_agreement_button;
  FILE *infile;

  info = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_position (GTK_WINDOW (info), GTK_WIN_POS_CENTER);
  gtk_widget_set_name (info, "info");
  gtk_object_set_data (GTK_OBJECT (info), "info", info);
  gtk_window_set_title (GTK_WINDOW (info), _("Info ..."));
  gtk_widget_realize (info);
  gnome_sticky (info->window);

  info_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (info_mainframe, "info_mainframe");
  gtk_object_set_data (GTK_OBJECT (info), "info_mainframe", info_mainframe);
  gtk_widget_show (info_mainframe);
  gtk_container_add (GTK_CONTAINER (info), info_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (info_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (info_mainframe), GTK_SHADOW_OUT);
#endif

  info_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (info_vbox, "info_vbox");
  gtk_object_set_data (GTK_OBJECT (info), "info_vbox", info_vbox);
  gtk_widget_show (info_vbox);
  gtk_container_add (GTK_CONTAINER (info_mainframe), info_vbox);

  info_topframe = gtk_frame_new (NULL);
  gtk_widget_set_name (info_topframe, "info_topframe");
  gtk_object_set_data (GTK_OBJECT (info), "info_topframe", info_topframe);
  gtk_widget_set_usize (info_topframe, 260, 100);
  gtk_widget_show (info_topframe);
  gtk_box_pack_start (GTK_BOX (info_vbox), info_topframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (info_topframe), 5);

  info_logo_pixmap = MyCreateFromPixmapFile (info, build_path (XFCE_LOGO));
  if (info_logo_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (info_logo_pixmap, "info_logo_pixmap");
  gtk_object_set_data (GTK_OBJECT (info), "info_logo_pixmap", info_logo_pixmap);
  gtk_widget_show (info_logo_pixmap);
  gtk_container_add (GTK_CONTAINER (info_topframe), info_logo_pixmap);

  info_centralframe = gtk_frame_new (NULL);
  gtk_widget_set_name (info_centralframe, "info_centralframe");
  gtk_object_set_data (GTK_OBJECT (info), "info_centralframe", info_centralframe);
  gtk_widget_show (info_centralframe);
  gtk_box_pack_start (GTK_BOX (info_vbox), info_centralframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (info_centralframe), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (info_centralframe), GTK_SHADOW_IN);

  info_copyright_label = gtk_label_new (_("By Olivier Fourdan (c) 1997-2002"));
  gtk_widget_set_name (info_copyright_label, "info_copyright_label");
  gtk_object_set_data (GTK_OBJECT (info), "info_copyright_label", info_copyright_label);
  gtk_widget_show (info_copyright_label);
  gtk_container_add (GTK_CONTAINER (info_centralframe), info_copyright_label);
  gtk_widget_set_usize (info_copyright_label, -2, 20);
  gtk_misc_set_padding (GTK_MISC (info_copyright_label), 10, 10);

  info_bottomframe = gtk_frame_new (NULL);
  gtk_widget_set_name (info_bottomframe, "info_bottomframe");
  gtk_object_set_data (GTK_OBJECT (info), "info_bottomframe", info_bottomframe);
  gtk_widget_show (info_bottomframe);
  gtk_box_pack_start (GTK_BOX (info_vbox), info_bottomframe, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (info_bottomframe), 5);

  info_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (info_bottomframe), info_scrolled_window);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (info_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_show (info_scrolled_window);

  info_license_text = gtk_text_new (NULL, NULL);
  gtk_widget_set_name (info_license_text, "info_license_text");
  gtk_object_set_data (GTK_OBJECT (info), "info_license_text", info_license_text);
  gtk_text_set_editable (GTK_TEXT (info_license_text), FALSE);
  gtk_widget_show (info_license_text);
  gtk_container_add (GTK_CONTAINER (info_scrolled_window), info_license_text);
  gtk_widget_set_usize (info_license_text, -2, 200);

  infile = fopen (build_path (XFCE_LICENSE), "r");

  gtk_text_freeze (GTK_TEXT (info_license_text));

  if (infile)
  {
    char *buffer;
    int nbytes_read, nbytes_alloc;

    nbytes_read = 0;
    nbytes_alloc = 1024;
    buffer = g_new (char, nbytes_alloc);
    while (1)
    {
      int len;
      if (nbytes_alloc < nbytes_read + 1024)
      {
	nbytes_alloc *= 2;
	buffer = g_realloc (buffer, nbytes_alloc);
      }
      len = fread (buffer + nbytes_read, 1, 1024, infile);
      nbytes_read += len;
      if (len < 1024)
	break;
    }

    gtk_text_insert (GTK_TEXT (info_license_text), NULL, NULL, NULL, buffer, nbytes_read);
    g_free (buffer);
    fclose (infile);
  }

  gtk_text_set_word_wrap (GTK_TEXT (info_license_text), TRUE);
  gtk_text_set_line_wrap (GTK_TEXT (info_license_text), TRUE);
  gtk_text_thaw (GTK_TEXT (info_license_text));

  info_agreement_button = gtk_button_new_with_label (_("Accept"));
  gtk_widget_set_name (info_agreement_button, "info_agreement_button");
  gtk_object_set_data (GTK_OBJECT (info), "info_agreement_button", info_agreement_button);
  gtk_widget_show (info_agreement_button);
  gtk_box_pack_start (GTK_BOX (info_vbox), info_agreement_button, FALSE, FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (info_agreement_button), 5);
  GTK_WIDGET_SET_FLAGS (info_agreement_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (info_agreement_button);

  gtk_signal_connect (GTK_OBJECT (info_agreement_button), "clicked", GTK_SIGNAL_FUNC (info_private_ok_cb), NULL);

  gtk_signal_connect (GTK_OBJECT (info), "delete_event", GTK_SIGNAL_FUNC (info_private_delete_event), NULL);

  return info;
}
