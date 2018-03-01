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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#else
#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#endif

#include "my_intl.h"
#include "constant.h"
#include "xfcolor.h"
#include "xpmext.h"
#include "fileutil.h"
#include "empty.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define ROUND(x) (int) ((double) x + .5)

static Atom xrootpmap_id = None;
static Atom esetroot_pmap_id = None;

static int
dummyErrorHandler (Display * dpy, XErrorEvent * err)
{
  return 0;
}

Pixmap
duppix (Pixmap pixmap, int width, int height)
{
  Display *tmpDpy;
  Pixmap copyP = 0;

  if ((!width) || (!height))
    return 0;

  if (!(tmpDpy = XOpenDisplay ("")))
    return 0;
  else
  {
    XSync (GDK_DISPLAY (), False);

    copyP = XCreatePixmap (tmpDpy, GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), width, height, DefaultDepth (tmpDpy, DefaultScreen (tmpDpy)));
    XCopyArea (tmpDpy, pixmap, copyP, DefaultGC (tmpDpy, DefaultScreen (tmpDpy)), 0, 0, width, height, 0, 0);

    XSync (tmpDpy, False);

    XSetCloseDownMode (tmpDpy, RetainPermanent);
    XCloseDisplay (tmpDpy);
  }

  return copyP;
}

void
setPixmapProperty (Pixmap pixmap)
{
  unsigned char *data;
  Atom type;
  int format;
  unsigned long length, after;

  XGrabServer (GDK_DISPLAY ());
  if (!xrootpmap_id)
    xrootpmap_id = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", False);
  if (!esetroot_pmap_id)
    esetroot_pmap_id = XInternAtom (GDK_DISPLAY (), "ESETROOT_PMAP_ID", False);
  XGetWindowProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), esetroot_pmap_id, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
  if ((type == XA_PIXMAP) && (format == 32) && (length == 1))
  {
    XSetErrorHandler (dummyErrorHandler);
    XKillClient (GDK_DISPLAY (), *((Pixmap *) data));
    XSync (GDK_DISPLAY (), False);
    XSetErrorHandler (NULL);
  }
  XFree ((char *) data);
  if (pixmap)
  {
    XChangeProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), xrootpmap_id, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
    XChangeProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), esetroot_pmap_id, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap, 1);
  }
  else
  {
    XDeleteProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), xrootpmap_id);
    XDeleteProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), esetroot_pmap_id);
  }
  XUngrabServer (GDK_DISPLAY ());
  XFlush (GDK_DISPLAY ());
}

void
GradientXpmImage (int sizex, int sizey, XpmImage * Imagedest, int r1, int g1, int b1, int r2, int g2, int b2, int ncolors)
{
  int i, j, v;

  if (ncolors <= 0)
    ncolors = 2;
  else if (ncolors > 255)
    ncolors = 255;

  Imagedest->width = sizex;
  Imagedest->height = sizey;
  Imagedest->cpp = 1;
  Imagedest->ncolors = ncolors;
  Imagedest->colorTable = malloc (ncolors * sizeof (XpmColor));
  Imagedest->data = malloc (sizex * sizey * sizeof (int));

  for (i = 0; i < ncolors; i++)
  {
    (Imagedest->colorTable)[i].string = NULL;
    (Imagedest->colorTable)[i].symbolic = NULL;
    (Imagedest->colorTable)[i].m_color = NULL;
    (Imagedest->colorTable)[i].g4_color = NULL;
    (Imagedest->colorTable)[i].g_color = NULL;
    (Imagedest->colorTable)[i].c_color = malloc (14);
    sprintf ((Imagedest->colorTable)[i].c_color, "#%02X00%02X00%02X00", (r1 + (i * (r2 - r1) / ncolors)), (g1 + (i * (g2 - g1) / ncolors)), (b1 + (i * (b2 - b1) / ncolors)));
  }
  for (i = 0; i < sizey; i++)
    for (j = 0; j < sizex; j++)
    {
      v = ((ncolors * i / sizey) + (i % 2) + (j % 2));
      Imagedest->data[i * sizex + j] = ((v < (ncolors - 1)) ? v : (ncolors - 1));
    }
}


