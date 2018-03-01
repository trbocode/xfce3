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
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "my_intl.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "stack.h"
#include "themes.h"
#include "xinerama.h"
#include "constant.h"

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_X11_XFT_XFT_H
extern Bool enable_xft;
#endif

static char *exec_shell_name = "/bin/sh";
char *ModulePath = "./";
/* button state strings must match the enumerated states */
static char *button_states[MaxButtonState] = { "Active", "Inactive", };

extern void KillModuleByName (char *name);

void
my_sleep (int n)
{
  struct timeval value;

  if (n > 0)
  {
    value.tv_usec = n % 1000000;
    value.tv_sec = n / 1000000;
    (void) select (1, 0, 0, 0, &value);
  }
}

void
Animate (int o_x, int o_y, int o_w, int o_h, int f_x, int f_y, int f_w, int f_h)
{
  int delta_x, delta_y, delta_w, delta_h, j;
  XGCValues gcv;

  if (Scr.Options & AnimateWin)
  {
    gcv.line_width = 2;

    XChangeGC (dpy, Scr.DrawGC, GCLineWidth, &gcv);

    MyXGrabServer (dpy);

    delta_x = (f_x - o_x);
    delta_y = (f_y - o_y);
    delta_w = (f_w - o_w);
    delta_h = (f_h - o_h);
    for (j = 0; j < 11; j++)
    {
      MoveOutline (0, o_x + j * delta_x / 12, o_y + j * delta_y / 12, o_w + j * delta_w / 12, o_h + j * delta_h / 12);
      XFlush (dpy);
      my_sleep (2);
    }
    MoveOutline (0, 0, 0, 0, 0);
    MyXUngrabServer (dpy);
    XFlush (dpy);
  }
}

void
ShowMeMouse (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XGCValues gcv;
  int xl, yt;
  int i;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &xl, &yt, &dummy_win_x, &dummy_win_y, &dummy_mask);

  gcv.line_width = 5;

  XChangeGC (dpy, Scr.DrawGC, GCLineWidth, &gcv);

  MyXGrabServer (dpy);

  for (i = 10; i < 120; i += 10)
  {
    XDrawArc (dpy, Scr.Root, Scr.DrawGC, xl - (i / 2), yt - (i / 2), i, i, 0, 360 * 64);
    XFlush (dpy);
    my_sleep (1);
    XDrawArc (dpy, Scr.Root, Scr.DrawGC, xl - (i / 2), yt - (i / 2), i, i, 0, 360 * 64);
  }
  MyXUngrabServer (dpy);
  XFlush (dpy);
}

/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to XfwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int
DeferExecution (XEvent * eventp, Window * w, XfwmWindow ** tmp_win, unsigned long *context, int cursor, int FinishEvent)
{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if ((*context != C_ROOT) && (*context != C_NO_CONTEXT))
  {
    if ((FinishEvent == ButtonPress) || ((FinishEvent == ButtonRelease) && (eventp->type != ButtonPress)))
    {
      return False;
    }
  }
  if (!GrabEm (cursor))
  {
    XBell (dpy, Scr.screen);
    xfwm_msg (WARN, "DeferExecution", "Cannot grab cursor");
    return True;
  }

  while (!finished)
  {
    done = 0;
    /* block until there is an event */
    XMaskEvent (dpy, ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | VisibilityChangeMask | ButtonMotionMask | PointerMotionMask
		/* | EnterWindowMask | LeaveWindowMask */ , eventp);

#ifdef REQUIRES_STASHEVENT
    StashEventTime (eventp);
#endif
    if (eventp->type == KeyPress)
      Keyboard_shortcuts (eventp, FinishEvent);
    if (eventp->type == FinishEvent)
      finished = 1;
    if (eventp->type == ButtonPress)
    {
      XAllowEvents (dpy, ReplayPointer, CurrentTime);
      done = 1;
    }
    if (eventp->type == ButtonRelease)
      done = 1;
    if (eventp->type == KeyPress)
      done = 1;

    if (!done)
    {
      DispatchEvent ();
    }
  }

  *w = eventp->xany.window;
  if (((*w == Scr.Root) || (*w == Scr.NoFocusWin)) && (eventp->xbutton.subwindow != (Window) 0))
  {
    *w = eventp->xbutton.subwindow;
    eventp->xany.window = *w;
  }
  if (*w == Scr.Root)
  {
    *context = C_ROOT;
    UngrabEm ();
    return True;
  }
  if (XFindContext (dpy, *w, XfwmContext, (caddr_t *) tmp_win) == XCNOENT)
  {
    *tmp_win = NULL;
    XBell (dpy, Scr.screen);
    xfwm_msg (WARN, "DeferExecution", "Cannot determine context");
    UngrabEm ();
    return (True);
  }

  if (*w == (*tmp_win)->Parent)
    *w = (*tmp_win)->w;

  if (original_w == (*tmp_win)->Parent)
    original_w = (*tmp_win)->w;

  if ((*w != original_w) && (original_w != Scr.Root) && (original_w != None) && (original_w != Scr.NoFocusWin))
    if (!((*w == (*tmp_win)->frame) && (original_w == (*tmp_win)->w)))
    {
      *context = C_ROOT;
      UngrabEm ();
      return True;
    }

  *context = GetContext (*tmp_win, eventp, &dummy);

  UngrabEm ();
  return False;
}

/**************************************************************************
 *
 * Moves focus to specified window 
 *
 *************************************************************************/
void
FocusOn (XfwmWindow * t, Bool DeIconifyFlag)
{
  if (!AcceptInput(t))
    return;

  if ((DeIconifyFlag) && (t->flags & ICONIFIED))
    DeIconify (t);

  if ((DeIconifyFlag) && (t->flags & SHADED))
    Unshade (t);

  if (t->Desk != Scr.CurrentDesk)
  {
    if ((t->Desk >= 0) && (t->Desk < Scr.ndesks))
      changeDesks (0, t->Desk, 1, 1, 1);
    else
      sendWindowsDesk (Scr.CurrentDesk, t);
  }

  SetFocus (t->w, t, True, False);
}



/**************************************************************************
 *
 * Moves pointer to specified window 
 *
 *************************************************************************/
void
WarpOn (XfwmWindow * t, int warp_x, int x_unit, int warp_y, int y_unit)
{
  int x, y;

  if (!t || (t->flags & ICONIFIED && t->icon_w == None))
    return;

  if (t->Desk != Scr.CurrentDesk)
  {
    if ((t->Desk >= 0) && (t->Desk < Scr.ndesks))
      changeDesks (0, t->Desk, 1, 1, 1);
    else
      sendWindowsDesk (Scr.CurrentDesk, t);
  }

  if (t->flags & ICONIFIED)
  {
    x = t->icon_xl_loc + t->icon_w_width / 2 + 2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT / 2 + 2;
  }
  else
  {
    if (x_unit != MyDisplayWidth (warp_x, warp_y))
      x = t->frame_x + 2 + warp_x;
    else
      x = t->frame_x + 2 + (t->frame_width - 4) * warp_x / 100;
    if (y_unit != MyDisplayHeight (warp_x, warp_y))
      y = t->frame_y + 2 + warp_y;
    else
      y = t->frame_y + 2 + (((t->flags & SHADED) ? t->shade_height : t->frame_height) - 4) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0)
  {
    XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (x, y) - 1, x), my_min (MyDisplayMaxY (x, y) - 1, y));
  }
  RaiseWindow (t);

  /* If the window is still not visible, make it visible! */
  if (((t->frame_x + ((t->flags & SHADED) ? t->shade_height : t->frame_height)) < 0) || (t->frame_y + t->frame_width < 0) || (t->frame_x > Scr.MyDisplayWidth) || (t->frame_y > Scr.MyDisplayHeight))
  {
    SetupFrame (t, 0, 0, t->frame_width, ((t->flags & SHADED) ? t->shade_height : t->frame_height), False, True);
    XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, 2, 2);
  }
}

/***********************************************************************
 *  
 *  Procedure:
 *	(Un)Shade a window.
 *  by Khadiyd Idris <khadx@francemel.com>
 ***********************************************************************/

void
Shade (XfwmWindow * tmp_win)
{
  if (tmp_win == NULL)
    return;

  if (!(tmp_win->flags & TITLE))
    return;

  if (tmp_win->flags & ICONIFIED)
    return;

  tmp_win->flags |= SHADED;
  if (tmp_win->Desk == Scr.CurrentDesk)
    Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->title_height + 2 * tmp_win->boundary_width);
  tmp_win->shade_x = tmp_win->frame_x;
  tmp_win->shade_y = tmp_win->frame_y;
  tmp_win->shade_width = tmp_win->frame_width;
  tmp_win->shade_height = tmp_win->title_height + 2 * tmp_win->boundary_width;

  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height, True, True);
  SetBorder (tmp_win, NULL, (Scr.Hilite == tmp_win), True, True, None);
  Broadcast (XFCE_M_SHADE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, 0, 0, 0, 0);
}

void
Unshade (XfwmWindow * tmp_win)
{
  int tmp;

  if (tmp_win == NULL)
    return;

  if (tmp_win->flags & ICONIFIED)
    return;

  tmp_win->flags &= ~SHADED;
  if (tmp_win->Desk == Scr.CurrentDesk)
    Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->title_height + 2 * tmp_win->boundary_width, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height);
  tmp = tmp_win->frame_height;
  tmp_win->frame_height = 0;	/* force redrawing frame */
  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp, True, True);
  if (tmp_win->Desk == Scr.CurrentDesk)
  {
    RaiseWindow (tmp_win);
    FocusOn (tmp_win, False);
  }
  SetBorder (tmp_win, NULL, (Scr.Hilite == tmp_win), True, True, None);
  Broadcast (XFCE_M_UNSHADE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, 0, 0, 0, 0);
}

