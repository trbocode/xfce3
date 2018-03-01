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

#ifndef __GNOME_PROTOCOL_H__
#define __GNOME_PROTOCOL_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define WinStateAllWorkspaces  (1L<<0)

void create_gnome_atoms (void);
void gnome_change_desk (int desk);
void gnome_set_desk_count (int desk);
void gnome_sticky (GdkWindow * win);
void gnome_layer (GdkWindow * win, gint layer);
gint gnome_desk_change (GdkEventProperty * event);
void gnome_set_root_event (GtkSignalFunc f);

#endif
