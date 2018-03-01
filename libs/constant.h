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



#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define XFCE3SIG   "# XFce 3 Configuration File - Do not edit"
#define XFCE3GTKRC "# gtkrc file generated automatically by XFce 3 - Do not edit"

#define SMALL_PANEL_ICONS  30
#define MEDIUM_PANEL_ICONS 45
#define LARGE_PANEL_ICONS  60

#define SMALL_POPUP_ICONS  20
#define MEDIUM_POPUP_ICONS 30
#define LARGE_POPUP_ICONS  45

#define XFCE_CURS    XC_left_ptr
#define MENU_CURS    XC_right_ptr

#ifndef PATH_MAX
#define DEFAULT_LENGTH 1024
#else
#if (PATH_MAX < 1024)
#define DEFAULT_LENGTH 1024
#else
#define DEFAULT_LENGTH PATH_MAX
#endif
#endif

#ifndef DEFAULTENGINE
#define DEFAULTENGINE 	"gtk"
#endif

#ifndef XFCE_THEME_ENGINE
#define XFCE_THEME_ENGINE	"xfce"
#endif

#ifndef DEFAULTTEXTURE
#define DEFAULTTEXTURE 	""
#endif

#ifndef DEFAULTFONT
#define DEFAULTFONT 	"-adobe-helvetica-medium-r-normal-*-12-*-*-*-p-*-iso8859-15"
#endif

#ifndef XFCEDEFAULTFONT
#define XFCEDEFAULTFONT	DEFAULTFONT
#endif

#ifndef TIPFONT
#define TIPFONT 	DEFAULTFONT
#endif

#ifndef BACKUPEXT
#define BACKUPEXT 	".bak"
#endif

#ifndef DEFAULT_SHELL
#define DEFAULT_SHELL 	"/bin/sh"
#endif

#ifndef XFCE_DIR
#define XFCE_DIR        "/usr/share/xfce"
#endif

#ifndef XFCE_PAL
#define XFCE_PAL 	"/palettes/"
#endif

#ifndef XFCE_BACKDROPS
#define XFCE_BACKDROPS 	"/backdrops/"
#endif

#ifndef XFCE_ICONS
#define XFCE_ICONS 	"/icons/"
#endif

#ifndef XFCE_SOUNDS
#define XFCE_SOUNDS 	"/sounds/"
#endif

#ifndef XBINDIR
#define XBINDIR 	"/usr/X11R6/bin/"
#endif

#ifndef XFCE_LOGO
#define XFCE_LOGO 	"/XFce3.logo"
#endif

#ifndef XFCE_LICENSE
#define XFCE_LICENSE 	"/COPYING"
#endif

#ifndef XFWM_TITLEFONT
#define XFWM_TITLEFONT 	DEFAULTFONT
#endif

#ifndef XFWM_MENUFONT
#define XFWM_MENUFONT 	DEFAULTFONT
#endif

#ifndef XFWM_ICONFONT
#define XFWM_ICONFONT 	DEFAULTFONT
#endif


#ifndef NBMAXITEMS
#define NBMAXITEMS 	15
#endif

#ifndef MINRESISTANCE
#define MINRESISTANCE 	6
#endif

#ifndef EDGEFACTOR
#define EDGEFACTOR 	3
#endif

#ifndef NBMAXITEMS
#define NBMAXITEMS 	15
#endif

#ifndef TOPHEIGHT
#define TOPHEIGHT 	12
#endif

#ifndef SCREEN_BUTTON_WIDTH
#define SCREEN_BUTTON_WIDTH 	80
#endif

#ifndef MAXSTRLEN
#define MAXSTRLEN 	DEFAULT_LENGTH
#endif

#ifndef NAME_MAX
#define NAME_MAX 	DEFAULT_LENGTH
#endif

#ifndef PATH_MAX
#define PATH_MAX 	DEFAULT_LENGTH
#endif

#ifndef TERMINAL
#define TERMINAL "xterm"
#endif

#ifndef MAILCHECK_PERIOD
#define MAILCHECK_PERIOD 10000
#endif

#ifndef MAX_LAYERS
#define MAX_LAYERS 	12
#endif

#ifndef DEFAULT_LAYER
#define DEFAULT_LAYER 	6
#endif

#define NBSCREENS   	 10
#define NBPOPUPS	 12
#define NBSELECTS 	 NBPOPUPS+1
#define NB_XFCE_COLORS 	 8
#define NB_PANEL_ICONS 	 13
#define DEFAULT_ICON_SEQ "0,5,1,4,2,3,6,7,8,9,10,11,12"
#endif
