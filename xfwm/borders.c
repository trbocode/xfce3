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
 * xfwm window border drawing code
 *
 ***********************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "xfwm.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "themes.h"
#include "xinerama.h"
#include "module.h"

#include <X11/extensions/shape.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern Window PressedW;
XGCValues Globalgcv;
unsigned long Globalgcm;
/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void
SetBorder (XfwmWindow * t, XRectangle *area, Bool onoroff, Bool force, Bool Mapped, Window expose_win)
{
  int y, i, x, fh;
  GC ReliefGC, ShadowGC;
  Pixel BackColor;
  Pixmap TextColor;
  Bool NewColor = False;
  unsigned long valuemask;
  static unsigned int corners[4];

  corners[0] = TOP_HILITE | LEFT_HILITE;
  corners[1] = TOP_HILITE | RIGHT_HILITE;
  corners[2] = BOTTOM_HILITE | LEFT_HILITE;
  corners[3] = BOTTOM_HILITE | RIGHT_HILITE;

  if (!t)
    return;

  if (t->flags & SHADED)
    fh = t->title_height + 2 * t->boundary_width;
  else
    fh = t->frame_height;

  if (onoroff)
  {
    if ((Scr.Hilite != t) || (force))
      NewColor = True;

    /* make sure that the previously highlighted window got unhighlighted */
    if ((Scr.Hilite != t) && (Scr.Hilite != NULL))
      SetBorder (Scr.Hilite, NULL, False, False, True, None);

    Scr.Hilite = t;
    TextColor = GetDecor (t, HiColors.fore);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    if (Scr.Hilite == t)
    {
      Scr.Hilite = NULL;
      NewColor = True;
    }
    if (force)
      NewColor = True;
    TextColor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  /* if forced redraw or the call is not issued from an expose event, don't use clipping area */
  if (NewColor || (expose_win == None))
  {
    area = NULL;
  }
   
  if (t->flags & ICONIFIED)
  {
    DrawIconWindow (t, area);
    return;
  }

  valuemask = 0;
  if (t->flags & TITLE)
  {
    RedrawLeftButtons (t, area, onoroff, NewColor, expose_win);
    RedrawRightButtons (t, area, onoroff, NewColor, expose_win);
    if ((expose_win == t->title_w) || (expose_win == None) || NewColor)
    {
      SetTitleBar (t,area, onoroff);
    }
  }

  if (t->flags & BORDER)
  {
    /* draw relief lines */
    y = fh - 2 * t->corner_width;
    x = t->frame_width - 2 * t->corner_width + t->bw;
    for (i = 0; i < 4; i++)
    {
      int vertical = i % 2;
      if ((expose_win == t->sides[i]) || (expose_win == None) || NewColor)
      {
	GC sgc, rgc;

	rgc = ReliefGC;
	sgc = ShadowGC;
	RelieveWindow (t, t->sides[i], area, 0, 0, ((vertical) ? t->boundary_width : x), ((vertical) ? y : t->boundary_width), rgc, sgc, (0x0001 << i));
      }
      if ((expose_win == t->corners[i]) || (expose_win == None) || NewColor)
      {
	GC rgc, sgc;

	rgc = ReliefGC;
	sgc = ShadowGC;
	RelieveWindow (t, t->corners[i], area, 0, 0, t->corner_width, t->corner_width, rgc, sgc, corners[i]);
      }
    }
  }
#if 0
#ifndef OLD_STYLE
  SetInnerBorder (t, onoroff);
#endif
#endif
}

void
RedrawLeftButtons (XfwmWindow * t, XRectangle *area, Bool onoroff, Bool NewColor, Window expose_win)
{
  int i;
  GC ReliefGC, ShadowGC;
  Pixel BackColor;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  for (i = 0; i < Scr.nr_left_buttons; ++i)
  {
    if (t->left_w[i] != None)
    {
      enum ButtonState bs = GetButtonState (t->left_w[i]);
      ButtonFace *bf = &GetDecor (t, left_buttons[i].state[bs]);
      if ((expose_win == t->left_w[i]) || (expose_win == None) || NewColor)
      {
	int inverted = PressedW == t->left_w[i];
	for (; bf; bf = bf->next)
	  DrawButton (t, t->left_w[i], area, t->title_height, t->title_height, bf, ReliefGC, ShadowGC, inverted, GetDecor (t, left_buttons[i].flags));
      }
    }
  }
}

void
RedrawRightButtons (XfwmWindow * t, XRectangle *area, Bool onoroff, Bool NewColor, Window expose_win)
{
  int i;
  GC ReliefGC, ShadowGC;
  Pixel BackColor;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  for (i = 0; i < Scr.nr_right_buttons; ++i)
  {
    if (t->right_w[i] != None)
    {
      enum ButtonState bs = GetButtonState (t->right_w[i]);
      ButtonFace *bf = &GetDecor (t, right_buttons[i].state[bs]);
      if ((expose_win == t->right_w[i]) || (expose_win == None) || NewColor)
      {
	int inverted = PressedW == t->right_w[i];
	for (; bf; bf = bf->next)
	  DrawButton (t, t->right_w[i], area, t->title_height, t->title_height, bf, ReliefGC, ShadowGC, inverted, GetDecor (t, right_buttons[i].flags));

      }
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *      Setupframe - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the XfwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/

void
SetupFrame (XfwmWindow * tmp_win, int x, int y, int w, int h, Bool sendEvent, Bool broadcast)
{
  XWindowChanges winwc;
  Bool Resized = False, Moved = False;
  int xwidth, ywidth, left, right, horig;
  int cx, cy, i;
  unsigned long winmask;
  int center_x, center_y;

  if (!tmp_win)
    return;

  horig = h;

  if (tmp_win->flags & TITLE)
    tmp_win->title_height = GetDecor (tmp_win, TitleHeight);

  if (tmp_win->flags & SHADED)
    h = tmp_win->title_height + 2 * tmp_win->boundary_width;

  /* if windows is not being maximized, save size in case of maximization */
  if (!(tmp_win->flags & MAXIMIZED))
  {
    tmp_win->orig_x = x;
    tmp_win->orig_y = y;
    tmp_win->orig_wd = w;
    tmp_win->orig_ht = horig;
  }
  center_x = x + (w / 2);
  center_y = y + (h / 2);
  if (x >= MyDisplayMaxX (center_x, center_y) - 16)
    x = MyDisplayMaxX (center_x, center_y) - 16;
  if (y >= MyDisplayMaxY (center_x, center_y) - 16)
    y = MyDisplayMaxY (center_x, center_y) - 16;
  if (x <= MyDisplayX (center_x, center_y) + 16 - w)
    x = MyDisplayX (center_x, center_y) + 16 - w;
  if (y <= MyDisplayY (center_x, center_y) + 16 - h)
    y = MyDisplayY (center_x, center_y) + 16 - h;

  if ((w != tmp_win->frame_width) || (h != tmp_win->frame_height))
    Resized = True;
  if ((x != tmp_win->frame_x || y != tmp_win->frame_y))
    Moved = True;

  left = tmp_win->nr_left_buttons;
  right = tmp_win->nr_right_buttons;

  tmp_win->title_width = w - (left + right) * tmp_win->title_height - 2 * tmp_win->boundary_width;

  if (tmp_win->title_width < 1)
    tmp_win->title_width = 1;

  winmask = (CWX | CWY | CWWidth | CWHeight);
  if (Resized)
  {
    if (tmp_win->flags & TITLE)
    {
      tmp_win->title_x = tmp_win->boundary_width + (left) * tmp_win->title_height;
      if (tmp_win->title_x >= w - tmp_win->boundary_width)
	tmp_win->title_x = -10;
      tmp_win->title_y = tmp_win->boundary_width;

      winwc.width = tmp_win->title_width;

      winwc.height = tmp_win->title_height;
      winwc.x = tmp_win->title_x;
      winwc.y = tmp_win->title_y;
      XMoveResizeWindow (dpy, tmp_win->title_w, winwc.x, winwc.y, winwc.width, winwc.height);
      /*
         XConfigureWindow(dpy, tmp_win->title_w, winmask, &winwc);
       */


      winwc.height = tmp_win->title_height;
      winwc.width = tmp_win->title_height;

      winwc.y = tmp_win->boundary_width;
      winwc.x = tmp_win->boundary_width;
      for (i = 0; i < Scr.nr_left_buttons; i++)
      {
	if (tmp_win->left_w[i] != None)
	{
	  if (winwc.x + tmp_win->title_height < w - tmp_win->boundary_width)
	    XMoveResizeWindow (dpy, tmp_win->left_w[i], winwc.x, winwc.y, winwc.width, winwc.height);
	  /*
	     XConfigureWindow(dpy, tmp_win->left_w[i], winmask, &winwc);
	   */
	  else
	  {
	    winwc.x = -tmp_win->title_height;
	    XMoveResizeWindow (dpy, tmp_win->left_w[i], winwc.x, winwc.y, winwc.width, winwc.height);
	    /*
	       XConfigureWindow(dpy, tmp_win->left_w[i], winmask, &winwc);
	     */
	  }
	  winwc.x += tmp_win->title_height;
	}
      }

      winwc.x = w - tmp_win->boundary_width;
      for (i = 0; i < Scr.nr_right_buttons; i++)
      {
	if (tmp_win->right_w[i] != None)
	{
	  winwc.x -= tmp_win->title_height;
	  if (winwc.x > tmp_win->boundary_width)
	    XMoveResizeWindow (dpy, tmp_win->right_w[i], winwc.x, winwc.y, winwc.width, winwc.height);
	  /*
	     XConfigureWindow(dpy, tmp_win->right_w[i], winmask, &winwc);
	   */
	  else
	  {
	    winwc.x = -tmp_win->title_height;
	    XMoveResizeWindow (dpy, tmp_win->right_w[i], winwc.x, winwc.y, winwc.width, winwc.height);
	    /*
	       XConfigureWindow(dpy, tmp_win->right_w[i], winmask, &winwc);
	     */
	  }
	}
      }
    }

    if (tmp_win->flags & BORDER)
    {
      tmp_win->corner_width = GetDecor (tmp_win, TitleHeight) + tmp_win->boundary_width;
#ifdef OLD_STYLE
      if (w < 2 * tmp_win->corner_width)
	tmp_win->corner_width = w / 3;
      if ((h < 2 * tmp_win->corner_width))
	tmp_win->corner_width = h / 3;
#else
      if (!(tmp_win->flags & TITLE))
      {
	if (w < 2 * tmp_win->corner_width)
	  tmp_win->corner_width = w / 3;
	if ((h < 2 * tmp_win->corner_width))
	  tmp_win->corner_width = h / 3;
      }
#endif
      xwidth = w - 2 * tmp_win->corner_width;
      ywidth = h - 2 * tmp_win->corner_width;
      if (xwidth < 2)
	xwidth = 2;
      if (ywidth < 2)
	ywidth = 2;

      for (i = 0; i < 4; i++)
      {
	if (i == 0)
	{
	  winwc.x = tmp_win->corner_width;
	  winwc.y = 0;
	  winwc.height = tmp_win->boundary_width;
	  winwc.width = xwidth;
	}
	else if (i == 1)
	{
	  winwc.x = w - tmp_win->boundary_width;
	  winwc.y = tmp_win->corner_width;
	  winwc.width = tmp_win->boundary_width;
	  winwc.height = ywidth;

	}
	else if (i == 2)
	{
	  winwc.x = tmp_win->corner_width;
	  winwc.y = h - tmp_win->boundary_width;
	  winwc.height = tmp_win->boundary_width;
	  winwc.width = xwidth;
	}
	else
	{
	  winwc.x = 0;
	  winwc.y = tmp_win->corner_width;
	  winwc.width = tmp_win->boundary_width;
	  winwc.height = ywidth;
	}
	XMoveResizeWindow (dpy, tmp_win->sides[i], winwc.x, winwc.y, winwc.width, winwc.height);
	/*
	   XConfigureWindow(dpy, tmp_win->sides[i], winmask, &winwc);
	 */
      }

      winwc.width = tmp_win->corner_width;
      winwc.height = tmp_win->corner_width;
      for (i = 0; i < 4; i++)
      {
	if (i % 2)
	  winwc.x = w - tmp_win->corner_width;
	else
	  winwc.x = 0;

	if (i / 2)
	  winwc.y = h - tmp_win->corner_width;
	else
	  winwc.y = 0;

	XMoveResizeWindow (dpy, tmp_win->corners[i], winwc.x, winwc.y, winwc.width, winwc.height);
	/*
	   XConfigureWindow(dpy, tmp_win->corners[i], winmask, &winwc);
	 */
      }
    }
  }

  tmp_win->attr.width = w - 2 * (tmp_win->boundary_width + tmp_win->bw);
  tmp_win->attr.height = horig - tmp_win->title_height - 2 * (tmp_win->boundary_width + tmp_win->bw);
  /* may need to omit the -1 for shaped windows, next two lines */
  cx = tmp_win->boundary_width;
  cy = tmp_win->title_height + tmp_win->boundary_width;

  XResizeWindow (dpy, tmp_win->w, tmp_win->attr.width, tmp_win->attr.height);
  if (tmp_win->flags & SHADED)
    XMoveResizeWindow (dpy, tmp_win->Parent, cx, cy - tmp_win->attr.height - tmp_win->title_height - tmp_win->boundary_width - 2 * tmp_win->bw, tmp_win->attr.width, tmp_win->attr.height);
  else
    XMoveResizeWindow (dpy, tmp_win->Parent, cx, cy, tmp_win->attr.width, tmp_win->attr.height);
  /*
   * fix up frame and assign size/location values in tmp_win
   */
  winwc.x = tmp_win->frame_x = tmp_win->shade_x = x;
  winwc.y = tmp_win->frame_y = tmp_win->shade_y = y;
  winwc.width = tmp_win->frame_width = tmp_win->shade_width = w;
  tmp_win->shade_height = tmp_win->title_height + 2 * tmp_win->boundary_width;
  tmp_win->frame_height = horig;
  winwc.height = h;
  /*
     XConfigureWindow (dpy, tmp_win->frame, winmask, &winwc);
   */
  XMoveResizeWindow (dpy, tmp_win->frame, winwc.x, winwc.y, winwc.width, winwc.height);

  if (ShapesSupported)
  {
    if ((Resized) && (tmp_win->wShaped))
      SetShape (tmp_win, w);
  }
  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if (sendEvent || (Moved && !Resized))
  {
    sendclient_event (tmp_win, x, y, w, horig);
  }
  if (broadcast)
    BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, tmp_win);
  XSync (dpy, 0);
}

/****************************************************************************
 *
 * Sets up the shaped window borders 
 * 
 ****************************************************************************/
void
SetShape (XfwmWindow * tmp_win, int w)
{
  if ((ShapesSupported) && (tmp_win->wShaped))
  {
    XRectangle rect;

    XShapeCombineShape (dpy, tmp_win->frame, ShapeBounding, tmp_win->boundary_width + tmp_win->bw, tmp_win->title_height + tmp_win->boundary_width + tmp_win->bw, tmp_win->w, ShapeBounding, ShapeSet);
    if (tmp_win->title_w)
    {
      /* windows w/ titles */
      rect.x = tmp_win->boundary_width;
      rect.y = tmp_win->title_y;
      rect.width = w - 2 * (tmp_win->boundary_width);
      rect.height = tmp_win->title_height;


      XShapeCombineRectangles (dpy, tmp_win->frame, ShapeBounding, 0, 0, &rect, 1, ShapeUnion, Unsorted);
#ifdef REQUIRES_XSYNC
      XSync (dpy, 0);
#endif
    }
  }
}
