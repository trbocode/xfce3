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

#ifndef __MODIFY_H__
#define __MODIFY_H__

GtkWidget *modify_command_entry;
GtkWidget *modify_icon_entry;
GtkWidget *modify_displayed_entry;
GtkWidget *modify_preview_frame;
GtkWidget *modify_preview_pixmap;
GtkWidget *modify_ok_button;
GtkObject *modify_pos_adj;
GtkWidget *modify_pos_hscale;
GtkWidget *modify_cancel_button;
GtkWidget *modify_remove_button;

guint signal_id1;
guint signal_id2;

GtkWidget *create_modify (void);

void open_modify (GtkWidget * modify, gint menu, gint item);

#endif
