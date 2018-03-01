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
 * This module is from original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "stack.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void
changeDesks_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int n, val1, val1_unit, val2, val2_unit;

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, 0, 0);
  changeDesks (val1, val2, True, True, True);
}

void
changeDesks (int val1, int val2, Bool handle_focus, Bool broadcast, Bool grab)
{

  int oldDesk;
  static XfwmWindow *StickyWin = NULL;
  XfwmWindow *FocusWin = NULL, *MouseWin = NULL, *t;
  XfwmWindowList *wl;
  Window w;

  oldDesk = Scr.CurrentDesk;


  if (val1 != 0)
  {
    Scr.CurrentDesk = Scr.CurrentDesk + val1;
    if (Scr.CurrentDesk < 0)
      Scr.CurrentDesk = Scr.ndesks - 1;
    else if (Scr.CurrentDesk > Scr.ndesks - 1)
      Scr.CurrentDesk = 0;
  }
  else
  {
    Scr.CurrentDesk = val2;
  }

  if (Scr.CurrentDesk > Scr.ndesks - 1)
    Scr.CurrentDesk = oldDesk;

  if (Scr.CurrentDesk == oldDesk)
    return;

  if (grab)
  {
    MyXGrabServer (dpy);
  }
  if (broadcast)
  {
    Broadcast (XFCE_M_NEW_DESK, 1, Scr.CurrentDesk, 0, 0, 0, 0, 0, 0);
  }
  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  /* Unmap from bottom to top of stack */
  for (wl = LastXfwmWindowList (Scr.stacklist); wl != NULL; wl = wl->prev)
  {
    t = wl->win;

    if (!((t->flags & ICONIFIED) && (t->flags & StickyIcon)) && (!(t->flags & STICKY)) && (!(t->flags & ICON_UNMAPPED)))
    {
      if (t->Desk == oldDesk)
      {
	if (Scr.Focus == t)
	  t->FocusDesk = oldDesk;
	else
	  t->FocusDesk = -1;
	UnmapIt (t);
      }
    }
  }
  /* And map new windows from top to bottom */
  for (wl = Scr.stacklist; wl != NULL; wl = wl->next)
  {
    t = wl->win;

    /* Only change mapping for non-sticky windows */
    if (!((t->flags & ICONIFIED) && (t->flags & StickyIcon)) && (!(t->flags & STICKY)) && (!(t->flags & ICON_UNMAPPED)))
    {
      if (t->Desk == Scr.CurrentDesk)
      {
	MapIt (t);
	if (t->FocusDesk == Scr.CurrentDesk)
	{
	  FocusWin = t;
	}
      }
    }
    else
    {
      /* Window is sticky */
      t->Desk = Scr.CurrentDesk;
      if (Scr.Focus == t)
      {
	t->FocusDesk = oldDesk;
	StickyWin = t;
      }
    }
    /* If its an icon, and its sticking, autoplace it so
     * that it doesn't wind up on top a a stationary
     * icon */

    if (((t->flags & STICKY) || (t->flags & StickyIcon)) && (t->flags & ICONIFIED) && (!(t->flags & ICON_MOVED)) && (!(t->flags & ICON_UNMAPPED)) && (!(t->flags & STARTICONIC)))
      AutoPlace (t, False);
  }

  if (handle_focus)
  {
    /* Dummy var for XQueryPointer */
    Window dummy_root;
    int dummy_x, dummy_y, dummy_win_x, dummy_win_y;
    unsigned int dummy_mask;

    /* Free some CPU, pause for 10 ms */
    sleep_a_little (10000);
    XSync (dpy, 0);
    MouseWin = NULL;
    if (!(Scr.Options & ClickToFocus))
    {
      MouseWin = NULL;
      if ((XQueryPointer (dpy, Scr.Root, &dummy_root, &w, &dummy_x, &dummy_y, &dummy_win_x, &dummy_win_y, &dummy_mask) && (w != None) && (XFindContext (dpy, w, XfwmContext, (caddr_t *) &MouseWin) == XCNOENT)))
      {
	MouseWin = NULL;
      }
    }

    if ((MouseWin) && !(Scr.Options & ClickToFocus) && AcceptInput(MouseWin))
      SetFocus (MouseWin->w, MouseWin, True, False);
    else if ((FocusWin) && AcceptInput(FocusWin))
      SetFocus (FocusWin->w, FocusWin, True, False);
    else if ((StickyWin) && AcceptInput(StickyWin) && (StickyWin->flags & STICKY))
      SetFocus (StickyWin->w, StickyWin, False, False);
    else
      SetFocus (Scr.NoFocusWin, NULL, False, False);
  }
  if (grab)
  {
    MyXUngrabServer (dpy);
    XFlush(dpy);
  }
}

/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void
changeWindowsDesk (XEvent * eventp, Window w, XfwmWindow * t, unsigned long context, char *action, int *Module)
{
  int val1, val2;
  int val1_unit, val2_unit, n;

  if (DeferExecution (eventp, &w, &t, &context, SELECT, ButtonRelease))
    return;

  if (t == NULL)
    return;

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, 0, 0);

  if (n != 2)
  {
    n = GetOneArgument (action, (long *) &val2, &val2_unit, 0, 0);
    val1 = 0;
  }

  if (val1 != 0)
    val1 += t->Desk;
  else
    val1 = val2;

  if (val1 == t->Desk)
    return;

  sendWindowsDesk (val1, t);
}

void
sendWindowsDesk (int desk, XfwmWindow * t)
{
  /* Only change mapping for non-sticky windows */
  if (!((t->flags & ICONIFIED) && (t->flags & StickyIcon)) && (!(t->flags & STICKY)) && (!(t->flags & ICON_UNMAPPED)))
  {
    if (t->Desk == Scr.CurrentDesk)
    {
      t->Desk = desk;
      UnmapIt (t);
      /* If its an icon, auto-place it */
      if (t->flags & ICONIFIED)
      {
	if ((!(t->flags & STARTICONIC)) && (!(t->flags & ICON_MOVED)))
	  AutoPlace (t, False);
	LowerWindow (t);
      }
    }
    else if (desk == Scr.CurrentDesk)
    {
      t->Desk = desk;
      /* If its an icon, auto-place it */
      if (t->flags & ICONIFIED)
      {
	if ((!(t->flags & STARTICONIC)) && (!(t->flags & ICON_MOVED)))
	  AutoPlace (t, False);
	LowerWindow (t);
      }
      MapIt (t);
    }
    else
      t->Desk = desk;
  }
  BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, t);
}
