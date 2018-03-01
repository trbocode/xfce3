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
 * SnapXY, snapxy
 * Copyright 1999 Khadiyd Idris <khadx@francemel.com>
 *
 */

/****************************************************************************
 * This module is from original code
 * by Rob Nation 
 *
 * This code does smart-placement initial window placement stuff
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 ****************************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "xinerama.h"

#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#  include <X11/extensions/Xinerama.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define CHANGE_HORZ 1
#define CHANGE_VERT 2
#define INTERSECT(t1,t2,t3,t4) (((t1 >= t3) && (t1 <= t4)) || ((t2 >= t3) && (t2 <= t4)) || \
                                ((t3 >= t1) && (t3 <= t2)) || ((t4 >= t1) && (t4 <= t2)))
#define BORNE(t1) ((t1 < Scr.SnapSize) && (t1 > -Scr.SnapSize))
#define INSIDE(x1,y1,x2,y2,x3,y3) ((x1 >= x2) && (x1 <= x3) && (y1 >= y2) && (y1 <= y3))

void
GetGravityOffsets (XfwmWindow * tmp_win)
{
  int g = ((tmp_win->hints.flags & PWinGravity) ? tmp_win->hints.win_gravity : NorthWestGravity);

  switch (g)
  {
    case NorthWestGravity:
      tmp_win->gravx = tmp_win->bw - tmp_win->old_bw;
      tmp_win->gravy = tmp_win->bw - tmp_win->old_bw;
      tmp_win->grav_align_y = 0;
      break;
    case NorthGravity:
      tmp_win->gravx = - (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = tmp_win->bw - tmp_win->old_bw;
      tmp_win->grav_align_y = 0;
      break;
    case NorthEastGravity:
      tmp_win->gravx = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = tmp_win->bw - tmp_win->old_bw;
      tmp_win->grav_align_y = 0;
      break;
    case WestGravity:
      tmp_win->gravx = tmp_win->bw - tmp_win->old_bw;
      tmp_win->gravy = (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case EastGravity:
      tmp_win->gravx = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case SouthWestGravity:
      tmp_win->gravx = tmp_win->bw - tmp_win->old_bw;
      tmp_win->gravy = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case SouthGravity:
      tmp_win->gravx = - (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case SouthEastGravity:
      tmp_win->gravx = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = - 2 * (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case ForgetGravity:
      break;
    case CenterGravity:
      tmp_win->gravx = - (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = + (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw) - tmp_win->title_height;
      tmp_win->grav_align_y = -1;
      break;
    case StaticGravity:
    default:
      tmp_win->gravx = - (tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->gravy = - (tmp_win->title_height + tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw);
      tmp_win->grav_align_y = -1;
      break;
  }
}

/* Compute rectangle overlap area */
unsigned long
overlap (int x0, int y0, int x1, int y1, int tx0, int ty0, int tx1, int ty1)
{
  /* Compute overlapping box */
  if (tx0 > x0)
    x0 = tx0;
  if (ty0 > y0)
    y0 = ty0;
  if (tx1 < x1)
    x1 = tx1;
  if (ty1 < y1)
    y1 = ty1;
  if (x1 <= x0 || y1 <= y0)
    return 0;
  return (x1 - x0) * (y1 - y0);
}

