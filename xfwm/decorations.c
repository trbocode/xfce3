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
 * 
 * This is from original code 
 * by Robert Nation 
 * which reads motif mwm window manager
 * hints from a window, and makes necessary adjustments for xfwm. 
 ****************************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xfwm.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "menus.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern Atom _XA_MwmAtom;

/* Motif WM window hints structure */
typedef struct
{
  CARD32 flags;			/* window hints */
  CARD32 functions;		/* requested functions */
  CARD32 decorations;		/* requested decorations */
  INT32 inputMode;		/* input mode */
  CARD32 status;		/* status (ignored) */
}
PropMotifWmHints;

typedef struct _extended_hints
{
  int flags;
  int desktop;
}
ExtendedHints;

typedef PropMotifWmHints PropMwmHints;

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

#define EXTENDED_HINT_STICKY            (1<<0)
#define EXTENDED_HINT_ONTOP             (1<<1)
#define EXTENDED_HINT_ONBOTTOM          (1<<2)
#define EXTENDED_HINT_NEVER_USE_AREA    (1<<3)
#define EXTENDED_HINT_DESKTOP           (1<<4)

#define KDE_noDecoration              0
#define KDE_normalDecoration          1
#define KDE_tinyDecoration            2

extern XfwmWindow *Tmp_win;

/****************************************************************************
 * 
 * Reads the property MOTIF_WM_HINTS
 *
 *****************************************************************************/
void
GetMwmHints (XfwmWindow * t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;

  if (t->mwm_hints)
    XFree ((char *) t->mwm_hints);
  t->mwm_hints = NULL;
  if ((XGetWindowProperty (dpy, t->w, _XA_MwmAtom, 0L, 20L, False, _XA_MwmAtom, &actual_type, &actual_format, &nitems, &bytesafter, (unsigned char **) &t->mwm_hints) == Success) && (t->mwm_hints))
  {
    if (nitems >= PROP_MOTIF_WM_HINTS_ELEMENTS)
      return;
  }

  t->mwm_hints = NULL;
}

void
GetExtHints (XfwmWindow * t)
{
  Atom a1, a3;
  unsigned long lnum, ldummy;
  int dummy;
  ExtendedHints *eh;

  a1 = XInternAtom (dpy, "WM_EXTENDED_HINTS", False);
  eh = NULL;

  if (XGetWindowProperty (dpy, t->w, a1, 0L, (long) (sizeof (ExtendedHints) / sizeof (unsigned long)), False, a1, &a3, &dummy, &lnum, &ldummy, (unsigned char **) &eh) == Success)
  {
    if (eh)
    {
      if (eh->flags & EXTENDED_HINT_STICKY)
	t->flags |= STICKY;
      if (eh->flags & EXTENDED_HINT_ONTOP)
	t->layer = MAX_LAYERS;
      if (eh->flags & EXTENDED_HINT_ONBOTTOM)
	t->layer = 0;
      if (eh->flags & EXTENDED_HINT_NEVER_USE_AREA)
	;			/* Not implemented */
      if (eh->flags & EXTENDED_HINT_DESKTOP)
	;			/* Not implemented */
      XFree (eh);
    }
  }
}

void
GetKDEHints (XfwmWindow * t)
{
  Atom a, dummy_a;
  unsigned long lnum, ldummy;
  int dummy;
  long *kh;

  kh = NULL;
  a = XInternAtom (dpy, "KWM_WIN_DECORATION", False);
  t->kde_hints = -1;
  if (XGetWindowProperty (dpy, t->w, a, 0L, 1L, False, a, &dummy_a, &dummy, &lnum, &ldummy, (unsigned char **) &kh) == Success);
  {
    if (kh)
    {
      t->kde_hints = (kh[0] & 3);
      XFree ((char *) kh);
    }
  }
}

/****************************************************************************
 * 
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *
 *****************************************************************************/
