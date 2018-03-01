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
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation
 ****************************************************************************/
/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * xfwm per-screen data include file
 *
 ***********************************************************************/

#ifndef _SCREEN_
#define _SCREEN_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "xfwm.h"
#include "menus.h"
#include "utils.h"

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3
#define MAX_WINDOW_WIDTH 32767
#define MAX_WINDOW_HEIGHT 32767


/* Cursor types */
#define POSITION 0		/* upper Left corner cursor */
#define TITLE_CURSOR 1		/* title-bar cursor */
#define DEFAULT 2		/* cursor for apps to inherit */
#define SYS 3			/* sys-menu and iconify boxes cursor */
#define MOVE 4			/* resize cursor */
#if defined(__alpha)
#ifdef WAIT
#undef WAIT
#endif /*WAIT */
#endif /*alpha */
#define WAIT 5			/* wait a while cursor */
#define MENU 6			/* menu cursor */
#define SELECT 7		/* dot cursor for f.move, etc. from menus */
#define DESTROY 8		/* skull and cross bones, f.destroy */
#define TOP 9
#define RIGHT 10
#define BOTTOM 11
#define LEFT 12
#define TOP_LEFT 13
#define TOP_RIGHT 14
#define BOTTOM_LEFT 15
#define BOTTOM_RIGHT 16
#define MAX_CURSORS 18

/* colormap focus styes */
#define COLORMAP_FOLLOWS_MOUSE 1	/* default */
#define COLORMAP_FOLLOWS_FOCUS 2


typedef struct name_list_struct
{
  struct name_list_struct *next;	/* pointer to the next name */
  char *name;			/* the name of the window */
  char *value;			/* icon name */
  int Desk;			/* Desktop number */
  unsigned long on_flags;
  unsigned long off_flags;
  int border_width;
  int layer;
  int resize_width;
  char *ForeColor;
  char *BackColor;
  unsigned long on_buttons;
  unsigned long off_buttons;

}
name_list;

/* used for parsing configuration */
struct config
{
  char *keyword;
#ifdef __STDC__
  void (*action) (char *, FILE *, char **, int *);
#else
  void (*action) ();
#endif
  char **arg;
  int *arg2;
};

/* used for parsing commands */
struct functions
{
  char *keyword;
#ifdef __STDC__
  void (*action) (XEvent *, Window, XfwmWindow *, unsigned long, char *, int *);
#else
  void (*action) ();
#endif
  int code;
  int type;
};

/* values for name_list flags */
/* The first 13 items are mapped directly into the XfwmWindow structures
 * flag value, so they MUST correspond to the first 8 entries in xfwm.h */
#define START_ICONIC_FLAG         (1L<<0)
#define STICKY_FLAG               (1L<<1)
#define LISTSKIP_FLAG             (1L<<2)
#define SUPPRESSICON_FLAG         (1L<<3)
#define STICKY_ICON_FLAG          (1L<<4)
#define CIRCULATE_SKIP_ICON_FLAG  (1L<<5)
#define CIRCULATESKIP_FLAG        (1L<<6)

#define CLICK_FOCUS_FLAG          (1L<<7)
#define SLOPPY_FOCUS_FLAG         (1L<<8)
#define NOTITLE_FLAG              (1L<<9)
#define NOBORDER_FLAG             (1L<<10)
#define ICON_FLAG                 (1L<<11)
#define STARTONDESK_FLAG          (1L<<12)
#define BW_FLAG                   (1L<<13)
#define FORE_COLOR_FLAG           (1L<<14)
#define BACK_COLOR_FLAG           (1L<<15)
#define MWM_BUTTON_FLAG           (1L<<16)
#define MWM_DECOR_FLAG            (1L<<17)
#define MWM_FUNCTIONS_FLAG        (1L<<18)
#define TRANSLATE_FLAG            (1L<<19)
#define ALLOWFREEMOVE_FLAG        (1L<<20)

#define ButtonFaceTypeMask      0x000F

#define WindowsCaptured            (1L<<0)

typedef enum
{
  XFCE_ENGINE,
  MOFIT_ENGINE,
  TRENCH_ENGINE,
  GTK_ENGINE,
  LINEA_ENGINE
}
EngineType;

typedef enum
{
  PixButton,
  SimpleButton,
  GradButton,
  SolidButton
}
ButtonFaceStyle;

typedef struct ButtonFace
{
  ButtonFaceStyle style;
  struct
  {
    Pixel back;
    ColorPair Relief;
    GC ShadowGC;		/* GC for button shadow */
    GC ReliefGC;		/* GC for button background */
    struct
    {
      int npixels;
      Pixel *pixels;
    }
    grad;
  }
  u;
  struct vector_coords
  {
    int num;
    int x[20];
    int y[20];
    int line_style[20];
  }
  vector;
  Pixmap bitmap;
  Pixmap bitmap_pressed;
  struct ButtonFace *next;
}
ButtonFace;

/* button style flags (per title button) */
enum
{
  /* MWM function hint button assignments */
  MWMDecorMenu = (1 << 0),
  MWMDecorMinimize = (1 << 1),
  MWMDecorMaximize = (1 << 2),
  DecorSticky = (1 << 3),
  DecorShaded = (1 << 4)
};

enum ButtonState
{
  Active, Inactive,
  MaxButtonState
};

typedef struct
{
  int flags;
  ButtonFace state[MaxButtonState];
}
TitleButton;

