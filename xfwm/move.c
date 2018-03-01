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

/*
 * This module is from original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for moving windows
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
#include <X11/keysym.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "xinerama.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern XEvent Event;
extern int menuFromFrameOrWindowOrTitlebar;
extern XfwmWindow *Tmp_win;	/* the current xfwm window */
extern int _xfwm_deskwrap (int x_off, int y_off);

/*forward*/
int XFwmMoveWindow (XfwmWindow * tmp_win, int xl, int yt, int Width, int Height, int desk, Bool opaque_move);

/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void
move_window (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int FinalX, FinalY;
  int val1, val2, val1_unit, val2_unit, n;
  int startDesk;
  int center_x;
  int center_y;
  Bool window_deleted = False;

  if (DeferExecution (eventp, &w, &tmp_win, &context, MOVE, ButtonPress))
    return;

  /* Undecorated managed windows should not be moved */
  if (!(tmp_win->flags & (TITLE | BORDER | ICONIFIED | FREEMOVE)))
    return;
    
  center_x = tmp_win->frame_x + (tmp_win->frame_width / 2);
  center_y = tmp_win->frame_y + (tmp_win->frame_height / 2);

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, center_x, center_y);

  startDesk = Scr.CurrentDesk;
  /* gotta have a window */
  w = tmp_win->frame;
  if (tmp_win->flags & ICONIFIED)
  {
    if (tmp_win->icon_pixmap_w != None)
    {
      XUnmapWindow (dpy, tmp_win->icon_w);
      w = tmp_win->icon_pixmap_w;
    }
    else
      w = tmp_win->icon_w;
  }
  if (n == 2)
  {
    FinalX = val1 * val1_unit / 100;
    FinalY = val2 * val2_unit / 100;
  }
  else
    window_deleted = InteractiveMove (&w, tmp_win, &FinalX, &FinalY, eventp);

  if (startDesk != Scr.CurrentDesk)
    Broadcast (XFCE_M_NEW_DESK, 1, Scr.CurrentDesk, 0, 0, 0, 0, 0, 0);

  if (window_deleted)
  {
    XSync (dpy, 0);
    return;
  }

  if (w == tmp_win->frame)
  {
    /* Moving a maximized window un-maximize it */
    if ((tmp_win->flags & MAXIMIZED) && !(tmp_win->flags & ICONIFIED))
    {
      tmp_win->flags &= ~MAXIMIZED;
      RedrawRightButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);
      RedrawLeftButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);
    }
    SetupFrame (tmp_win, FinalX, FinalY, tmp_win->frame_width, tmp_win->frame_height, True, True);
    if (startDesk != Scr.CurrentDesk)
    {
      MapIt (tmp_win);
      if (AcceptInput(tmp_win))
      {
        SetFocus (tmp_win->w, tmp_win, False, False);
      }
      else if (Scr.PreviousFocus && (Scr.PreviousFocus->Desk) != Scr.CurrentDesk)
      {
        Scr.PreviousFocus = NULL;
	SetFocus (Scr.NoFocusWin, NULL, False, False);
      }
    }
  }
  else
    /* icon window */
  {
    tmp_win->flags |= ICON_MOVED;
    tmp_win->icon_x_loc = FinalX;
    tmp_win->icon_xl_loc = FinalX - (tmp_win->icon_w_width - tmp_win->icon_p_width) / 2;
    tmp_win->icon_y_loc = FinalY;
    Broadcast (XFCE_M_ICON_LOCATION, 7, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->icon_x_loc, tmp_win->icon_y_loc, tmp_win->icon_w_width, tmp_win->icon_w_height + tmp_win->icon_p_height);
    XMoveWindow (dpy, tmp_win->icon_w, tmp_win->icon_xl_loc, FinalY + tmp_win->icon_p_height);
    if (tmp_win->icon_pixmap_w != None)
    {
      XMapWindow (dpy, tmp_win->icon_w);
      XMoveWindow (dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc, FinalY);
      XMapWindow (dpy, w);
    }
  }

  return;
}

/*
 * moveLoop : Returns True if window has been deleted during operation
 */