void
shade_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  if (tmp_win == NULL)
    return;

  if (!(tmp_win->flags & TITLE))
    return;

  if (tmp_win->flags & ICONIFIED)
    return;

  if (tmp_win->flags & SHADED)
  {
    Unshade (tmp_win);
  }
  else
  {
    Shade (tmp_win);
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void
Maximize (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int new_width, new_height, new_x, new_y;
  int val1, val2, val1_unit, val2_unit, n;
  int center_x;
  int center_y;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  if (tmp_win == NULL)
    return;

  center_x = tmp_win->frame_x + (tmp_win->frame_width / 2);
  center_y = tmp_win->frame_y + (tmp_win->frame_height / 2);

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, center_x, center_y);
  if (n != 2)
  {
    val1 = 100;
    val2 = 100;
    val1_unit = MyDisplayWidth (center_x, center_y);
    val2_unit = MyDisplayHeight (center_x, center_y);
  }

  if (tmp_win->flags & MAXIMIZED)
  {
    tmp_win->flags &= ~MAXIMIZED;
    if (!(tmp_win->flags & ICONIFIED))
    {
      Broadcast (XFCE_M_DEMAXIMIZE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, 0, 0, 0, 0);
      if (tmp_win->flags & SHADED)
	Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->title_height + 2 * tmp_win->boundary_width, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd, tmp_win->title_height + 2 * tmp_win->boundary_width);
      else
	Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd, tmp_win->orig_ht);
    }

    SetupFrame (tmp_win, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd, tmp_win->orig_ht, True, True);
    SetBorder (tmp_win, NULL, Scr.Hilite == tmp_win, True, True, None);
  }
  else
  {

    /* Transient windows can't be maximized separately */
    if (tmp_win->flags & TRANSIENT)
      return;

    new_width = tmp_win->frame_width;
    new_height = tmp_win->frame_height;
    new_x = tmp_win->frame_x;
    new_y = tmp_win->frame_y;
    if (val1 > 0)
    {
      new_x = MyDisplayX (center_x, center_y) + Scr.Margin[1];
      new_width = (val1 * val1_unit / 100) - (Scr.Margin[1] + Scr.Margin[3]);
    }
    if (val2 > 0)
    {
      new_y = MyDisplayY (center_x, center_y) + Scr.Margin[0];
      new_height = (val2 * val2_unit / 100) - (Scr.Margin[0] + Scr.Margin[2]);
    }
    if ((val1 == 0) && (val2 == 0))
    {
      new_x = MyDisplayX (center_x, center_y) + Scr.Margin[1];
      new_y = MyDisplayY (center_x, center_y) + Scr.Margin[0];
      new_height = MyDisplayHeight (center_x, center_y) - (Scr.Margin[0] + Scr.Margin[2]);
      new_width = MyDisplayWidth (center_x, center_y) - (Scr.Margin[1] + Scr.Margin[3]);
    }
    tmp_win->flags |= MAXIMIZED;
    ConstrainSize (tmp_win, &new_width, &new_height);
    /* 
       Just make sure that the window is not off screen once ConstrainSize
       has been called...
     */
    if ((new_x + new_width) > MyDisplayX (center_x, center_y) + MyDisplayWidth (center_x, center_y))
    {
      new_x = MyDisplayX (center_x, center_y);
    }
    if ((new_y + new_height) > MyDisplayY (center_x, center_y) + MyDisplayHeight (center_x, center_y))
    {
      new_y = MyDisplayY (center_x, center_y);
    }
    if (!(tmp_win->flags & ICONIFIED))
    {
      Broadcast (XFCE_M_MAXIMIZE, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, 0, 0, 0, 0);
      if (tmp_win->flags & SHADED)
	Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->title_height + 2 * tmp_win->boundary_width, new_x, new_y, new_width, tmp_win->title_height + 2 * tmp_win->boundary_width);
      else
	Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height, new_x, new_y, new_width, new_height);
    }
    SetupFrame (tmp_win, new_x, new_y, new_width, new_height, True, True);
    SetBorder (tmp_win, NULL, Scr.Hilite == tmp_win, True, True, None);
  }
}

MenuRoot *
FindPopup (char *action)
{
  char *tmp;
  MenuRoot *mr;

  GetNextToken (action, &tmp);

  if (tmp == NULL)
    return NULL;

  mr = Scr.AllMenus;
  while (mr != NULL)
  {
    if (mr->name != NULL)
      if (mystrcasecmp (tmp, mr->name) == 0)
      {
	free (tmp);
	return mr;
      }
    mr = mr->next;
  }
  free (tmp);
  return NULL;
}

void
Bell (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XBell (dpy, Scr.screen);
}


char *last_menu = NULL;
void
add_item_to_menu (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *mr;
  char *token, *rest, *item;

  rest = GetNextToken (action, &token);
  mr = FindPopup (token);
  if (mr == NULL)
    mr = NewMenuRoot (token, 0);
  if (last_menu != NULL)
    free (last_menu);
  last_menu = token;
  rest = GetNextToken (rest, &item);

  AddToMenu (mr, (strlen (item) ? _(item) : item), rest);
  free (item);

  MakeMenu (mr);
  return;
}


void
add_another_item (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *mr;
  char *rest, *item;

  if (last_menu == NULL)
    return;

  mr = FindPopup (last_menu);
  if (mr == NULL)
    return;
  rest = GetNextToken (action, &item);

  AddToMenu (mr, (strlen (item) ? _(item) : item), rest);
  free (item);

  MakeMenu (mr);
  return;
}

void
destroy_menu (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *mr;

  char *token, *rest;

  rest = GetNextToken (action, &token);
  mr = FindPopup (token);
  free (token);
  if (mr == NULL)
    return;
  DestroyMenu (mr);
  return;

}

void
add_item_to_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *mr;

  char *token, *rest, *item;

  rest = GetNextToken (action, &token);
  mr = FindPopup (token);
  if (mr == NULL)
    mr = NewMenuRoot (token, 1);
  if (last_menu != NULL)
    free (last_menu);
  last_menu = token;
  rest = GetNextToken (rest, &item);

  AddToMenu (mr, item, rest);
  free (item);

  return;
}

void
Nop_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
}

void
movecursor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int x, y;
  int val1, val2, val1_unit, val2_unit, n;
  int warpx, warpy;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &x, &y, &dummy_win_x, &dummy_win_y, &dummy_mask);
  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, x, y);
  warpx = x + val1 * val1_unit / 100;
  warpy = y + val2 * val2_unit / 100;
  XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx), my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy));
}


void
iconify_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);

  if (tmp_win->flags & ICONIFIED)
  {
    if (val1 <= 0)
      DeIconify (tmp_win);
  }
  else if (val1 >= 0)
    Iconify (tmp_win, eventp->xbutton.x_root - 5, eventp->xbutton.y_root - 5, True);
}

void
Iconify_all (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindowList *t;
  unsigned int opt;

  opt = Scr.Options & AnimateWin;
  Scr.Options &= ~AnimateWin;

  t = LastXfwmWindowList (Scr.stacklist);
  while (t)
  {
    if (!((t->win)->flags & ICONIFIED) && !((t->win)->flags & TRANSIENT) && ((t->win)->flags & TITLE) && ((t->win)->Desk == Scr.CurrentDesk))
    {
      Iconify ((t->win), 0, 0, False);
    }
    t = t->prev;
  }
  LowerIcons ();
  Scr.Options |= opt;
}

void
raise_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  if (tmp_win)
    RaiseWindow (tmp_win);
}

void
lower_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *MouseWin = NULL;
  Window mw = None;
  /* Dummy var for XQueryPointer */
  Window dummy_root;
  int dummy_x, dummy_y, dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  if (tmp_win)
  {
    LowerWindow (tmp_win);
  }
  /* Now handle focus transition */
  MouseWin = NULL;
  if ((XQueryPointer (dpy, Scr.Root, &dummy_root, &mw, &dummy_x, &dummy_y, &dummy_win_x, &dummy_win_y, &dummy_mask) && (mw != None) && (XFindContext (dpy, mw, XfwmContext, (caddr_t *) &MouseWin) == XCNOENT)))
  {
    MouseWin = NULL;
  }
  if ((MouseWin) && AcceptInput (MouseWin))
  {
    SetFocus (MouseWin->w, MouseWin, False, False);
  }
}

void
destroy_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_bw, dummy_depth;

  if (DeferExecution (eventp, &w, &tmp_win, &context, DESTROY, ButtonRelease))
    return;

  if (XGetGeometry (dpy, tmp_win->w, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth) == 0)
  {
    Destroy (tmp_win);
    tmp_win = NULL;
  }
  else
  {
    XKillClient (dpy, tmp_win->w);
  }
}

void
delete_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (DeferExecution (eventp, &w, &tmp_win, &context, DESTROY, ButtonRelease))
    return;

  if (tmp_win->flags & DoesWmDeleteWindow)
  {
    send_clientmessage (dpy, tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
    return;
  }
  else
  {
    XBell (dpy, Scr.screen);
  }
}

void
close_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_bw, dummy_depth;

  if (DeferExecution (eventp, &w, &tmp_win, &context, DESTROY, ButtonRelease))
    return;

  if (tmp_win->flags & DoesWmDeleteWindow)
  {
    delete_function (eventp, w, tmp_win, context, action, Module);
    return;
  }
  else if (XGetGeometry (dpy, tmp_win->w, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth) == 0)
  {
    Destroy (tmp_win);
    tmp_win = NULL;
  }
  else
  {
    XKillClient (dpy, tmp_win->w);
  }
}

void
restart_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  Done (XFWM_RESTART, action);
}

void
exec_setup (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *arg = NULL;

  action = GetNextToken (action, &arg);

  if (arg && (strcmp (arg, "") != 0))	/* specific shell was specified */
  {
    exec_shell_name = strdup (arg);
  }
  else
    /* no arg, so use $SHELL -- not working??? */
  {
    if (getenv ("SHELL"))
      exec_shell_name = strdup (getenv ("SHELL"));
    else
      exec_shell_name = strdup ("/bin/sh");	/* if $SHELL not set, use default */
  }
}

