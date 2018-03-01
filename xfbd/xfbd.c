/*  xfbd
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#ifdef HAVE_GDK_IMLIB
#include <fcntl.h>
#include <gdk_imlib.h>
#else
#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#endif

#include "my_intl.h"
#include "xfbd_cb.h"
#include "xfbd.h"
#include "constant.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"
#include "empty.h"

#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#include <X11/extensions/Xinerama.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfbd_icon.xpm"

enum
{
  TARGET_STRING,
  TARGET_ROOTWIN,
  TARGET_URL
};

static GtkTargetEntry xfbd_target_table[] = {
  {"STRING", 0, TARGET_STRING},
  {"text/plain", 0, TARGET_STRING},
  {"text/uri-list", 0, TARGET_URL},
};

static guint n_xfbd_targets = sizeof (xfbd_target_table) / sizeof (xfbd_target_table[0]);

#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H)
XineramaScreenInfo *xinerama_infos;
int xinerama_heads;
Bool enable_xinerama;
#endif

void
display_error (char *s)
{
  fprintf (stderr, "%s\n", s);
  my_alert (s);
}

int
getdisplaywidth ()
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H)
  static int dpw = -1;
  int head;

  if (dpw < 0)
  {
    if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    {
      /* Xinerama extensions are disabled */
      dpw = (int) gdk_screen_width ();
    }
    else
    {
      for (head = 0; head < xinerama_heads; head++)
      {
	if (xinerama_infos[head].width > dpw)
	{
	  dpw = xinerama_infos[head].width;
	}
      }
    }
  }
  return dpw;
#else
  return ((int) gdk_screen_width ());
#endif
}

int
getdisplayheight ()
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H)
  static int dph = -1;
  int head;

  if (dph < 0)
  {
    if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    {
      /* Xinerama extensions are disabled */
      dph = (int) gdk_screen_height ();
    }
    else
    {
      for (head = 0; head < xinerama_heads; head++)
      {
	if (xinerama_infos[head].height > dph)
	{
	  dph = xinerama_infos[head].height;
	}
      }
    }
  }
  return dph;
#else
  return ((int) gdk_screen_height ());
#endif
}

