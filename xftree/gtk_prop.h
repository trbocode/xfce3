/*
 * gtk_prop.h
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

#ifndef __GTK_PROP_H__
#define __GTK_PROP_H__

/* flags
 */
#define IS_MULTI		1
#define IS_STALE_LINK	2

typedef struct
{
  mode_t mode;
  uid_t uid;
  gid_t gid;
  time_t ctime;
  time_t mtime;
  time_t atime;
  off_t size;
}
fprop;
/* dlg_prop is deprecated. use xf_dlg_prop instead */

int xf_dlg_prop (GtkWidget *parent,char *path, fprop * prop, int multi);
int dlg_prop (char *path, fprop * prop, int multi);
#endif
