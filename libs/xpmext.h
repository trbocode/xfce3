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

#ifndef __XPMEXT_H__
#define __XPMEXT_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include "constant.h"
#include "xfcolor.h"

#define DEFAULT_DEPTH DefaultDepth(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()))

extern Pixmap duppix (Pixmap, int, int);
extern void setPixmapProperty (Pixmap);
extern int BuildXpmGradient (int, int, int, int, int, int);
extern void ApplyRootColor (XFCE_palette *, gboolean, int);
int MyCreateDataFromXpmImage (XpmImage *, char ***);
GdkPixmap *MyCreateGdkPixmapFromFile (char *, GtkWidget *, GdkBitmap **, gboolean);
GdkPixmap *MyCreateGdkPixmapFromData (char **, GtkWidget *, GdkBitmap **, gboolean);
void MySetPixmapData (GtkWidget *, GtkWidget *, char **);
void MySetPixmapFile (GtkWidget *, GtkWidget *, char *);
GtkWidget *MyCreateFromPixmapData (GtkWidget *, char **);
GtkWidget *MyCreateFromPixmapFile (GtkWidget *, char *);

#endif