int
setroot (char *passedstr, char *tiled)
{
  char *str = NULL;
#ifdef HAVE_GDK_IMLIB
  GdkImlibImage *im;
  GdkPixmapPrivate *pixmap_private;
  GdkPixmap *rootXpm = 0;
  Pixmap xpixmap;
#else
#ifdef HAVE_GDK_PIXBUF
  GdkPixmapPrivate *pixmap_private;
  GdkPixbuf *Pixbuf;
  GdkPixbuf *temp;
  GdkPixmap *rootXpm = 0;
  GdkBitmap *shapeMask = 0;
  Pixmap xpixmap;
#else
  Display *tmpDpy;
  XWindowAttributes root_attr;
  int val;
  Pixmap rootXpm = 0;
  Pixmap shapeMask = 0;
  XpmAttributes xpm_attributes;
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */

  int pix_width = 0;
  int pix_height = 0;

  str = malloc (sizeof (char) * MAXSTRLEN);

  select_backdrp (passedstr, str);

  if (my_strncasecmp (str, NOBACK, strlen (NOBACK)) && existfile (str))
  {
#ifdef HAVE_GDK_IMLIB
    im = gdk_imlib_load_image (str);
    if (im)
    {
      if (!strcmp (tiled, "stretched"))
      {
	pix_width = getdisplaywidth ();
	pix_height = getdisplayheight ();
      }
      else if (!strcmp (tiled, "tiled"))
      {
	pix_width = im->rgb_width;
	pix_height = im->rgb_height;
      }
      else if ((im->rgb_width > (getdisplaywidth () / 2)) && (im->rgb_height > (getdisplayheight () / 2)))
      {
	pix_width = getdisplaywidth ();
	pix_height = getdisplayheight ();
      }
      else
      {
	pix_width = im->rgb_width;
	pix_height = im->rgb_height;
      }

      gdk_imlib_render (im, pix_width, pix_height);
      rootXpm = gdk_imlib_copy_image (im);
      gdk_window_set_back_pixmap (GDK_ROOT_PARENT (), rootXpm, 0);
      pixmap_private = (GdkPixmapPrivate *) rootXpm;
      xpixmap = duppix (pixmap_private->xwindow, pix_width, pix_height);
      setPixmapProperty (xpixmap);
      gdk_window_clear (GDK_ROOT_PARENT ());
      gdk_flush ();
      gdk_imlib_destroy_image (im);
    }
#else
#ifdef HAVE_GDK_PIXBUF
    Pixbuf = gdk_pixbuf_new_from_file (str);
    if (Pixbuf)
    {
      if (!strcmp (tiled, "stretched"))
      {
	pix_width = getdisplaywidth ();
	pix_height = getdisplayheight ();
      }
      else if (!strcmp (tiled, "tiled"))
      {
	pix_width = gdk_pixbuf_get_width (Pixbuf);
	pix_height = gdk_pixbuf_get_height (Pixbuf);
      }
      else if ((gdk_pixbuf_get_width (Pixbuf) > (getdisplaywidth () / 2)) && (gdk_pixbuf_get_height (Pixbuf) > (getdisplayheight () / 2)))
      {
	pix_width = getdisplaywidth ();
	pix_height = getdisplayheight ();
      }
      else
      {
	pix_width = gdk_pixbuf_get_width (Pixbuf);
	pix_height = gdk_pixbuf_get_height (Pixbuf);
      }
      temp = gdk_pixbuf_scale_simple (Pixbuf, pix_width, pix_height, GDK_INTERP_BILINEAR);
      gdk_pixbuf_render_pixmap_and_mask (temp, &rootXpm, &shapeMask, 0);
      gdk_window_set_back_pixmap (GDK_ROOT_PARENT (), rootXpm, 0);
      pixmap_private = (GdkPixmapPrivate *) rootXpm;
      xpixmap = duppix (pixmap_private->xwindow, pix_width, pix_height);
      setPixmapProperty (xpixmap);
      gdk_window_clear (GDK_ROOT_PARENT ());
      gdk_flush ();
      gdk_pixbuf_unref (Pixbuf);
      gdk_pixbuf_unref (temp);
    }
#else
    if (!(tmpDpy = XOpenDisplay ("")))
      return 0;

    XGetWindowAttributes (tmpDpy, GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), &root_attr);
    xpm_attributes.colormap = root_attr.colormap;
    xpm_attributes.valuemask = XpmSize | XpmReturnPixels | XpmColormap;
    if ((val = XpmReadFileToPixmap (tmpDpy, GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), str, &rootXpm, &shapeMask, &xpm_attributes)) != XpmSuccess)
    {
      if (val == XpmOpenFailed)
	display_error (_("Error\nCannot open file"));
      else if (val == XpmColorFailed)
	display_error (_("Error\nCannot allocate colors"));
      else if (val == XpmFileInvalid)
	display_error (_("Error\nFile format invalid"));
      else if (val == XpmColorError)
	display_error (_("Error\nColor definition invalid"));
      else if (val == XpmNoMemory)
	display_error (_("Error\nNot enough Memory"));
      return (0);
    }
    pix_width = xpm_attributes.width;
    pix_height = xpm_attributes.height;
    XSetWindowBackgroundPixmap (tmpDpy, GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), rootXpm);
    setPixmapProperty (rootXpm);
    XClearWindow (tmpDpy, GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()));
    XpmFreeAttributes (&xpm_attributes);
    XSetCloseDownMode (tmpDpy, RetainPermanent);
    XCloseDisplay (tmpDpy);
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */
  }
  free (str);
  return (1);
}

void
readstr (char *str, char *tiled)
{
  char *homedir;
  FILE *f = NULL;

  homedir = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);
  if (existfile (homedir))
  {
    f = fopen (homedir, "r");
  }
  else
  {
    fprintf (stderr, "%s File not found.\n", homedir);
    snprintf (homedir, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, RCFILE);
    if (existfile (homedir))
    {
      f = fopen (homedir, "r");
    }
  }
  if (f)
  {
    if (!feof (f))
      fgets (str, MAXSTRLEN + 1, f);
    if (strlen (str))
      str[strlen (str) - 1] = 0;
    if (!feof (f))
      fgets (tiled, MAXSTRLEN + 1, f);
    if (strlen (tiled))
      tiled[strlen (tiled) - 1] = 0;
    fclose (f);
  }
  else
  {
    strcpy (str, NOBACK);
    strcpy (tiled, "auto");
  }
  free (homedir);
}

