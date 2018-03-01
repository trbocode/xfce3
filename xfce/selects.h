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



#ifndef __SELECTS_H__
#define __SELECTS_H__

#include "constant.h"
#include <string.h>

typedef struct
{
  char *command;
  int icon_nbr;
  char *ext_icon;
}
ST_select;

ST_select selects[NBSELECTS + 1];	/* One more for the lock icon */

void alloc_selects (void);
void free_selects (void);

int load_icon_str (char *);
char *save_icon_str (void);
void setup_icon (void);
void default_icon_str (void);
int get_icon_nbr (int);
char *get_exticon_str (int);
void set_exticon_str (int, char *);
void set_icon_nbr (int, int);
void init_choice_str (void);
void set_choice_value (int);
void set_command (int, char *);
char *get_command (int no_sel);

#endif
