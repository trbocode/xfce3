/*
 * xtree_dnd.h
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

#ifndef __XTREE_DND_H__
#define __XTREE_DND_H__

#define ROW_TOP_YPIXEL(clist,row) (((clist)->row_height*(row))+\
	(((row))+1)*1+(clist)->voffset)

void on_drag_data (GtkWidget * ctree, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint itme, void *client);

void on_drag_data_get (GtkWidget * widget, GdkDragContext * context, GtkSelectionData * selection_data, guint info, guint time, gpointer data);
gboolean on_drag_motion (GtkWidget * widget, GdkDragContext * dc, gint x, gint y, guint t, gpointer data);
void on_drag_end (GtkWidget * widget, GdkDragContext * dc, gpointer data);
#endif
