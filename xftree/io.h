/*
 * io.h
 *
 * Copyright (C) 1998,1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia Copywrite 2001 for Xfce project.
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

#ifndef __IO_H__
#define __IO_H__
#include <glob.h>
#include <gtk/gtk.h>

int io_is_exec (char *file);
int io_can_exec (char *file);
int io_can_write_to_parent (char *file);
int io_system (char **argv,GtkWidget *parent);
int io_system_var (char **cmd, int len);
int io_is_directory (char *path);
int io_is_file (char *file);
int io_item_exists (char *file);

#define io_is_root(s)	 (s && (s[0] == '/') && (s[1] == '\0'))
#ifdef GLOB_PERIOD 
#define io_is_hidden(s)	  ((s && (s[0] == '.'))?TRUE:FALSE) 
#else
/* fall back method */
#define io_is_hidden(s)	 ( (!(preferences&FILTER_OPTION)) && ((s && (s[0] == '.'))?TRUE:FALSE) )
#endif
#define io_is_current(s) (s && (s[0]=='.') && (s[1]=='\0') ? TRUE : FALSE)
#define io_is_dirup(s)	 (s && (s[0]=='.') && (s[1]=='.') && (s[2]=='\0')? TRUE:FALSE)
#define io_is_valid(s)   (!io_is_current(s) && !io_is_dirup(s))

#endif