int
writestr (char *str, char *tiled)
{
  char *homedir;
  FILE *f;

  homedir = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);

  if ((f = fopen (homedir, "w")))
  {
    fprintf (f, "%s\n", str);
    fprintf (f, "%s\n", tiled);
    fclose (f);
  }
  free (homedir);
  return ((f != NULL));
}

void
display_back (char *str)
{
  char *bkdrop;
  bkdrop = malloc (sizeof (char) * MAXSTRLEN);
  if ((str) && (my_strncasecmp (str, NOBACK, strlen (NOBACK)) && existfile (str)))
  {
    select_backdrp (str, bkdrop);
    MySetPixmapFile (preview_pixmap, preview_pixmap_frame, bkdrop);
  }
  else
  {
    MySetPixmapData (preview_pixmap, preview_pixmap_frame, empty);
  }
  free (bkdrop);
}

GtkWidget *
create_xfbd ()
{
  GtkWidget *frame1;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *tiled_hbox;
  GSList *tiled_hbox_group = NULL;
  GtkWidget *frame3;
  GtkWidget *hbuttonbox1;
  GtkWidget *ok_button;
  GtkWidget *browse_button;
  GtkWidget *apply_button;
  GtkWidget *clear_button;
  GtkWidget *cancel_button;
  GtkAccelGroup *accel_group;

  xfbd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (xfbd, "xfbd");
  gtk_object_set_data (GTK_OBJECT (xfbd), "xfbd", xfbd);
  gtk_window_set_title (GTK_WINDOW (xfbd), _("XFbd - XFce Backdrop Manager"));
  gtk_window_position (GTK_WINDOW (xfbd), GTK_WIN_POS_CENTER);

  frame1 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame1, "frame1");
  gtk_object_set_data (GTK_OBJECT (xfbd), "frame1", frame1);
  gtk_widget_show (frame1);
  gtk_container_add (GTK_CONTAINER (xfbd), frame1);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
#endif

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (xfbd), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (frame1), vbox1);

  preview_pixmap_frame = gtk_frame_new (NULL);
  gtk_widget_set_name (preview_pixmap_frame, "preview_pixmap_frame");
  gtk_object_set_data (GTK_OBJECT (xfbd), "preview_pixmap_frame", preview_pixmap_frame);
  gtk_widget_set_usize (preview_pixmap_frame, 500, 300);
  gtk_widget_show (preview_pixmap_frame);
  gtk_box_pack_start (GTK_BOX (vbox1), preview_pixmap_frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (preview_pixmap_frame), 5);
  gtk_frame_set_shadow_type (GTK_FRAME (preview_pixmap_frame), GTK_SHADOW_IN);

  preview_pixmap = MyCreateFromPixmapData (xfbd, empty);
  if (preview_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_set_name (preview_pixmap, "preview_pixmap");
  gtk_object_set_data (GTK_OBJECT (xfbd), "preview_pixmap", preview_pixmap);
  gtk_widget_show (preview_pixmap);
  gtk_container_add (GTK_CONTAINER (preview_pixmap_frame), preview_pixmap);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_object_set_data (GTK_OBJECT (xfbd), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  label1 = gtk_label_new (_("Filename : "));
  gtk_widget_set_name (label1, "label1");
  gtk_object_set_data (GTK_OBJECT (xfbd), "label1", label1);
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label1), 1, 0.5);

  filename_entry = gtk_entry_new ();
  gtk_widget_set_name (filename_entry, "filename_entry");
  gtk_object_set_data (GTK_OBJECT (xfbd), "filename_entry", filename_entry);
  /* gtk_widget_set_style(filename_entry, pal->cm[4]); */
  gtk_widget_show (filename_entry);
  gtk_box_pack_start (GTK_BOX (hbox1), filename_entry, TRUE, TRUE, 5);

  tiled_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (tiled_hbox, "tiled_hbox");
  gtk_object_set_data (GTK_OBJECT (xfbd), "tiled_hbox", tiled_hbox);
#if defined (HAVE_GDK_IMLIB) || defined (HAVE_GDK_PIXBUF)
  gtk_widget_show (tiled_hbox);
