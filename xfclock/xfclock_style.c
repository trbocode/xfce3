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
#include <glib.h>
#include "xfcolor.h"
#include "lightdark.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
setfgstyle (GtkStyle * style, gdouble * color)
{
  guint r, g, b;

  r = ((guint) (color[0] * COLOR_GDK));
  g = ((guint) (color[1] * COLOR_GDK));
  b = ((guint) (color[2] * COLOR_GDK));
  style->text[GTK_STATE_NORMAL].red = r;
  style->text[GTK_STATE_NORMAL].green = g;
  style->text[GTK_STATE_NORMAL].blue = b;
  style->text[GTK_STATE_ACTIVE].red = r;
  style->text[GTK_STATE_ACTIVE].green = g;
  style->text[GTK_STATE_ACTIVE].blue = b;
  style->text[GTK_STATE_PRELIGHT].red = r;
  style->text[GTK_STATE_PRELIGHT].green = g;
  style->text[GTK_STATE_PRELIGHT].blue = b;
  style->text[GTK_STATE_SELECTED].red = r;
  style->text[GTK_STATE_SELECTED].green = g;
  style->text[GTK_STATE_SELECTED].blue = b;
  style->text[GTK_STATE_INSENSITIVE].red = r;
  style->text[GTK_STATE_INSENSITIVE].green = g;
  style->text[GTK_STATE_INSENSITIVE].blue = b;
  style->fg[GTK_STATE_NORMAL].red = r;
  style->fg[GTK_STATE_NORMAL].green = g;
  style->fg[GTK_STATE_NORMAL].blue = b;
  style->fg[GTK_STATE_ACTIVE].red = r;
  style->fg[GTK_STATE_ACTIVE].green = g;
  style->fg[GTK_STATE_ACTIVE].blue = b;
  style->fg[GTK_STATE_PRELIGHT].red = r;
  style->fg[GTK_STATE_PRELIGHT].green = g;
  style->fg[GTK_STATE_PRELIGHT].blue = b;
  style->fg[GTK_STATE_SELECTED].red = r;
  style->fg[GTK_STATE_SELECTED].green = g;
  style->fg[GTK_STATE_SELECTED].blue = b;
  style->fg[GTK_STATE_INSENSITIVE].red = r;
  style->fg[GTK_STATE_INSENSITIVE].green = g;
  style->fg[GTK_STATE_INSENSITIVE].blue = b;
}

