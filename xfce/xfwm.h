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


#ifndef __XFWM_H__
#define __XFWM_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "xfcolor.h"

/*
   Define some commands to send to XFwm (or FVWM) 
 */

#define FVWM 1
#define XFWM 2

#define MOVE_CMD	    "Move"
#define DESK_CMD	    "Desk 0 %i"
#define QUIT_CMD	    "Quit"
#define ICON_CMD	    "Iconify"
#define NOP_CMD	            "Nop"
#define FOCUSCLICK_CMD	    "FocusMode ClickToFocus"
#define FOCUSMOUSE_CMD	    "FocusMode FollowMouse"
#define OPAQUEMOVEON_CMD    "OpaqueMove On"
#define OPAQUEMOVEOFF_CMD   "OpaqueMove Off"
#define OPAQUERESIZEON_CMD  "OpaqueResize On"
#define OPAQUERESIZEOFF_CMD "OpaqueResize Off"
#define AUTORAISEON_CMD     "AutoRaise On"
#define AUTORAISEOFF_CMD    "AutoRaise Off"
#define MAPFOCUSON_CMD      "MapFocus On"
#define MAPFOCUSOFF_CMD     "MapFocus Off"
#define REFRESH_CMD	    "Refresh 0"
#define ACTIVECOLOR_CMD	    "ActiveColor \"%s\" \"%s\""
#define INACTIVECOLOR_CMD   "InactiveColor \"%s\" \"%s\""
#define MENUCOLOR_CMD	    "MenuColor \"%s\" \"%s\" \"%s\" \"%s\""
#define TITLESTYLE_CMD	    "TitleStyle %s %s \"%s\" \"%s\""
#define CURSORCOLOR_CMD	    "CursorColor \"%s\" \"%s\""
#define WINDOWFONT_CMD	    "WindowFont \"%s\""
#define MENUFONT_CMD	    "MenuFont \"%s\""
#define ICONFONT_CMD	    "IconFont \"%s\""
#define ARRANGEICONS_CMD    "ArrangeIcons %u %u"
#define ICONPOS_CMD  	    "IconPos %u"
#define ACTIVE		    "Active"
#define INACTIVE	    "Inactive"
#define GRADIENT	    "Gradient"
#define SOLID		    "Solid"
#define SNAPSIZE_CMD	    "SnapSize %i"
#define DESTROYMENU_CMD     "DestroyMenu \"%s\""
#define ADDTOMENUDESK_CMD   "AddToMenu \"%s\" \"%s %u (%s)\" WindowsDesk 0 %u"
#define DESKMENU	    "__builtin_sendtodesk_menu__"
#define ENGINE_XFCE_CMD	    "Engine Xfce"
#define ENGINE_MOFIT_CMD    "Engine Mofit"
#define ENGINE_TRENCH_CMD   "Engine Trench"
#define ENGINE_GTK_CMD      "Engine Gtk"
#define ENGINE_LINEA_CMD    "Engine Linea"

void assume_event (unsigned long type, unsigned long *body);

/*
void 
DeadPipe (int);
 */

int initwm (int argc, char *argv[]);

void switch_to_screen (XFCE_palette *, int);

void quit_wm (void);

void init_watchpipe (void);

void stop_watchpipe (void);

void apply_wm_colors (const XFCE_palette *);

void apply_wm_fonts (void);

void apply_wm_iconpos (void);

void apply_wm_options (void);

void apply_wm_snapsize (void);

void apply_wm_engine (void);

void startup_wm_modules (void);

void apply_wm_desk_names (int);

#endif
