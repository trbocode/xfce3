/*  Note: You are free to use whatever license you want.
    Eventually you will be able to edit it within Glade. */

/*  gxfce
 *  Copyright (C) <YEAR> <AUTHORS>
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

#ifndef __XFCE_MAIN_H__
#define __XFCE_MAIN_H__

#include <gtk/gtk.h>
#include "xfcolor.h"

#define end_XFCE(x) exit(x)

GtkWidget *gxfce;
GtkWidget *setup;
GtkWidget *fileselect;
GtkWidget *colorselect;
GtkWidget *modify;
GtkWidget *action;
GtkWidget *screen;
GtkWidget *info_scr;
GtkWidget *startup;
GtkWidget *fontselectiondialog;

XFCE_palette *pal;

void reap (int sig);

int main (int argc, char *argv[]);

#endif