void
setbgstyle (GtkStyle * style, gdouble * color)
{
  guint r, g, b;

  r = ((guint) (color[0] * COLOR_GDK));
  g = ((guint) (color[1] * COLOR_GDK));
  b = ((guint) (color[2] * COLOR_GDK));
  style->bg[GTK_STATE_NORMAL].red = r;
  style->bg[GTK_STATE_NORMAL].green = g;
  style->bg[GTK_STATE_NORMAL].blue = b;
  style->base[GTK_STATE_NORMAL].red = r;
  style->base[GTK_STATE_NORMAL].green = g;
  style->base[GTK_STATE_NORMAL].blue = b;
  style->mid[GTK_STATE_NORMAL].red = r;
  style->mid[GTK_STATE_NORMAL].green = g;
  style->mid[GTK_STATE_NORMAL].blue = b;
  style->light[GTK_STATE_NORMAL].red = shift16 (r, HI_MULT);
  style->light[GTK_STATE_NORMAL].green = shift16 (g, HI_MULT);
  style->light[GTK_STATE_NORMAL].blue = shift16 (b, HI_MULT);
  style->dark[GTK_STATE_NORMAL].red = shift16 (r, LO_MULT);
  style->dark[GTK_STATE_NORMAL].green = shift16 (g, LO_MULT);
  style->dark[GTK_STATE_NORMAL].blue = shift16 (b, LO_MULT);

  style->bg[GTK_STATE_ACTIVE].red = r;
  style->bg[GTK_STATE_ACTIVE].green = g;
  style->bg[GTK_STATE_ACTIVE].blue = b;
  style->base[GTK_STATE_ACTIVE].red = r;
  style->base[GTK_STATE_ACTIVE].green = g;
  style->base[GTK_STATE_ACTIVE].blue = b;
  style->mid[GTK_STATE_ACTIVE].red = r;
  style->mid[GTK_STATE_ACTIVE].green = g;
  style->mid[GTK_STATE_ACTIVE].blue = b;
  style->light[GTK_STATE_ACTIVE].red = shift16 (r, HI_MULT);
  style->light[GTK_STATE_ACTIVE].green = shift16 (g, HI_MULT);
  style->light[GTK_STATE_ACTIVE].blue = shift16 (b, HI_MULT);
  style->dark[GTK_STATE_ACTIVE].red = shift16 (r, LO_MULT);
  style->dark[GTK_STATE_ACTIVE].green = shift16 (g, LO_MULT);
  style->dark[GTK_STATE_ACTIVE].blue = shift16 (b, LO_MULT);

  style->bg[GTK_STATE_PRELIGHT].red = r;
  style->bg[GTK_STATE_PRELIGHT].green = g;
  style->bg[GTK_STATE_PRELIGHT].blue = b;
  style->base[GTK_STATE_PRELIGHT].red = r;
  style->base[GTK_STATE_PRELIGHT].green = g;
  style->base[GTK_STATE_PRELIGHT].blue = b;
  style->mid[GTK_STATE_PRELIGHT].red = r;
  style->mid[GTK_STATE_PRELIGHT].green = g;
  style->mid[GTK_STATE_PRELIGHT].blue = b;
  style->light[GTK_STATE_PRELIGHT].red = shift16 (r, HI_MULT);
  style->light[GTK_STATE_PRELIGHT].green = shift16 (g, HI_MULT);
  style->light[GTK_STATE_PRELIGHT].blue = shift16 (b, HI_MULT);
  style->dark[GTK_STATE_PRELIGHT].red = shift16 (r, LO_MULT);
  style->dark[GTK_STATE_PRELIGHT].green = shift16 (g, LO_MULT);
  style->dark[GTK_STATE_PRELIGHT].blue = shift16 (b, LO_MULT);

  style->bg[GTK_STATE_SELECTED].red = r;
  style->bg[GTK_STATE_SELECTED].green = g;
  style->bg[GTK_STATE_SELECTED].blue = b;
  style->base[GTK_STATE_SELECTED].red = r;
  style->base[GTK_STATE_SELECTED].green = g;
  style->base[GTK_STATE_SELECTED].blue = b;
  style->mid[GTK_STATE_SELECTED].red = r;
  style->mid[GTK_STATE_SELECTED].green = g;
  style->mid[GTK_STATE_SELECTED].blue = b;
  style->light[GTK_STATE_SELECTED].red = shift16 (r, HI_MULT);
  style->light[GTK_STATE_SELECTED].green = shift16 (g, HI_MULT);
  style->light[GTK_STATE_SELECTED].blue = shift16 (b, HI_MULT);
  style->dark[GTK_STATE_SELECTED].red = shift16 (r, LO_MULT);
  style->dark[GTK_STATE_SELECTED].green = shift16 (g, LO_MULT);
  style->dark[GTK_STATE_SELECTED].blue = shift16 (b, LO_MULT);

  style->bg[GTK_STATE_INSENSITIVE].red = r;
  style->bg[GTK_STATE_INSENSITIVE].green = g;
  style->bg[GTK_STATE_INSENSITIVE].blue = b;
  style->base[GTK_STATE_INSENSITIVE].red = r;
  style->base[GTK_STATE_INSENSITIVE].green = g;
  style->base[GTK_STATE_INSENSITIVE].blue = b;
  style->mid[GTK_STATE_INSENSITIVE].red = r;
  style->mid[GTK_STATE_INSENSITIVE].green = g;
  style->mid[GTK_STATE_INSENSITIVE].blue = b;
  style->light[GTK_STATE_INSENSITIVE].red = shift16 (r, HI_MULT);
  style->light[GTK_STATE_INSENSITIVE].green = shift16 (g, HI_MULT);
  style->light[GTK_STATE_INSENSITIVE].blue = shift16 (b, HI_MULT);
  style->dark[GTK_STATE_INSENSITIVE].red = shift16 (r, LO_MULT);
  style->dark[GTK_STATE_INSENSITIVE].green = shift16 (g, LO_MULT);
  style->dark[GTK_STATE_INSENSITIVE].blue = shift16 (b, LO_MULT);
}

void
setfontstyle (GtkStyle * style, char *fnt)
{
  if ((style->font = gdk_font_load (fnt)) == NULL)
    if ((style->font = gdk_font_load (DEFAULTFONT)) == NULL)
      style->font = gdk_font_load ("fixed");
}