#ifndef HAVE_GDK_IMLIB
#ifndef HAVE_GDK_PIXBUF

/* The following function is from DFM (c) 1997 by Achim Kaiser */

void
XpmShapeToFit_dither (int newsizex, int newsizey, XpmImage * I, XpmImage * Imagedest)
{
  int getr, getg, getb;
  int *shrinknumbers;
  double *shrinkred;
  double *shrinkgreen;
  double *shrinkblue;
  int *myshape;
  XColor *mycolor;
  int i, k, r, g, b, x, y;

  mycolor = g_malloc (I->ncolors * sizeof (XColor));
  myshape = g_malloc (I->ncolors * sizeof (int));

  Imagedest->width = MIN (newsizex, I->width);
  Imagedest->height = MIN (newsizey, I->height);
  Imagedest->cpp = 2;
  Imagedest->ncolors = 65;
  Imagedest->colorTable = malloc (Imagedest->ncolors * sizeof (XpmColor));
  Imagedest->data = malloc (newsizex * newsizey * sizeof (int));

  shrinknumbers = g_malloc (newsizex * newsizey * sizeof (int));
  shrinkred = g_malloc (newsizex * newsizey * sizeof (double));
  shrinkgreen = g_malloc (newsizex * newsizey * sizeof (double));
  shrinkblue = g_malloc (newsizex * newsizey * sizeof (double));

  for (r = 0; r < 4; r++)
  {
    getr = (r << 2) | r;
    for (g = 0; g < 4; g++)
    {
      getg = (g << 2) | g;
      for (b = 0; b < 4; b++)
      {
	getb = (b << 2) | b;
	i = r * 16 + g * 4 + b;
	(Imagedest->colorTable)[i].string = malloc (3);
	(Imagedest->colorTable)[i].string[0] = 'a' + (i / 26);
	(Imagedest->colorTable)[i].string[1] = 'a' + (i % 26);
	(Imagedest->colorTable)[i].string[2] = '\0';
	(Imagedest->colorTable)[i].symbolic = NULL;
	(Imagedest->colorTable)[i].m_color = NULL;
	(Imagedest->colorTable)[i].g4_color = NULL;
	(Imagedest->colorTable)[i].g_color = NULL;
	(Imagedest->colorTable)[i].c_color = malloc (5);
	sprintf ((Imagedest->colorTable)[i].c_color, "#%X%X%X", getr, getg, getb);
      }
    }
  }
  (Imagedest->colorTable)[Imagedest->ncolors - 1].string = malloc (3);
  strcpy ((Imagedest->colorTable)[Imagedest->ncolors - 1].string, "..");
  (Imagedest->colorTable)[Imagedest->ncolors - 1].symbolic = NULL;
  (Imagedest->colorTable)[Imagedest->ncolors - 1].m_color = NULL;
  (Imagedest->colorTable)[Imagedest->ncolors - 1].g4_color = NULL;
  (Imagedest->colorTable)[Imagedest->ncolors - 1].g_color = NULL;
  (Imagedest->colorTable)[Imagedest->ncolors - 1].c_color = malloc (5);
  sprintf ((Imagedest->colorTable)[Imagedest->ncolors - 1].c_color, "None");

  for (i = 0; i < (I->ncolors); i++)
  {
    if (XParseColor (GDK_DISPLAY (), GDK_COLORMAP_XCOLORMAP (gdk_colormap_get_system ()), (I->colorTable)[i].c_color, &(mycolor[i])) == 0)
      myshape[i] = True;
    else
      myshape[i] = False;
  }
  for (i = 0; i < newsizey * newsizex; i++)
  {
    shrinknumbers[i] = 0;
    shrinkred[i] = 0.0;
    shrinkgreen[i] = 0.0;
    shrinkblue[i] = 0.0;
  }
  k = 0;
  for (y = 0; y < (I->height); y++)
  {
    for (x = 0; x < (I->width); x++)
    {
      if (myshape[I->data[k]] == False)
      {
	i = (int) ((newsizex - 1) * (double) x / (I->width - 1)) + newsizex * (int) ((newsizey - 1) * (double) y / (I->height - 1));
	shrinknumbers[i]++;
	shrinkred[i] = shrinkred[i] + mycolor[I->data[k]].red;
	shrinkgreen[i] = shrinkgreen[i] + mycolor[I->data[k]].green;
	shrinkblue[i] = shrinkblue[i] + mycolor[I->data[k]].blue;
      }
      k++;
    }
  }
  for (i = 0; i < newsizey * newsizex; i++)
  {
    if (shrinknumbers[i] == 0)
      Imagedest->data[i] = Imagedest->ncolors - 1;
    else
      Imagedest->data[i] = (((int) (shrinkred[i] / shrinknumbers[i]) >> 14) << 4) | (((int) (shrinkgreen[i] / shrinknumbers[i]) >> 14) << 2) | ((int) (shrinkblue[i] / shrinknumbers[i]) >> 14);
  }

  g_free (mycolor);
  g_free (myshape);
  g_free (shrinknumbers);
  g_free (shrinkred);
  g_free (shrinkgreen);
  g_free (shrinkblue);
}

