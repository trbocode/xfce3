/*
 * gtk_exec.h
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

#ifndef __GTK_EXEC_H__
#define __GTK_EXEC_H__

int xf_dlg_open_with (GtkWidget *ctree,char *xap, char *defval, char *file);
/* depredated (use xf_dlg_open_with instead): */
int dlg_open_with (char *xap, char *defval, char *file);
GList *free_app_list (void);
/* deprected: #define dlg_execute(x,d) dlg_open_with(x, d, NULL)*/
#define xf_dlg_execute(p,x,d) xf_dlg_open_with(p,x, d, NULL)
#endif