#endif
  gtk_box_pack_start (GTK_BOX (vbox1), tiled_hbox, TRUE, TRUE, 5);

  label2 = gtk_label_new (_("Display image : "));
  gtk_widget_set_name (label1, "label2");
  gtk_object_set_data (GTK_OBJECT (xfbd), "label2", label2);
  gtk_widget_show (label2);
  gtk_box_pack_start (GTK_BOX (tiled_hbox), label2, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label1), 1, 0.5);

  setup_tiled = gtk_radio_button_new_with_label (tiled_hbox_group, _("Tiled"));
  tiled_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_tiled));
  gtk_widget_set_name (setup_tiled, "setup_tiled");
  gtk_object_set_data (GTK_OBJECT (xfbd), "setup_tiled", setup_tiled);
  gtk_widget_show (setup_tiled);
  gtk_box_pack_start (GTK_BOX (tiled_hbox), setup_tiled, TRUE, TRUE, 10);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_tiled), (!strcmp (istiled, "tiled")));

  setup_stretched = gtk_radio_button_new_with_label (tiled_hbox_group, _("Stretched"));
  tiled_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_stretched));
  gtk_widget_set_name (setup_stretched, "setup_stretched");
  gtk_object_set_data (GTK_OBJECT (xfbd), "setup_stretched", setup_stretched);
  gtk_widget_show (setup_stretched);
  gtk_box_pack_start (GTK_BOX (tiled_hbox), setup_stretched, TRUE, TRUE, 10);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_stretched), (!strcmp (istiled, "stretched")));

  setup_auto = gtk_radio_button_new_with_label (tiled_hbox_group, _("Auto"));
  tiled_hbox_group = gtk_radio_button_group (GTK_RADIO_BUTTON (setup_auto));
  gtk_widget_set_name (setup_auto, "setup_auto");
  gtk_object_set_data (GTK_OBJECT (xfbd), "setup_auto", setup_auto);
  gtk_widget_show (setup_auto);
  gtk_box_pack_start (GTK_BOX (tiled_hbox), setup_auto, TRUE, TRUE, 10);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setup_auto), (strcmp (istiled, "tiled") && strcmp (istiled, "stretched")));

  frame3 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame3, "frame3");
  gtk_object_set_data (GTK_OBJECT (xfbd), "frame3", frame3);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame3), 5);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
  gtk_object_set_data (GTK_OBJECT (xfbd), "hbuttonbox1", hbuttonbox1);
  gtk_widget_show (hbuttonbox1);
  gtk_container_add (GTK_CONTAINER (frame3), hbuttonbox1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);

  ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (ok_button, "ok_button");
  gtk_object_set_data (GTK_OBJECT (xfbd), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), ok_button);
  gtk_container_border_width (GTK_CONTAINER (ok_button), 5);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (xfbd), accel_group);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (browse_button, "browse_button");
  gtk_object_set_data (GTK_OBJECT (xfbd), "browse_button", browse_button);
  gtk_widget_show (browse_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), browse_button);
  gtk_container_border_width (GTK_CONTAINER (browse_button), 5);
  GTK_WIDGET_SET_FLAGS (browse_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (browse_button, "clicked", accel_group, GDK_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_set_name (apply_button, "apply_button");
  gtk_object_set_data (GTK_OBJECT (xfbd), "apply_button", apply_button);
  gtk_widget_show (apply_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), apply_button);
  gtk_container_border_width (GTK_CONTAINER (apply_button), 5);
  GTK_WIDGET_SET_FLAGS (apply_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (apply_button, "clicked", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  clear_button = gtk_button_new_with_label (_("Clear"));
  gtk_widget_set_name (clear_button, "clear_button");
  gtk_object_set_data (GTK_OBJECT (xfbd), "clear_button", clear_button);
  gtk_widget_show (clear_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), clear_button);
  gtk_container_border_width (GTK_CONTAINER (clear_button), 5);
  GTK_WIDGET_SET_FLAGS (clear_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (clear_button, "clicked", accel_group, GDK_Delete, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (cancel_button, "cancel_button");
  gtk_object_set_data (GTK_OBJECT (xfbd), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), cancel_button);
  gtk_container_border_width (GTK_CONTAINER (cancel_button), 5);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_drag_dest_set (preview_pixmap_frame, GTK_DEST_DEFAULT_ALL, xfbd_target_table, n_xfbd_targets, GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gtk_signal_connect (GTK_OBJECT (preview_pixmap_frame), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), (gpointer) NULL);

  gtk_signal_connect (GTK_OBJECT (apply_button), "clicked", GTK_SIGNAL_FUNC (apply_cb), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (browse_button), "clicked", GTK_SIGNAL_FUNC (browse_cb), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (clear_button), "clicked", GTK_SIGNAL_FUNC (clear_cb), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (xfbd), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) NULL);

  return xfbd;
}

void
select_backdrp (char *theListName, char *theSelectedFile)
{
  FILE *theFile;
  char testStr[22];
  int Number, i;
  GList *files = NULL;
  char *afile, *result, *buffer;

  if ((theFile = fopen (theListName, "r")) != NULL)
  {
    fgets (testStr, 21, theFile);
    if (strncmp (testStr, "# xfce backdrop list", 20) == 0)
    {
      Number = 0;
      buffer = (char *) malloc (MAXSTRLEN * sizeof (char));
      do
      {
	result = fgets (buffer, MAXSTRLEN - 1, theFile);
	if ((result != NULL) && strlen (buffer))
	{
	  Number++;
	  buffer[strlen (buffer) - 1] = '\0';
	  afile = g_strdup ((gchar *) buffer);
	  files = g_list_append (files, afile);
	}
      }
      while (result);
      free (buffer);
      if (Number > 1)
      {
	i = (rand () % (Number - 1)) + 1;
	strcpy (theSelectedFile, (char *) g_list_nth_data (files, i));
      }
      else
      {
	strcpy (theListName, NOBACK);
	strcpy (theSelectedFile, NOBACK);
	g_list_free (files);
      }
    }
    else
    {
      strcpy (theSelectedFile, theListName);
    }
  }
  else
  {
    strcpy (theListName, NOBACK);
    strcpy (theSelectedFile, NOBACK);
  }

}

int
main (int argc, char *argv[])
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H)
  int xinerama_major, xinerama_minor;
