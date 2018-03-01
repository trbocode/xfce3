/*  gxfce
 *  Copyright (C) 2001 Olivier Fourdan (fourdan@xfce.org)
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


#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include "xfwm.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#  include <X11/extensions/Xinerama.h>
#endif

#include "misc.h"
#include "screen.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


/* Xinerama handling routines */

#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
static int
findhead (int x, int y)
{
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  static int cache_x;
  static int cache_y;
  static int cache_head;
  static Bool cache_init = False;

  int head, closest_head;
  int dx, dy;
  int center_x, center_y;
  int distsquare, min_distsquare;

  /* Cache system */
  if ((cache_init) && (x == cache_x) && (y == cache_y))
    return (cache_head);

  /* Okay, now we consider the cache has been initialized */
  cache_init = True;
  cache_x = x;
  cache_y = y;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
  {
    cache_head = 0;
    return (0);
  }

  for (head = 0; head < xinerama_heads; head++)
  {
    if ((xinerama_infos[head].x_org <= x) && ((xinerama_infos[head].x_org + xinerama_infos[head].width) > x) && (xinerama_infos[head].y_org <= y) && ((xinerama_infos[head].y_org + xinerama_infos[head].height) > y))
    {
      cache_head = head;
      return (head);
    }
  }
  /* No head has been eligible, use the closest one */

  center_x = xinerama_infos[0].x_org + (xinerama_infos[0].width / 2);
  center_y = xinerama_infos[0].y_org + (xinerama_infos[0].height / 2);

  dx = x - center_x;
  dy = y - center_y;

  min_distsquare = (dx * dx) + (dy * dy);
  closest_head = 0;

  for (head = 1; head < xinerama_heads; head++)
  {
    center_x = xinerama_infos[head].x_org + (xinerama_infos[head].width / 2);
    center_y = xinerama_infos[head].y_org + (xinerama_infos[head].height / 2);

    dx = x - center_x;
    dy = y - center_y;

    distsquare = (dx * dx) + (dy * dy);

    if (distsquare < min_distsquare)
    {
      min_distsquare = distsquare;
      closest_head = head;
    }
  }
  cache_head = closest_head;
  return (closest_head);
}
#endif

int
MyDisplayHeight (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (Scr.MyDisplayHeight);

  head = findhead (x, y);
  return (xinerama_infos[head].height);
#else
  return (Scr.MyDisplayHeight);
#endif
}

int
MyDisplayWidth (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (Scr.MyDisplayWidth);

  head = findhead (x, y);
  return (xinerama_infos[head].width);
#else
  return (Scr.MyDisplayWidth);
#endif
}

int
MyDisplayX (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (0);

  head = findhead (x, y);
  return (xinerama_infos[head].x_org);
#else
  return (0);
#endif
}


int
MyDisplayY (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (0);

  head = findhead (x, y);
  return (xinerama_infos[head].y_org);
#else
  return (0);
#endif
}

int
MyDisplayMaxX (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (Scr.MyDisplayWidth);

  head = findhead (x, y);
  return (xinerama_infos[head].x_org + xinerama_infos[head].width);
#else
  return (Scr.MyDisplayWidth);
#endif
}

int
MyDisplayMaxY (int x, int y)
{
#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
  extern XineramaScreenInfo *xinerama_infos;
  extern int xinerama_heads;
  extern Bool enable_xinerama;
  int head;

  if ((xinerama_heads == 0) || (xinerama_infos == NULL) || (!enable_xinerama))
    /* Xinerama extensions are disabled */
    return (Scr.MyDisplayHeight);

  head = findhead (x, y);
  return (xinerama_infos[head].y_org + xinerama_infos[head].height);
#else
  return (Scr.MyDisplayHeight);
#endif
}