/* This one is mine ;-) */

void
XpmShapeToFit_fast (int newsizex, int newsizey, XpmImage * I, XpmImage * Imagedest)
{
  int k, x, y;
  double step_x, step_y;

  Imagedest->width = MIN (newsizex, I->width);
  Imagedest->height = MIN (newsizey, I->height);
  Imagedest->cpp = I->cpp;
  Imagedest->ncolors = I->ncolors;
  Imagedest->colorTable = malloc (I->ncolors * sizeof (XpmColor));
  Imagedest->data = malloc (newsizex * newsizey * sizeof (int));

  for (k = 0; k < I->ncolors; k++)
  {

    (Imagedest->colorTable)[k].string = ((I->colorTable)[k].string ? strdup ((I->colorTable)[k].string) : NULL);
    (Imagedest->colorTable)[k].symbolic = ((I->colorTable)[k].symbolic ? strdup ((I->colorTable)[k].symbolic) : NULL);
    (Imagedest->colorTable)[k].m_color = ((I->colorTable)[k].m_color ? strdup ((I->colorTable)[k].m_color) : NULL);
    (Imagedest->colorTable)[k].g4_color = ((I->colorTable)[k].g4_color ? strdup ((I->colorTable)[k].g4_color) : NULL);
    (Imagedest->colorTable)[k].g_color = ((I->colorTable)[k].g_color ? strdup ((I->colorTable)[k].g_color) : NULL);
    (Imagedest->colorTable)[k].c_color = ((I->colorTable)[k].c_color ? strdup ((I->colorTable)[k].c_color) : NULL);
  }

  k = 0;
  step_x = ((double) (I->width) / (double) (newsizex));
  step_y = ((double) (I->height) / (double) (newsizey));

  for (y = 0; y < newsizey; y++)
    for (x = 0; x < newsizex; x++)
    {
      Imagedest->data[k] = I->data[ROUND ((double) x * step_x) + ((ROUND ((double) y * step_y)) * (I->width))];
      k++;
    }
}

void
XpmShapeToFit (int newsizex, int newsizey, XpmImage * I, XpmImage * Imagedest)
{
  if (DEFAULT_DEPTH > 8)
    XpmShapeToFit_fast (newsizex, newsizey, I, Imagedest);
  else
    XpmShapeToFit_dither (newsizex, newsizey, I, Imagedest);
}

int
MyCreateDataFromXpmImage (XpmImage * Img, char ***data_return)
{
  XpmInfo info;
  int status;

  info.valuemask = 0;
  status = XpmCreateDataFromXpmImage (data_return, Img, &info);
  return (status);
}

#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */

