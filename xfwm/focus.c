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

/***********************************************************************
 *
 * xfwm focus-setting code
 *
 ***********************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/********************************************************************
 *
 * Sets the input focus to the indicated window.
 *
 **********************************************************************/

void
SetFocus (Window w, XfwmWindow * Fw, Bool FocusByMouse, Bool force)
{
#ifdef REQUIRES_STASHEVENT
  extern Time lastTimestamp;
#endif

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering SetFocus ()\n");
#endif
  /* if there are hints, only set focus if inputhint isn't False */
  if ((Fw) && !AcceptInput(Fw))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : SetFocus canceled because input hint is false\n");
    fprintf (stderr, "xfwm : Leaving SetFocus ()\n");
#endif
    return;
  }
  
  if ((FocusByMouse) && (Fw) && (Fw != Scr.Focus) && (Fw != &Scr.XfwmRoot))
  {
    XfwmWindow *tmp_win1, *tmp_win2;

    tmp_win1 = Fw->prev;
    tmp_win2 = Fw->next;

    if (tmp_win1 != tmp_win2)
    {
      if (tmp_win1)
	tmp_win1->next = tmp_win2;
      if (tmp_win2)
	tmp_win2->prev = tmp_win1;
    }
    if (Fw != Scr.XfwmRoot.next)
    {
      Fw->next = Scr.XfwmRoot.next;
      if (Scr.XfwmRoot.next)
	Scr.XfwmRoot.next->prev = Fw;
      Scr.XfwmRoot.next = Fw;
      Fw->prev = &Scr.XfwmRoot;
    }
  }

  if (Scr.NumberOfScreens > 1)
  {
    /* Dummy var for XQueryPointer */
    Window dummy_root, dummy_child;
    int dummy_x, dummy_y, dummy_win_x, dummy_win_y;
    unsigned int dummy_mask;

    XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &dummy_x, &dummy_y, &dummy_win_x, &dummy_win_y, &dummy_mask);
    if (dummy_root != Scr.Root)
    {
      if (Scr.Focus != NULL)
      {
	Scr.Focus = NULL;
#ifdef REQUIRES_STASHEVENT
	XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
#else
	XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);
#endif
      }
      XSync (dpy, 0);
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving SetFocus ()\n");
#endif
      return;
    }
  }

  if ((Fw) && (force))
  {
    Scr.Focus = NULL;
  }
  
  if ((Fw) && (Fw->Desk != Scr.CurrentDesk))
  {
    Fw = NULL;
    w = Scr.NoFocusWin;
  }

  if ((Fw) && (Fw->flags & ICONIFIED) && (Fw->icon_w))
    w = Fw->icon_w;

#ifdef REQUIRES_STASHEVENT
  XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
#else
  XSetInputFocus (dpy, w, RevertToParent, CurrentTime);
#endif

  Scr.Focus = Fw;
  Scr.UnknownWinFocused = None;

  if ((Fw) && (Fw->flags & DoesWmTakeFocus))
#ifdef REQUIRES_STASHEVENT
    send_clientmessage (dpy, w, _XA_WM_TAKE_FOCUS, lastTimestamp);
#else
    send_clientmessage (dpy, w, _XA_WM_TAKE_FOCUS, CurrentTime);
#endif

#ifdef DEBUG
  if (Fw)
    fprintf (stderr, "xfwm : SetFocus now set to \"%s\"\n", Fw->name);
  fprintf (stderr, "xfwm : Leaving SetFocus ()\n");
#endif
}
