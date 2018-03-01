/*
 * xtree_cfg.h
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

#ifndef __XTREE_CFG_H__
#define __XTREE_CFG_H__

#include <gtk/gtk.h>
#include "xtree_mess.h"
#include "xtree_go.h"
#include "ft_types.h"

typedef struct
{
  void *compare;
  char *trash;
  char *xap;			/* xap home-dir, default: $HOME/.xap */
  GList *reg;			/* registered programs */
  int dnd_row;
  char *dnd_data;
  int timer;
  /* geometry */
  int width;
  int height;
  char *iconname;
  GtkWidget *restore_menu;
  GtkWidget *top;
  GtkWidget *toolbar;
  GtkWidget *toolbarO;
  GtkWidget *menu;
  GtkWidget *filter;
  GtkWidget *status;
  GtkWidget *autotype_C;
  GtkWidget *autotype_D;
  GtkWidget *autotar_C;
  GtkCTreeNode *status_node;
  golist *gogo;
  int filterOpts;
  unsigned int preferences;
  unsigned int stateTB[2];
  gboolean source_set_sem;
}
cfg, cfg_t;

#endif