typedef struct XfwmDecor
{
  ColorPair HiColors;		/* standard fore/back colors */
  ColorPair LoColors;		/* standard fore/back colors */
  ColorPair HiRelief;
  ColorPair LoRelief;
  GC HiReliefGC;		/* GC for active window relief */
  GC HiShadowGC;		/* GC for active window shadow */
  GC HiBackGC;			/* GC for active window background */
  GC LoReliefGC;		/* GC for inactive window relief */
  GC LoShadowGC;		/* GC for inactive window shadow */
  GC LoBackGC;			/* GC for inactive window background */

  int TitleHeight;		/* height of the title bar window */
  XfwmFont WindowFont;		/* font structure for window titles */

  /* titlebar buttons */
  TitleButton left_buttons[3];
  TitleButton right_buttons[3];
  TitleButton titlebar;
  struct BorderStyle
  {
    ButtonFace active, inactive;
  }
  BorderStyle;
}
XfwmDecor;


typedef struct ScreenInfo
{

  unsigned long screen;
  int d_depth;			/* copy of DefaultDepth(dpy, screen) */
  int NumberOfScreens;		/* number of screens on display */
  int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;		/* my copy of DisplayHeight(dpy, screen) */

  XfwmWindow XfwmRoot;		/* the head of the xfwm window list */
  Window Root;			/* the root window */
  Window NoFocusWin;		/* Window which will own focus when no other window have focus */
  Window GnomeProxyWin;		/* Root proxy */

  Binding *AllBindings;
  MenuRoot *AllMenus;

  int root_pushes;		/* current push level to install root
				   * colormap windows */
  XfwmWindow *pushed_window;	/* saved window to install when pushes drops
				   * to zero */
  Cursor XfwmCursors[MAX_CURSORS];

  name_list *TheList;		/* list of window names with attributes */
  char *DefaultIcon;		/* Icon to use when no other icons are found */

  ColorPair MenuColors;
  ColorPair MenuSelColors;
  ColorPair MenuRelief;

  XfwmFont StdFont;		/* font structure */
  XfwmFont IconFont;		/* for icon labels */
  XfwmFont WindowFont;		/* font structure for window titles */

  GC TransMaskGC;		/* GC for transparency masks */
  GC DrawGC;			/* GC to draw lines for move and resize */
  GC HintsGC;			/* GC to draw lines and strings */
  GC MenuGC;
  GC MenuReliefGC;
  GC MenuShadowGC;
  GC MenuSelGC;
  GC MenuSelReliefGC;
  GC MenuSelShadowGC;
  GC ScratchGC1;
  GC ScratchGC3;
  GC BlackGC;

  int SizeStringWidth;		/* minimum width of size window */
  int CornerWidth;		/* corner width for decoratedwindows */
  int BoundaryWidth;		/* frame width for decorated windows */
  int NoBoundaryWidth;		/* frame width for decorated windows */

  XfwmDecor DefaultDecor;	/* decoration style(s) */

  int nr_left_buttons;		/* number of left-side title-bar buttons */
  int nr_right_buttons;		/* number of right-side title-bar buttons */

  XfwmWindow *Hilite;		/* the xfwm window that is highlighted
				   * except for networking delays, this is the
				   * window which REALLY has the focus */
  XfwmWindow *Focus;		/* Last window which Xfwm gave the focus to
				   * NOT the window that really has the focus */
  Window UnknownWinFocused;	/* None, if the focus is nowhere or on an xfwm
				   * * managed window. Set to id of otherwindow 
				   * * with focus otherwise */
  XfwmWindow *PreviousFocus;	/* Window which had focus before xfwm stole it
				   * to do moves/menus/etc. */
  int EntryHeight;		/* menu entry height */
  int EdgeScrollX;		/* #pixels to scroll on screen edge */
  int EdgeScrollY;		/* #pixels to scroll on screen edge */
  unsigned long flags;
  XfwmWindow *LastWindowRaised;	/* Last window which was raised */
  XfwmWindow *LastWindowLowered;	/* Last window which was lowered */
  int ClickTime;		/*Max button-click delay for Function built-in */
  int AutoRaiseDelay;		/* Delay between setting focus and raising win */
  int CurrentDesk;		/* The current desktop number */
  int ColormapFocus;		/* colormap focus style */
  int iconbox;			/* 0 = top, 1 = left, 2 = bottom 3 = right */
  int icongrid;
  int iconspacing;
  EngineType engine;

  XfwmWindowList *stacklist;
#ifdef MANAGE_OVERRIDES
  WindowList *overrides;
#endif
  /*
   * ** some additional global options which will probably become window
   * ** specific options later on:
   */
  int ClickToFocusPassesClick;
  int SnapSize;
  unsigned int Options;

  /* AutoDeskSwitch */
  int ndesks;
  int sessionwait;

  /* Screen margins */
  unsigned int Margin[4];
}
ScreenInfo;

/*
 * Macro which gets specific decor or default decor.
 * This saves an indirection in case you don't want
 * the UseDecor mechanism.
 */
#define GetDecor(window,part) (Scr.DefaultDecor.part)

/* some protos for the decoration structures */
void LoadDefaultLeftButton (ButtonFace * bf, int i);
void LoadDefaultRightButton (ButtonFace * bf, int i);
void LoadDefaultButton (ButtonFace * bf, int i);
void ResetAllButtons (XfwmDecor * fl);
void InitXfwmDecor (XfwmDecor * fl);
void DestroyXfwmDecor (XfwmDecor * fl);

extern ScreenInfo Scr;

#endif /* _SCREEN_ */