Bool moveLoop (XfwmWindow * tmp_win, int XOffset, int YOffset, int Width, int Height, int *FinalX, int *FinalY, Bool opaque_move, Bool AddWindow)
{
  XGCValues gcv;
  Bool finished = False;
  Bool window_deleted = False;
  Bool done;
  int xl, yt;
  int olddesk = tmp_win->Desk;
  int newdesk = tmp_win->Desk;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &xl, &yt, &dummy_win_x, &dummy_win_y, &dummy_mask);
  *FinalX = xl += XOffset;
  *FinalY = yt += YOffset;

  if (!opaque_move)
  {
    gcv.line_width = 2;
    XChangeGC (dpy, Scr.DrawGC, GCLineWidth, &gcv);

    MoveOutline (tmp_win, xl, yt, Width, Height);
  }

  while (!finished)
  {
    XSync (dpy, 0);
    XNextEvent (dpy, &Event);
#ifdef REQUIRES_STASHEVENT
    StashEventTime (&Event);
#endif
    if (Event.type == MotionNotify)
    {
      while (XCheckMaskEvent (dpy, PointerMotionMask | ButtonMotionMask, &Event));
#ifdef REQUIRES_STASHEVENT
      {
	StashEventTime (&Event);
      }
#endif
    }

    done = False;
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if (Event.type == KeyPress)
      Keyboard_shortcuts (&Event, ButtonRelease);
    switch (Event.type)
    {
    case FocusIn:
    case FocusOut:
    case EnterNotify:
    case LeaveNotify:
      /* simply ignore those events */
      break;
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym (&(Event.xkey), 0) == XK_Escape)
      {
	if (!(tmp_win->flags & ICONIFIED))
	{
	  *FinalX = tmp_win->frame_x;
	  *FinalY = tmp_win->frame_y;
	  SnapMove (FinalX, FinalY, Width, Height, tmp_win);
	}
	else
	{
	  *FinalX = tmp_win->icon_x_loc;
	  *FinalY = tmp_win->icon_y_loc;
	}
	finished = True;
      }
      done = True;
      break;
    case ButtonPress:
      XAllowEvents (dpy, ReplayPointer, CurrentTime);
      done = 1;
      break;
    case ButtonRelease:
      xl = Event.xmotion.x_root + XOffset;
      yt = Event.xmotion.y_root + YOffset;

      if (!(tmp_win->flags & ICONIFIED))
	SnapMove (&xl, &yt, Width, Height, tmp_win);
      else
      {
	xl = ((int) (xl / Scr.icongrid)) * Scr.icongrid;
	yt = ((int) (yt / Scr.icongrid)) * Scr.icongrid;
      }

      *FinalX = xl;
      *FinalY = yt;

      done = True;
      finished = True;
      break;
    case MotionNotify:
      xl = Event.xmotion.x_root;
      yt = Event.xmotion.y_root;
      /* redraw the rubberband */
      xl += XOffset;
      yt += YOffset;

      if (!(tmp_win->flags & ICONIFIED))
      {
	SnapMove (&xl, &yt, Width, Height, tmp_win);
	newdesk = XFwmMoveWindow (tmp_win, xl, yt, Width, Height, newdesk, opaque_move);
      }
      else
      {
	xl = ((int) (xl / Scr.icongrid)) * Scr.icongrid;
	yt = ((int) (yt / Scr.icongrid)) * Scr.icongrid;
	tmp_win->icon_x_loc = xl;
	tmp_win->icon_xl_loc = xl - (tmp_win->icon_w_width - tmp_win->icon_p_width) / 2;
	tmp_win->icon_y_loc = yt;
	if (tmp_win->icon_pixmap_w != None)
	  XMoveWindow (dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc, yt);
	else if (tmp_win->icon_w != None)
	  XMoveWindow (dpy, tmp_win->icon_w, tmp_win->icon_xl_loc, yt + tmp_win->icon_p_height);

      }
      done = True;
      break;
    case UnmapNotify:
      if (Event.xunmap.window == tmp_win->w)
      {
	finished = True;
	done = True;
	window_deleted = True;
      }
      DispatchEvent ();
      break;
    case DestroyNotify:
      if ((Event.xdestroywindow.window == tmp_win->frame) || (Event.xdestroywindow.window == tmp_win->w))
      {
	finished = True;
	done = True;
	window_deleted = True;
      }
      DispatchEvent ();
      break;
    case ConfigureRequest:
      /* Here we shall override the value_mask to avoid programs such as
         xmovie to generate a change in their geometry during resize 
         operation, otherwise xfwm enters an infinite loop */
      if (Event.xconfigurerequest.window == tmp_win->w)
	Event.xconfigurerequest.value_mask &= ~(CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
      DispatchEvent ();
      break;
    default:
      if (!opaque_move)
	MoveOutline (tmp_win, 0, 0, 0, 0);
      DispatchEvent ();
      if (!opaque_move)
	MoveOutline (tmp_win, xl, yt, Width, Height);
      break;
    }
  }
  /* erase the rubber-band */
  if (!opaque_move)
    MoveOutline (tmp_win, 0, 0, 0, 0);

  if (!(window_deleted) && (newdesk != olddesk))
    tmp_win->Desk = newdesk;
  return (window_deleted);
}

