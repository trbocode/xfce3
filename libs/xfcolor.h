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
#include <gdk/gdkx.h>

#ifndef __COLOR_H__
#define __COLOR_H__

#include "constant.h"

#define brightlim  	 85
#define darklim    	 45
#define fadeblack 	 60

#define COLOR_GDK	65535.0
#define COLOR_XFCE	255

typedef struct
{
  /* GtkStyle *cm[NB_XFCE_COLORS]; */
  int r[NB_XFCE_COLORS];
  int g[NB_XFCE_COLORS];
  int b[NB_XFCE_COLORS];
  char *fnt;
  char *texture;
  char *engine;
}
XFCE_palette;

extern void apply_xpalette (XFCE_palette *, gboolean);
extern void get_rgb_from_palette (XFCE_palette *, short int, int *, int *, int *);
extern int brightness_pal (const XFCE_palette *, int);
extern char *color_to_hex (char *, const XFCE_palette *, int);
extern void set_color_table (XFCE_palette *, int, gdouble color_table[]);
extern void set_selcolor (XFCE_palette *, int, gdouble color_table[]);
extern void set_palcolor (XFCE_palette *, int, gdouble color_table[]);
extern unsigned long get_pixel_from_palette (XFCE_palette *, int);
extern XFCE_palette *newpal (void);
extern void set_font (XFCE_palette *, char *);
extern void set_texture (XFCE_palette *, char *);
extern void set_engine (XFCE_palette *, char *);
extern void freepal (XFCE_palette *);
extern XFCE_palette *copypal (XFCE_palette *, const XFCE_palette *);
extern XFCE_palette *copyvaluepal (XFCE_palette *, const XFCE_palette *);
extern void initpal (XFCE_palette *);
extern void create_gtkrc_file (XFCE_palette *, char *name);
extern void create_temp_gtkrc_file (XFCE_palette *);
extern void apply_temp_gtkrc_file (XFCE_palette *, GtkWidget *);
extern void defpal (XFCE_palette *);
extern int savenamepal (XFCE_palette *, const char *);
extern int loadnamepal (XFCE_palette *, const char *);
extern int savepal (XFCE_palette *);
extern int loadpal (XFCE_palette *);
extern void reg_xfce_app (GtkWidget * widget, XFCE_palette * p);
extern void applypal_to_all (void);
extern gboolean repaint_in_progress (void);
extern void init_xfce_rcfile (void);

#endif