int
MyCreatePixmapFromXpmImage (XpmImage * Img, Pixmap * pixmap, Pixmap * mask)
{
  XpmAttributes attributes;
  int status;
  XWindowAttributes root_attr;

  XGetWindowAttributes (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), &root_attr);
  attributes.colormap = root_attr.colormap;
  attributes.valuemask = XpmCloseness | XpmColormap;
  attributes.closeness = 40000;

  status = XpmCreatePixmapFromXpmImage (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), Img, pixmap, mask, &attributes);
  return (status);
}

GdkPixmap *
MyCreateGdkPixmapFromFile (char *filename, GtkWidget * widget, GdkBitmap ** mask, gboolean fit)
{
#ifdef HAVE_GDK_IMLIB
  GdkImlibImage *im;
#else
#ifdef HAVE_GDK_PIXBUF
  GdkPixbuf *source, *destination;
#else
  int result = -1;
  XpmImage I;
  XpmImage I2;
  GdkColormap *colormap;
  char **pix_data;
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */
  GdkPixmap *pixmap = NULL;
  GtkRequisition requisition;
  gint w = 0;
  gint h = 0;

#ifdef HAVE_GDK_IMLIB
  im = gdk_imlib_load_image (filename);
  if (!im)
  {
    fprintf (stderr, _("*** WARNING ***: Cannot load image file %s\n"), filename);
    im = gdk_imlib_create_image_from_xpm_data (empty);
  }
  if (im)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    if ((fit) && (w > 0) && (h > 0) && ((im->rgb_width > w) || (im->rgb_height > h)))
      gdk_imlib_render (im, w, h);
    else
      gdk_imlib_render (im, im->rgb_width, im->rgb_height);

    pixmap = gdk_imlib_copy_image (im);

    if (mask)
    {
      *mask = gdk_imlib_copy_mask (im);
    }
    
    gdk_imlib_kill_image (im);
  }
#else /* without HAVE_GDK_IMLIB */
#ifdef HAVE_GDK_PIXBUF
  source = gdk_pixbuf_new_from_file (filename);
  if (!source)
  {
    fprintf (stderr, _("*** WARNING ***: Cannot load image file %s\n"), filename);
    source = gdk_pixbuf_new_from_xpm_data ((const char **) empty);
  }
  if (source)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    if ((fit) && (w > 0) && (h > 0) && ((gdk_pixbuf_get_width (source) > w) || gdk_pixbuf_get_height (source) > h))
      destination = gdk_pixbuf_scale_simple (source, w, h, GDK_INTERP_BILINEAR);
    else
      destination = gdk_pixbuf_scale_simple (source, gdk_pixbuf_get_width (source), gdk_pixbuf_get_height (source), GDK_INTERP_BILINEAR);
    gdk_pixbuf_render_pixmap_and_mask (destination, &pixmap, mask, gdk_pixbuf_get_has_alpha (destination));
    gdk_pixbuf_unref (source);
    gdk_pixbuf_unref (destination);
  }
#else
  if (existfile (filename))
    result = XpmReadFileToXpmImage (filename, &I, NULL);
  if (result)
  {
    fprintf (stderr, _("*** WARNING ***: Cannot load pixmap file %s\n"), filename);
    result = XpmCreateXpmImageFromData (empty, &I, NULL);
  }
  if (result == 0)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    colormap = gdk_colormap_get_system ();
    if ((fit) && ((w > 0) && (h > 0) && ((I.width > w) || (I.height > h))) || DEFAULT_DEPTH <= 8)
    {
      XpmShapeToFit (((I.width > w) ? w : I.width), ((I.height > h) ? h : I.height), &I, &I2);
      result = MyCreateDataFromXpmImage (&I2, &pix_data);
      XpmFreeXpmImage (&I2);
    }
    else
    {
      result = MyCreateDataFromXpmImage (&I, &pix_data);
    }
    pixmap = gdk_pixmap_colormap_create_from_xpm_d (widget->window, colormap, mask, NULL, (gchar **) pix_data);
    XpmFree (pix_data);
    XpmFreeXpmImage (&I);
  }
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */
  return (pixmap);
}

