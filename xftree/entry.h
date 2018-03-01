/*
 * entry.h
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia Copyright 2001-2002
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

#ifndef __ENTRY_H__
#define __ENTRY_H__
#include <sys/types.h>
#include <sys/stat.h>
typedef struct
{
  int year;
  int month;
  int day;
  int hour;
  int min;
}
mdate;

typedef struct
{
  gchar *path;
  gchar *label;
  int type;
  int flags;
  void *org_mem; /* what this for? FIXME?*/
  mdate date;
  struct stat st;
}
entry;

#include "ft_types.h"


#define EN_IS_TAR(en) (en->type & FT_TAR)
#define EN_IS_TARCHILD(en) (en->type & FT_TARCHILD)

#define EN_IS_DIR(en) (en->type & FT_DIR)
#define EN_IS_LINK(en) (en->type & FT_LINK)
#define EN_IS_DIRUP(en) (en->type & FT_DIR_UP)
#define EN_IS_DUMMY(en) (en->type & FT_DUMMY)
#define EN_IS_FIFO(en) (en->type & FT_FIFO)
#define EN_IS_SOCKET(en) (en->type & FT_SOCKET)
#define EN_IS_DEVICE(en) ((en->type & FT_CHAR_DEV)||(en->type & FT_BLOCK_DEV))
#ifndef ERROR
#define ERROR -1
#endif

void entry_free (entry *);
entry *entry_new (void);
entry *entry_dupe (entry *);
entry *entry_new_by_path (char *path);
entry *entry_new_by_path_and_label (char *path, char *label);
entry *entry_new_by_type (char *path, int type);
int entry_update (entry *);

#endif