/* A true best place computation based on overlapping surfaces */
void
bestplace (XfwmWindow * tmp_win, int px, int py)
{
  int test_x = 0, test_y = 0;
  XfwmWindow *t;
  int xmax, ymax;
  int best_x, best_y;
  int bw, bh;
  unsigned long best_overlaps = 0;
  Bool first = True;

  bw = tmp_win->attr.width + 2 * (tmp_win->boundary_width + tmp_win->bw);
  bh = tmp_win->attr.height + 2 * (tmp_win->boundary_width + tmp_win->bw) + tmp_win->title_height;
  xmax = MyDisplayMaxX (px, py) - bw - Scr.Margin[3];
  ymax = MyDisplayMaxY (px, py) - bh - Scr.Margin[2];
  best_x = MyDisplayX (px, py) + Scr.Margin[1];
  best_y = MyDisplayY (px, py) + Scr.Margin[0];

  for (test_y = MyDisplayY (px, py) + Scr.Margin[0]; test_y < ymax; test_y += 8)
  {
    for (test_x = MyDisplayX (px, py) + Scr.Margin[1]; test_x < xmax; test_x += 8)
    {
      unsigned long count_overlaps = 0;
      t = Scr.XfwmRoot.next;
      while ((t != NULL) && ((count_overlaps < best_overlaps) || first))
      {
	if ((t == tmp_win) || ((t->Desk) != (tmp_win->Desk)) || ((t->layer) != (tmp_win->layer)) || (t->flags & (ICONIFIED)))
	{
	  t = t->next;
	  continue;
	}
	count_overlaps += overlap (test_x, test_y, test_x + bw, test_y + bh, t->frame_x, t->frame_y, t->frame_x + ((t->flags & SHADED) ? t->shade_width : t->frame_width), t->frame_y + ((t->flags & SHADED) ? t->shade_height : t->frame_height));
	t = t->next;
      }
      if (count_overlaps == 0)
      {
	tmp_win->attr.x = test_x;
	tmp_win->attr.y = test_y;
	return;
      }
      else if ((count_overlaps < best_overlaps) || (first))
      {
	best_x = test_x;
	best_y = test_y;
	best_overlaps = count_overlaps;
      }
      if (first)
      {
	first = False;
      }
    }
  }
  tmp_win->attr.x = best_x;
  tmp_win->attr.y = best_y;
  return;
}

/**************************************************************************
 *
 * Handles initial placement and sizing of a new window
 * Returns False in the event of a lost window.
 *
 **************************************************************************/