GdkPixmap *
MyCreateGdkPixmapFromData (char **data, GtkWidget * widget, GdkBitmap ** mask, gboolean fit)
{
#ifdef HAVE_GDK_IMLIB
  GdkImlibImage *im;
#else
#ifdef HAVE_GDK_PIXBUF
  GdkPixbuf *source, *destination;
#else
  int result;
  XpmImage I;
  XpmImage I2;
  GdkColormap *colormap;
  char **pix_data;
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */
  GdkPixmap *pixmap = NULL;
  GtkRequisition requisition;
  gint w = 0;
  gint h = 0;

#ifdef HAVE_GDK_IMLIB
  im = gdk_imlib_create_image_from_xpm_data (data);
  if (!im)
    im = gdk_imlib_create_image_from_xpm_data (empty);
  if (im)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    if ((fit) && (w > 0) && (h > 0) && ((im->rgb_width > w) || (im->rgb_height > h)))
      gdk_imlib_render (im, w, h);
    else
      gdk_imlib_render (im, im->rgb_width, im->rgb_height);
 
    pixmap = gdk_imlib_copy_image (im);

    if (mask)
    {
      *mask = gdk_imlib_copy_mask (im);
    }

    gdk_imlib_kill_image (im);
  }
#else /* without HAVE_GDK_IMLIB */
#ifdef HAVE_GDK_PIXBUF
  source = gdk_pixbuf_new_from_xpm_data ((const char **) data);
  if (!source)
    source = gdk_pixbuf_new_from_xpm_data ((const char **) empty);
  if (source)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    if ((fit) && (w > 0) && (h > 0) && ((gdk_pixbuf_get_width (source) > w) || (gdk_pixbuf_get_height (source) > h)))
      destination = gdk_pixbuf_scale_simple (source, w, h, GDK_INTERP_BILINEAR);
    else
      destination = gdk_pixbuf_scale_simple (source, gdk_pixbuf_get_width (source), gdk_pixbuf_get_height (source), GDK_INTERP_BILINEAR);
    gdk_pixbuf_render_pixmap_and_mask (destination, &pixmap, mask, gdk_pixbuf_get_has_alpha (destination));
    gdk_pixbuf_unref (source);
    gdk_pixbuf_unref (destination);
  }
#else
  result = XpmCreateXpmImageFromData (data, &I, NULL);
  if (result == 0)
  {
    gtk_widget_size_request ((GtkWidget *) widget, &requisition);
    w = requisition.width - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    h = requisition.height - (GTK_CONTAINER (widget)->border_width + 2) * 2;
    colormap = gdk_colormap_get_system ();
    if ((fit) && (w > 0) && (h > 0) && ((I.width > w) || (I.height > h)))
    {
      XpmShapeToFit (((I.width > w) ? w : I.width), ((I.height > h) ? h : I.height), &I, &I2);
      result = MyCreateDataFromXpmImage (&I2, &pix_data);
      XpmFreeXpmImage (&I2);
    }
    else
    {
      result = MyCreateDataFromXpmImage (&I, &pix_data);
    }
    pixmap = gdk_pixmap_colormap_create_from_xpm_d (widget->window, colormap, mask, NULL, (gchar **) pix_data);
    XpmFree (pix_data);
    XpmFreeXpmImage (&I);
  }
#endif /* HAVE_GDK_PIXBUF */
#endif /* HAVE_GDK_IMLIB */
  return (pixmap);
}

void
MySetPixmapData (GtkWidget * widget, GtkWidget * parent, char **data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;

  pixmap = MyCreateGdkPixmapFromData (data, parent, &mask, TRUE);
  gtk_pixmap_set (GTK_PIXMAP (widget), pixmap, mask);
  /*
     if (pixmap) gdk_pixmap_unref (pixmap);
     if (mask)   gdk_bitmap_unref (mask);
   */
}