/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard 
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void
Keyboard_shortcuts (XEvent * Event, int ReturnEvent)
{
  int x, y, x_root, y_root;
  int move_size, x_move, y_move;
  KeySym keysym;
  /* Dummy var for XQueryPointer */
  Window dummy_root;
  unsigned int dummy_mask;

  /* Pick the size of the cursor movement */
  move_size = Scr.EntryHeight;
  if (Event->xkey.state & ControlMask)
    move_size = 1;
  if (Event->xkey.state & ShiftMask)
    move_size = 100;

  keysym = XLookupKeysym (&Event->xkey, 0);

  x_move = 0;
  y_move = 0;
  switch (keysym)
  {
  case XK_Up:
  case XK_k:
  case XK_p:
    y_move = -move_size;
    break;
  case XK_Down:
  case XK_n:
  case XK_j:
    y_move = move_size;
    break;
  case XK_Left:
  case XK_b:
  case XK_h:
    x_move = -move_size;
    break;
  case XK_Right:
  case XK_f:
  case XK_l:
    x_move = move_size;
    break;
  case XK_Return:
  case XK_space:
    /* beat up the event */
    Event->type = ReturnEvent;
    break;
  case XK_Escape:
    /* simple code to bag out of move - CKH */
    /* return keypress event instead */
    Event->type = KeyPress;
    Event->xkey.keycode = XKeysymToKeycode (Event->xkey.display, keysym);
    break;
  default:
    break;
  }
  XQueryPointer (dpy, Scr.Root, &dummy_root, &Event->xany.window, &x_root, &y_root, &x, &y, &dummy_mask);

  if ((x_move != 0) || (y_move != 0))
  {
    /* beat up the event */
    XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, 0, 0, x_root + x_move, y_root + y_move);

    /* beat up the event */
    Event->type = MotionNotify;
    Event->xkey.x += x_move;
    Event->xkey.y += y_move;
    Event->xkey.x_root += x_move;
    Event->xkey.y_root += y_move;
  }
}

/*
 * InteractiveMove : Returns True if window has been deleted during operation
 */

Bool InteractiveMove (Window * win, XfwmWindow * tmp_win, int *FinalX, int *FinalY, XEvent * eventp)
{
  extern int Stashed_X, Stashed_Y;
  int origDragX, origDragY, DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset;
  Window w;
  Bool window_deleted = False, iconified;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  unsigned int dummy_bw, dummy_depth;

  w = *win;

  InstallRootColormap ();
  if (menuFromFrameOrWindowOrTitlebar)
  {
    XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, 0, 0, Stashed_X, Stashed_Y);
  }

  DragX = eventp->xbutton.x_root;
  DragY = eventp->xbutton.y_root;

  if (!GrabEm (MOVE))
  {
    XBell (dpy, Scr.screen);
    return False;
  }

  XGetGeometry (dpy, w, &dummy_root, &origDragX, &origDragY, (unsigned int *) &DragWidth, (unsigned int *) &DragHeight, &dummy_bw, &dummy_depth);

  DragWidth += dummy_bw;
  DragHeight += dummy_bw;
  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  iconified = tmp_win->flags & ICONIFIED;

  if (!(Scr.Options & MoveOpaqueWin) && !(iconified))
    MyXGrabServer (dpy);
  window_deleted = moveLoop (tmp_win, XOffset, YOffset, DragWidth, DragHeight, FinalX, FinalY, (Scr.Options & MoveOpaqueWin) || (iconified), False);
  if (!(Scr.Options & MoveOpaqueWin) && !(iconified))
    MyXUngrabServer (dpy);
  UninstallRootColormap ();

  UngrabEm ();

  /* Discard all ConfigureRequest relative to resized window */
  while ((!window_deleted) && XPending (dpy))
  {
    XNextEvent (dpy, &Event);
    if ((Event.type == ConfigureRequest) && (Event.xconfigurerequest.window == tmp_win->w))
      Event.xconfigurerequest.value_mask &= ~(CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
    else if ((Event.type == UnmapNotify) && (Event.xunmap.window == tmp_win->w))
      window_deleted = True;
    else if ((Event.type == DestroyNotify) && (Event.xdestroywindow.window == tmp_win->frame))
      window_deleted = True;
    DispatchEvent ();
  }
  return (window_deleted);
}