void
exec_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *cmd = NULL;
  int nulldev;

  /* if it doesn't already have an 'exec' as the first word, add that
   * to keep down number of procs started */
  /* need to parse string better to do this right though, so not doing this
   * for now... */
  if (0 && mystrncasecmp (action, "exec", 4) != 0)
  {
    cmd = (char *) safemalloc (strlen (action) + 6);
    strcpy (cmd, "exec ");
    strcat (cmd, action);
  }
  else
  {
    cmd = strdup (action);
  }
  /* Use to grab the pointer here, but the fork guarantees that
   * we wont be held up waiting for the function to finish,
   * so the pointer-gram just caused needless delay and flashing
   * on the screen */
  switch (fork ())
  {
  case 0:			/* child process */
    /* The following is to avoid X locking when executing
       terminal based application that requires user input */
    if ((nulldev = open ("/dev/null", O_RDWR)))
    {
      close (0);
      dup (nulldev);
    }
    signal (SIGCHLD, SIG_DFL);
    if (execl (exec_shell_name, exec_shell_name, "-c", cmd, NULL) == -1)
    {
      xfwm_msg (ERR, "exec_function", "execl failed (%s)", strerror (errno));
      exit (100);
    }
    break;
  case -1:
    xfwm_msg (WARN, "exec_function", "Cannot fork process");
    break;
  default:
    break;
  }
  free (cmd);
  return;
}

void
refresh_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  XfwmWindow *tmp;

  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
  attributes.backing_store = NotUseful;
  w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) Scr.MyDisplayWidth, (unsigned int) Scr.MyDisplayHeight, (unsigned int) 0, CopyFromParent, (unsigned int) CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);

  /* redraw all windows */
  tmp = Scr.XfwmRoot.next;
  while (tmp != NULL)
  {
    SetBorder (tmp, NULL, (Scr.Hilite == tmp), True, True, tmp->frame);
    tmp = tmp->next;
  }
}


void
refresh_win_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
  attributes.backing_store = NotUseful;
  w = XCreateWindow (dpy, (context == C_ICON) ? (tmp_win->icon_w) : (tmp_win->frame), 0, 0, (unsigned int) Scr.MyDisplayWidth, (unsigned int) Scr.MyDisplayHeight, (unsigned int) 0, CopyFromParent, (unsigned int) CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
}

void
stick_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *mode = NULL;
  unsigned long old_flags;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  action = GetNextToken (action, &mode);
  old_flags = tmp_win->flags;

  if ((mystrncasecmp (mode, "On", 2) == 0) || (mystrncasecmp (mode, "Y", 1) == 0) || (mystrncasecmp (mode, "1", 1) == 0))
  {
    tmp_win->flags |= STICKY;
#ifdef DEBUG
    fprintf (stderr, "stick_function: setting state sticky for win %s\n", tmp_win->name);
#endif
  }
  else if ((mystrncasecmp (mode, "Of", 2) == 0) || (mystrncasecmp (mode, "N", 1) == 0) || (mystrncasecmp (mode, "0", 1) == 0))
  {
    tmp_win->flags &= ~STICKY;
#ifdef DEBUG
    fprintf (stderr, "stick_function: removing state sticky for win %s\n", tmp_win->name);
#endif
  }
  else
  {
    if (tmp_win->flags & STICKY)
    {
      tmp_win->flags &= ~STICKY;
#ifdef DEBUG
      fprintf (stderr, "stick_function: toggle off state sticky for win %s\n", tmp_win->name);
#endif
    }
    else
    {
      tmp_win->flags |= STICKY;
#ifdef DEBUG
      fprintf (stderr, "stick_function: toggle on state sticky for win %s\n", tmp_win->name);
#endif
    }
  }
  free (mode);
  if (old_flags != tmp_win->flags)
  {
    RedrawRightButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);
    RedrawLeftButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);
    BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, tmp_win);
  }
}

void
wait_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  Bool done = False;
  extern XfwmWindow *Tmp_win;

  while (!done)
  {
    if (My_XNextEvent (dpy, &Event))
    {
      DispatchEvent ();
      if (Event.type == MapNotify)
      {
	if ((Tmp_win) && (matchWildcards (action, Tmp_win->name) == True))
	  done = True;
	if ((Tmp_win) && (Tmp_win->class.res_class) && (matchWildcards (action, Tmp_win->class.res_class) == True))
	  done = True;
	if ((Tmp_win) && (Tmp_win->class.res_name) && (matchWildcards (action, Tmp_win->class.res_name) == True))
	  done = True;
      }
    }
  }
}


void
flip_focus_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  /* Reorder the window list */
  if (Scr.Focus)
  {
    if (Scr.Focus->next)
      Scr.Focus->next->prev = Scr.Focus->prev;
    if (Scr.Focus->prev)
      Scr.Focus->prev->next = Scr.Focus->next;
    Scr.Focus->next = Scr.XfwmRoot.next;
    Scr.Focus->prev = &Scr.XfwmRoot;
    if (Scr.XfwmRoot.next)
      Scr.XfwmRoot.next->prev = Scr.Focus;
    Scr.XfwmRoot.next = Scr.Focus;
  }
  if (tmp_win != Scr.Focus)
  {
    if (tmp_win->next)
      tmp_win->next->prev = tmp_win->prev;
    if (tmp_win->prev)
      tmp_win->prev->next = tmp_win->next;
    tmp_win->next = Scr.XfwmRoot.next;
    tmp_win->prev = &Scr.XfwmRoot;
    if (Scr.XfwmRoot.next)
      Scr.XfwmRoot.next->prev = tmp_win;
    Scr.XfwmRoot.next = tmp_win;
  }

  FocusOn (tmp_win, False);
}


void
focus_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  FocusOn (tmp_win, False);
}


void
warp_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1_unit, val2_unit, n;
  int val1, val2;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, 0, 0);

  if (n == 2)
    WarpOn (tmp_win, val1, val1_unit, val2, val2_unit);
  else
    WarpOn (tmp_win, 0, 0, 0, 0);
}


void
popup_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;

  menu = FindPopup (action);
  if (menu == NULL)
  {
    xfwm_msg (ERR, "popup_func", "No such menu %s", action);
    return;
  }
  ActiveItem = NULL;
  ActiveMenu = NULL;
  menuFromFrameOrWindowOrTitlebar = False;
  do_menu (menu, 0);
}

void
staysup_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;
  char *menu_name = NULL;

  action = GetNextToken (action, &menu_name);
  menu = FindPopup (menu_name);
  if (menu == NULL)
  {
    if (menu_name != NULL)
    {
      xfwm_msg (ERR, "staysup_func", "No such menu %s", menu_name);
      free (menu_name);
    }
    return;
  }
  ActiveItem = NULL;
  ActiveMenu = NULL;
  menuFromFrameOrWindowOrTitlebar = False;

  /* See bottom of windows.c for rationale behind this */
  if (eventp->type == ButtonPress)
    do_menu (menu, 1);
  else
    do_menu (menu, 0);

  if (menu_name != NULL)
    free (menu_name);
}


void
quit_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (master_pid != getpid ())
    kill (master_pid, SIGTERM);
  Done (0, NULL);
}

void
quit_screen_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  Done (0, NULL);
}

void
echo_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  if (action && *action)
  {
    int len = strlen (action);
    if (action[len - 1] == '\n')
      action[len - 1] = '\0';
    xfwm_msg (INFO, "Echo", action);
  }
}

void
raiselower_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *MouseWin = NULL;
  Window mw = None;
  /* Dummy var for XQueryPointer */
  Window dummy_root;
  int dummy_x, dummy_y, dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  if (DeferExecution (eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;
  if (tmp_win == NULL)
    return;

  if ((tmp_win == Scr.LastWindowRaised) || (tmp_win->flags & VISIBLE))
  {
    LowerWindow (tmp_win);
  }
  else
  {
    RaiseWindow (tmp_win);
  }
  /* Now handle focus transition */
  MouseWin = NULL;
  if ((XQueryPointer (dpy, Scr.Root, &dummy_root, &mw, &dummy_x, &dummy_y, &dummy_win_x, &dummy_win_y, &dummy_mask) && (mw != None) && (XFindContext (dpy, mw, XfwmContext, (caddr_t *) &MouseWin) == XCNOENT)))
  {
    MouseWin = NULL;
  }
  if ((MouseWin) && AcceptInput (MouseWin))
  {
    SetFocus (MouseWin->w, MouseWin, False, False);
  }
}

void
SetColormapFocus (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if (mystrncasecmp (style, "FollowsFocus", 12) == 0)
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_FOCUS;
  }
  else if (mystrncasecmp (style, "FollowsMouse", 12) == 0)
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;
  }
  else
  {
    xfwm_msg (ERR, "SetColormapFocus", "ColormapFocus requires 1 arg: FollowsFocus or FollowsMouse");
  }
  free (style);
}

void
SetClick (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetClick", "ClickTime requires 1 argument");
    return;
  }

  Scr.ClickTime = val1;
}

void
SetXOR (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;
  XGCValues gcv;
  unsigned long gcm;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetXOR", "XORValue requires 1 argument");
    return;
  }

  gcm = GCFunction | GCLineWidth | GCForeground | GCSubwindowMode | GCCapStyle;
  gcv.function = GXxor;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  gcv.foreground = (val1) ? (val1) : (BlackPixel (dpy, Scr.screen) ^ WhitePixel (dpy, Scr.screen));
  gcv.subwindow_mode = IncludeInferiors;
  if (Scr.DrawGC)
  {
    XChangeGC (dpy, Scr.DrawGC, gcm, &gcv);
  }
  else
  {
    Scr.DrawGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }
}

void
setModulePath (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if (ptemp == NULL)
    ptemp = ModulePath;

  if ((ModulePath != ptemp) && (ModulePath != NULL))
    free (ModulePath);
  tmp = stripcpy (action);
  ModulePath = envDupExpand (tmp, 0);
  free (tmp);
}