void
MySetPixmapFile (GtkWidget * widget, GtkWidget * parent, char *filename)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;

  pixmap = MyCreateGdkPixmapFromFile (filename, parent, &mask, TRUE);
  gtk_pixmap_set (GTK_PIXMAP (widget), pixmap, mask);
  /*
     if (pixmap) gdk_pixmap_unref (pixmap);
     if (mask)   gdk_bitmap_unref (mask);
   */
}

GtkWidget *
MyCreateFromPixmapData (GtkWidget * widget, char **data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  GtkWidget *result;

  pixmap = MyCreateGdkPixmapFromData (data, widget, &mask, TRUE);
  result = gtk_pixmap_new (pixmap, mask);
  /*
     if (pixmap) gdk_pixmap_unref (pixmap);
     if (mask)   gdk_bitmap_unref (mask);
   */
  return result;
}

GtkWidget *
MyCreateFromPixmapFile (GtkWidget * widget, char *file)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  GtkWidget *result;

  pixmap = MyCreateGdkPixmapFromFile (file, widget, &mask, TRUE);
  result = gtk_pixmap_new (pixmap, mask);
  /*
     if (pixmap) gdk_pixmap_unref (pixmap);
     if (mask)   gdk_bitmap_unref (mask);
   */
  return result;
}

int
BuildXpmGradient (int r1, int g1, int b1, int r2, int g2, int b2)
{
  XpmImage I;
  Pixmap xpixmap = 0;
  Pixmap rootXpm = 0;
  Pixmap mask = 0;
  int status;
  int gradient_steps;

  if (DEFAULT_DEPTH > 16)
    gradient_steps = 128;
  else
    gradient_steps = 64;

  GradientXpmImage (10, gdk_screen_height (), &I, r1, g1, b1, r2, g2, b2, gradient_steps);
  status = MyCreatePixmapFromXpmImage (&I, &rootXpm, &mask);
  if (status == XpmSuccess)
  {
    XSetWindowBackgroundPixmap (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), rootXpm);
    XClearWindow (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()));
    xpixmap = duppix (rootXpm, 10, gdk_screen_height ());
    setPixmapProperty (xpixmap);
  }
  XpmFreeXpmImage (&I);
  if (rootXpm)
    XFreePixmap (GDK_DISPLAY (), rootXpm);
  if (mask)
    XFreePixmap (GDK_DISPLAY (), mask);
  return (status);
}

void
ApplyRootColor (XFCE_palette * pal, gboolean gradient, int col_index)
{
  unsigned char *data;
  Atom type;
  int format;
  unsigned long length, after;
  int r = 0;
  int g = 0;
  int b = 0;
  int ci;

  ci = ((col_index >= 0) ? col_index : 0);

  if ((DEFAULT_DEPTH > 8) && (gradient))
  {
    r = pal->r[ci];
    g = pal->g[ci];
    b = pal->b[ci];
    BuildXpmGradient (r, g, b, 0, 0, 0);
  }
  else
  {
    if (!xrootpmap_id)
      xrootpmap_id = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", False);
    if (!esetroot_pmap_id)
      esetroot_pmap_id = XInternAtom (GDK_DISPLAY (), "ESETROOT_PMAP_ID", False);
    XGrabServer (GDK_DISPLAY ());
    XGetWindowProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), esetroot_pmap_id, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
    if ((type == XA_PIXMAP) && (format == 32) && (length == 1))
    {
      XSetErrorHandler (dummyErrorHandler);
      XKillClient (GDK_DISPLAY (), *((Pixmap *) data));
      XSync (GDK_DISPLAY (), False);
      XSetErrorHandler (NULL);
    }
    XFree ((char *) data);
    XDeleteProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), xrootpmap_id);
    XDeleteProperty (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), esetroot_pmap_id);
    XSetWindowBackground (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), get_pixel_from_palette (pal, ci));
    XClearWindow (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()));
    XUngrabServer (GDK_DISPLAY ());
    XFlush (GDK_DISPLAY ());
  }
}
