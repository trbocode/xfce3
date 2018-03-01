/*
 * top.c
 *
 * Copyright (C) 1998 Rasca, Berlin
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
#ifdef HAVE_CONFIG_H   
#  include <config.h>   
#endif

#include <glib.h>
#include "top.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GList *list;

/*
 * save the pointer to the toplevel
 */
void
top_register (void *data)
{
  list = g_list_append (list, data);
}

/*
 */
void
top_delete (void *data)
{
  list = g_list_remove (list, data);
}

/*
 */
void *
top_has_more (void)
{
  return list;
}

int
top_length (void)
{
  return g_list_length (list);
}
