/*  gxfce
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

/****************************************************************************
 * This module was originally based on the twm module of the same name. 
 * Since its use and contents have changed so dramatically, I have removed
 * the original twm copyright, and inserted my own.
 *
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/

/**********************************************************************
 *
 * Codes for xfwm builtins 
 *
 **********************************************************************/

#ifndef _PARSE_
#define _PARSE_

#define F_NOP			0
#define F_BEEP			1
#define F_QUIT			2
#define F_RESTART               3
#define F_REFRESH		4
#define F_TITLE			5
#define F_CIRCULATE_UP          6
#define F_CIRCULATE_DOWN        7
#define F_WINDOWLIST            8
#define F_MOVECURSOR            9
#define F_FUNCTION              10
#define F_MODULE                11
#define F_DESK                  12
#define F_CHANGE_WINDOWS_DESK   13
#define F_EXEC			14	/* string */
#define F_POPUP			15	/* string */
#define F_WAIT                  16
#define F_CLOSE                 17
#define F_SET_MASK              18
#define F_ADDMENU               19
#define F_ADDFUNC               20
#define F_STYLE                 21
#define F_PIXMAP_PATH           22
#define F_ICON_PATH             23
#define F_MODULE_PATH           24
#define F_HICOLOR               25
#define F_LOCOLOR               26
#define F_MOUSE                 27
#define F_KEY                   28
#define F_OPAQUE                29
#define F_XOR                   30
#define F_CLICK                 31
#define F_MENUCOLOR             32
#define F_ICONFONT              33
#define F_WINDOWFONT            34
#define F_BUTTON_STYLE          35
#define F_READ                  36
#define F_ADDMENU2              37
#define F_NEXT                  38
#define F_PREV                  39
#define F_NONE                  40
#define F_STAYSUP	        41	/* string */
#define F_RECAPTURE             42
#define F_CONFIG_LIST	        43
#define F_DESTROY_MENU	        44
#define F_ZAP	                45
#define F_QUIT_SCREEN		46
#define F_COLORMAP_FOCUS        47
#define F_TITLESTYLE            48
#define F_EXEC_SETUP            49
#define F_CURSOR_STYLE          50
#define F_CURRENT               51
#define F_CURSOR_COLOR          52
#define F_MENUFONT              53
#define F_SETFOCUSMODE          54
#define F_ANIMATE               55
#define F_OPAQUEMOVE            56
#define F_AUTORAISE             57
#define F_AUTORAISEDELAY        58
#define F_ICONPOS               59
#define F_ARRANGEICONS          60
#define F_SNAPSIZE		61
#define F_OPAQUERESIZE          62
#define F_WAITSESSION           63
#define F_SESSIONMGT            64
#define F_ICONGRID              65
#define F_ICONSPACING           66
#define F_ENGINE                67
#define F_SHOWBUTTONS	        68
#define F_SETCLICKRAISE	        69
#define F_SETFORCEFOCUS	        70
#define F_SHOWMEMOUSE	        71
#define F_SWITCHFUNC	        72
#define F_SETMAPFOCUS	        73
#define F_MARGIN	        74
#define F_READCFG               75
#define F_SETHONORWMFOCUSHINT   76
#define F_SETUSESHAPEDICONS     77
/* Functions which require a target window */
#define F_RESIZE		100
#define F_RAISE			101
#define F_LOWER			102
#define F_DESTROY		103
#define F_DELETE		104
#define F_MOVE			105
#define F_ICONIFY		106
#define F_STICK                 107
#define F_RAISELOWER            108
#define F_MAXIMIZE              109
#define F_SHADE                 109
#define F_FOCUS                 110
#define F_WARP                  111
#define F_SEND_STRING           112
#define F_ADD_MOD               113
#define F_DESTROY_MOD           114
#define F_FLIP_FOCUS            115
#define F_ECHO                  116
#define F_GLOBAL_OPTS           118
#define F_WINDOWID              119
#define F_ADD_BUTTON_STYLE      120
#define F_ADD_TITLE_STYLE       121
#define F_WINDOW_SHADE          125
#define F_ICONIFYALL		126

/* Functions for use by modules only! */
#define F_SEND_WINDOW_LIST     1000

/* Functions for internal  only! */
/* #define F_RAISE_IT              2000 */

#endif /* _PARSE_ */
