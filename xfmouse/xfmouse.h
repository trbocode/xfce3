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

#ifndef __XFMOUSE_H__
#define __XFMOUSE_H__

#include <gtk/gtk.h>

#define ACCEL_MIN	1
#define ACCEL_MAX	30
#define DENOMINATOR	3
#define THRESH_MIN	1
#define THRESH_MAX	20
#define RCFILE          "xfmouserc"

typedef struct
{
  int button;
  int accel;
  int thresh;
}
XFMouse;

XFMouse mouseval;

GtkWidget *xfmouse;
GtkWidget *leftbtn;
GtkWidget *rightbtn;
GtkObject *accel;
GtkObject *thresh;
char *homedir;
char *tempstr;

void mouse_values (XFMouse * s);
void apply_mouse_values (XFMouse * s);
GtkWidget *create_xfmouse (void);
void show_xfmouse (XFMouse *);
void loadcfg (XFMouse * s);
int savecfg (XFMouse * s);

#endif