Bool PlaceWindow (XfwmWindow * tmp_win, unsigned long tflag, int Desk)
{
  XfwmWindow *t;
  extern Bool PPosOverride;
  int xl, yt;

  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;

#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int center_x, center_y;
#endif

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &xl, &yt, &dummy_win_x, &dummy_win_y, &dummy_mask);

  /* Select a desk to put the window on (in list of priority):
   * 1. Sticky Windows stay on the current desk.
   * 2. Windows specified with StartOnDesk go where specified
   * 3. Put it on the desk it was on before the restart.
   * 4. Transients go on the same desk as their parents.
   * 5. Window groups stay together (completely untested)
   */
  tmp_win->Desk = Scr.CurrentDesk;
  if (tflag & STICKY_FLAG)
    tmp_win->Desk = Scr.CurrentDesk;
  else if (tflag & STARTONDESK_FLAG)
    tmp_win->Desk = Desk;
  else
  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;

    if ((tmp_win->wmhints) && (tmp_win->wmhints->flags & WindowGroupHint) && (tmp_win->wmhints->window_group != None) && (tmp_win->wmhints->window_group != Scr.Root))
    {
      /* Try to find the group leader or another window
       * in the group */
      for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
      {
	if (t->w == tmp_win->wmhints->window_group)
	  tmp_win->Desk = t->Desk;
      }
    }
    if ((tmp_win->flags & TRANSIENT) && (tmp_win->transientfor != None) && (tmp_win->transientfor != Scr.Root))
    {
      /* Try to find the parent's desktop */
      for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
      {
	if (t->w == tmp_win->transientfor)
	  tmp_win->Desk = t->Desk;
      }
    }

    if ((XGetWindowProperty (dpy, tmp_win->w, _XA_WM_DESKTOP, 0L, 1L, True, _XA_WM_DESKTOP, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
    {
      if (prop != NULL)
      {
	tmp_win->Desk = *(unsigned long *) prop;
	XFree (prop);
      }
    }
  }
  /* Desk has been selected, now pick a location for the window */
  if (!(tmp_win->flags & TRANSIENT) && !(tmp_win->hints.flags & (USPosition | PPosition)) && !(PPosOverride) && !((tmp_win->wmhints) && (tmp_win->wmhints->flags & StateHint) && (tmp_win->wmhints->initial_state == IconicState)))
  {
    bestplace (tmp_win, xl, yt);
  }
  else
  {
    GetGravityOffsets (tmp_win);
    tmp_win->attr.x += tmp_win->gravx;
    tmp_win->attr.y += tmp_win->gravy;
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
    if ((!(tmp_win->flags & RECAPTURE)) && (xinerama_heads != 0) && (xinerama_infos != NULL) && (enable_xinerama))
    {
      /* Translate coodinates to center on physical screen */
      if ((abs (tmp_win->attr.x - ((Scr.MyDisplayWidth - tmp_win->attr.width) / 2)) < 20) && (abs (tmp_win->attr.y - ((Scr.MyDisplayHeight - tmp_win->attr.height) / 2)) < 20))
      {
	/* We consider that the windows is centered on screen,
	 * Thus, will move it so its center on the current 
	 * physical screen
	 */
	tmp_win->attr.x = MyDisplayX (xl, yt) + (MyDisplayWidth (xl, yt) - tmp_win->attr.width) / 2;
	tmp_win->attr.y = MyDisplayY (xl, yt) + (MyDisplayHeight (xl, yt) - tmp_win->attr.height) / 2;
      }
      /* If TRANSLATE_FLAG is set, the base screen is the one where
       * the mouse resides.
       */
      if (tflag & TRANSLATE_FLAG)
      {
	center_x = xl;
	center_y = yt;
      }
      else
      {
	center_x = tmp_win->attr.x + (tmp_win->attr.width / 2);
	center_y = tmp_win->attr.y + (tmp_win->attr.height / 2);
      }
      /* Translate coodinates if off physical screen */
      if ((tmp_win->attr.x + tmp_win->attr.width + 2 * (tmp_win->boundary_width + tmp_win->bw)) > MyDisplayMaxX (center_x, center_y))
      {
	tmp_win->attr.x = MyDisplayMaxX (center_x, center_y) - tmp_win->attr.width - 2 * (tmp_win->boundary_width + tmp_win->bw);
      }
      if ((tmp_win->attr.y + tmp_win->attr.height + 2 * (tmp_win->boundary_width + tmp_win->bw) + tmp_win->title_height) > MyDisplayMaxY (center_x, center_y))
      {
	tmp_win->attr.y = MyDisplayMaxY (center_x, center_y) - tmp_win->attr.height - tmp_win->title_height - 2 * (tmp_win->boundary_width + tmp_win->bw);
      }
      if (tmp_win->attr.x < MyDisplayX (center_x, center_y))
      {
	tmp_win->attr.x = MyDisplayX (center_x, center_y);
      }
      if (tmp_win->attr.y < MyDisplayY (center_x, center_y))
      {
	tmp_win->attr.y = MyDisplayY (center_x, center_y);
      }
    }
#endif
  }
  tmp_win->xdiff = tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw;
  tmp_win->ydiff = tmp_win->boundary_width + tmp_win->bw - tmp_win->old_bw + tmp_win->title_height;
  /* Just in case the window title is off the screen */
  if (tmp_win->attr.y < 0)
    tmp_win->attr.y = 0;
  if (tmp_win->attr.x < 0)
    tmp_win->attr.x = 0;
  return True;
}

int
snapxy (int *x, int *y, int minx, int miny, int maxx, int maxy)
{
  int left, right, top, bottom;
  int changed = 0;

  if (!INSIDE (*x, *y, minx - Scr.SnapSize, miny - Scr.SnapSize, maxx + Scr.SnapSize, maxy + Scr.SnapSize))
    return (0);

  left = minx - *x;
  if (left < 0)
    left = -left;

  top = miny - *y;
  if (top < 0)
    top = -top;

  right = maxx - *x;
  if (right < 0)
    right = -right;

  bottom = maxy - *y;
  if (bottom < 0)
    bottom = -bottom;

  if (right < Scr.SnapSize)
  {
    *x = maxx;
    changed |= CHANGE_HORZ;
  }
  if (bottom < Scr.SnapSize)
  {
    *y = maxy;
    changed |= CHANGE_VERT;
  }
  if (left < Scr.SnapSize)
  {
    *x = minx;
    changed |= CHANGE_HORZ;
  }
  if (top < Scr.SnapSize)
  {
    *y = miny;
    changed |= CHANGE_VERT;
  }
  return (changed);
}

int
snapxywh (int *x, int *y, int w, int h, int minx, int miny, int maxx, int maxy, int inside)
{
  int changed = 0;

  if (INTERSECT (*y, *y + h, miny, maxy) && BORNE ((*x + w) - (inside ? maxx : minx)))
  {
    *x = (inside ? maxx : minx) - w;
    changed |= CHANGE_HORZ;
  }

  if (INTERSECT (*x, *x + w, minx, maxx) && BORNE ((*y + h) - (inside ? maxy : miny)))
  {
    *y = (inside ? maxy : miny) - h;
    changed |= CHANGE_VERT;
  }

  if (INTERSECT (*x, *x + w, minx, maxx) && BORNE (*y - (inside ? miny : maxy)))
  {
    *y = (inside ? miny : maxy);
    changed |= CHANGE_VERT;
  }

  if (INTERSECT (*y, *y + h, miny, maxy) && BORNE (*x - (inside ? minx : maxx)))
  {
    *x = (inside ? minx : maxx);
    changed |= CHANGE_HORZ;
  }

  return (changed);
}

void
SnapXY (int *x, int *y, XfwmWindow * skip)
{
  XfwmWindow *t;
  int nx, ny, xch, ych, he;
  int changed = 0;

  if (!Scr.SnapSize)
    return;

  xch = 0;
  ych = 0;

  nx = *x;
  ny = *y;

  changed = snapxy (&nx, &ny, MyDisplayX (*x, *y), MyDisplayY (*x, *y), MyDisplayMaxX (*x, *y), MyDisplayMaxY (*x, *y));

  if (changed & CHANGE_HORZ)
  {
    *x = nx;
    xch = 1;
  }

  if (changed & CHANGE_VERT)
  {
    *y = ny;
    ych = 1;
  }

  if (xch && ych)
    return;

  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    if ((t == skip) || (t->Desk != skip->Desk) || (t->flags & ICONIFIED))
      continue;
    nx = *x;
    ny = *y;
    if (t->flags & SHADED)
      he = t->title_height + 2 * t->boundary_width;
    else
      he = t->frame_height;

    changed |= snapxy (&nx, &ny, t->frame_x, t->frame_y, t->frame_x + t->frame_width, t->frame_y + he);

    if ((changed & CHANGE_HORZ) && (!xch))
    {
      *x = nx;
      xch = 1;
    }
    if ((changed & CHANGE_VERT) && (!ych))
    {
      *y = ny;
      ych = 1;
    }
    if (xch && ych)
      return;
  }
}

