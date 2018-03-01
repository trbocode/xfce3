/*
 * xtree_functions.h
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

#ifndef __XTREE_FUNCTIONS_H__
#define __XTREE_FUNCTIONS_H__
#include <sys/types.h>
#include <gtk/gtk.h>
#include "xtree_cfg.h"
#include "xtree_gui.h"
#include "entry.h"
typedef struct status_info {
	off_t howmany;
	off_t howmuch;
} status_info;

int count_selection (GtkCTree * ctree, GtkCTreeNode ** first);
int selection_type (GtkCTree * ctree, GtkCTreeNode ** first);
XErrorHandler ErrorHandler (Display * dpy, XErrorEvent * event);
void tree_unselect  (GtkCTree *ctree,GList *node,gpointer user_data);
gint on_delete (GtkWidget * w, GdkEvent * event, gpointer data);
void on_expand (GtkCTree * ctree, GtkCTreeNode * node, char *path);
void on_collapse (GtkCTree * ctree, GtkCTreeNode * node, char *path);
void menu_detach (void);

void node_destroy (gpointer p);
void ctree_thaw (GtkCTree * ctree);
void ctree_freeze (GtkCTree * ctree);
void add_subtree (GtkCTree * ctree, GtkCTreeNode * root, char *path, int depth, int flags);

/*void set_title (GtkWidget * w, const char *path);*/
void set_title_ctree (GtkWidget * w, const char *path);
gint update_timer (GtkCTree * ctree);
void on_dotfiles (GtkWidget * item, GtkCTree * ctree);
char *our_host_name(GtkWidget *top);
void reset_icon(GtkCTree * ctree, GtkCTreeNode * node);
#endif
