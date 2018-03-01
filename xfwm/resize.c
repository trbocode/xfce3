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
 * window resizing borrowed from the "wm" window manager
 *
 ***********************************************************************/
#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "xfwm.h"
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "xinerama.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static int dragx;		/* all these variables are used */
static int dragy;		/* in resize operations */
static int dragWidth;
static int dragHeight;

static int origx;
static int origy;
static int origWidth;
static int origHeight;
static int initHeight;

static int ymotion = 0, xmotion = 0;
static int last_width, last_height;
extern int menuFromFrameOrWindowOrTitlebar;
extern XfwmWindow *Tmp_win;	/* the current xfwm window */

extern Window PressedW;

/****************************************************************************
 *
 * Starts a window resize operation
 *
 ****************************************************************************/
void
resize_window (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  Bool finished = False, done = False, abort = False, window_deleted = False;
  int x, y, mouse_x, mouse_y;
  int delta_x;
  int delta_y;
  Window ResizeWindow;
  extern int Stashed_X, Stashed_Y;
  int val1, val2, val1_unit, val2_unit, n;
  int center_x;
  int center_y;
  XGCValues gcv;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  unsigned int dummy_bw, dummy_depth;


  if (DeferExecution (eventp, &w, &tmp_win, &context, MOVE, ButtonPress))
    return;

  /* Undecorated managed windows should not be moved */
  if (!(tmp_win->flags & (TITLE | BORDER | ICONIFIED | FREEMOVE)))
    return;
    
  tmp_win->flags &= ~MAXIMIZED;
  RedrawRightButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);
  RedrawLeftButtons (tmp_win, NULL, (Scr.Hilite == tmp_win), True, None);

  center_x = tmp_win->frame_x + (tmp_win->frame_width / 2);
  center_y = tmp_win->frame_y + (tmp_win->frame_height / 2);

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit, center_x, center_y);

  ResizeWindow = tmp_win->frame;

  if (!(Scr.Options & ResizeOpaqueWin))
  {
    gcv.line_width = 2;
    XChangeGC (dpy, Scr.DrawGC, GCLineWidth, &gcv);
  }

  if (n == 2)
  {
    dragWidth = val1 * val1_unit / 100;
    dragHeight = val2 * val2_unit / 100;
    dragWidth += 2 * (tmp_win->boundary_width + tmp_win->bw);
    dragHeight += tmp_win->title_height + 2 * (tmp_win->boundary_width + tmp_win->bw);


    ConstrainSize (tmp_win, &dragWidth, &dragHeight);
    SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y, dragWidth, dragHeight, False, True);

    ResizeWindow = None;
    return;
  }

  InstallRootColormap ();
  if (menuFromFrameOrWindowOrTitlebar)
  {
    XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (Stashed_X, Stashed_Y) - 1, Stashed_X), my_min (MyDisplayMaxY (Stashed_X, Stashed_Y) - 1, Stashed_Y));
  }

  if (!GrabEm (MOVE))
  {
    XBell (dpy, Scr.screen);
    return;
  }

  if (!(Scr.Options & ResizeOpaqueWin))
    MyXGrabServer (dpy);

  if (tmp_win->flags & SHADED)
  {
    dragx = tmp_win->shade_x;
    dragy = tmp_win->shade_y;
    dragWidth = tmp_win->shade_width;
    initHeight = tmp_win->frame_height;
    dragHeight = tmp_win->shade_height;
  }
  else
  {
    XGetGeometry (dpy, (Drawable) ResizeWindow, &dummy_root, &dragx, &dragy, (unsigned int *) &dragWidth, (unsigned int *) &dragHeight, &dummy_bw, &dummy_depth);
    initHeight = dragHeight;
  }
  origx = dragx;
  origy = dragy;
  origWidth = dragWidth;
  origHeight = dragHeight;

  ymotion = xmotion = 0;

  last_width = 0;
  last_height = 0;

  delta_x = 0;
  delta_y = 0;

  /* Get the current position to determine which border to resize */
  if ((PressedW != Scr.Root) && (PressedW != None))
  {
    mouse_x = eventp->xbutton.x_root;
    mouse_y = eventp->xbutton.y_root;
    if (PressedW == tmp_win->sides[0])	/* top */
    {
      xmotion = 0;
      ymotion = 1;
      delta_y = dragy - mouse_y;
    }
    else if (PressedW == tmp_win->sides[1])	/* right */
    {
      xmotion = -1;
      ymotion = 0;
      delta_x = dragx + dragWidth - mouse_x;
    }
    else if (PressedW == tmp_win->sides[2])	/* bottom */
    {
      xmotion = 0;
      ymotion = -1;
      delta_y = dragy + dragHeight - mouse_y;
    }
    else if (PressedW == tmp_win->sides[3])	/* left */
    {
      xmotion = 1;
      ymotion = 0;
      delta_x = dragx - mouse_x;
    }
    else if (PressedW == tmp_win->corners[0])	/* upper-left */
    {
      xmotion = 1;
      ymotion = 1;
      delta_x = dragx - mouse_x;
      delta_y = dragy - mouse_y;
    }
    else if (PressedW == tmp_win->corners[1])	/* upper-right */
    {
      xmotion = -1;
      ymotion = 1;
      delta_x = dragx + dragWidth - mouse_x;
      delta_y = dragy - mouse_y;
    }
    else if (PressedW == tmp_win->corners[2])	/* lower left */
    {
      xmotion = 1;
      ymotion = -1;
      delta_x = dragx - mouse_x;
      delta_y = dragy + dragHeight - mouse_y;
    }
    else if (PressedW == tmp_win->corners[3])	/* lower right */
    {
      xmotion = -1;
      ymotion = -1;
      delta_x = dragx + dragWidth - mouse_x;
      delta_y = dragy + dragHeight - mouse_y;
    }
  }
  /* draw the rubber-band window */
  if (!(Scr.Options & ResizeOpaqueWin))
  {
    MoveOutline (tmp_win, dragx, dragy, dragWidth, dragHeight);
  }
  /* loop to resize */
  while (!finished)
  {
    XSync (dpy, 0);
    XNextEvent (dpy, &Event);
#ifdef REQUIRES_STASHEVENT
    StashEventTime (&Event);
#endif
    if (Event.type == MotionNotify)
    {
      while (XCheckMaskEvent (dpy, PointerMotionMask | ButtonMotionMask, &Event))
      {
#ifdef REQUIRES_STASHEVENT
	StashEventTime (&Event);
#endif
      };
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
    case ButtonPress:
      XAllowEvents (dpy, ReplayPointer, CurrentTime);
    case KeyPress:
      /* simple code to bag out of move - CKH */
      if (XLookupKeysym (&(Event.xkey), 0) == XK_Escape)
      {
	abort = True;
	finished = True;
      }
      done = True;
      break;
    case ButtonRelease:
      finished = True;
      done = True;
      break;
    case MotionNotify:
      x = Event.xmotion.x_root + delta_x;
      y = Event.xmotion.y_root + delta_y;
      DoResize (x, y, tmp_win);
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
      if (!(Scr.Options & ResizeOpaqueWin))
	MoveOutline (tmp_win, 0, 0, 0, 0);
      DispatchEvent ();
      if (!(Scr.Options & ResizeOpaqueWin))
	MoveOutline (tmp_win, dragx, dragy, dragWidth, dragHeight);
      break;
    }
  }

  /* erase the rubber-band */
  if (!(Scr.Options & ResizeOpaqueWin))
    MoveOutline (tmp_win, 0, 0, 0, 0);

  if (!window_deleted)
  {
    if (!abort)
    {
      ConstrainSize (tmp_win, &dragWidth, &dragHeight);
      SetupFrame (tmp_win, dragx, dragy, dragWidth, ((tmp_win->flags & SHADED) ? initHeight : dragHeight), False, True);
    }
    else
      SetupFrame (tmp_win, origx, origy, origWidth, ((tmp_win->flags & SHADED) ? initHeight : origHeight), False, True);
  }
  UninstallRootColormap ();
  ResizeWindow = None;
  if (!(Scr.Options & ResizeOpaqueWin))
    MyXUngrabServer (dpy);
  UngrabEm ();
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

  xmotion = 0;
  ymotion = 0;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root  - the X coordinate in the root window
 *      y_root  - the Y coordinate in the root window
 *      tmp_win - the current xfwm window
 *
 ************************************************************************/