void
SelectDecor (XfwmWindow * t, unsigned long tflags, int border_width)
{
  int decor, i;
  PropMwmHints *prop;
  int bw = 0;
#if 0
#ifndef OLD_STYLE
  int bw = 1;
#else
  int bw = 0;
#endif
#endif
  if (!(tflags & BW_FLAG))
  {
    border_width = Scr.NoBoundaryWidth;
  }

  for (i = 0; i < 3; i++)
  {
    t->left_w[i] = 1;
    t->right_w[i] = 1;
  }

  decor = MWM_DECOR_ALL;
  t->functions = MWM_FUNC_ALL;
  if (t->mwm_hints)
  {
    prop = (PropMwmHints *) t->mwm_hints;
    if (tflags & MWM_DECOR_FLAG)
      if (prop->flags & MWM_HINTS_DECORATIONS)
	decor = prop->decorations;
    if (tflags & MWM_FUNCTIONS_FLAG)
      if (prop->flags & MWM_HINTS_FUNCTIONS)
	t->functions = prop->functions;
  }

  /* functions affect the decorations! if the user says
   * no iconify function, then the iconify button doesn't show
   * up. */
  if (t->functions & MWM_FUNC_ALL)
  {
    /* If we get ALL + some other things, that means to use
     * ALL except the other things... */
    t->functions &= ~MWM_FUNC_ALL;
    t->functions = (MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE) & (~(t->functions));
  }
  if ((tflags & MWM_FUNCTIONS_FLAG) && (t->flags & TRANSIENT))
  {
    t->functions &= ~(MWM_FUNC_MAXIMIZE | MWM_FUNC_MINIMIZE);
  }

  if (decor & MWM_DECOR_ALL)
  {
    /* If we get ALL + some other things, that means to use
     * ALL except the other things... */
    decor &= ~MWM_DECOR_ALL;
    decor = (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE) & (~decor);
  }

  if (!(t->functions & MWM_FUNC_RESIZE))
    decor &= ~MWM_DECOR_RESIZEH;
  if (!(t->functions & MWM_FUNC_MINIMIZE))
    decor &= ~MWM_DECOR_MINIMIZE;
  if (!(t->functions & MWM_FUNC_MAXIMIZE))
    decor &= ~MWM_DECOR_MAXIMIZE;
  if (decor & (MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
    decor |= MWM_DECOR_TITLE;

  if (tflags & NOTITLE_FLAG)
    decor &= ~MWM_DECOR_TITLE;

  if (tflags & NOBORDER_FLAG)
    decor &= ~(MWM_DECOR_RESIZEH | MWM_DECOR_BORDER);

  if ((tflags & MWM_DECOR_FLAG) && (t->flags & TRANSIENT))
  {
    decor &= ~(MWM_DECOR_MAXIMIZE | MWM_DECOR_MINIMIZE);
  }

  if (t->kde_hints == KDE_normalDecoration)
  {
    decor |= MWM_DECOR_ALL;
    t->functions |= MWM_FUNC_ALL;
  }
  else if (t->kde_hints == KDE_noDecoration)
  {
    decor &= ~(MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE);
    t->functions &= ~MWM_FUNC_ALL;
    border_width = Scr.NoBoundaryWidth;
    bw = 0;
  }
  else if (t->kde_hints == KDE_tinyDecoration)
  {
    decor &= ~(MWM_DECOR_MAXIMIZE | MWM_DECOR_MINIMIZE | MWM_DECOR_TITLE | MWM_DECOR_RESIZEH);
    t->functions &= ~(MWM_DECOR_MENU | MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE | MWM_DECOR_TITLE);
    border_width = 3;
    bw = 0;
  }

  if (ShapesSupported)
  {
    if (t->wShaped)
      decor &= ~(MWM_DECOR_BORDER | MWM_DECOR_RESIZEH);
  }

  t->flags &= ~(BORDER | TITLE);
  t->boundary_width = 0;
  t->corner_width = 0;
  t->title_height = 0;
  t->bw = 0;
  if (decor & MWM_DECOR_TITLE)
  {
    t->flags |= TITLE;
    t->title_height = GetDecor (t, TitleHeight);
  }
  if (decor & (MWM_DECOR_RESIZEH | MWM_DECOR_BORDER))
  {
    t->flags |= BORDER;
    t->boundary_width = border_width;
    t->bw = bw;
    t->corner_width = GetDecor (t, TitleHeight) + t->boundary_width;
  }
  if (!(decor & MWM_DECOR_MENU))
  {
    for (i = 0; i < 3; ++i)
    {
      if (GetDecor (t, left_buttons[i].flags) & MWMDecorMenu)
	t->left_w[i] = None;
      if (GetDecor (t, right_buttons[i].flags) & MWMDecorMenu)
	t->right_w[i] = None;
    }
  }
  if (!(decor & MWM_DECOR_MINIMIZE))
  {
    for (i = 0; i < 3; ++i)
    {
      if (GetDecor (t, left_buttons[i].flags) & MWMDecorMinimize)
	t->left_w[i] = None;
      if (GetDecor (t, right_buttons[i].flags) & MWMDecorMinimize)
	t->right_w[i] = None;
    }
  }
  if (!(decor & MWM_DECOR_MAXIMIZE))
  {
    for (i = 0; i < 3; ++i)
    {
      if (GetDecor (t, left_buttons[i].flags) & MWMDecorMaximize)
	t->left_w[i] = None;
      if (GetDecor (t, right_buttons[i].flags) & MWMDecorMaximize)
	t->right_w[i] = None;
    }
  }

  if (tflags & SUPPRESSICON_FLAG)
    t->flags |= SUPPRESSICON;

  t->nr_left_buttons = Scr.nr_left_buttons;
  t->nr_right_buttons = Scr.nr_right_buttons;

  for (i = 0; i < Scr.nr_left_buttons; i++)
    if (t->left_w[i] == None)
      t->nr_left_buttons--;

  for (i = 0; i < Scr.nr_right_buttons; i++)
    if (t->right_w[i] == None)
      t->nr_right_buttons--;

  if (t->boundary_width <= 0)
    t->boundary_width = 0;
  if (t->boundary_width == 0)
    t->flags &= ~BORDER;
}

/****************************************************************************
 * 
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 * 
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int
check_allowed_function (MenuItem * mi)
{
  /* Complex functions are a little tricky... ignore them for now */

  if ((Tmp_win) && (!(Tmp_win->flags & DoesWmDeleteWindow)) && (mi->func_type == F_DELETE))
    return 0;

  /* Move is a funny hint. Keeps it out of the menu, but you're still allowed
   * to move. */
  if ((mi->func_type == F_MOVE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MOVE)))
    return 0;

  if ((mi->func_type == F_RESIZE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_RESIZE)))
    return 0;

  if ((mi->func_type == F_ICONIFY) && (Tmp_win) && (!(Tmp_win->flags & ICONIFIED)) && (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)))
    return 0;

  if ((mi->func_type == F_MAXIMIZE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE)))
    return 0;

  if ((mi->func_type == F_DELETE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if ((mi->func_type == F_CLOSE) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if ((mi->func_type == F_DESTROY) && (Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if (mi->func_type == F_FUNCTION)
  {
    /* Hard part! What to do now? */
    /* Hate to do it, but for lack of a better idea,
     * check based on the menu entry name */
    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MOVE)) && (mystrncasecmp (mi->item, MOVE_STRING, strlen (MOVE_STRING)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_RESIZE)) && (mystrncasecmp (mi->item, RESIZE_STRING1, strlen (RESIZE_STRING1)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_RESIZE)) && (mystrncasecmp (mi->item, RESIZE_STRING2, strlen (RESIZE_STRING2)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)) && (!(Tmp_win->flags & ICONIFIED)) && (mystrncasecmp (mi->item, MINIMIZE_STRING, strlen (MINIMIZE_STRING)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)) && (mystrncasecmp (mi->item, MINIMIZE_STRING2, strlen (MINIMIZE_STRING2)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE)) && (mystrncasecmp (mi->item, MAXIMIZE_STRING, strlen (MAXIMIZE_STRING)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) && (mystrncasecmp (mi->item, CLOSE_STRING1, strlen (CLOSE_STRING1)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) && (mystrncasecmp (mi->item, CLOSE_STRING2, strlen (CLOSE_STRING2)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) && (mystrncasecmp (mi->item, CLOSE_STRING3, strlen (CLOSE_STRING3)) == 0))
      return 0;

    if ((Tmp_win) && (!(Tmp_win->functions & MWM_FUNC_CLOSE)) && (mystrncasecmp (mi->item, CLOSE_STRING4, strlen (CLOSE_STRING4)) == 0))
      return 0;

  }
  return 1;
}