#endif

  xfce_init (&argc, &argv);

#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
  if (!XineramaQueryExtension (GDK_DISPLAY (), &xinerama_major, &xinerama_minor))
  {
    enable_xinerama = False;
  }
  else
  {
    enable_xinerama = True;
    xinerama_infos = XineramaQueryScreens (GDK_DISPLAY (), &xinerama_heads);
  }
#endif

  srand ((unsigned) time (NULL));

  backdrp = (char *) malloc (sizeof (char) * MAXSTRLEN);
  istiled = (char *) malloc (sizeof (char) * MAXSTRLEN);
  filename = (char *) malloc (sizeof (char) * MAXSTRLEN);
  readstr (filename, istiled);

  if ((argc > 2) && existfile (argv[argc - 1]))
  {
    strcpy (backdrp, argv[argc - 1]);
    writestr (backdrp, istiled);
    setroot (backdrp, istiled);
    free (backdrp);
    free (filename);
    return (0);
  }
  else if (argc == 2 && strcmp (argv[1], "-i") == 0)
  {
    xfbd = create_xfbd ();
    gtk_entry_set_text (GTK_ENTRY (filename_entry), filename);
    gtk_entry_set_position (GTK_ENTRY (filename_entry), 0);
    display_back (filename);
    gtk_widget_show (xfbd);
    set_icon (xfbd, "XFbd", xfbd_icon_xpm);
    gtk_main ();
    free (backdrp);
    free (filename);
    xfce_end ((gpointer) NULL, 0);
  }
  else if ((argc == 1) || (argc == 2 && strcmp (argv[1], "-d") == 0))
  {
    setroot (filename, istiled);
    free (backdrp);
    free (filename);
    return (0);
  }

  fprintf (stderr, _("Usage : %s [OPTIONS]\n"), argv[0]);
  fprintf (stderr, _("   Where OPTIONS are :\n"));
  fprintf (stderr, _("   -i : interactive, prompts for backdrop to display\n"));
  fprintf (stderr, _("   -d : display, reads configuration and exit (default)\n\n"));
  fprintf (stderr, _("%s is part of the XFce distribution, written by Olivier Fourdan\n\n"), argv[0]);
  return (0);
}