void
SnapXYWH (int *x, int *y, int w, int h, XfwmWindow * skip)
{
  XfwmWindow *t;
  int nx, ny, xch, ych, he;
  int changed = 0;

  if (!Scr.SnapSize)
    return;

  xch = 0;
  ych = 0;

  nx = *x;
  ny = *y;

  changed = snapxywh (&nx, &ny, w, h, MyDisplayX (*x, *y), MyDisplayY (*x, *y), MyDisplayMaxX (*x, *y), MyDisplayMaxY (*x, *y), 1 /* inside */ );

  if (changed & CHANGE_HORZ)
  {
    *x = nx;
    xch = 1;
  }

  if (changed & CHANGE_VERT)
  {
    *y = ny;
    ych = 1;
  }

  if (xch && ych)
    return;

  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    if ((t == skip) || (t->Desk != skip->Desk) || (t->flags & ICONIFIED))
      continue;
    nx = *x;
    ny = *y;
    if (t->flags & SHADED)
      he = t->title_height + 2 * t->boundary_width;
    else
      he = t->frame_height;

    changed |= snapxywh (&nx, &ny, w, h, t->frame_x, t->frame_y, t->frame_x + t->frame_width, t->frame_y + he, 0 /* inside */ );

    if ((changed & CHANGE_HORZ) && (!xch))
    {
      *x = nx;
      xch = 1;
    }
    if ((changed & CHANGE_VERT) && (!ych))
    {
      *y = ny;
      ych = 1;
    }
    if (xch && ych)
      return;
  }
}

void
SnapMove (int *FinalX, int *FinalY, int Width, int Height, XfwmWindow * skip)
{
  int xw, yh;

  if (!Scr.SnapSize)
    return;

  xw = *FinalX;
  yh = *FinalY;

  SnapXYWH (&xw, &yh, Width, Height, skip);

  if (xw != *FinalX)
    *FinalX = xw;
  if (yh != *FinalY)
    *FinalY = yh;
}
