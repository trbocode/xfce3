/*  xfmouse
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
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_GDK_IMLIB
#include <fcntl.h>
#include <gdk_imlib.h>
#endif

#include "my_intl.h"
#include "xfmouse_cb.h"
#include "xfmouse.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfmouse.xpm"
#include "xfmouse_icon.xpm"

void
mouse_values (XFMouse * s)
{
  unsigned char map[5] = "\0\0\0\0\0";
  unsigned int buttons = 0;
  Display *tmpDpy;

  if (!(tmpDpy = XOpenDisplay ("")))
  {
    fprintf (stderr, _("xfmouse: Error, cannot open display.\n"));
    fprintf (stderr, _("Is X running and $DISPLAY correctly set ?\n"));
    return;
  }
  XSync (tmpDpy, False);
  XChangePointerControl (tmpDpy, 1, 1, s->accel, DENOMINATOR, s->thresh);
  buttons = XGetPointerMapping (tmpDpy, map, 5);
  if (s->button)
  {
    if (buttons > 2)
    {
      map[0] = 1;
      map[1] = 2;
      map[2] = 3;
    }
    else
    {
      map[0] = 2;
      map[1] = 1;
    }
  }
  else
  {
    if (buttons > 2)
    {
      map[0] = 3;
      map[1] = 2;
      map[2] = 1;
    }
    else
    {
      map[0] = 1;
      map[1] = 2;
    }
  }
  XSetPointerMapping (tmpDpy, map, buttons);
  XFlush (tmpDpy);
  XCloseDisplay (tmpDpy);
}

void
apply_mouse_values (XFMouse * s)
{
  s->button = (int) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (rightbtn));
  s->accel = (int) GTK_ADJUSTMENT (accel)->value;
  s->thresh = (int) GTK_ADJUSTMENT (thresh)->value;
  mouse_values (s);
}

GtkWidget *
create_xfmouse ()
{
  GtkWidget *xfmouse_mainframe;
  GtkWidget *vbox1;
  GtkWidget *frame2;
  GtkWidget *vbox2;
  GtkWidget *frame4;
  GtkWidget *hbox3;
  GtkWidget *pix_frame;
  GtkWidget *pixmap1;
  GSList *hbox3_group = NULL;
  GtkWidget *frame5;
  GtkWidget *table1;
  GtkWidget *hscale3;
  GtkWidget *hscale4;
  GtkWidget *label3;
  GtkWidget *label2;
  GtkWidget *frame3;
  GtkWidget *hbuttonbox1;
  GtkWidget *ok_button;
  GtkWidget *apply_button;
  GtkWidget *cancel_button;
  GtkAccelGroup *accel_group;

  xfmouse = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (xfmouse, "xfmouse");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "xfmouse", xfmouse);
  gtk_widget_set_usize (xfmouse, 340, -2);
  gtk_window_set_title (GTK_WINDOW (xfmouse), _("XFMouse - XFce Mouse Configuration"));
  gtk_window_position (GTK_WINDOW (xfmouse), GTK_WIN_POS_CENTER);

  xfmouse_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (xfmouse_mainframe, "xfmouse_mainframe");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "xfmouse_mainframe", xfmouse_mainframe);
  gtk_widget_show (xfmouse_mainframe);
  gtk_container_add (GTK_CONTAINER (xfmouse), xfmouse_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (xfmouse_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (xfmouse_mainframe), GTK_SHADOW_OUT);
#endif

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (xfmouse_mainframe), vbox1);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame2, "frame2");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "frame2", frame2);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox1), frame2, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame2), 5);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame2), vbox2);

  frame4 = gtk_frame_new (_("Button Settings"));
  gtk_widget_set_name (frame4, "frame4");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "frame4", frame4);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (vbox2), frame4, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame4), 5);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox3, "hbox3");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "hbox3", hbox3);
  gtk_widget_show (hbox3);
  gtk_container_add (GTK_CONTAINER (frame4), hbox3);

  pix_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (pix_frame, "pix_frame");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "pix_frame", pix_frame);
  gtk_widget_set_usize (pix_frame, 50, 60);
  gtk_frame_set_shadow_type (GTK_FRAME (pix_frame), GTK_SHADOW_NONE);
  gtk_widget_show (pix_frame);
  gtk_box_pack_start (GTK_BOX (hbox3), pix_frame, TRUE, FALSE, 0);

  pixmap1 = MyCreateFromPixmapData (xfmouse, xfmouse_xpm);
  if (pixmap1 == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (pixmap1, "pixmap1");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "pixmap1", pixmap1);
  gtk_widget_show (pixmap1);
  gtk_container_add (GTK_CONTAINER (pix_frame), pixmap1);

  leftbtn = gtk_radio_button_new_with_label (hbox3_group, _("Left"));
  hbox3_group = gtk_radio_button_group (GTK_RADIO_BUTTON (leftbtn));
  gtk_widget_set_name (leftbtn, "leftbtn");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "leftbtn", leftbtn);
  gtk_widget_show (leftbtn);
  gtk_box_pack_start (GTK_BOX (hbox3), leftbtn, TRUE, TRUE, 0);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (xfmouse), accel_group);
  gtk_widget_add_accelerator (leftbtn, "toggled", accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  rightbtn = gtk_radio_button_new_with_label (hbox3_group, _("Right"));
  hbox3_group = gtk_radio_button_group (GTK_RADIO_BUTTON (rightbtn));
  gtk_widget_set_name (rightbtn, "rightbtn");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "rightbtn", rightbtn);
  gtk_widget_show (rightbtn);
  gtk_box_pack_start (GTK_BOX (hbox3), rightbtn, TRUE, TRUE, 0);
  gtk_widget_add_accelerator (rightbtn, "toggled", accel_group, GDK_r, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (rightbtn), TRUE);

  frame5 = gtk_frame_new (_("Motion Settings"));
  gtk_widget_set_name (frame5, "frame5");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "frame5", frame5);
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox2), frame5, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame5), 5);

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_set_name (table1, "table1");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "table1", table1);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame5), table1);

  hscale3 = gtk_hscale_new (GTK_ADJUSTMENT (accel = gtk_adjustment_new (6, 1, 30, 1, 1, 1)));
  gtk_widget_set_name (hscale3, "hscale3");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "hscale3", hscale3);
  gtk_widget_show (hscale3);
  gtk_table_attach (GTK_TABLE (table1), hscale3, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_digits (GTK_SCALE (hscale3), 0);

  hscale4 = gtk_hscale_new (GTK_ADJUSTMENT (thresh = gtk_adjustment_new (4, 1, 20, 1, 1, 1)));
  gtk_widget_set_name (hscale4, "hscale4");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "hscale4", hscale4);
  gtk_widget_show (hscale4);
  gtk_table_attach (GTK_TABLE (table1), hscale4, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_digits (GTK_SCALE (hscale4), 0);

  label3 = gtk_label_new (_("Threshold : "));
  gtk_widget_set_name (label3, "label3");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "label3", label3);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 1, 2, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label3), 5, 0);

  label2 = gtk_label_new (_("Acceleration : "));
  gtk_widget_set_name (label2, "label2");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "label2", label2);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 0, 1, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label2), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label2), 5, 0);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame3, "frame3");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "frame3", frame3);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame3), 5);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "hbuttonbox1", hbuttonbox1);
  gtk_widget_show (hbuttonbox1);
  gtk_container_add (GTK_CONTAINER (frame3), hbuttonbox1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);

  ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (ok_button, "ok_button");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), ok_button);
  gtk_container_border_width (GTK_CONTAINER (ok_button), 5);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_set_name (apply_button, "apply_button");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "apply_button", apply_button);
  gtk_widget_show (apply_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), apply_button);
  gtk_container_border_width (GTK_CONTAINER (apply_button), 5);
  GTK_WIDGET_SET_FLAGS (apply_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (apply_button, "clicked", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (cancel_button, "cancel_button");
  gtk_object_set_data (GTK_OBJECT (xfmouse), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), cancel_button);
  gtk_container_border_width (GTK_CONTAINER (cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (apply_button), "clicked", GTK_SIGNAL_FUNC (apply_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (xfmouse), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

  set_icon (xfmouse, "XFMouse", xfmouse_xpm);
  return xfmouse;
}

void
show_xfmouse (XFMouse * s)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rightbtn), (s->button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (leftbtn), !(s->button));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (accel), (gfloat) s->accel);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (thresh), (gfloat) s->thresh);
  gtk_widget_show (xfmouse);
}


void
readstr (int i, char *str, FILE * f)
{
  if (str)
  {
    if ((f) && (fgets (str, i - 1, f)) && (strlen (str)))
    {
      str[strlen (str) - 1] = '\0';
    }
    else
    {
      strcpy (str, "\0");
    }
  }
}

void
loadcfg (XFMouse * s)
{
  FILE *f = NULL;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);
  if (existfile (homedir))
  {
    f = fopen (homedir, "r");
  }
  else
  {
    snprintf (homedir, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, RCFILE);
    if (existfile (homedir))
    {
      f = fopen (homedir, "r");
    }
  }

  if (f)
  {
    readstr (10, tempstr, f);
    s->button = (my_strncasecmp (tempstr, "Right", strlen ("Right")) == 0);
    readstr (10, tempstr, f);
    s->accel = atoi (tempstr);
    readstr (10, tempstr, f);
    s->thresh = atoi (tempstr);
    if (s->accel < ACCEL_MIN)
      s->accel = ACCEL_MIN;
    else if (s->accel > ACCEL_MAX)
      s->accel = ACCEL_MIN;
    if (s->thresh < THRESH_MIN)
      s->thresh = THRESH_MIN;
    else if (s->thresh > THRESH_MAX)
      s->thresh = THRESH_MIN;
    fclose (f);
  }
  else
  {
    s->button = 1;
    s->accel = 2 * DENOMINATOR;
    s->thresh = 4;
  }
}

int
savecfg (XFMouse * s)
{
  FILE *f;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);

  if ((f = fopen (homedir, "w")))
  {
    fprintf (f, "%s\n", ((s->button) ? "Right" : "Left"));
    fprintf (f, "%i\n", s->accel);
    fprintf (f, "%i\n", s->thresh);
    fclose (f);
  }
  return ((f != NULL));
}

int
main (int argc, char *argv[])
{
  signal_setup ();

  homedir = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
  tempstr = (char *) malloc (16 * sizeof (char));
  loadcfg (&mouseval);

  if (argc == 2 && strcmp (argv[1], "-i") == 0)
  {
    xfce_init (&argc, &argv);

    xfmouse = create_xfmouse ();
    set_icon (xfmouse, "XFMouse", xfmouse_icon_xpm);
    show_xfmouse (&mouseval);
    gtk_main ();
    xfce_end ((gpointer) NULL, 0);
  }
  if ((argc == 1) || (argc == 2 && strcmp (argv[1], "-d") == 0))
  {
    mouse_values (&mouseval);
    return (0);
  }

  fprintf (stderr, _("Usage : %s [OPTIONS]\n"), argv[0]);
  fprintf (stderr, _("   Where OPTIONS are :\n"));
  fprintf (stderr, _("   -i : interactive\n"));
  fprintf (stderr, _("   -d : apply configuration and exit (default)\n\n"));
  fprintf (stderr, _("%s is part of the XFce distribution, written by Olivier Fourdan\n\n"), argv[0]);

  return (0);
}