void
DoResize (int x_root, int y_root, XfwmWindow * tmp_win)
{
  int action = 0;

  if ((y_root <= origy) || ((ymotion == 1) && (y_root < origy + origHeight - 1)))
  {
    SnapXY (&x_root, &y_root, tmp_win);
    dragy = y_root;
    if (!(tmp_win->flags & SHADED))
      dragHeight = origy + origHeight - y_root;
    action = 1;
    ymotion = 1;
  }
  else if ((y_root >= origy + origHeight - 1) || ((ymotion == -1) && (y_root > origy)))
  {
    y_root++;
    SnapXY (&x_root, &y_root, tmp_win);
    dragy = origy;
    if (!(tmp_win->flags & SHADED))
      dragHeight = y_root - dragy;
    action = 1;
    ymotion = -1;
  }

  if ((x_root <= origx) || ((xmotion == 1) && (x_root < origx + origWidth - 1)))
  {
    SnapXY (&x_root, &y_root, tmp_win);
    dragx = x_root;
    dragWidth = origx + origWidth - x_root;
    action = 1;
    xmotion = 1;
  }
  else if ((x_root >= origx + origWidth - 1) || ((xmotion == -1) && (x_root > origx)))
  {
    x_root++;
    SnapXY (&x_root, &y_root, tmp_win);
    dragx = origx;
    dragWidth = x_root - origx;
    action = 1;
    xmotion = -1;
  }

  if (action)
  {
    ConstrainSize (tmp_win, &dragWidth, &dragHeight);
    if (xmotion == 1)
      dragx = origx + origWidth - dragWidth;
    if (ymotion == 1)
      dragy = origy + origHeight - dragHeight;

    if (!(Scr.Options & ResizeOpaqueWin))
      MoveOutline (tmp_win, dragx, dragy, dragWidth, dragHeight);
    else
      SetupFrame (tmp_win, dragx, dragy, dragWidth, ((tmp_win->flags & SHADED) ? initHeight : dragHeight), False, False);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 * 
 ***********************************************************************/

void
ConstrainSize (XfwmWindow * tmp_win, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;

  dwidth -= 2 * (tmp_win->boundary_width + tmp_win->bw);
  dheight -= tmp_win->title_height + 2 * (tmp_win->boundary_width + tmp_win->bw);

  minWidth = tmp_win->hints.min_width;
  minHeight = tmp_win->hints.min_height;

  baseWidth = tmp_win->hints.base_width;
  baseHeight = tmp_win->hints.base_height;

  maxWidth = tmp_win->hints.max_width;
  maxHeight = tmp_win->hints.max_height;

  xinc = tmp_win->hints.width_inc;
  yinc = tmp_win->hints.height_inc;

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;


  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


  /*
   * Third, adjust for aspect ratio
   */
#define maxAspectX tmp_win->hints.max_aspect.x
#define maxAspectY tmp_win->hints.max_aspect.y
#define minAspectX tmp_win->hints.min_aspect.x
#define minAspectY tmp_win->hints.min_aspect.y
  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   * 
   */

  if (tmp_win->hints.flags & PAspect)
  {
    if ((minAspectX * dheight > minAspectY * dwidth) && (xmotion == 0))
    {
      /* Change width to match */
      delta = makemult (minAspectX * dheight / minAspectY - dwidth, xinc);
      if (dwidth + delta <= maxWidth)
	dwidth += delta;
    }
    if (minAspectX * dheight > minAspectY * dwidth)
    {
      delta = makemult (dheight - dwidth * minAspectY / minAspectX, yinc);
      if (dheight - delta >= minHeight)
	dheight -= delta;
      else
      {
	delta = makemult (minAspectX * dheight / minAspectY - dwidth, xinc);
	if (dwidth + delta <= maxWidth)
	  dwidth += delta;
      }
    }

    if ((maxAspectX * dheight < maxAspectY * dwidth) && (ymotion == 0))
    {
      delta = makemult (dwidth * maxAspectY / maxAspectX - dheight, yinc);
      if (dheight + delta <= maxHeight)
	dheight += delta;
    }
    if ((maxAspectX * dheight < maxAspectY * dwidth))
    {
      delta = makemult (dwidth - maxAspectX * dheight / maxAspectY, xinc);
      if (dwidth - delta >= minWidth)
	dwidth -= delta;
      else
      {
	delta = makemult (dwidth * maxAspectY / maxAspectX - dheight, yinc);
	if (dheight + delta <= maxHeight)
	  dheight += delta;
      }
    }
  }


  /*
   * Fourth, account for border width and title height
   */
  *widthp = dwidth + 2 * (tmp_win->boundary_width + tmp_win->bw);
  if (!(tmp_win->flags & SHADED))
    *heightp = dheight + tmp_win->title_height + 2 * (tmp_win->boundary_width + tmp_win->bw);
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	MoveOutline - move a window outline
 *
 *  Inputs:
 *	root	    - the window we are outlining
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *
 ***********************************************************************/
void
MoveOutline (XfwmWindow * tmp_win, int x, int y, int width, int height)
{
  static int lastx = 0;
  static int lasty = 0;
  static int lastWidth = 0;
  static int lastHeight = 0;

  if ((x == lastx) && (y == lasty) && (width == lastWidth) && (height == lastHeight))
    return;

  /* undraw the old one, if any */
  if (lastWidth || lastHeight)
  {
    XDrawRectangle (dpy, Scr.Root, Scr.DrawGC, lastx, lasty, lastWidth, lastHeight);
    if (tmp_win)
      DisplayPosition (tmp_win, lastx, lasty, lastWidth, lastHeight);
  }

  lastx = x;
  lasty = y;
  lastWidth = width;
  lastHeight = height;

  /* draw the new one, if any */
  if (lastWidth || lastHeight)
  {
    if (tmp_win)
      DisplayPosition (tmp_win, lastx, lasty, lastWidth, lastHeight);
    XDrawRectangle (dpy, Scr.Root, Scr.DrawGC, lastx, lasty, lastWidth, lastHeight);
  }
}

void
DisplayPosition (XfwmWindow * tmp_win, int x, int y, int w, int h)
{
  char strxy[20], strw[10], strh[10], strd[10];
  int dwidth, dheight;
  int offsetx, offsetw, offsetd;
  XPoint points[3];

  if (!tmp_win)
    return;

  dheight = h - tmp_win->title_height - 2 * (tmp_win->boundary_width + tmp_win->bw);
  dwidth = w - 2 * (tmp_win->boundary_width + tmp_win->bw);

  dwidth -= tmp_win->hints.base_width;
  dheight -= tmp_win->hints.base_height;
  if (tmp_win->hints.width_inc)
    dwidth /= tmp_win->hints.width_inc;
  if (tmp_win->hints.height_inc)
    dheight /= tmp_win->hints.height_inc;

  sprintf (strxy, "(X=%d, Y=%d)", x, y);
  sprintf (strw, "(W=%d)", dwidth);
  sprintf (strh, "(H=%d)", dheight);
  sprintf (strd, "[Desk %d]", Scr.CurrentDesk + 1);
  offsetx = XTextWidth (Scr.StdFont.font, strxy, strlen (strxy));
  offsetw = XTextWidth (Scr.StdFont.font, strw, strlen (strw));
  offsetd = XTextWidth (Scr.StdFont.font, strd, strlen (strd));

  XDrawString (dpy, Scr.Root, Scr.HintsGC, x + (w - offsetx) / 2, y + 5 + Scr.StdFont.height, strxy, strlen (strxy));
  XDrawString (dpy, Scr.Root, Scr.HintsGC, x + (w - offsetw) / 2, y + h + 5 + Scr.StdFont.height, strw, strlen (strw));
  XDrawString (dpy, Scr.Root, Scr.HintsGC, x + w + 5, y + (h + Scr.StdFont.height) / 2, strh, strlen (strh));
  if (h > 3 * Scr.StdFont.height)
    XDrawString (dpy, Scr.Root, Scr.HintsGC, x + (w - offsetd) / 2, y + (h + Scr.StdFont.height) / 2, strd, strlen (strd));
  points[0].x = x + 15;
  points[0].y = y + h + 5;
  points[1].x = x;
  points[1].y = y + h + 10;
  points[2].x = x + 15;
  points[2].y = y + h + 15;
  XFillPolygon (dpy, Scr.Root, Scr.HintsGC, points, 3, Convex, CoordModeOrigin);
  points[0].x = x + w - 15;
  points[0].y = y + h + 5;
  points[1].x = x + w;
  points[1].y = y + h + 10;
  points[2].x = x + w - 15;
  points[2].y = y + h + 15;
  XFillPolygon (dpy, Scr.Root, Scr.HintsGC, points, 3, Convex, CoordModeOrigin);
  points[0].x = x + w + 5;
  points[0].y = y + 15;
  points[1].x = x + w + 10;
  points[1].y = y;
  points[2].x = x + w + 15;
  points[2].y = y + 15;
  XFillPolygon (dpy, Scr.Root, Scr.HintsGC, points, 3, Convex, CoordModeOrigin);
  points[0].x = x + w + 5;
  points[0].y = y + h - 15;
  points[1].x = x + w + 10;
  points[1].y = y + h;
  points[2].x = x + w + 15;
  points[2].y = y + h - 15;
  XFillPolygon (dpy, Scr.Root, Scr.HintsGC, points, 3, Convex, CoordModeOrigin);
  XDrawLine (dpy, Scr.Root, Scr.HintsGC, x + 15, y + h + 10, x + (w - offsetw) / 2 - 5, y + h + 10);
  XDrawLine (dpy, Scr.Root, Scr.HintsGC, x + (w + offsetw) / 2 + 5, y + h + 10, x + w - 15, y + h + 10);
  XDrawLine (dpy, Scr.Root, Scr.HintsGC, x + w + 10, y + 15, x + w + 10, y + (h - Scr.StdFont.height) / 2 - 5);
  XDrawLine (dpy, Scr.Root, Scr.HintsGC, x + w + 10, y + (h + Scr.StdFont.height) / 2 + 5, x + w + 10, y + h - 15);
}