void
domove (XfwmWindow * tmp_win, int xl, int yt, int Width, int Height, Bool opaque_move)
{
  if (!opaque_move)
    MoveOutline (tmp_win, xl, yt, Width, Height);
  else
  {
    tmp_win->frame_x = tmp_win->shade_x = xl;
    tmp_win->frame_y = tmp_win->shade_y = yt;
    XMoveWindow (dpy, tmp_win->frame, xl, yt);
  }
}

int
XFwmMoveWindow (XfwmWindow * tmp_win, int xl, int yt, int Width, int Height, int desk, Bool opaque_move)
{
  int newdesk = desk;
  int evx, evy;
  int warpx, warpy;
  
  if (Scr.SnapSize < MINRESISTANCE)
  {				/* do the standard stuff */
    domove (tmp_win, xl, yt, Width, Height, opaque_move);
    return (tmp_win->Desk);
  }
  else
  {
    evx = Event.xmotion.x_root;
    evy = Event.xmotion.y_root;
    if (evx == 0)
    {
      Scr.EdgeScrollX--;
    }
    else if (evy == 0)
    {
      Scr.EdgeScrollY--;
    }
    else if (evx >= Scr.MyDisplayWidth - 1)
    {
      Scr.EdgeScrollX++;
    }
    else if (evy >= Scr.MyDisplayHeight - 1)
    {
      Scr.EdgeScrollY++;
    }
    else
    {
      Scr.EdgeScrollY = Scr.EdgeScrollX = 0;
    }
    /* now check if some EdgeCounter has reached Snapsize, so that
       we shall emit signals to switch to the next desk (computed in 
       the deskwrap-helper)
     */
    if (Scr.EdgeScrollX <= -Scr.SnapSize)
    {				/* move desktop left */
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = Scr.MyDisplayWidth;
      warpy = evy;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx) - Scr.SnapSize, my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy));
      if (!opaque_move)
	MoveOutline (tmp_win, 0, 0, 0, 0);
      newdesk = _xfwm_deskwrap (-1, 0);
      if (opaque_move)
	tmp_win->Desk = newdesk;
      changeDesks (0, newdesk, False, False, !opaque_move);
      DispatchEvent ();
      domove (tmp_win, xl + (Scr.MyDisplayWidth - Scr.SnapSize), yt, Width, Height, opaque_move);
      return (newdesk);
    }
    if (Scr.EdgeScrollX >= +Scr.SnapSize)
    {				/* move desktop right */
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = 0;
      warpy = evy;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayX (warpx, warpy), warpx) + Scr.SnapSize, my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy));
      if (!opaque_move)
	MoveOutline (tmp_win, 0, 0, 0, 0);
      newdesk = _xfwm_deskwrap (+1, 0);
      if (opaque_move)
	tmp_win->Desk = newdesk;
      changeDesks (0, newdesk, False, False, !opaque_move);
      DispatchEvent ();
      domove (tmp_win, xl - (Scr.MyDisplayWidth - Scr.SnapSize), yt, Width, Height, opaque_move);
      return (newdesk);
    }
    if (Scr.EdgeScrollY <= -Scr.SnapSize)
    {				/* move desktop upper */
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = evx;
      warpy = Scr.MyDisplayHeight;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx), my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy) - Scr.SnapSize);
      if (!opaque_move)
	MoveOutline (tmp_win, 0, 0, 0, 0);
      newdesk = _xfwm_deskwrap (0, -1);
      if (opaque_move)
	tmp_win->Desk = newdesk;
      changeDesks (0, newdesk, False, False, !opaque_move);
      DispatchEvent ();
      domove (tmp_win, xl, yt + (Scr.MyDisplayHeight - Scr.SnapSize), Width, Height, opaque_move);
      return (newdesk);
    }
    if (Scr.EdgeScrollY >= +Scr.SnapSize)
    {				/* move desktop lower */
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = evx;
      warpy = 0;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx), my_min (MyDisplayY (warpx, warpy), warpy) + Scr.SnapSize);
      if (!opaque_move)
	MoveOutline (tmp_win, 0, 0, 0, 0);
      newdesk = _xfwm_deskwrap (0, +1);
      if (opaque_move)
	tmp_win->Desk = newdesk;
      changeDesks (0, newdesk, False, False, !opaque_move);
      DispatchEvent ();
      domove (tmp_win, xl, yt - (Scr.MyDisplayHeight - Scr.SnapSize), Width, Height, opaque_move);
      return (newdesk);
    }
    domove (tmp_win, xl, yt, Width, Height, opaque_move);
    return (newdesk);
  }
  return (newdesk);
}
