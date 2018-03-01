/*
 * xtree_misc.c
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

#ifdef HAVE_CONFIG_H   
#  include <config.h>   
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include "xtree_misc.h"
#include "entry.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/*
 * return a list of the files which are selected
 */
int
list_from_selection (GtkCTree * ctree, GList ** list)
{
  GList *sel;
  int num = 0;
  entry *en;
  GtkCTreeNode *node;

  *list = NULL;
  sel = GTK_CLIST (ctree)->selection;

  while (sel)
  {
    num++;
    node = sel->data;
    en = gtk_ctree_node_get_row_data (ctree, node);
    *list = g_list_append (*list, g_strdup (en->path));
    sel = sel->next;
  }
  return (num);
}

/*
 * build a list of entries by the selection
 */
int
entry_list_from_selection (GtkCTree * ctree, GList ** list, int *state)
{
  GList *sel;
  int num = 0;
  entry *en, *new_en;

  *list = NULL;
  sel = GTK_CLIST (ctree)->selection;

  while (sel && state)
  {
    num++;
    en = gtk_ctree_node_get_row_data (ctree, GTK_CTREE_NODE (sel->data));
    new_en = entry_dupe (en);
    new_en->org_mem = en;
    *list = g_list_append (*list, new_en);
    sel = sel->next;
  }
  return (num);
}

/*
 */
GList *
entry_list_free (GList * list)
{
  GList *t = list;
  if (!list)
    return (NULL);
  while (t)
  {
    entry_free ((entry *) t->data);
    t = t->next;
  }
  g_list_free (list);
  return (NULL);
}