void
SetHiColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XGCValues gcv;
  unsigned long gcm;
  char *hifore = NULL, *hiback = NULL;
  XfwmDecor *fl = &Scr.DefaultDecor;

  action = GetNextToken (action, &hifore);
  GetNextToken (action, &hiback);

  if (hifore != NULL)
  {
    fl->HiColors.fore = GetColor (hifore);
  }
  if (hiback != NULL)
  {
    fl->HiColors.back = GetColor (hiback);
  }
  fl->HiRelief.fore = GetHilite (fl->HiColors.back);
  fl->HiRelief.back = GetShadow (fl->HiColors.back);
  if (hifore)
    free (hifore);
  if (hiback)
    free (hiback);
  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;
  gcv.foreground = fl->HiRelief.fore;
  gcv.background = fl->HiRelief.back;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  if (fl->HiReliefGC)
  {
    XChangeGC (dpy, fl->HiReliefGC, gcm, &gcv);
  }
  else
  {
    fl->HiReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = fl->HiRelief.back;
  gcv.background = fl->HiRelief.fore;
  if (fl->HiShadowGC)
  {
    XChangeGC (dpy, fl->HiShadowGC, gcm, &gcv);
  }
  else
  {
    fl->HiShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = fl->HiColors.back;

  if (brightness (gcv.foreground) > 45)
    gcv.background = BlackPixel (dpy, Scr.screen);
  else
    gcv.background = WhitePixel (dpy, Scr.screen);
  if (fl->HiBackGC)
  {
    XChangeGC (dpy, fl->HiBackGC, gcm, &gcv);
  }
  else
  {
    fl->HiBackGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if ((Scr.flags & WindowsCaptured) && (Scr.Hilite != NULL))
  {
    SetBorder (Scr.Hilite, NULL, True, True, True, None);
  }
}

void
SetLoColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XGCValues gcv;
  unsigned long gcm;
  char *lofore = NULL, *loback = NULL;
  XfwmDecor *fl = &Scr.DefaultDecor;

  action = GetNextToken (action, &lofore);
  GetNextToken (action, &loback);
  if (lofore != NULL)
  {
    fl->LoColors.fore = GetColor (lofore);
  }
  if (loback != NULL)
  {
    fl->LoColors.back = GetColor (loback);
  }
  fl->LoRelief.back = GetShadow (fl->LoColors.back);
  fl->LoRelief.fore = GetHilite (fl->LoColors.back);
  if (lofore)
    free (lofore);
  if (loback)
    free (loback);
  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;
  gcv.foreground = fl->LoRelief.fore;
  gcv.background = fl->LoRelief.back;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  if (fl->LoReliefGC)
  {
    XChangeGC (dpy, fl->LoReliefGC, gcm, &gcv);
  }
  else
  {
    fl->LoReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = fl->LoRelief.back;
  gcv.background = fl->LoRelief.fore;
  if (fl->LoShadowGC)
  {
    XChangeGC (dpy, fl->LoShadowGC, gcm, &gcv);
  }
  else
  {
    fl->LoShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = fl->LoColors.back;
  if (brightness (gcv.foreground) > 45)
  {
    gcv.background = fl->LoRelief.back;
  }
  else
  {
    gcv.background = fl->LoRelief.fore;
  }
  if (fl->LoBackGC)
  {
    XChangeGC (dpy, fl->LoBackGC, gcm, &gcv);
  }
  else
  {
    fl->LoBackGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }
}

void
SafeDefineCursor (Window w, Cursor cursor)
{
  if (w)
    XDefineCursor (dpy, w, cursor);
}

void
CursorColor (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *hifore = NULL, *hiback = NULL;
  XColor back, fore;

  action = GetNextToken (action, &hifore);
  action = GetNextToken (action, &hiback);
  if (!hifore || !hiback)
  {
    xfwm_msg (ERR, "CursorColor", "Requires 2 parameters");
    return;
  }

  fore = GetXColor (hifore);
  back = GetXColor (hiback);
  XRecolorCursor (dpy, Scr.XfwmCursors[POSITION], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[DEFAULT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[SYS], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[TITLE_CURSOR], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[MOVE], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[MENU], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[WAIT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[SELECT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[DESTROY], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[LEFT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[RIGHT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[TOP], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[BOTTOM], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[TOP_LEFT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[TOP_RIGHT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[BOTTOM_LEFT], &fore, &back);
  XRecolorCursor (dpy, Scr.XfwmCursors[BOTTOM_RIGHT], &fore, &back);
  free (hifore);
  free (hiback);
}

void
SetMenuColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XGCValues gcv;
  unsigned long gcm;
  char *fore = NULL, *back = NULL, *selfore = NULL, *selback = NULL;
  Pixel MenuSelHiColor, MenuSelLoColor;

  action = GetNextToken (action, &fore);
  action = GetNextToken (action, &back);
  action = GetNextToken (action, &selfore);
  action = GetNextToken (action, &selback);

  if (fore != NULL)
  {
    Scr.MenuColors.fore = GetColor (fore);
  }
  if (back != NULL)
  {
    Scr.MenuColors.back = GetColor (back);
  }
  if (selfore != NULL)
  {
    Scr.MenuSelColors.fore = GetColor (selfore);
  }
  if (back != NULL)
  {
    Scr.MenuSelColors.back = GetColor (selback);
  }
  Scr.MenuRelief.back = GetShadow (Scr.MenuColors.back);
  Scr.MenuRelief.fore = GetHilite (Scr.MenuColors.back);

  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;

  gcv.foreground = Scr.MenuRelief.fore;
  gcv.background = Scr.MenuRelief.back;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  if (Scr.MenuReliefGC)
  {
    XChangeGC (dpy, Scr.MenuReliefGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = Scr.MenuRelief.back;
  gcv.background = Scr.MenuRelief.fore;
  if (Scr.MenuShadowGC)
  {
    XChangeGC (dpy, Scr.MenuShadowGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = Scr.MenuColors.fore;
  gcv.background = Scr.MenuColors.back;
  if (Scr.MenuGC)
  {
    XChangeGC (dpy, Scr.MenuGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }
  gcv.foreground = Scr.MenuSelColors.fore;
  gcv.background = Scr.MenuSelColors.back;
  if (Scr.MenuSelGC)
  {
    XChangeGC (dpy, Scr.MenuSelGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuSelGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  MenuSelHiColor = GetHilite (Scr.MenuSelColors.back);
  MenuSelLoColor = GetShadow (Scr.MenuSelColors.back);

  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;
  gcv.foreground = MenuSelHiColor;
  gcv.background = Scr.MenuSelColors.back;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  if (Scr.MenuSelReliefGC)
  {
    XChangeGC (dpy, Scr.MenuSelReliefGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuSelReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcv.foreground = MenuSelLoColor;
  if (Scr.MenuSelShadowGC)
  {
    XChangeGC (dpy, Scr.MenuSelShadowGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuSelShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if (fore != NULL)
    free (fore);
  if (back != NULL)
    free (back);
  if (selfore != NULL)
    free (selfore);
  if (selback != NULL)
    free (selback);
}

void
SetMenuFont (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XGCValues gcv;
  unsigned long gcm;
  char *font = NULL;

  action = GetNextToken (action, &font);
  if (!font)
  {
    fprintf (stderr, "[SetMenuFont]: Error -- Couldn't load NULL font\n");
    return;
  }
  if (Scr.StdFont.font)
  {
    XFreeFont (dpy, Scr.StdFont.font);
    Scr.StdFont.font = NULL;
  }
  if (Scr.StdFont.fontset)
  {
    XFreeFontSet (dpy, Scr.StdFont.fontset);
    Scr.StdFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (Scr.StdFont.xftfont)
  {
    XftFontClose (dpy, Scr.StdFont.xftfont);
    Scr.StdFont.xftfont = NULL;
  }
  Scr.StdFont.use_xft = enable_xft;
#endif
  GetFontOrFixed (dpy, font, &(Scr.StdFont));
#ifdef HAVE_X11_XFT_XFT_H
  if ((Scr.StdFont.font == NULL) && (Scr.StdFont.fontset == NULL) && (Scr.StdFont.xftfont == NULL))
#else
  if ((Scr.StdFont.font == NULL) && (Scr.StdFont.fontset == NULL))
#endif
  {
    fprintf (stderr, "[SetMenuFont]: Error -- Couldn't load font '%s'\n", font);
    free (font);
    return;
  }
  
  if (Scr.StdFont.fontset)
  {				/* Temporarily, set FontStruct for calculating. */
    XFontStruct **fs_list = NULL;
    char **fontset_namelist = NULL;
    int font_num = 0, i;
    font_num = XFontsOfFontSet (Scr.StdFont.fontset, &fs_list, &fontset_namelist);
    Scr.StdFont.font = fs_list[0];
    for (i = 1; i < font_num; i++)
      if (Scr.StdFont.font->ascent < fs_list[i]->ascent)
	Scr.StdFont.font->ascent = fs_list[i]->ascent;
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if ((enable_xft) && (Scr.StdFont.xftfont))
  {
    Scr.StdFont.height = Scr.StdFont.xftfont->ascent + Scr.StdFont.xftfont->descent;
    Scr.StdFont.y = Scr.StdFont.xftfont->ascent;
  }
  else
  {
    Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
    Scr.StdFont.y = Scr.StdFont.font->ascent;
  }
#else
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
  Scr.StdFont.y = Scr.StdFont.font->ascent;
#endif
  Scr.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;

  gcm = Scr.StdFont.font ? GCFont : 0;
  gcv.font = Scr.StdFont.font ? Scr.StdFont.font->fid : 0;

  if (Scr.MenuReliefGC)
  {
    XChangeGC (dpy, Scr.MenuReliefGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if (Scr.MenuShadowGC)
  {
    XChangeGC (dpy, Scr.MenuShadowGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if (Scr.MenuGC)
  {
    XChangeGC (dpy, Scr.MenuGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if (Scr.MenuSelGC)
  {
    XChangeGC (dpy, Scr.MenuSelGC, gcm, &gcv);
  }
  else
  {
    Scr.MenuSelGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  if (Scr.HintsGC)
  {
    XChangeGC (dpy, Scr.HintsGC, gcm, &gcv);
  }
  else
  {
    Scr.HintsGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  MakeMenus ();
  free (font);
}

Bool ReadButtonFace (char *s, ButtonFace * bf, int button, int verbose);
void FreeButtonFace (Display * dpy, ButtonFace * bf);

char *ReadTitleButton (char *s, TitleButton * tb, Bool append, int button);

void
SetTitleStyle (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *prev = action;
  XfwmDecor *fl = &Scr.DefaultDecor;

  ReadTitleButton (prev, &fl->titlebar, False, -1);
}

void
LoadIconFont (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *font;
  XfwmWindow *tmp;

  action = GetNextToken (action, &font);
  if (!font)
  {
    fprintf (stderr, "[LoadIconFont]: Error -- Couldn't load NULL font\n");
    return;
  }
  if (Scr.IconFont.font)
  {
    XFreeFont (dpy, Scr.IconFont.font);
    Scr.IconFont.font = NULL;
  }
  if (Scr.IconFont.fontset)
  {
    XFreeFontSet (dpy, Scr.IconFont.fontset);
    Scr.IconFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (Scr.IconFont.xftfont)
  {
    XftFontClose (dpy, Scr.IconFont.xftfont);
    Scr.IconFont.xftfont = NULL;
  }
  Scr.IconFont.use_xft = enable_xft;
#endif
  GetFontOrFixed (dpy, font, &(Scr.IconFont));
#ifdef HAVE_X11_XFT_XFT_H
  if ((Scr.IconFont.font == NULL) && (Scr.IconFont.fontset == NULL) && (Scr.IconFont.xftfont == NULL))
#else
  if ((Scr.IconFont.font == NULL) && (Scr.IconFont.fontset == NULL))
#endif
  {
    xfwm_msg (ERR, "LoadIconFont", "Couldn't load font '%s' or 'fixed'\n", font);
    free (font);
    return;
  }
  
  if (Scr.IconFont.fontset)
  {				/* Temporarily, set FontStruct for calculating. */
    XFontStruct **fs_list = NULL;
    char **fontset_namelist = NULL;
    int font_num = 0, i;
    font_num = XFontsOfFontSet (Scr.IconFont.fontset, &fs_list, &fontset_namelist);
    Scr.IconFont.font = fs_list[0];
    for (i = 1; i < font_num; i++)
      if (Scr.IconFont.font->ascent < fs_list[i]->ascent)
	Scr.IconFont.font->ascent = fs_list[i]->ascent;
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if ((enable_xft) && (Scr.IconFont.xftfont))
  {
    Scr.IconFont.height = Scr.IconFont.xftfont->ascent + Scr.IconFont.xftfont->descent;
    Scr.IconFont.y = Scr.IconFont.xftfont->ascent;
  }
  else
  {
    Scr.IconFont.height = Scr.IconFont.font->ascent + Scr.IconFont.font->descent;
    Scr.IconFont.y = Scr.IconFont.font->ascent;
  }
#else
  Scr.IconFont.height = Scr.IconFont.font->ascent + Scr.IconFont.font->descent;
  Scr.IconFont.y = Scr.IconFont.font->ascent;
#endif
  free (font);
  tmp = Scr.XfwmRoot.next;
  while (tmp != NULL)
  {
    RedoIconName (tmp, NULL);

    if (tmp->flags & ICONIFIED)
    {
      DrawIconWindow (tmp, NULL);
    }
    tmp = tmp->next;
  }
}

void
LoadWindowFont (XEvent * eventp, Window win, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *font;
  XfwmWindow *tmp, *hi;
  int x, y, w, h;
  int extra_height = 0;
  XfwmDecor *fl = &Scr.DefaultDecor;

  action = GetNextToken (action, &font);
  if (!font)
  {
    fprintf (stderr, "[LoadWindowFont]: Error -- Couldn't load NULL font\n");
    return;
  }
  if (fl->WindowFont.font)
  {
    XFreeFont (dpy, fl->WindowFont.font);
    fl->WindowFont.font = NULL;
  }
  if (fl->WindowFont.fontset)
  {
    XFreeFontSet (dpy, fl->WindowFont.fontset);
    fl->WindowFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (fl->WindowFont.xftfont)
  {
    XftFontClose (dpy, fl->WindowFont.xftfont);
    fl->WindowFont.xftfont = NULL;
  }
  fl->WindowFont.use_xft = enable_xft;
#endif
  GetFontOrFixed (dpy, font, &(fl->WindowFont));
#ifdef HAVE_X11_XFT_XFT_H
  if ((fl->WindowFont.font == NULL) && (fl->WindowFont.fontset == NULL) && (fl->WindowFont.xftfont == NULL))
#else
  if ((fl->WindowFont.font == NULL) && (fl->WindowFont.fontset == NULL))
#endif
  {
    xfwm_msg (ERR, "LoadWindowFont", "Couldn't load font '%s' or 'fixed'\n", font);
    free (font);
    return;
  }

  if (fl->WindowFont.fontset)
  {				/* Temporarily, set FontStruct for calculating. */
    XFontStruct **fs_list = NULL;
    char **fontset_namelist = NULL;
    int font_num = 0, i;
    font_num = XFontsOfFontSet (fl->WindowFont.fontset, &fs_list, &fontset_namelist);
    fl->WindowFont.font = fs_list[0];
    for (i = 1; i < font_num; i++)
      if (fl->WindowFont.font->ascent < fs_list[i]->ascent + 1)
	fl->WindowFont.font->ascent = fs_list[i]->ascent + 1;
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if ((enable_xft) && (fl->WindowFont.xftfont))
  {
    fl->WindowFont.height = (fl->WindowFont.xftfont)->ascent + (fl->WindowFont.xftfont)->descent;
    fl->WindowFont.y = (fl->WindowFont.xftfont)->ascent + 1;
    extra_height = fl->TitleHeight;
    fl->TitleHeight = (fl->WindowFont.xftfont)->ascent + (fl->WindowFont.xftfont)->descent + 4;
  }
  else
  {
    fl->WindowFont.height = (fl->WindowFont.font)->ascent + (fl->WindowFont.font)->descent;

    fl->WindowFont.y = (fl->WindowFont.font)->ascent + 1;
    extra_height = fl->TitleHeight;
    fl->TitleHeight = (fl->WindowFont.font)->ascent + (fl->WindowFont.font)->descent + 4;
  }
#else
  fl->WindowFont.height = (fl->WindowFont.font)->ascent + (fl->WindowFont.font)->descent;

  fl->WindowFont.y = (fl->WindowFont.font)->ascent + 1;
  extra_height = fl->TitleHeight;
  fl->TitleHeight = (fl->WindowFont.font)->ascent + (fl->WindowFont.font)->descent + 4;
#endif
  if (fl->TitleHeight & 0x1)
  {
    fl->TitleHeight++;
  }
  if (fl->TitleHeight < 16)
  {
    fl->WindowFont.y += (16 - fl->TitleHeight) / 2;
    fl->TitleHeight = 16;
  }

  extra_height -= fl->TitleHeight;
  tmp = Scr.XfwmRoot.next;
  hi = Scr.Hilite;
  while (tmp != NULL)
  {
    if (!(tmp->flags & TITLE))
    {
      tmp = tmp->next;
      continue;
    }
    GetGravityOffsets (tmp);
    x = tmp->frame_x;
    y = tmp->frame_y - (tmp->grav_align_y * extra_height);
    w = tmp->frame_width;
    h = tmp->frame_height - extra_height;
    tmp->frame_x = 0;
    tmp->frame_y = 0;
    tmp->frame_height = 0;
    tmp->frame_width = 0;
    SetTitleBar (tmp, NULL, (tmp == Scr.Hilite));
    SetupFrame (tmp, x, y, w, h, False, True);
    tmp = tmp->next;
  }
  free (font);
}

void
FreeButtonFace (Display * dpy, ButtonFace * bf)
{
  switch (bf->style)
  {
  case GradButton:
    free (bf->u.grad.pixels);
    bf->u.grad.pixels = NULL;
    if (bf->u.ShadowGC)
    {
      XFreeGC (dpy, bf->u.ShadowGC);
      bf->u.ShadowGC = (GC) NULL;
    }
    if (bf->u.ReliefGC)
    {
      XFreeGC (dpy, bf->u.ReliefGC);
      bf->u.ReliefGC = (GC) NULL;
    }
    break;
  case PixButton:
    if (bf->bitmap != None)
    {
      XFreePixmap (dpy, bf->bitmap);
      bf->bitmap = None;
    }
    if (bf->bitmap_pressed != None)
    {
      XFreePixmap (dpy, bf->bitmap_pressed);
      bf->bitmap_pressed = None;
    }
    break;
  case SolidButton:
    if (bf->u.ShadowGC)
    {
      XFreeGC (dpy, bf->u.ShadowGC);
      bf->u.ShadowGC = (GC) NULL;
    }
    if (bf->u.ReliefGC)
    {
      XFreeGC (dpy, bf->u.ReliefGC);
      bf->u.ReliefGC = (GC) NULL;
    }
    break;
  default:
    break;
  }

  if (bf->next)
  {
    FreeButtonFace (dpy, bf->next);
    free (bf->next);
  }
  bf->next = NULL;
  bf->style = SimpleButton;
}

/*****************************************************************************
 * 
 * Reads a button face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
Bool ReadButtonFace (char *s, ButtonFace * bf, int button, int verbose)
{
  XGCValues gcv;
  unsigned long gcm;
  int offset;
  char style[256], *file;
  char *action = s;

  if (sscanf (s, "%s%n", style, &offset) < 1)
  {
    if (verbose)
      xfwm_msg (ERR, "ReadButtonFace", "error in face: %s", action);
    return False;
  }

  s += offset;
  FreeButtonFace (dpy, bf);

  /* Prepare GC creation */
  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;

  /* determine button style */
  if (mystrncasecmp (style, "Simple", 6) == 0)
  {
    bf->style = SimpleButton;
  }
  else if (mystrncasecmp (style, "Default", 7) == 0)
  {
    int b = -1, n = sscanf (s, "%d%n", &b, &offset);

    if (n < 1)
    {
      if (button == -1)
      {
	if (verbose)
	  xfwm_msg (ERR, "ReadButtonFace", "need default button number to load");
	return False;
      }
      b = button;
    }
    s += offset;
    if ((b > 0) && (b <= 6))
      LoadDefaultButton (bf, b);
    else
    {
      if (verbose)
	xfwm_msg (ERR, "ReadButtonFace", "button number out of range: %d", b);
      return False;
    }
  }
  else if (mystrncasecmp (style, "Solid", 5) == 0)
  {
    s = GetNextToken (s, &file);
    if (file && *file)
    {
      bf->style = SolidButton;
      bf->u.back = GetColor (file);
      bf->u.Relief.fore = GetHilite (bf->u.back);
      bf->u.Relief.back = GetShadow (bf->u.back);
      gcv.foreground = bf->u.Relief.fore;
      gcv.background = bf->u.back;
      bf->u.ReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
      gcv.foreground = bf->u.Relief.back;
      bf->u.ShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
    }
    else
    {
      if (verbose)
	xfwm_msg (ERR, "ReadButtonFace", "no color given for Solid face type: %s", action);
      return False;
    }
    free (file);
  }
  else if (mystrncasecmp (style, "Gradient", 8) == 0)
  {
    char *item, **s_colors;
    int npixels, *perc;
    Pixel *pixels;

    npixels = ((Scr.d_depth > 8) ? 32 : 8);

    if (!(s = GetNextToken (s, &item)) || (item == NULL))
    {
      if (verbose)
	xfwm_msg (ERR, "ReadButtonFace", "incomplete gradient style");
      return False;
    }

    s_colors = (char **) safemalloc (sizeof (char *) * 2);
    perc = (int *) safemalloc (sizeof (int));
    s_colors[0] = item;
    s = GetNextToken (s, &s_colors[1]);

    perc[0] = 100;

    pixels = AllocNonlinearGradient (s_colors, perc, 1, npixels);
    free (s_colors[0]);
    free (s_colors[1]);
    free (s_colors);
    free (perc);

    if (!pixels)
    {
      if (verbose)
	xfwm_msg (ERR, "ReadButtonFace", "couldn't create gradient");
      return False;
    }

    bf->u.back = pixels[0];
    bf->u.grad.pixels = pixels;
    bf->u.grad.npixels = npixels;
    bf->style = GradButton;
    bf->u.Relief.fore = GetHilite (bf->u.back);
    bf->u.Relief.back = GetShadow (bf->u.back);
    gcv.foreground = bf->u.Relief.fore;
    gcv.background = bf->u.back;
    bf->u.ReliefGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
    gcv.foreground = bf->u.Relief.back;
    bf->u.ShadowGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }
  else
  {
    if (verbose)
      xfwm_msg (ERR, "ReadButtonFace", "unknown style %s: %s", style, action);
    return False;
  }
  return True;
}


/*****************************************************************************
 * 
 * Reads a title button description (veliaa@rpi.edu)
 *
 ****************************************************************************/
char *
ReadTitleButton (char *s, TitleButton * tb, Bool append, int button)
{
  char *end = NULL, *spec;
  ButtonFace tmpbf;
  enum ButtonState bs = MaxButtonState;
  int i = 0, all = 0, pstyle = 0;

  while (isspace (*s))
    ++s;
  for (; i < MaxButtonState; ++i)
    if (mystrncasecmp (button_states[i], s, strlen (button_states[i])) == 0)
    {
      bs = i;
      break;
    }
  if (bs != MaxButtonState)
    s += strlen (button_states[bs]);
  else
    all = 1;
  while (isspace (*s))
    ++s;
  spec = s;

  while (isspace (*spec))
    ++spec;
  /* setup temporary in case button read fails */
  tmpbf.style = SimpleButton;
  tmpbf.next = NULL;

  if (ReadButtonFace (spec, &tmpbf, button, True))
  {
    int b = all ? 0 : bs;
    FreeButtonFace (dpy, &tb->state[b]);
    tb->state[b] = tmpbf;
    if (all)
      for (i = 1; i < MaxButtonState; ++i)
	ReadButtonFace (spec, &tb->state[i], button, False);
  }
  if (pstyle)
  {
    free (spec);
    ++end;
    while (isspace (*end))
      ++end;
  }
  return end;
}


/*****************************************************************************
 * 
 * Updates window decoration styles (veliaa@rpi.edu)
 *
 ****************************************************************************/
void
UpdateDecor (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindowList *fw;

  /* Scan the window list, from bottom to top to reduce ExposeEvent */
  for (fw = LastXfwmWindowList (Scr.stacklist); fw != NULL; fw = fw->prev)
  {
    {
      SetBorder (fw->win, NULL, ((fw->win) == Scr.Hilite), True, True, None);
    }
  }
}


/*****************************************************************************
 * 
 * Changes a button decoration style (changes by veliaa@rpi.edu)
 *
 ****************************************************************************/
#define SetButtonFlag(a) \
        do { \
             int i; \
             if (multi) { \
               if (multi&1) \
                 for (i=0;i<2;++i) \
                   if (set) \
                     fl->left_buttons[i].flags |= (a); \
                   else \
                     fl->left_buttons[i].flags &= ~(a); \
               if (multi&2) \
                 for (i=0;i<2;++i) \
                   if (set) \
                     fl->right_buttons[i].flags |= (a); \
                   else \
                     fl->right_buttons[i].flags &= ~(a); \
             } else \
                 if (set) \
                   tb->flags |= (a); \
                 else \
                   tb->flags &= ~(a); } while (0)

/**************************************************************************
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation 
 * Direction = 0 ==> operation on current window (returns pass or fail)
 *
 **************************************************************************/
XfwmWindow *
Circulate (char *action, int Direction, char **restofline)
{
  int l, pass = 0;
  XfwmWindow *fw, *found = NULL;
  char *t, *tstart, *name = NULL, *expression, *condition, *prev_condition = NULL;
  char *orig_expr;
  Bool needsCurrentDesk = 0;
  Bool needsCurrentPage = 0;

  Bool needsName = 0;
  Bool needsNotName = 0;
  Bool useCirculateHit = (Direction) ? 0 : 1;	/* override for Current [] */
  Bool useCirculateHitIcon = (Direction) ? 0 : 1;
  unsigned long onFlags = 0;
  unsigned long offFlags = 0;

  l = 0;

  if (action == NULL)
    return NULL;

  t = action;
  while (isspace (*t) && (*t != 0))
    t++;
  if (*t == '[')
  {
    t++;
    tstart = t;

    while ((*t != 0) && (*t != ']'))
    {
      t++;
      l++;
    }
    if (*t == 0)
    {
      xfwm_msg (ERR, "Circulate", "Conditionals require closing brace");
      return NULL;
    }

    *restofline = t + 1;

    orig_expr = expression = safemalloc (l + 1);
    strncpy (expression, tstart, l);
    expression[l] = 0;
    expression = GetNextToken (expression, &condition);
    while ((condition != NULL) && (strlen (condition) > 0))
    {
      if (mystrcasecmp (condition, "Iconic") == 0)
	onFlags |= ICONIFIED;
      else if (mystrcasecmp (condition, "!Iconic") == 0)
	offFlags |= ICONIFIED;
      else if (mystrcasecmp (condition, "Visible") == 0)
	onFlags |= VISIBLE;
      else if (mystrcasecmp (condition, "!Visible") == 0)
	offFlags |= VISIBLE;
      else if (mystrcasecmp (condition, "Sticky") == 0)
	onFlags |= STICKY;
      else if (mystrcasecmp (condition, "!Sticky") == 0)
	offFlags |= STICKY;
      else if (mystrcasecmp (condition, "Maximized") == 0)
	onFlags |= MAXIMIZED;
      else if (mystrcasecmp (condition, "!Maximized") == 0)
	offFlags |= MAXIMIZED;
      else if (mystrcasecmp (condition, "Transient") == 0)
	onFlags |= TRANSIENT;
      else if (mystrcasecmp (condition, "!Transient") == 0)
	offFlags |= TRANSIENT;
      else if (mystrcasecmp (condition, "Raised") == 0)
	onFlags |= RAISED;
      else if (mystrcasecmp (condition, "!Raised") == 0)
	offFlags |= RAISED;
      else if (mystrcasecmp (condition, "CurrentDesk") == 0)
	needsCurrentDesk = 1;
      else if (mystrcasecmp (condition, "CurrentPage") == 0)
      {
	needsCurrentDesk = 1;
	needsCurrentPage = 1;
      }
      else if (mystrcasecmp (condition, "CurrentPageAnyDesk") == 0 || mystrcasecmp (condition, "CurrentScreen") == 0)
	needsCurrentPage = 1;
      else if (mystrcasecmp (condition, "CirculateHit") == 0)
	useCirculateHit = 1;
      else if (mystrcasecmp (condition, "CirculateHitIcon") == 0)
	useCirculateHitIcon = 1;
      else if (!needsName && !needsNotName)	/* only 1st name to avoid mem leak */
      {
	name = condition;
	condition = NULL;
	if (name[0] == '!')
	{
	  needsNotName = 1;
	  name++;
	}
	else
	  needsName = 1;
      }
      if (prev_condition)
	free (prev_condition);
      prev_condition = condition;
      expression = GetNextToken (expression, &condition);
    }
    if (prev_condition != NULL)
      free (prev_condition);
    if (orig_expr != NULL)
      free (orig_expr);
  }
  else
    *restofline = t;

  if (Scr.Focus != NULL)
  {
    if (Direction == 1)
      fw = Scr.Focus->prev;
    else if (Direction == -1)
      fw = Scr.Focus->next;
    else
      fw = Scr.Focus;
  }
  else
    fw = Scr.XfwmRoot.prev;

  while ((pass < 3) && (found == NULL))
  {
    while ((fw != NULL) && (found == NULL) && (fw != &Scr.XfwmRoot))
    {
      /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
      if (AcceptInput(fw) && ((onFlags & fw->flags) == onFlags) && ((offFlags & fw->flags) == 0) && (useCirculateHit || !(fw->flags & CirculateSkip)) && ((useCirculateHitIcon && fw->flags & ICONIFIED) || !(fw->flags & CirculateSkipIcon && fw->flags & ICONIFIED)) && (!needsCurrentDesk || fw->Desk == Scr.CurrentDesk) && (!needsCurrentPage || (fw->frame_x < MyDisplayMaxX (fw->frame_x, fw->frame_y) && fw->frame_y < MyDisplayMaxY (fw->frame_x, fw->frame_y) && fw->frame_x + fw->frame_width > MyDisplayX (fw->frame_x, fw->frame_y) && fw->frame_y + fw->frame_height > MyDisplayY (fw->frame_x, fw->frame_y)))
	  && ((!needsName || matchWildcards (name, fw->name) || matchWildcards (name, fw->icon_name) || (fw->class.res_class && matchWildcards (name, fw->class.res_class)) || (fw->class.res_name && matchWildcards (name, fw->class.res_name))) && (!needsNotName || !(matchWildcards (name, fw->name) || matchWildcards (name, fw->icon_name) || (fw->class.res_class && matchWildcards (name, fw->class.res_class)) || (fw->class.res_name && matchWildcards (name, fw->class.res_name))))))
	found = fw;
      else
      {
	if (Direction == 1)
	  fw = fw->prev;
	else
	  fw = fw->next;
      }
      if (Direction == 0)
      {
	if (needsName)
	  free (name);
	else if (needsNotName)
	  free (name - 1);
	return found;
      }
    }
    if ((fw == NULL) || (fw == &Scr.XfwmRoot))
    {
      if (Direction == 1)
      {
	/* Go to end of list */
	fw = &Scr.XfwmRoot;
	while ((fw) && (fw->next != NULL))
	{
	  fw = fw->next;
	}
      }
      else
      {
	/* GO to top of list */
	fw = Scr.XfwmRoot.next;
      }
    }
    pass++;
  }
  if (needsName)
    free (name);
  else if (needsNotName)
    free (name - 1);
  return found;

}

void
PrevFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *found;
  char *restofline;

  found = Circulate (action, -1, &restofline);
  if (found != NULL)
  {
    ExecuteFunction (restofline, found, eventp, C_WINDOW, *Module);
  }
}

void
NextFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *found;
  char *restofline;

  found = Circulate (action, 1, &restofline);
  if (found != NULL)
  {
    ExecuteFunction (restofline, found, eventp, C_WINDOW, *Module);
  }

}

void
RedrawSwitchWindow (Window w, char *name, int width, int height)
{
  XFontSet fontset = Scr.StdFont.fontset;

  if (w == None)
    return;

  XClearWindow (dpy, w);

  if (fontset)
  {
    XmbDrawString (dpy, w, fontset, Scr.MenuGC, 10, (height + Scr.StdFont.y) / 2, name, strlen (name));
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if (enable_xft && Scr.StdFont.xftfont)
  {
    Pixel TextColor = 0;
    XftDraw *xftdraw;
    XftColor color_fg;
    XWindowAttributes attributes;
    XColor dummyc;

    TextColor = Scr.MenuColors.fore;

    XGetWindowAttributes (dpy, Scr.Root, &attributes);

    xftdraw = XftDrawCreate (dpy, (Drawable) w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

    dummyc.pixel = TextColor;
    XQueryColor (dpy, attributes.colormap, &dummyc);
    color_fg.color.red = dummyc.red;
    color_fg.color.green = dummyc.green;
    color_fg.color.blue = dummyc.blue;
    color_fg.color.alpha = 0xffff;
    color_fg.pixel = TextColor;

    XftDrawString8 (xftdraw, &color_fg, Scr.StdFont.xftfont, 10, (height + Scr.StdFont.y) / 2, (XftChar8 *) name, strlen (name));
    XftDrawDestroy (xftdraw);
  }
#endif
  else
  {
    XDrawString (dpy, w, Scr.MenuGC, 10, (height + Scr.StdFont.y) / 2, name, strlen (name));
  }

  RelieveRectangle (w, NULL, 1, 1, width - 2, height - 2, Scr.MenuReliefGC, Scr.MenuShadowGC);
  RelieveRectangle (w, NULL, 0, 0, width, height, Scr.BlackGC, Scr.BlackGC);
}

void
SwitchFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *t, *tp;
  Bool finished = False;
  Bool namechanged = False;
  Bool abort = False;
  int width = 1;
  int height = 1;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  Window taskw = None;
  KeySym keysym;
  XFontSet fontset = Scr.StdFont.fontset;
  int dumb;
  int mousex, mousey;
  int wx, wy;
  int countw;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  extern XContext MenuContext;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &mousex, &mousey, &dummy_win_x, &dummy_win_y, &dummy_mask);

  /* Count how many windows we have */
  countw = 0;
  t = NULL;
  for (tp = Scr.XfwmRoot.next; tp != NULL; tp = tp->next)
  {
    if (AcceptInput (tp) && !(tp->flags & CirculateSkip) && !((tp->flags & CirculateSkipIcon) && (tp->flags & ICONIFIED)))
    {
      countw++;
      /* Select the second window in the list */
      if (countw == 2)
      {
	t = tp;
      }
    }
  }
  /* if we have 0 or 1 window, forget it */
  if (countw < 2)
  {
    return;
  }

  if (!GrabEm (MENU))
  {
    XBell (dpy, Scr.screen);
    return;
  }

  InstallRootColormap ();
  namechanged = True;

  while (!finished)
  {
    if (namechanged)
    {
      if (fontset)
      {
	XRectangle rect1, rect2;
	XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
	width = rect2.width;
      }
#ifdef HAVE_X11_XFT_XFT_H
      else if ((enable_xft) && (Scr.StdFont.xftfont))
      {
	XGlyphInfo extents;

	XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) t->name, strlen (t->name), &extents);
	width = extents.xOff;
      }
#endif
      else
      {
	width = XTextWidth (Scr.StdFont.font, t->name, strlen (t->name));
      }
      width += 20;
      height = Scr.EntryHeight + 20;

      wx = MyDisplayX (mousex, mousey) + (MyDisplayWidth (mousex, mousey) - width) / 2;
      wy = MyDisplayY (mousex, mousey) + (MyDisplayHeight (mousex, mousey) - height) / 2;

      if (taskw != None)
	XMoveResizeWindow (dpy, taskw, wx, wy, width, height);
      else
      {
	/* Create and display window */
	valuemask = (CWBackPixel | CWCursor | CWSaveUnder);
	attributes.background_pixel = Scr.MenuColors.back;
	attributes.cursor = Scr.XfwmCursors[MENU];
	attributes.save_under = True;
	taskw = XCreateWindow (dpy, Scr.Root, wx, wy, width, height, 0, CopyFromParent, InputOutput, (Visual *) CopyFromParent, valuemask, &attributes);
	XSaveContext (dpy, taskw, MenuContext, (caddr_t) taskw);
	XMapRaised (dpy, taskw);
      }
      RedrawSwitchWindow (taskw, t->name, width, height);
      namechanged = False;
    }
    /* block until there is an event */
    XMaskEvent (dpy, ExposureMask | KeyPressMask | KeyReleaseMask | VisibilityChangeMask | EnterWindowMask | LeaveWindowMask, &Event);

#ifdef REQUIRES_STASHEVENT
    StashEventTime (&Event);
#endif
    switch (Event.type)
    {
    case KeyPress:
      namechanged = True;
      keysym = XLookupKeysym (&(Event.xkey), 0);
      if (keysym == XK_Tab)
      {
	do
	{
	  t = t->next;
	  if (t == NULL)
	  {
	    t = Scr.XfwmRoot.next;
	  }
	}
	while (!AcceptInput(t) || (t->flags & CirculateSkip) || ((t->flags & CirculateSkipIcon) && (t->flags & ICONIFIED)));
      }
      else if (keysym == XK_space)
      {
	finished = True;
	abort = False;
      }
      else if (keysym == XK_Escape)
      {
	finished = True;
	abort = True;
      }
      break;
    case KeyRelease:
      keysym = XLookupKeysym (&(Event.xkey), 0);
      if (keysym == XK_Alt_L)
      {
	finished = True;
	abort = False;
      }
      break;
    case EnterNotify:
    case LeaveNotify:
      break;
    default:
      if ((XFindContext (dpy, Event.xany.window, MenuContext, (caddr_t *) &dumb) != XCNOENT))
      {
	RedrawSwitchWindow (taskw, t->name, width, height);
      }
      else
      {
	DispatchEvent ();
      }
      break;
    }
  }
  if (taskw != None)
  {
    XDeleteContext (dpy, taskw, MenuContext);
    XUnmapWindow (dpy, taskw);
    XDestroyWindow (dpy, taskw);
    discard_events (EnterWindowMask | LeaveWindowMask);  
  }

  UninstallRootColormap ();
  UngrabEm ();

  if ((!abort) && (t != NULL))
  {
    /* switch to the window (without warping the pointer) */
    WarpOn (t, -1, 1, -1, 1);
    /* Focus on window and deiconify if necessary */
    FocusOn (t, True);
  }
}

void
NoneFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *found;
  char *restofline;

  found = Circulate (action, 1, &restofline);
  if (found == NULL)
  {
    ExecuteFunction (restofline, NULL, eventp, C_ROOT, *Module);
  }
}

void
CurrentFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *found;
  char *restofline;

  found = Circulate (action, 0, &restofline);
  if (found != NULL)
  {
    ExecuteFunction (restofline, found, eventp, C_WINDOW, *Module);
  }
}

void
WindowIdFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *found = NULL, *t;
  char *restofline, *num;
  unsigned long win;

  restofline = strdup (action);
  num = GetToken (&restofline);
  win = (unsigned long) strtol (num, NULL, 0);	/* SunOS doesn't have strtoul */
  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    if (t->w == win)
    {
      found = t;
      break;
    }
  }
  if (found)
  {
    ExecuteFunction (restofline, found, eventp, C_WINDOW, *Module);
  }
  if (restofline)
    free (restofline);
  if (num)
    free (num);
}


void
module_zapper (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *condition;

  GetNextToken (action, &condition);
  KillModuleByName (condition);
  free (condition);
}

void
Recapture (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  BlackoutScreen ();		/* if they want to hide the recapture */
  CaptureAllWindows ();
  UnBlackoutScreen ();
}

void
SetFocusMode (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if (mystrncasecmp (style, "Click", 5) == 0)
  {
    if (!(Scr.Options & ClickToFocus))
    {
      Scr.Options |= ClickToFocus;
      Scr.Options &= ~AutoRaiseWin;
    }
  }
  else if ((mystrncasecmp (style, "Mouse", 5) == 0) || (mystrncasecmp (style, "Follo", 5) == 0))
  {
    if (Scr.Options & ClickToFocus)
      Scr.Options &= ~ClickToFocus;
  }
  else
  {
    xfwm_msg (ERR, "SetFocusMode", "Unknown focus behaviour, using follow mouse mode");
    Scr.Options &= ~ClickToFocus;
  }
  free (style);
}

void
SetAutoRaise (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
  {
    if (!(Scr.Options & AutoRaiseWin))
    {
      Scr.Options |= AutoRaiseWin;
      Scr.Options &= ~ClickToFocus;
    }
  }
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
  {
    if (Scr.Options & AutoRaiseWin)
      Scr.Options &= ~AutoRaiseWin;
  }
  else
  {
    xfwm_msg (ERR, "SetAutoRaise", "Unknown parameter, using AutoRaise On");
    Scr.Options |= AutoRaiseWin;
    Scr.Options &= ~ClickToFocus;
  }
  free (style);
}

void
SetAutoRaiseDelay (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetAutoRaiseDelay", "AutoRaiseDelay requires 1 argument");
    return;
  }

  Scr.AutoRaiseDelay = val1;
}

void
SetAnimate (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= AnimateWin;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~AnimateWin;
  else
  {
    xfwm_msg (ERR, "SetAnimate", "Unknown parameter, using Animate On");
    Scr.Options |= AnimateWin;
  }
  free (style);
}

void
SetOpaqueMove (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= MoveOpaqueWin;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~MoveOpaqueWin;
  else
  {
    xfwm_msg (ERR, "SetOpaqueMove", "Unknown parameter, using opaque move On");
    Scr.Options |= MoveOpaqueWin;
  }
  free (style);
}

void
SetOpaqueResize (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= ResizeOpaqueWin;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~ResizeOpaqueWin;
  else
  {
    xfwm_msg (ERR, "SetOpaqueResize", "Unknown parameter, using opaque resize On");
    Scr.Options |= ResizeOpaqueWin;
  }
  free (style);
}

void
SetIconPos (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;
  XfwmWindowList *t;
  int n;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "T", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.iconbox = 0;
  else if ((mystrncasecmp (style, "L", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.iconbox = 1;
  else if ((mystrncasecmp (style, "B", 1) == 0) || (mystrncasecmp (style, "2", 1) == 0))
    Scr.iconbox = 2;
  else if ((mystrncasecmp (style, "R", 1) == 0) || (mystrncasecmp (style, "3", 1) == 0))
    Scr.iconbox = 3;
  else
  {
    xfwm_msg (ERR, "SetIconPos", "Unknown parameter, using 'top'");
    Scr.iconbox = 0;
  }
  for (n = 0; n <= NBSCREENS; n++)
    for (t = LastXfwmWindowList (Scr.stacklist); t != NULL; t = t->prev)
    {
      if (((t->win)->flags & ICONIFIED) && ((t->win)->Desk == n) && !((t->win)->flags & ICON_MOVED))
      {
	AutoPlace (t->win, True);
	(t->win)->icon_arranged = True;
      }
    }
  for (t = LastXfwmWindowList (Scr.stacklist); t != NULL; t = t->prev)
    (t->win)->icon_arranged = False;
  free (style);
}

void
Arrange_Icons (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindow *t;
  XfwmWindowList *wl, *u;
  int n, val1, val1_unit, val2, val2_unit;

  if (!GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, 0, 0))
    val1 = val2 = Scr.CurrentDesk;
  wl = WindowSorted ();
  for (n = val1; n <= val2; n++)
    for (u = wl; u != NULL; u = u->next)
    {
      if ((u->win->flags & ICONIFIED) && (u->win->Desk == n))
      {
	u->win->flags &= ~ICON_MOVED;
	AutoPlace (u->win, True);
	u->win->icon_arranged = True;
      }
    }
  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
    t->icon_arranged = False;
  FreeXfwmWindowList (wl);
}

void
SetSnapSize (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetSnapSize", "Snap Size requires 1 argument");
    return;
  }

  Scr.SnapSize = ((val1 > 0) ? val1 : 0);
}

void
wait_session (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "WaitSession", "WaitSession requires 1 argument");
    return;
  }

  Scr.sessionwait = ((val1 > 0) ? val1 : 0);
}

void
SetSessionMgt (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= SessionMgt;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~SessionMgt;
  else
  {
    xfwm_msg (ERR, "SetSessionMgt", "Unknown parameter, using session management On");
    Scr.Options |= SessionMgt;
  }
  free (style);
}

void
SetIconGrid (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetIconGrid", "Icon grid requires 1 argument");
    return;
  }

  Scr.icongrid = ((val1 > 1) ? val1 : 1);
}

void
SetIconSpacing (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetIconSpacing", "Icon spacing requires 1 argument");
    return;
  }

  Scr.iconspacing = ((val1 > 1) ? val1 : 1);
}

void
engine_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;
  XfwmWindowList *t;

  action = GetNextToken (action, &style);

  if (mystrncasecmp (style, "X", 1) == 0)
    Scr.engine = XFCE_ENGINE;
  else if (mystrncasecmp (style, "M", 1) == 0)
    Scr.engine = MOFIT_ENGINE;
  else if (mystrncasecmp (style, "T", 1) == 0)
    Scr.engine = TRENCH_ENGINE;
  else if (mystrncasecmp (style, "G", 1) == 0)
    Scr.engine = GTK_ENGINE;
  else if (mystrncasecmp (style, "L", 1) == 0)
    Scr.engine = LINEA_ENGINE;
  else
  {
    xfwm_msg (ERR, "Engine", "Unknown parameter, using Xfce engine");
    Scr.engine = XFCE_ENGINE;
  }

  /* redraw all windows */
  for (t = LastXfwmWindowList (Scr.stacklist); t != NULL; t = t->prev)
  {
    SetBorder (t->win, NULL, (Scr.Hilite == (t->win)), True, True, (t->win)->frame);
  }
  free (style);
}

void
show_buttons (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindowList *t;
  int val1;
  int val1_unit, n;
  Bool left = False;
  Bool right = False;
  char *side;

  action = GetNextToken (action, &side);
  if (mystrncasecmp (side, "L", 1) == 0)
    left = True;
  else if (mystrncasecmp (side, "R", 1) == 0)
    right = True;
  free (side);

  if ((!left) && (!right))
  {
    xfwm_msg (ERR, "ShowButtons", "Which side ?");
    return;
  }

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "ShowButtons", "Missing argument !");
    return;
  }

  if (left && (val1 >= 0) && (val1 <= 3))
    Scr.nr_left_buttons = val1;
  else if (right && (val1 >= 0) && (val1 <= 3))
    Scr.nr_right_buttons = val1;
  else
    xfwm_msg (ERR, "ShowButtons", "Value out of range");

  /* redraw all windows */
  for (t = LastXfwmWindowList (Scr.stacklist); t != NULL; t = t->prev)
  {
    (t->win)->nr_left_buttons = Scr.nr_left_buttons;
    (t->win)->nr_right_buttons = Scr.nr_right_buttons;
    SetBorder ((t->win), NULL, (Scr.Hilite == (t->win)), True, True, (t->win)->frame);
  }
}

void
SetClickRaise (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= ClickRaise;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~ClickRaise;
  else
  {
    xfwm_msg (ERR, "SetClickRaise", "Unknown parameter, using ClickRaise Off");
    Scr.Options &= ~ClickRaise;
  }
  free (style);
}

void
SetForceFocus (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= ForceFocus;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~ForceFocus;
  else
  {
    xfwm_msg (ERR, "SetForceFocus", "Unknown parameter, using ForceFocus Off");
    Scr.Options &= ~ForceFocus;
  }
  free (style);
}

void
SetMapFocus (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= MapFocus;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~MapFocus;
  else
  {
    xfwm_msg (ERR, "SetMapFocus", "Unknown parameter, using MapFocus On");
    Scr.Options |= MapFocus;
  }
  free (style);
}

void
SetMargin (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int val1;
  int val1_unit, n;
  int where = 0;
  char *side;

  action = GetNextToken (action, &side);
  if ((mystrncasecmp (side, "T", 1) == 0) || (mystrncasecmp (side, "0", 1) == 0))
    where = 0;
  else if ((mystrncasecmp (side, "L", 1) == 0) || (mystrncasecmp (side, "1", 1) == 0))
    where = 1;
  else if ((mystrncasecmp (side, "B", 1) == 0) || (mystrncasecmp (side, "2", 1) == 0))
    where = 2;
  else if ((mystrncasecmp (side, "R", 1) == 0) || (mystrncasecmp (side, "3", 1) == 0))
    where = 3;
  else
  {
    xfwm_msg (ERR, "SetMargin", "Unknown parameter, using 'top'");
    where = 0;
  }
  free (side);

  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  if (n != 1)
  {
    xfwm_msg (ERR, "SetMargin", "Missing argument !");
    return;
  }

  Scr.Margin[where] = val1;
}

void
SetHonorWMFocusHint (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= HonorWMFocusHint;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~HonorWMFocusHint;
  else
  {
    xfwm_msg (ERR, "SetHonorWMFocusHint", "Unknown parameter, using HonorWMFocusHint On");
    Scr.Options |= HonorWMFocusHint;
  }
  free (style);
}

void
SetUseShapedIcons (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *style = NULL;

  action = GetNextToken (action, &style);

  if ((mystrncasecmp (style, "On", 2) == 0) || (mystrncasecmp (style, "Y", 1) == 0) || (mystrncasecmp (style, "1", 1) == 0))
    Scr.Options |= UseShapedIcons;
  else if ((mystrncasecmp (style, "Of", 2) == 0) || (mystrncasecmp (style, "N", 1) == 0) || (mystrncasecmp (style, "0", 1) == 0))
    Scr.Options &= ~UseShapedIcons;
  else
  {
    xfwm_msg (ERR, "UseShapedIcons", "Unknown parameter, using UseShapedIcons On");
    Scr.Options |= UseShapedIcons;
  }
  free (style);
}

