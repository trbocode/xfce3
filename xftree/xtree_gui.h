/*
 * xtree_gui.h
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia 2001 for Xfce project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __XTREE_GUI_H__
#define __XTREE_GUI_H__
#include <gtk/gtk.h>
#include "xtree_cfg.h"
#define __XTREE_VERSION__ "3.8.18"

#define SPACING 	5
#define TIMERVAL 	6000
#define MAXBUF		8192

#define WINCFG		1
#define TOPWIN		2

#define yes		1
#define no		0
#define ERROR 		-1

typedef struct
{
  int x;
  int y;
  int width;
  int height;
}
wgeo_t;

typedef struct autotype_t{
	char *extension;
	char *command;
	char *label;
	char *querypath;
}autotype_t;

enum
{
  MN_NONE = 0,
  MN_DIR = 1,
  MN_FILE = 2,
  MN_MIXED = 3,
  MN_HLP = 4,
  MN_TARCHILD = 5,
  MENUS
};

enum
{
  COL_NAME,
  COL_SIZE,
  COL_DATE,
  COL_UID,
  COL_GID,
  COL_MODE,
  COLUMNS			/* number of columns */
};


void gui_main (char *path, char *xap, char *trash, char *reg, wgeo_t *, int);
char *mode_txt(mode_t mode);
cfg *new_top (char *path, char *xap, char *trash, GList * reg, int width, int height, int flags);

#endif
