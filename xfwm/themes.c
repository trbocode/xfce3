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
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include "xfwm.h"
#include "misc.h"
#include "menus.h"
#include "parse.h"
#include "screen.h"
#include "themes.h"
#include "xinerama.h"

#include <X11/extensions/shape.h>

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_X11_XFT_XFT_H
extern Bool enable_xft;
#endif

extern Window PressedW;

void ClearArea (Window win, XRectangle *area)
{
  if (area)
  {
    XClearArea(dpy, win, area->x, area->y, area->width, area->height, False);
  }
  else
  {
    XClearWindow(dpy, win);
  }
}

void
RelieveRoundedRectangle (Window win, XRectangle *area, int x, int y, int w, int h, GC Hilite, GC Shadow)
{
  if (w <= 0)
    return;
  if (w > h)
  {
    if (area)
    {
      XSetClipRectangles(dpy, Hilite, 0, 0, area, 1, Unsorted);
      XSetClipRectangles(dpy, Shadow, 0, 0, area, 1, Unsorted);
    }

    XDrawLine (dpy, win, Hilite, x + h / 2, y, w + x - h / 2, y);
    XDrawArc (dpy, win, Hilite, x, y, h, h - 1, 90 * 64, 135 * 64);
    XDrawArc (dpy, win, Shadow, x, y, h, h - 1, 270 * 64, -45 * 64);

    XDrawLine (dpy, win, Shadow, x + h / 2, h + y - 1, w + x - h / 2, h + y - 1);
    XDrawArc (dpy, win, Shadow, w + x - h - 1, y, h, h - 1, 270 * 64, 135 * 64);
    XDrawArc (dpy, win, Hilite, w + x - h - 1, y, h, h - 1, 90 * 64, -45 * 64);

    if (area)
    {
      XSetClipMask(dpy, Hilite, None);
      XSetClipMask(dpy, Shadow, None);
    }
  }
  else
    RelieveRectangle (win, area, x, y, w, h, Hilite, Shadow);
}

void
DrawLinePattern (Window win, XRectangle *area, GC Hilite, GC Shadow, struct vector_coords *coords, int w, int h)
{
  int i = 1;

  if (area)
  {
    XSetClipRectangles(dpy, Hilite, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Shadow, 0, 0, area, 1, Unsorted);
  }

  for (; i < coords->num; ++i)
  {
    XDrawLine (dpy, win, coords->line_style[i] ? Hilite : Shadow, (w * coords->x[i - 1] / 100.0 + .5) - 1, (h * coords->y[i - 1] / 100.0 + .5) - 1, (w * coords->x[i] / 100.0 + .5) - 1, (h * coords->y[i] / 100.0 + .5) - 1);
  }

  if (area)
  {
    XSetClipMask(dpy, Hilite, None);
    XSetClipMask(dpy, Shadow, None);
  }
}

void
RelieveRectangle (Window win, XRectangle *area, int x, int y, int w, int h, GC Hilite, GC Shadow)
{
  if (area)
  {
    XSetClipRectangles(dpy, Hilite, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Shadow, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, Hilite, x, y, w + x - 1, y);
  XDrawLine (dpy, win, Hilite, x, y, x, h + y - 1);

  XDrawLine (dpy, win, Shadow, x + 1, h + y - 1, w + x - 1, h + y - 1);
  XDrawLine (dpy, win, Shadow, w + x - 1, y + 1, w + x - 1, h + y - 1);

  if (area)
  {
    XSetClipMask(dpy, Hilite, None);
    XSetClipMask(dpy, Shadow, None);
  }
}

void
RelieveRectangleGtk (Window win, XRectangle *area, int x, int y, int w, int h, GC Hilite, GC Shadow)
{
  if (area)
  {
    XSetClipRectangles(dpy, Hilite, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Shadow, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, Hilite, x, y, w + x - 1, y);
  XDrawLine (dpy, win, Hilite, x, y, x, h + y - 1);

  XDrawLine (dpy, win, Shadow, x, h + y - 1, w + x - 1, h + y - 1);
  XDrawLine (dpy, win, Shadow, w + x - 1, y, w + x - 1, h + y - 1);

  if (area)
  {
    XSetClipMask(dpy, Hilite, None);
    XSetClipMask(dpy, Shadow, None);
  }
}

void
DrawUnderline (Window w, XRectangle *area, GC gc, int x, int y, char *txt, int posn)
{
  XFontSet fontset = Scr.StdFont.fontset;
  int off1;
  int off2;

  if (fontset)
  {
    XRectangle rect1, rect2;
    XmbTextExtents (fontset, txt, posn, &rect1, &rect2);
    off1 = rect2.width;
    XmbTextExtents (fontset, txt, posn + 1, &rect1, &rect2);
    off2 = rect2.width - 1;
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if ((enable_xft) && (Scr.StdFont.xftfont))
  {
    XGlyphInfo extents;

    XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) txt, posn, &extents);
    off1 = extents.xOff;

    XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) txt, posn + 1, &extents);
    off2 = extents.xOff - 1;
  }
#endif
  else
  {
    off1 = XTextWidth (Scr.StdFont.font, txt, posn);
    off2 = XTextWidth (Scr.StdFont.font, txt, posn + 1) - 1;
  }

  XDrawLine (dpy, w, gc, x + off1, y + 2, x + off2, y + 2);
}

void
DrawSeparator (Window w, XRectangle *area, GC TopGC, GC BottomGC, int x1, int y1, int x2, int y2, int extra_off)
{
  XDrawLine (dpy, w, TopGC, x1, y1, x2, y2);
  XDrawLine (dpy, w, BottomGC, x1 - extra_off, y1 + 1, x2 + extra_off, y2 + 1);
}

void
DrawIconPixmap(XfwmWindow * tmp_win, XRectangle *area, GC gc, int tx, int ty)
{
  if (!area)
  {
    flush_expose (tmp_win->icon_pixmap_w);
  }

  if (tmp_win->iconDepth == 1)
  {
    XCopyPlane(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w, gc, 0, 0, tmp_win->icon_p_width - 2*tx, tmp_win->icon_p_height - 2*ty, tx, ty, 1);
  }
  else
  {
    static GC clipPixmapGC = None;
    XGCValues gcv;
    if (tmp_win->flags & SHAPED_ICON)
    {
      gcv.clip_mask = tmp_win->icon_maskPixmap;
      gcv.clip_x_origin = tx;
      gcv.clip_y_origin = ty;
      if (clipPixmapGC == None) 
      {
          clipPixmapGC = XCreateGC(dpy, tmp_win->icon_pixmap_w, GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
      }
      else
      {
	XChangeGC(dpy, clipPixmapGC, GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
      }
      XCopyArea(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w, clipPixmapGC, 0, 0, tmp_win->icon_p_width - 2*tx, tmp_win->icon_p_height - 2*ty, tx, ty);
      gcv.clip_mask = None;
      XChangeGC(dpy, clipPixmapGC, GCClipMask, &gcv);
    }
    else
    {
      XCopyArea(dpy, tmp_win->iconPixmap, tmp_win->icon_pixmap_w, gc, 0, 0, tmp_win->icon_p_width - 2*tx, tmp_win->icon_p_height - 2*ty, tx, ty);
    }
  }
}

void
RedoIconName (XfwmWindow * Tmp_win, XRectangle *area)
{
  XFontSet fontset = Scr.IconFont.fontset;

  if (Tmp_win->flags & SUPPRESSICON)
    return;

  if (Tmp_win->icon_w == (int) NULL)
    return;

  if (!area)
  {
    flush_expose (Tmp_win->icon_w);
  }

  if (fontset)
  {
    XRectangle rect1, rect2;
    XmbTextExtents (fontset, Tmp_win->icon_name, strlen (Tmp_win->icon_name), &rect1, &rect2);
    Tmp_win->icon_t_width = rect2.width;
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if ((enable_xft) && (Scr.IconFont.xftfont))
  {
    XGlyphInfo extents;

    XftTextExtents8 (dpy, Scr.IconFont.xftfont, (XftChar8 *) Tmp_win->icon_name, strlen (Tmp_win->icon_name), &extents);
    Tmp_win->icon_t_width = extents.xOff;
  }
#endif
  else
  {
    Tmp_win->icon_t_width = XTextWidth (Scr.IconFont.font, Tmp_win->icon_name, strlen (Tmp_win->icon_name));
  }
  /* clear the icon window, and trigger a re-draw via an expose event */
  if (Tmp_win->flags & ICONIFIED)
    XClearArea (dpy, Tmp_win->icon_w, 0, 0, 0, 0, True);
}

void
DrawIconWindow (XfwmWindow * Tmp_win, XRectangle *area)
{
  Pixel TextColor = 0;
  Pixel BackColor = 0;
  GC Shadow = (GC) NULL;
  GC Relief = (GC) NULL;
  int x;

  if (!Tmp_win)
    return;

  if (Scr.Hilite == Tmp_win)
  {
    ButtonFace *bf;
    bf = &GetDecor (t, titlebar.state[Active]);
    if (bf->u.ReliefGC)
    {
      Relief = bf->u.ReliefGC;
    }
    else
    {
      Relief = GetDecor (Tmp_win, HiReliefGC);
    }
    if (bf->u.ShadowGC)
    {
      Shadow = bf->u.ShadowGC;
    }
    else
    {
      Shadow = GetDecor (Tmp_win, HiShadowGC);
    }
    TextColor = GetDecor (Tmp_win, HiColors.fore);
    BackColor = GetDecor (Tmp_win, titlebar.state[Active].u.back);
  }
  else
  {
    ButtonFace *bf;
    bf = &GetDecor (t, titlebar.state[Inactive]);
    if (bf->u.ReliefGC)
    {
      Relief = bf->u.ReliefGC;
    }
    else
    {
      Relief = GetDecor (Tmp_win, LoReliefGC);
    }
    if (bf->u.ShadowGC)
    {
      Shadow = bf->u.ShadowGC;
    }
    else
    {
      Shadow = GetDecor (Tmp_win, LoShadowGC);
    }
    TextColor = GetDecor (Tmp_win, LoColors.fore);
    BackColor = GetDecor (Tmp_win, titlebar.state[Inactive].u.back);
  }
   
  if (Tmp_win->icon_w != None)
  {
    XSetWindowBackground (dpy, Tmp_win->icon_w, BackColor);
  }
  if ((Tmp_win->icon_pixmap_w != None) && (Tmp_win->flags & ICON_OURS) && !(Scr.Options & UseShapedIcons))
  {
    XSetWindowBackground (dpy, Tmp_win->icon_pixmap_w, BackColor);
    ClearArea (Tmp_win->icon_pixmap_w, area);
  }

  if (Scr.Hilite == Tmp_win)
  {
    /* resize the icon name window */
    if (Tmp_win->icon_w != None)
    {
      Tmp_win->icon_w_width = Tmp_win->icon_t_width + 6;
      if (Tmp_win->icon_w_width < Tmp_win->icon_p_width)
	Tmp_win->icon_w_width = Tmp_win->icon_p_width;
      Tmp_win->icon_xl_loc = Tmp_win->icon_x_loc - (Tmp_win->icon_w_width - Tmp_win->icon_p_width) / 2;
      /* start keep label on screen. dje 8/7/97 */
      if (Tmp_win->icon_xl_loc < MyDisplayX (Tmp_win->icon_x_loc + (Tmp_win->icon_p_width / 2), Tmp_win->icon_y_loc + Tmp_win->icon_p_height))
      {				/* if new loc neg (off left edge) move to edge */
	Tmp_win->icon_xl_loc = MyDisplayX (Tmp_win->icon_x_loc + (Tmp_win->icon_p_width / 2), Tmp_win->icon_y_loc + Tmp_win->icon_p_height);
      }
      else
      {				/* if (new loc + width) > screen width (off edge on right) */
	if ((Tmp_win->icon_xl_loc + Tmp_win->icon_w_width) > MyDisplayMaxX (Tmp_win->icon_x_loc + (Tmp_win->icon_p_width / 2), Tmp_win->icon_y_loc + Tmp_win->icon_p_height))
	{			/* position up against right edge */
	  Tmp_win->icon_xl_loc = MyDisplayMaxX (Tmp_win->icon_x_loc + (Tmp_win->icon_p_width / 2), Tmp_win->icon_y_loc + Tmp_win->icon_p_height) - Tmp_win->icon_w_width;
	}
	/* end keep label on screen. dje 8/7/97 */
      }
    }
  }
  else
  {
    if (Tmp_win->icon_w != None)
    {
      XSetWindowBackground (dpy, Tmp_win->icon_w, BackColor);
    }
    if ((Tmp_win->icon_pixmap_w != None) && (Tmp_win->flags & ICON_OURS) && !(Scr.Options & UseShapedIcons))
    {
      XSetWindowBackground (dpy, Tmp_win->icon_pixmap_w, BackColor);
      ClearArea (Tmp_win->icon_pixmap_w, area);
    }
    /* resize the icon name window */
    if (Tmp_win->icon_w != None)
    {
      Tmp_win->icon_w_width = Tmp_win->icon_p_width;
      Tmp_win->icon_xl_loc = Tmp_win->icon_x_loc;
    }
  }

  /* write the icon label */
  if (Scr.IconFont.font)
  {
    NewFontAndColor (Scr.IconFont.font->fid, TextColor, BackColor);
  }
  else
  {
    NewFontAndColor (0, TextColor, BackColor);
  }

  if (Tmp_win->icon_pixmap_w != None)
  {
    XMoveWindow (dpy, Tmp_win->icon_pixmap_w, Tmp_win->icon_x_loc, Tmp_win->icon_y_loc);
  }
  
  if (Tmp_win->icon_w != None)
  {
    Tmp_win->icon_w_height = ICON_HEIGHT;
    XMoveResizeWindow (dpy, Tmp_win->icon_w, Tmp_win->icon_xl_loc, Tmp_win->icon_y_loc + Tmp_win->icon_p_height, Tmp_win->icon_w_width, ICON_HEIGHT);
    ClearArea (Tmp_win->icon_w, area);
  }

  if ((Tmp_win->iconPixmap != None) && !((Tmp_win->flags & SHAPED_ICON) && (Scr.Options & UseShapedIcons)))
  {
    RelieveIconPixmap (Tmp_win->icon_pixmap_w, area, Tmp_win->icon_p_width, Tmp_win->icon_p_height, Relief, Shadow);
  }

  if ((Tmp_win->icon_pixmap_w != None) && (Tmp_win->flags & ICON_OURS))
  {
    DrawIconPixmap (Tmp_win, area, Scr.ScratchGC3, 2, 2);
  }

  if (area)
  {
    XSetClipRectangles(dpy, Relief, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Shadow, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
  }

  if (Tmp_win->icon_w != None)
  {
    XFontSet fontset = Scr.IconFont.fontset;
    /* text position */
    x = (Tmp_win->icon_w_width - Tmp_win->icon_t_width) / 2;
    if (x < 3)
      x = 3;

    if (fontset)
    {
      XmbDrawString (dpy, Tmp_win->icon_w, fontset, Scr.ScratchGC3, x, Tmp_win->icon_w_height - Scr.IconFont.height + Scr.IconFont.y - 3, Tmp_win->icon_name, strlen (Tmp_win->icon_name));
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && Scr.IconFont.xftfont)
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftdraw = XftDrawCreate (dpy, (Drawable) Tmp_win->icon_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      dummyc.pixel = TextColor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = TextColor;

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      XftDrawString8 (xftdraw, &color_fg, Scr.IconFont.xftfont, x, Tmp_win->icon_w_height - Scr.IconFont.height + Scr.IconFont.y - 3, (XftChar8 *) Tmp_win->icon_name, strlen (Tmp_win->icon_name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      XDrawString (dpy, Tmp_win->icon_w, Scr.ScratchGC3, x, Tmp_win->icon_w_height - Scr.IconFont.height + Scr.IconFont.y - 3, Tmp_win->icon_name, strlen (Tmp_win->icon_name));
    }
    RelieveIconTitle (Tmp_win->icon_w, area, Tmp_win->icon_w_width, ICON_HEIGHT, Relief, Shadow);
  }

  if (area)
  {
    XSetClipMask(dpy, Relief, None);
    XSetClipMask(dpy, Shadow, None);
    XSetClipMask(dpy, Scr.ScratchGC3, None);
  }
}

void
PaintEntry (MenuRoot * mr, MenuItem * mi)
{
  int y_offset, text_y, d, y_height;
  GC ShadowGC, ReliefGC, currentGC;
  XFontSet fontset = Scr.StdFont.fontset;
  Bool Selected = False;

  y_offset = mi->y_offset;
  y_height = mi->y_height;
  text_y = y_offset + Scr.StdFont.y;

  ShadowGC = Scr.MenuShadowGC;
  ReliefGC = Scr.MenuReliefGC;
  currentGC = Scr.MenuGC;

  /* active cursor over entry? */
  if ((mi->state) && (mi->func_type != F_TITLE) && (mi->func_type != F_NOP) && *mi->item)
  {

    DrawSelectedEntry (mr->w, NULL, 2, y_offset, mr->width - 4, mi->y_height, &currentGC);
    Selected = True;
  }
  else
  {
    if ((!mi->prev) || (!mi->prev->state))
      XClearArea (dpy, mr->w, 0, y_offset - 1, mr->width, y_height + 2, 0);
    else
      XClearArea (dpy, mr->w, 0, y_offset + 1, mr->width, y_height - 1, 0);
  }

  RelieveHalfRectangle (mr->w, NULL, 0, y_offset - 1, mr->width, y_height + 2, ReliefGC, ShadowGC, (mi == mr->first));

  text_y += HEIGHT_EXTRA >> 1;
  if (mi->func_type == F_TITLE)
  {
    if (mi->next != NULL)
    {
      DrawSeparator (mr->w, NULL, ShadowGC, ReliefGC, 5, y_offset + y_height - 3, mr->width - 6, y_offset + y_height - 3, 1);
    }
    if (mi != mr->first)
    {
      text_y += HEIGHT_EXTRA_TITLE >> 1;
      DrawSeparator (mr->w, NULL, ShadowGC, ReliefGC, 5, y_offset + 1, mr->width - 6, y_offset + 1, 1);
    }
  }

  if (mi->func_type == F_NOP && *mi->item == 0)
  {
    DrawSeparator (mr->w, NULL, ShadowGC, ReliefGC, 2, y_offset - 1 + HEIGHT_SEPARATOR / 2, mr->width - 3, y_offset - 1 + HEIGHT_SEPARATOR / 2, 0);
  }
  if (mi == mr->first)
  {
    DrawTopMenu (mr->w, NULL, mr->width, ReliefGC, ShadowGC);
  }

  if (mi->next == NULL)
    DrawBottomMenu (mr->w, NULL, 2, mr->height - 2, mr->width - 2, mr->height - 2, ReliefGC, ShadowGC);

  if (fontset)
  {
    if (*mi->item)
      XmbDrawString (dpy, mr->w, fontset, currentGC, mi->x, text_y, mi->item, mi->strlen);
    if (mi->strlen2 > 0)
      XmbDrawString (dpy, mr->w, fontset, currentGC, mi->x2, text_y, mi->item2, mi->strlen2);
  }
#ifdef HAVE_X11_XFT_XFT_H
  else if (enable_xft && Scr.StdFont.xftfont)
  {
    XGCValues gcv;
    unsigned long gcm;
    Pixel TextColor = 0;
    XftDraw *xftdraw;
    XftColor color_fg;
    XWindowAttributes attributes;
    XColor dummyc;

    /* 
       This is to be compatible with all type of themes :
       We actually don't know the textcolor value, since
       this function is generic and used with all themes, 
       so get it from the current GC... Which is set by 
       the function DrawSelectedEntry_<theme_name>
     */
    gcm = GCForeground;
    XGetGCValues (dpy, currentGC, gcm, &gcv);
    TextColor = gcv.foreground;

    XGetWindowAttributes (dpy, Scr.Root, &attributes);

    xftdraw = XftDrawCreate (dpy, (Drawable) mr->w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

    dummyc.pixel = TextColor;
    XQueryColor (dpy, attributes.colormap, &dummyc);
    color_fg.color.red = dummyc.red;
    color_fg.color.green = dummyc.green;
    color_fg.color.blue = dummyc.blue;
    color_fg.color.alpha = 0xffff;
    color_fg.pixel = TextColor;


    if (*mi->item)
    {
      XftDrawString8 (xftdraw, &color_fg, Scr.StdFont.xftfont, mi->x, text_y, (XftChar8 *) mi->item, mi->strlen);
    }
    if (mi->strlen2 > 0)
    {
      XftDrawString8 (xftdraw, &color_fg, Scr.StdFont.xftfont, mi->x2, text_y, (XftChar8 *) mi->item2, mi->strlen2);
    }
    XftDrawDestroy (xftdraw);
  }
#endif
  else
  {
    if (*mi->item)
    {
      XDrawString (dpy, mr->w, currentGC, mi->x, text_y, mi->item, mi->strlen);
    }
    if (mi->strlen2 > 0)
    {
      XDrawString (dpy, mr->w, currentGC, mi->x2, text_y, mi->item2, mi->strlen2);
    }
  }

  if (mi->hotkey > 0)
    DrawUnderline (mr->w, NULL, currentGC, mi->x, text_y, mi->item, mi->hotkey - 1);
  if (mi->hotkey < 0)
    DrawUnderline (mr->w, NULL, currentGC, mi->x2, text_y, mi->item2, -1 - mi->hotkey);
  d = (Scr.EntryHeight - 7) / 2;
  if (mi->func_type == F_POPUP)
  {
    if (mi->state)
    {
      DrawTrianglePattern (mr->w, NULL, Scr.MenuSelShadowGC, Scr.MenuSelReliefGC, currentGC, mr->width - d - 8, y_offset + d - 1, mr->width - d - 1, y_offset + d + 7, mi->state);
    }
    else
    {
      DrawTrianglePattern (mr->w, NULL, ReliefGC, ShadowGC, currentGC, mr->width - d - 8, y_offset + d - 1, mr->width - d - 1, y_offset + d + 7, mi->state);
    }
  }
}

/*
 * Entry points for themes routines
 */

void
SetInnerBorder (XfwmWindow * t, Bool onoroff)
{
  if (Scr.engine == XFCE_ENGINE)
    SetInnerBorder_xfce (t, onoroff);
  else if (Scr.engine == TRENCH_ENGINE)
    SetInnerBorder_trench (t, onoroff);
  else if (Scr.engine == GTK_ENGINE)
    SetInnerBorder_gtk (t, onoroff);
  else if (Scr.engine == LINEA_ENGINE)
    SetInnerBorder_linea (t, onoroff);
  else
    SetInnerBorder_mofit (t, onoroff);
}

void
DrawButton (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  if (!area)
  {
    flush_expose (win);
  }

  if (Scr.engine == XFCE_ENGINE)
    DrawButton_xfce (t, win, area, w, h, bf, ReliefGC, ShadowGC, inverted, stateflags);
  else if (Scr.engine == TRENCH_ENGINE)
    DrawButton_trench (t, win, area, w, h, bf, ReliefGC, ShadowGC, inverted, stateflags);
  else if (Scr.engine == GTK_ENGINE)
    DrawButton_gtk (t, win, area, w, h, bf, ReliefGC, ShadowGC, inverted, stateflags);
  else if (Scr.engine == LINEA_ENGINE)
    DrawButton_linea (t, win, area, w, h, bf, ReliefGC, ShadowGC, inverted, stateflags);
  else
    DrawButton_mofit (t, win, area, w, h, bf, ReliefGC, ShadowGC, inverted, stateflags);
}

Bool 
RedrawTitleOnButtonPress (void)
{
  if (Scr.engine == XFCE_ENGINE)
    return RedrawTitleOnButtonPress_xfce ();
  else if (Scr.engine == TRENCH_ENGINE)
    return RedrawTitleOnButtonPress_trench ();
  else if (Scr.engine == GTK_ENGINE)
    return RedrawTitleOnButtonPress_gtk ();
  else if (Scr.engine == LINEA_ENGINE)
    return RedrawTitleOnButtonPress_linea ();
  else
    return RedrawTitleOnButtonPress_mofit ();
}

void
SetTitleBar (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (!area)
  {
    flush_expose (t->title_w);
  }

  if (Scr.engine == XFCE_ENGINE)
    SetTitleBar_xfce (t, area, onoroff);
  else if (Scr.engine == TRENCH_ENGINE)
    SetTitleBar_trench (t, area, onoroff);
  else if (Scr.engine == GTK_ENGINE)
    SetTitleBar_gtk (t, area, onoroff);
  else if (Scr.engine == LINEA_ENGINE)
    SetTitleBar_linea (t, area, onoroff);
  else
    SetTitleBar_mofit (t, area, onoroff);
}

void
RelieveWindow (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  if (!area)
  {
    flush_expose (win);
  }

  if (Scr.engine == XFCE_ENGINE)
    RelieveWindow_xfce (t, win, area, x, y, w, h, ReliefGC, ShadowGC, hilite);
  else if (Scr.engine == TRENCH_ENGINE)
    RelieveWindow_trench (t, win, area, x, y, w, h, ReliefGC, ShadowGC, hilite);
  else if (Scr.engine == GTK_ENGINE)
    RelieveWindow_gtk (t, win, area, x, y, w, h, ReliefGC, ShadowGC, hilite);
  else if (Scr.engine == LINEA_ENGINE)
    RelieveWindow_linea (t, win, area, x, y, w, h, ReliefGC, ShadowGC, hilite);
  else
    RelieveWindow_mofit (t, win, area, x, y, w, h, ReliefGC, ShadowGC, hilite);
}

void
RelieveIconTitle (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (Scr.engine == XFCE_ENGINE)
    RelieveIconTitle_xfce (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == TRENCH_ENGINE)
    RelieveIconTitle_trench (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == GTK_ENGINE)
    RelieveIconTitle_gtk (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == LINEA_ENGINE)
    RelieveIconTitle_linea (win, area, w, h, ReliefGC, ShadowGC);
  else
    RelieveIconTitle_mofit (win, area, w, h, ReliefGC, ShadowGC);
}

void
RelieveIconPixmap (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (Scr.engine == XFCE_ENGINE)
    RelieveIconPixmap_xfce (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == TRENCH_ENGINE)
    RelieveIconPixmap_trench (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == GTK_ENGINE)
    RelieveIconPixmap_gtk (win, area, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == LINEA_ENGINE)
    RelieveIconPixmap_linea (win, area, w, h, ReliefGC, ShadowGC);
  else
    RelieveIconPixmap_mofit (win, area, w, h, ReliefGC, ShadowGC);
}

void
RelieveHalfRectangle (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (Scr.engine == XFCE_ENGINE)
    RelieveHalfRectangle_xfce (win, area, x, y, w, h, ReliefGC, ShadowGC, Top);
  else if (Scr.engine == TRENCH_ENGINE)
    RelieveHalfRectangle_trench (win, area, x, y, w, h, ReliefGC, ShadowGC, Top);
  else if (Scr.engine == GTK_ENGINE)
    RelieveHalfRectangle_gtk (win, area, x, y, w, h, ReliefGC, ShadowGC, Top);
  else if (Scr.engine == LINEA_ENGINE)
    RelieveHalfRectangle_linea (win, area, x, y, w, h, ReliefGC, ShadowGC, Top);
  else
    RelieveHalfRectangle_mofit (win, area, x, y, w, h, ReliefGC, ShadowGC, Top);
}

void
DrawSelectedEntry (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  if (Scr.engine == XFCE_ENGINE)
    DrawSelectedEntry_xfce (win, area, x, y, w, h, currentGC);
  else if (Scr.engine == TRENCH_ENGINE)
    DrawSelectedEntry_trench (win, area, x, y, w, h, currentGC);
  else if (Scr.engine == GTK_ENGINE)
    DrawSelectedEntry_gtk (win, area, x, y, w, h, currentGC);
  else if (Scr.engine == LINEA_ENGINE)
    DrawSelectedEntry_linea (win, area, x, y, w, h, currentGC);
  else
    DrawSelectedEntry_mofit (win, area, x, y, w, h, currentGC);
}

void
DrawTopMenu (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  if (Scr.engine == XFCE_ENGINE)
    DrawTopMenu_xfce (win, area, w, ReliefGC, ShadowGC);
  else if (Scr.engine == TRENCH_ENGINE)
    DrawTopMenu_trench (win, area, w, ReliefGC, ShadowGC);
  else if (Scr.engine == GTK_ENGINE)
    DrawTopMenu_gtk (win, area, w, ReliefGC, ShadowGC);
  else if (Scr.engine == LINEA_ENGINE)
    DrawTopMenu_linea (win, area, w, ReliefGC, ShadowGC);
  else
    DrawTopMenu_mofit (win, area, w, ReliefGC, ShadowGC);
}

void
DrawBottomMenu (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (Scr.engine == XFCE_ENGINE)
    DrawBottomMenu_xfce (win, area, x, y, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == TRENCH_ENGINE)
    DrawBottomMenu_trench (win, area, x, y, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == GTK_ENGINE)
    DrawBottomMenu_gtk (win, area, x, y, w, h, ReliefGC, ShadowGC);
  else if (Scr.engine == LINEA_ENGINE)
    DrawBottomMenu_linea (win, area, x, y, w, h, ReliefGC, ShadowGC);
  else
    DrawBottomMenu_mofit (win, area, x, y, w, h, ReliefGC, ShadowGC);
}

void
DrawTrianglePattern (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  if (Scr.engine == XFCE_ENGINE)
    DrawTrianglePattern_xfce (w, area, ReliefGC, ShadowGC, BackGC, l, t, r, b, state);
  else if (Scr.engine == TRENCH_ENGINE)
    DrawTrianglePattern_trench (w, area, ReliefGC, ShadowGC, BackGC, l, t, r, b, state);
  else if (Scr.engine == GTK_ENGINE)
    DrawTrianglePattern_gtk (w, area, ReliefGC, ShadowGC, BackGC, l, t, r, b, state);
  else if (Scr.engine == LINEA_ENGINE)
    DrawTrianglePattern_linea (w, area, ReliefGC, ShadowGC, BackGC, l, t, r, b, state);
  else
    DrawTrianglePattern_mofit (w, area, ReliefGC, ShadowGC, BackGC, l, t, r, b, state);
}

/*
 * Xfce theme specific routines
 */

void
SetInnerBorder_xfce (XfwmWindow * t, Bool onoroff)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  if (t->bw)
  {
    valuemask = CWBorderPixel;
    if (onoroff)
      attributes.border_pixel = BlackPixel (dpy, Scr.screen);
    else
      attributes.border_pixel = GetDecor (t, LoRelief.back);
    XChangeWindowAttributes (dpy, t->Parent, valuemask, &attributes);
  }
}

void
DrawButton_xfce (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  int type = bf->style & ButtonFaceTypeMask;
  GC BackGC = NULL;

  if (w - t->boundary_width + t->bw <= 0)
    return;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  switch (type)
  {
  case SolidButton:
    {
      XRectangle bounds;
      bounds.x = bounds.y = 0;
      bounds.width = w;
      bounds.height = h;

      XSetForeground (dpy, Scr.TransMaskGC, bf->u.back);

      if (area)
      {
	XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
	XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
	XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
	XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
      }

      if (w > h)
      {
#ifndef OLD_STYLE      
	XFillRectangle (dpy, win, BackGC, 0, 0, h / 2, h - 1);
        XDrawLine (dpy, win, ReliefGC, 0, h - 1, h / 2, h - 1);
#else
	XFillRectangle (dpy, win, BackGC, 0, 0, h / 2, h);
#endif
	XFillArc (dpy, win, Scr.TransMaskGC, 0, 0, h, h - 1, 90 * 64, 180 * 64);
	XFillRectangle (dpy, win, Scr.TransMaskGC, h / 2, 1, w - h, h - 1);
#ifndef OLD_STYLE
	XFillRectangle (dpy, win, BackGC, w - (h / 2), 0, h / 2, h - 1);
        XDrawLine (dpy, win, ReliefGC, w - (h / 2), h - 1, w, h - 1);
#else
	XFillRectangle (dpy, win, BackGC, w - (h / 2), 0, h / 2, h);
#endif
	XFillArc (dpy, win, Scr.TransMaskGC, w - h - 1, 0, h, h - 1, 90 * 64, -180 * 64);
      }
      else
	XFillRectangle (dpy, win, Scr.TransMaskGC, 0, 0, w, h - 1);

      if (area)
      {
	XSetClipMask(dpy, BackGC, None);
	XSetClipMask(dpy, Scr.TransMaskGC, None);
      }
    }
    break;

  case PixButton:
    XFillRectangle (dpy, win, BackGC, 0, 0, w, h);
    if (((stateflags & MWMDecorMaximize) && (t->flags & MAXIMIZED)) || ((stateflags & DecorSticky) && (t->flags & STICKY)) || ((stateflags & DecorShaded) && (t->flags & SHADED)))
    {
      XSetClipOrigin (dpy, BackGC, (w - 16) / 2, (h - 16) / 2);
      XCopyPlane (dpy, bf->bitmap_pressed, win, BackGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
    }
    else
    {
      XSetClipOrigin (dpy, BackGC, (w - 16) / 2, (h - 16) / 2);
      XCopyPlane (dpy, bf->bitmap, win, BackGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
    }

    if (inverted)
    {
#ifdef OLD_STYLE
      RelieveRectangle (win, NULL, 0, 0, w, h, ShadowGC, ReliefGC);
#else
      RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ShadowGC, ReliefGC);
      RelieveRectangle (win, NULL, 2, 2, w - 4, h - 4, Scr.BlackGC, ShadowGC);
    }
    else
    {
      XDrawLine (dpy, win, ShadowGC, 0, h - 2, w - 1, h - 2);
#endif
    }
#ifndef OLD_STYLE
    XDrawLine (dpy, win, Scr.BlackGC, 0, h - 1, w - 1, h - 1);
#endif
    break;

  case GradButton:
    {
      XRectangle bounds;
      bounds.x = bounds.y = 0;
      bounds.width = w;
      bounds.height = h;
      {
	int i = 0, dw = ((w > h) ? bounds.width : 0) / bf->u.grad.npixels + 1;
	XSetForeground (dpy, Scr.TransMaskGC, bf->u.grad.pixels[i]);
	if (area)
	{
	  XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
	  XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
	}

	if (w > h)
	{
#ifndef OLD_STYLE
	  XFillRectangle (dpy, win, BackGC, 0, 0, h / 2, h - 1);
          XDrawLine (dpy, win, ReliefGC, 0, h - 1, h / 2, h - 1);
#else	
	  XFillRectangle (dpy, win, BackGC, 0, 0, h / 2, h);
#endif
	  XFillArc (dpy, win, Scr.TransMaskGC, 0, 0, h, h - 1, 90 * 64, 180 * 64);
	}
	while (i < bf->u.grad.npixels)
	{
	  unsigned short x = ((w > h) ? (bounds.height / 2) : 0) + i * (bounds.width - ((w > h) ? bounds.height : 0)) / bf->u.grad.npixels;
	  XSetForeground (dpy, Scr.TransMaskGC, bf->u.grad.pixels[i++]);
	  XFillRectangle (dpy, win, Scr.TransMaskGC, bounds.x + x, bounds.y + 1, dw, bounds.height - 2);
	}
	if (w > h)
	{
#ifndef OLD_STYLE
	  XFillRectangle (dpy, win, BackGC, w - (h / 2), 0, h / 2, h - 1);
          XDrawLine (dpy, win, ReliefGC, w - (h / 2), h - 1, w, h - 1);
#else
	  XFillRectangle (dpy, win, BackGC, w - (h / 2), 0, h / 2, h);
#endif
	  XFillArc (dpy, win, Scr.TransMaskGC, w - h - 1, 0, h, h - 1, 90 * 64, -180 * 64);
	}

	if (area)
	{
	  XSetClipMask(dpy, ReliefGC, None);
	  XSetClipMask(dpy, ShadowGC, None);
	  XSetClipMask(dpy, BackGC, None);
	  XSetClipMask(dpy, Scr.TransMaskGC, None);
	}
      }
    }
    break;

  default:
    break;
  }
}

Bool 
RedrawTitleOnButtonPress_xfce (void)
{
  return (False);
}

void
SetTitleBar_xfce (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  int hor_off, w;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC;
  GC BackGC = NULL;
  Pixel Forecolor, BackColor;
  ButtonFace *bf;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
    BackGC = GetDecor (t, HiBackGC);
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
    BackGC = GetDecor (t, LoBackGC);
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
      w = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (GetDecor (t, WindowFont.xftfont)))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, GetDecor (t, WindowFont.xftfont), (XftChar8 *) t->name, strlen (t->name), &extents);
      w = extents.xOff;
    }
#endif
    else
    {
      w = XTextWidth (GetDecor (t, WindowFont.font), t->name, strlen (t->name));
    }
    if (w > t->title_width - 12)
      w = t->title_width - 4;
    if (w < 0)
      w = 0;
  }
  else
    w = 0;

  title_state = GetButtonState (t->title_w);
  tb_style = GetDecor (t, titlebar.state[title_state].style);
  tb_flags = GetDecor (t, titlebar.flags);
  hor_off = 10;

  if (GetDecor (t, WindowFont.font))
  {
    NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
  }
  else
  {
    NewFontAndColor (0, Forecolor, BackColor);
  }

#ifndef OLD_STYLE
  bf = &GetDecor (t, titlebar.state[title_state]);
  DrawButton_xfce (t, t->title_w, area, t->title_width, t->title_height - 1, bf, ShadowGC, ReliefGC, True, 0);
  XDrawLine (dpy, t->title_w, Scr.BlackGC, 0, t->title_height - 1, t->title_width, t->title_height - 1);
#else
  bf = &GetDecor (t, titlebar.state[title_state]);
  DrawButton_xfce (t, t->title_w, area, t->title_width, t->title_height, bf, ShadowGC, ReliefGC, True, 0);
#endif

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && GetDecor (t, WindowFont.xftfont))
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XftFont *xftfont;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftfont = GetDecor (t, WindowFont.xftfont);
      xftdraw = XftDrawCreate (dpy, (Drawable) t->title_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      dummyc.pixel = Forecolor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = Forecolor;

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off, GetDecor (t, WindowFont.y) + 1, (XftChar8 *) t->name, strlen (t->name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
  }

#ifndef OLD_STYLE
  RelieveRoundedRectangle (t->title_w, NULL, 0, 0, t->title_width, t->title_height - 1, ShadowGC, ReliefGC);
#else
  RelieveRoundedRectangle (t->title_w, NULL, 0, 0, t->title_width, t->title_height, ShadowGC, ReliefGC);
#endif

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, BackGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
    XSetClipMask(dpy, Scr.ScratchGC3, None);
  }
}

void
RelieveWindow_xfce (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[10];
  GC BackGC = NULL;
#ifndef OLD_STYLE
  GC HiGC, LoGC, BlGC;
#endif
  int i;
  int edge;

  edge = 0;
  if ((win == t->sides[0]) || (win == t->sides[1]) || (win == t->sides[2]) || (win == t->sides[3]))
    edge = -1;
  else if (win == t->corners[0])
    edge = 1;
  else if (win == t->corners[1])
    edge = 2;
  else if (win == t->corners[2])
    edge = 3;
  else if (win == t->corners[3])
    edge = 4;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));
#ifndef OLD_STYLE
  if (t->flags & TITLE)
  {
    HiGC = ReliefGC;
    LoGC = ShadowGC;
    BlGC = Scr.BlackGC;
  }
  else
  {
    HiGC = BackGC;
    LoGC = BackGC;
    BlGC = BackGC;
  }
#endif

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
  }

  /* window sides */
  if (edge == -1)
  {
    switch (hilite)
    {
    case LEFT_HILITE:
#ifndef OLD_STYLE
      XFillRectangle (dpy, win, BackGC, x + 2, y, w - 4, h + y + 1);
#else
      XFillRectangle (dpy, win, BackGC, x + 2, y, w - 2, h + y + 1);
#endif
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);

#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, LoGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, BlGC, seg, i);
#endif
      break;

    case TOP_HILITE:
      XFillRectangle (dpy, win, BackGC, x, y + 2, w + x + 1, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      break;

    case RIGHT_HILITE:
#ifndef OLD_STYLE
      XFillRectangle (dpy, win, BackGC, x + 2, y, w + x - 4, h + y + 1);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, BlGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, HiGC, seg, i);
#else
      XFillRectangle (dpy, win, BackGC, x, y, w + x - 2, h + y + 1);
#endif
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case BOTTOM_HILITE:
#ifndef OLD_STYLE
      XFillRectangle (dpy, win, BackGC, x, y + 2, w + x + 1, h + y - 4);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, BlGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
#else
      XFillRectangle (dpy, win, BackGC, x, y, w + x + 1, h + y - 2);
#endif
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }
  /* corners */
  else if (edge >= 1 && edge <= 4)
  {
    switch (edge)
    {
    case 1:
      XFillRectangle (dpy, win, BackGC, x + 2, y + 2, x + w, y + h);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      XDrawPoint (dpy, win, BlGC, x + t->boundary_width - 1, y + h - 1);
#endif
      break;

    case 2:
      XFillRectangle (dpy, win, BackGC, x, y + 2, x + w - 2, y + h);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x + w - t->boundary_width + 1;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = x + w - t->boundary_width + 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = x + w - t->boundary_width + 1;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      XDrawPoint (dpy, win, BlGC, x + w - t->boundary_width, y + h - 1);
#endif
      break;

    case 3:
      XFillRectangle (dpy, win, BackGC, x + 2, y, x + w, y + h - 2);
#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y + h - t->boundary_width + 1;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, BlGC, seg, i);
#endif
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = h + y - t->boundary_width + 1;
      XDrawSegments (dpy, win, LoGC, seg, i);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y - t->boundary_width;
      XDrawSegments (dpy, win, BlGC, seg, i);
#endif
      break;

    case 4:
      XFillRectangle (dpy, win, BackGC, x, y, x + w - 2, y + h - 2);
#ifndef OLD_STYLE
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width + 1;
      seg[i].x2 = x + w - t->boundary_width + 1;
      seg[i++].y2 = y + h - t->boundary_width + 1;
      seg[i].x1 = x + w - t->boundary_width + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width + 1;
      seg[i++].y2 = y + h - t->boundary_width + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, BlGC, seg, i);
#endif
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
    XSetClipMask(dpy, BackGC, None);
  }
}

void
RelieveHalfRectangle_xfce (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, ShadowGC, x, y, x, h + y);
  XDrawLine (dpy, win, ReliefGC, x + 1, y, x + 1, h + y);

  XDrawLine (dpy, win, ShadowGC, w + x - 2, y + (Top ? 1 : 0), w + x - 2, h + y);
  XDrawLine (dpy, win, Scr.BlackGC, w + x - 1, y - 1, w + x - 1, h + y);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

void
DrawSelectedEntry_xfce (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  Globalgcv.foreground = Scr.MenuSelColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);

  if (area)
  {
    XSetClipRectangles(dpy, Scr.MenuSelReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.MenuSelShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

#ifndef OLD_STYLE
#if 0
  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 2, y + 2, w - 4, h - 4);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuSelShadowGC,  Scr.MenuSelReliefGC);
  RelieveRectangle (win, NULL, x + 1, y + 1, w - 2, h - 2, Scr.BlackGC, Scr.MenuSelShadowGC);
#else
  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 2, y + 2, w - 4, h - 4);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuSelShadowGC,  Scr.BlackGC);
  RelieveRectangle (win, NULL, x + 1, y + 1, w - 2, h - 2, Scr.MenuSelReliefGC, Scr.MenuSelShadowGC);
#endif
#else
  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 1, y + 1, w - 2, h - 2);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuSelShadowGC, Scr.MenuSelReliefGC);
#endif

  if (area)
  {
    XSetClipMask(dpy, Scr.MenuSelShadowGC, None);
    XSetClipMask(dpy, Scr.MenuSelReliefGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }

  *currentGC = Scr.MenuSelGC;
}

void
DrawTopMenu_xfce (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  XDrawLine (dpy, win, ShadowGC, 0, 0, w - 2, 0);
  XDrawLine (dpy, win, ReliefGC, 1, 1, w - 3, 1);
}

void
DrawBottomMenu_xfce (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  DrawSeparator (win, area, ShadowGC, Scr.BlackGC, x, y, w, h, 1);
}

void
DrawTrianglePattern_xfce (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  int m;
  XPoint points[3];

  m = (t + b) >> 1;

  points[0].x = l + 2;
  points[0].y = t;
  points[1].x = l + 2;
  points[1].y = b;
  points[2].x = r;
  points[2].y = m;

  XFillPolygon (dpy, w, BackGC, points, 3, Convex, CoordModeOrigin);
}

void
RelieveIconTitle_xfce (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, Scr.BlackGC, Scr.BlackGC);
  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

void
RelieveIconPixmap_xfce (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, Scr.BlackGC, Scr.BlackGC);
  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

/*
 * Mofit theme specific routines
 */

void
SetInnerBorder_mofit (XfwmWindow * t, Bool onoroff)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  if (t->bw)
  {
    valuemask = CWBorderPixel;
    if (onoroff)
      attributes.border_pixel = GetDecor (t, HiColors.back);
    else
      attributes.border_pixel = GetDecor (t, LoColors.back);
    XChangeWindowAttributes (dpy, t->Parent, valuemask, &attributes);
  }
}

void
DrawButton_mofit (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  int type = bf->style & ButtonFaceTypeMask;
  GC BackGC = NULL;

  if (w - t->boundary_width + t->bw <= 0)
    return;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  if (area)
  {
    XFillRectangle (dpy, win, BackGC, area->x, area->y, area->width, area->height);
  }
  else
  {
    XFillRectangle (dpy, win, BackGC, 0, 0, w, h);
  }
  
  switch (type)
  {
  case PixButton:
    if (((stateflags & MWMDecorMaximize) && (t->flags & MAXIMIZED)) || ((stateflags & DecorSticky) && (t->flags & STICKY)) || ((stateflags & DecorShaded) && (t->flags & SHADED)))
    {
      DrawLinePattern (win, area, ShadowGC, ReliefGC, &bf->vector, w, h);
    }
    else
    {
      DrawLinePattern (win, area, ReliefGC, ShadowGC, &bf->vector, w, h);
    }

    if (inverted)
      RelieveRectangle (win, area, 0, 0, w, h, ShadowGC, ReliefGC);
    else
      RelieveRectangle (win, area, 0, 0, w, h, ReliefGC, ShadowGC);

    break;

  default:
    {
      if (inverted)
	RelieveRectangle (win, area, 0, 0, w, h, ShadowGC, ReliefGC);
      else
	RelieveRectangle (win, area, 0, 0, w, h, ReliefGC, ShadowGC);
    }
  }
}

Bool 
RedrawTitleOnButtonPress_mofit (void)
{
  return (True);
}

void
SetTitleBar_mofit (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  int hor_off, w;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC;
  GC BackGC = NULL;
  Pixel Forecolor, BackColor;
  ButtonFace *bf;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
    BackGC = GetDecor (t, HiBackGC);
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
    BackGC = GetDecor (t, LoBackGC);
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
      w = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (GetDecor (t, WindowFont.xftfont)))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, GetDecor (t, WindowFont.xftfont), (XftChar8 *) t->name, strlen (t->name), &extents);
      w = extents.xOff;
    }
#endif
    else
    {
      w = XTextWidth (GetDecor (t, WindowFont.font), t->name, strlen (t->name));
    }
    if (w > t->title_width - 12)
      w = t->title_width - 4;
    if (w < 0)
      w = 0;
  }
  else
    w = 0;

  title_state = GetButtonState (t->title_w);
  tb_style = GetDecor (t, titlebar.state[title_state].style);
  tb_flags = GetDecor (t, titlebar.flags);
  hor_off = (t->title_width - w) / 2;

  if (GetDecor (t, WindowFont.font))
  {
    NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
  }
  else
  {
    NewFontAndColor (0, Forecolor, BackColor);
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
  }

  bf = &GetDecor (t, titlebar.state[title_state]);
  DrawButton_mofit (t, t->title_w, NULL, t->title_width, t->title_height, bf, ReliefGC, ShadowGC, (t->title_w == PressedW), 0);

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && GetDecor (t, WindowFont.xftfont))
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XftFont *xftfont;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftfont = GetDecor (t, WindowFont.xftfont);
      xftdraw = XftDrawCreate (dpy, (Drawable) t->title_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      dummyc.pixel = Forecolor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = Forecolor;

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off, GetDecor (t, WindowFont.y) + 1, (XftChar8 *) t->name, strlen (t->name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, BackGC, None);
    XSetClipMask(dpy, Scr.ScratchGC3, None);
  }
}

void
RelieveWindow_mofit (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[10];
  GC BackGC = NULL;
  int i;
  int edge;

  edge = 0;
  if ((win == t->sides[0]) || (win == t->sides[1]) || (win == t->sides[2]) || (win == t->sides[3]))
    edge = -1;
  else if (win == t->corners[0])
    edge = 1;
  else if (win == t->corners[1])
    edge = 2;
  else if (win == t->corners[2])
    edge = 3;
  else if (win == t->corners[3])
    edge = 4;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
  }

  /* window sides */
  if (edge == -1)
  {
    switch (hilite)
    {
    case LEFT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 2, y + 1, w - 3, h + y - 1);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);

      i = 0;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case TOP_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 1, y + 2, w + x - 1, y + h - 3);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = x;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case RIGHT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 1, y + 1, w + x - 3, h + y - 1);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case BOTTOM_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 1, y + 1, w + x - 1, h + y - 3);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = x;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x + 1;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }
  /* corners */
  else if (edge >= 1 && edge <= 4)
  {
    switch (edge)
    {
    case 1:
      XFillRectangle (dpy, win, BackGC, x + 2, y + 2, x + w - 2, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y + t->boundary_width - 1;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + t->boundary_width;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + t->boundary_width - 1;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 2:
      XFillRectangle (dpy, win, BackGC, x + 1, y + 2, x + w - 2, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = x;
      seg[i++].y2 = y + t->boundary_width - 1;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y + 1;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - t->boundary_width + 1;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = w + x - t->boundary_width - 1;
      seg[i++].y2 = y + t->boundary_width - 1;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 3:
      XFillRectangle (dpy, win, BackGC, x + 2, y, x + w, y + h - 2);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y + h - 1;
      seg[i].x1 = x + 1;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x + 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y - t->boundary_width - 1;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 4:
      XFillRectangle (dpy, win, BackGC, x, y, x + w - 2, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + 1;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, BackGC, None);
  }
}

void
RelieveHalfRectangle_mofit (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, ReliefGC, x, y, x, h + y);
  XDrawLine (dpy, win, ReliefGC, x + 1, y, x + 1, h + y);

  XDrawLine (dpy, win, ShadowGC, w + x - 2, y + (Top ? 1 : 0), w + x - 2, h + y);
  XDrawLine (dpy, win, ShadowGC, w + x - 1, y - 1, w + x - 1, h + y);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
  }
}

void
DrawSelectedEntry_mofit (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  Globalgcv.foreground = Scr.MenuColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);

  if (area)
  {
    XSetClipRectangles(dpy, Scr.MenuSelReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.MenuSelShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC1, 0, 0, area, 1, Unsorted);
  }

  XFillRectangle (dpy, win, Scr.ScratchGC1, x, y, w, h);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuReliefGC, Scr.MenuShadowGC);
  RelieveRectangle (win, NULL, x + 1, y + 1, w - 2, h - 2, Scr.MenuReliefGC, Scr.MenuShadowGC);

  if (area)
  {
    XSetClipMask(dpy, Scr.MenuSelShadowGC, None);
    XSetClipMask(dpy, Scr.MenuSelReliefGC, None);
    XSetClipMask(dpy, Scr.ScratchGC1, None);
  }

  *currentGC = Scr.MenuGC;
}

void
DrawTopMenu_mofit (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  XDrawLine (dpy, win, ReliefGC, 0, 0, w - 2, 0);
  XDrawLine (dpy, win, ReliefGC, 1, 1, w - 3, 1);
}

void
DrawBottomMenu_mofit (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  DrawSeparator (win, NULL, ShadowGC, ShadowGC, x, y, w, h, 1);
}

void
DrawTrianglePattern_mofit (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  int m, i;
  XSegment seg[10];
  GC MenuReliefGC = NULL;
  GC MenuShadowGC = NULL;

  if (state)
  {
    MenuReliefGC = Scr.MenuReliefGC;
    MenuShadowGC = Scr.MenuShadowGC;
  }
  else
  {
    MenuReliefGC = Scr.MenuShadowGC;
    MenuShadowGC = Scr.MenuReliefGC;
  }
  m = (t + b) >> 1;

  i = 0;
  seg[i].x1 = l;
  seg[i].y1 = t;
  seg[i].x2 = l;
  seg[i++].y2 = b;
  seg[i].x1 = l + 1;
  seg[i].y1 = t + 1;
  seg[i].x2 = l + 1;
  seg[i++].y2 = b - 1;
  seg[i].x1 = l;
  seg[i].y1 = t;
  seg[i].x2 = r;
  seg[i++].y2 = m;
  seg[i].x1 = l + 1;
  seg[i].y1 = t + 1;
  seg[i].x2 = r - 1;
  seg[i++].y2 = m;
  XDrawSegments (dpy, w, MenuShadowGC, seg, i);
  i = 0;
  seg[i].x1 = l;
  seg[i].y1 = b;
  seg[i].x2 = r;
  seg[i++].y2 = m;
  seg[i].x1 = l + 1;
  seg[i].y1 = b - 1;
  seg[i].x2 = r - 1;
  seg[i++].y2 = m;
  XDrawSegments (dpy, w, MenuReliefGC, seg, i);
}

void
RelieveIconTitle_mofit (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, ReliefGC, ShadowGC);
  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
  }
}

void
RelieveIconPixmap_mofit (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, ReliefGC, ShadowGC);
  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
  }
}

/*
 * Trench theme specific routines
 */

void
SetInnerBorder_trench (XfwmWindow * t, Bool onoroff)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  if (t->bw)
  {
    valuemask = CWBorderPixel;
    if (onoroff)
      attributes.border_pixel = BlackPixel (dpy, Scr.screen);
    else
      attributes.border_pixel = GetDecor (t, LoRelief.back);
    XChangeWindowAttributes (dpy, t->Parent, valuemask, &attributes);
  }
}

void
DrawStripes_trench (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, ButtonFace * bf, Bool onoroff)
{
  GC BackGC = NULL;
  GC ReliefGC, ShadowGC;
  int i;
  int rh;

  XSetForeground (dpy, Scr.TransMaskGC, bf->u.back);
  if (onoroff)
  {
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  if (bf->u.ReliefGC)
  {
    ReliefGC = bf->u.ReliefGC;
  }
  if (bf->u.ShadowGC)
  {
    ShadowGC = bf->u.ShadowGC;
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
  }

  rh = ((int) ((h - 2) / 3)) * 3;
  for (i = 0; i < rh; i++)
  {
    if ((i % 3) == 0)
    {
      XDrawPoint (dpy, win, Scr.TransMaskGC, x + w - 1, y + i);
      XDrawLine (dpy, win, ReliefGC, x, y + i, x + w - 2, y + i);
    }
    else if((i % 3) == 1)
    {
      XDrawPoint (dpy, win, Scr.TransMaskGC, x, y + i);
      XDrawLine (dpy, win, ShadowGC, x + 1, y + i, x + w, y + i);
    }
    else
    {
      XDrawLine (dpy, win, Scr.TransMaskGC, x, y + i, x + w, y + i);
    }
  }
  for (i = rh; i < h; i++)
  {
    XDrawLine (dpy, win, Scr.TransMaskGC, x, y + i, x + w, y + i);
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.TransMaskGC, None);
  }
}

void
DrawButton_trench (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  int type = bf->style & ButtonFaceTypeMask;
  enum ButtonState title_state;
  ButtonFace *bftitle;
  Pixel Forecolor;
  GC HiGC = ReliefGC;
  GC LoGC = ShadowGC;
  GC ButtonGC = NULL;
  Bool onoroff;

  if (w - t->boundary_width + t->bw <= 0)
    return;

  onoroff = (Scr.Hilite == t);
  title_state = GetButtonState (t->title_w);
  bftitle = &GetDecor (t, titlebar.state[title_state]);
  if (bftitle->u.ReliefGC)
  {
    HiGC = bftitle->u.ReliefGC;
  }
  if (bftitle->u.ShadowGC)
  {
    LoGC = bftitle->u.ShadowGC;
  }

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    ButtonGC = HiGC;
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    ButtonGC = LoGC;
  }

  XSetForeground (dpy, Scr.TransMaskGC, bftitle->u.back);
  XSetBackground (dpy, Scr.TransMaskGC, Forecolor);

  switch (type)
  {
  case PixButton:
    XFillRectangle (dpy, win, Scr.TransMaskGC, 0, 0, w, h);
    if (((stateflags & MWMDecorMaximize) && (t->flags & MAXIMIZED)) || ((stateflags & DecorSticky) && (t->flags & STICKY)) || ((stateflags & DecorShaded) && (t->flags & SHADED)))
    {
      XSetClipOrigin (dpy, Scr.TransMaskGC, (w - 16) / 2, (h - 16) / 2);
      if (inverted)
      {
        XCopyPlane (dpy, bf->bitmap_pressed, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2, 1);
      }
      else
      {
        XCopyPlane (dpy, bf->bitmap_pressed, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2 - 1, 1);
      }
    }
    else
    {
      XSetClipOrigin (dpy, Scr.TransMaskGC, (w - 16) / 2, (h - 16) / 2);
      if (inverted)
      {
        XCopyPlane (dpy, bf->bitmap, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2, 1);
      }
      else
      {
        XCopyPlane (dpy, bf->bitmap, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2 - 1, 1);
      }
    }
    XDrawLine (dpy, win, LoGC, 0, h - 2, w - 1, h - 2);
    XDrawLine (dpy, win, Scr.BlackGC, 0, h - 1, w - 1, h - 1);
    break;
   
  default:
    {
      DrawStripes_trench (t, win, area, 0, 0, w, h, bf, (Scr.Hilite == t));
    }
  }
}

Bool 
RedrawTitleOnButtonPress_trench (void)
{
  return (False);
}

void
SetTitleBar_trench (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  int hor_off, w;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC;
  GC BackGC = NULL;
  Pixel Forecolor, BackColor;
  ButtonFace *bf;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  title_state = GetButtonState (t->title_w);
  tb_style = GetDecor (t, titlebar.state[title_state].style);
  tb_flags = GetDecor (t, titlebar.flags);
  bf = &GetDecor (t, titlebar.state[title_state]);
  XSetForeground (dpy, Scr.TransMaskGC, bf->u.back);

  if (bf->u.ReliefGC)
  {
    ReliefGC = bf->u.ReliefGC;
  }
  if (bf->u.ShadowGC)
  {
    ShadowGC = bf->u.ShadowGC;
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
      w = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (GetDecor (t, WindowFont.xftfont)))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, GetDecor (t, WindowFont.xftfont), (XftChar8 *) t->name, strlen (t->name), &extents);
      w = extents.xOff;
    }
#endif
    else
    {
      w = XTextWidth (GetDecor (t, WindowFont.font), t->name, strlen (t->name));
    }
    if (w > t->title_width - 12)
      w = t->title_width - 4;
    if (w < 0)
      w = 0;
  }
  else
    w = 0;

  hor_off = (t->title_width - w) / 2;

  if (GetDecor (t, WindowFont.font))
  {
    NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
  }
  else
  {
    NewFontAndColor (0, Forecolor, BackColor);
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
  }

  if (onoroff)
  {
    DrawStripes_trench (t, t->title_w, NULL, 0, 0, hor_off - 5, t->title_height - 1, bf, onoroff);
    XFillRectangle (dpy, t->title_w, Scr.TransMaskGC, hor_off - 5, 0, w + 10, t->title_height - 1);
    DrawStripes_trench (t, t->title_w, NULL, hor_off + w + 5, 0, t->title_width - (hor_off + w + 5), t->title_height - 1, bf, onoroff);
  }
  else
  {
    XFillRectangle (dpy, t->title_w, Scr.TransMaskGC, 0, 0, t->title_width, t->title_height - 1);
  }
  XDrawLine (dpy, t->title_w, ShadowGC, 0, t->title_height - 2, t->title_width - 1, t->title_height - 2);
  XDrawLine (dpy, t->title_w, Scr.BlackGC, 0, t->title_height - 1, t->title_width - 1, t->title_height - 1);

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y), t->name, strlen (t->name));
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && GetDecor (t, WindowFont.xftfont))
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XftFont *xftfont;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftfont = GetDecor (t, WindowFont.xftfont);
      xftdraw = XftDrawCreate (dpy, (Drawable) t->title_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      dummyc.pixel = Forecolor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = Forecolor;

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off, GetDecor (t, WindowFont.y), (XftChar8 *) t->name, strlen (t->name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y), t->name, strlen (t->name));
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
    XSetClipMask(dpy, Scr.ScratchGC3, None);
    XSetClipMask(dpy, Scr.TransMaskGC, None);
  }
}

void
RelieveWindow_trench (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[10];
  GC BackGC = NULL;
  GC HiGC = ReliefGC;
  GC LoGC = ShadowGC;
  GC BackColorGC = NULL;
  enum ButtonState title_state;
  ButtonFace *bftitle;
  Bool onoroff;
  int i;
  int edge;

  edge = 0;
  if ((win == t->sides[0]) || (win == t->sides[1]) || (win == t->sides[2]) || (win == t->sides[3]))
    edge = -1;
  else if (win == t->corners[0])
    edge = 1;
  else if (win == t->corners[1])
    edge = 2;
  else if (win == t->corners[2])
    edge = 3;
  else if (win == t->corners[3])
    edge = 4;

  onoroff = (Scr.Hilite == t);

  BackGC = (onoroff ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  if (t->flags & TITLE)
  {
    title_state = GetButtonState (t->title_w);
    bftitle = &GetDecor (t, titlebar.state[title_state]);
    XSetForeground (dpy, Scr.TransMaskGC, bftitle->u.back);
    BackColorGC = Scr.TransMaskGC;

    if (bftitle->u.ReliefGC)
    {
      HiGC = bftitle->u.ReliefGC;
    }
    if (bftitle->u.ShadowGC)
    {
      LoGC = bftitle->u.ShadowGC;
    }
  }
  else
  {
    HiGC = ReliefGC;
    LoGC = ShadowGC;
    BackColorGC = BackGC;
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    if (HiGC != ReliefGC)
    {
      XSetClipRectangles(dpy, HiGC, 0, 0, area, 1, Unsorted);
    }
    if (LoGC != ShadowGC)
    {
      XSetClipRectangles(dpy, LoGC, 0, 0, area, 1, Unsorted);
    }
    XSetClipRectangles(dpy, LoGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
  }

  /* window sides */
  if (edge == -1)
  {
    switch (hilite)
    {
    case LEFT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 2, y + 2, w - 3, h + y - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x + w;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case TOP_HILITE:
      XFillRectangle (dpy, win, BackColorGC, x, y + 2, w + x + 1, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      break;

    case RIGHT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 2, y + 2, w + x - 3, h + y - 2);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x + w;
      seg[i++].y2 = y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = y + h;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case BOTTOM_HILITE:
      XFillRectangle (dpy, win, BackColorGC, x, y + 2, w + x + 1, h + y - 3);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      break;

    }
  }
  /* corners */
  else if (edge >= 1 && edge <= 4)
  {
    switch (edge)
    {
    case 1:
      XFillRectangle (dpy, win, BackColorGC, x + 2, y + 2, x + w, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = x + t->boundary_width;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x + 2;
      seg[i].y1 = y + h - 2;
      seg[i].x2 = x + t->boundary_width;
      seg[i++].y2 = y + h - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      break;

    case 2:
      XFillRectangle (dpy, win, BackColorGC, x, y + 2, x + w - 2, y + h - 2);
      i = 0;
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y + h - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = y + h - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      break;

    case 3:
      XFillRectangle (dpy, win, BackGC, x + 2, y, x + w, y + h - t->boundary_width - 2);
      XFillRectangle (dpy, win, BackColorGC, x + 2, y + h - t->boundary_width + 2, x + w, y + h - 3);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - t->boundary_width - 2;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = h + y - t->boundary_width + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - t->boundary_width + 1;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - t->boundary_width - 1;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = y + h - t->boundary_width - 1;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y - t->boundary_width - 2;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = h + y - t->boundary_width - 2;
      seg[i].x1 = x + 2;
      seg[i].y1 = y + h - t->boundary_width - 2;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = y + h - t->boundary_width - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 4:
      XFillRectangle (dpy, win, BackGC, x, y, x + w - 2, y + h - t->boundary_width - 2);
      XFillRectangle (dpy, win, BackColorGC, x, y + h - t->boundary_width + 1, x + w - 2, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width + 1;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - t->boundary_width + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - t->boundary_width + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width + 1;
      seg[i++].y2 = y + h - t->boundary_width - 2;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - t->boundary_width - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - t->boundary_width - 1;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width - 2;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - t->boundary_width - 2;
      seg[i].x1 = x + w - t->boundary_width + 2;
      seg[i].y1 = h + y - t->boundary_width - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - t->boundary_width - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = h + y - t->boundary_width + 1;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, LoGC, seg, i);
      break;
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    if (HiGC != ReliefGC)
    {
      XSetClipMask(dpy, HiGC, None);
    }
    if (LoGC != ShadowGC)
    {
      XSetClipMask(dpy, LoGC, None);
    }
    XSetClipMask(dpy, BackGC, None);
    XSetClipMask(dpy, Scr.TransMaskGC, None);
  }
}

void
RelieveHalfRectangle_trench (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, Scr.BlackGC, x, y, x, h + y);
  XDrawLine (dpy, win, ReliefGC, x + 1, y, x + 1, h + y);

  XDrawLine (dpy, win, ShadowGC, w + x - 2, y + (Top ? 1 : 0), w + x - 2, h + y);
  XDrawLine (dpy, win, Scr.BlackGC, w + x - 1, y - 1, w + x - 1, h + y);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}


void
DrawSelectedEntry_trench (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  Globalgcv.foreground = Scr.MenuSelColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
  if (area)
  {
    XSetClipRectangles(dpy, Scr.MenuSelReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.MenuSelShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 1, y + 1, w - 2, h - 2);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuSelReliefGC, Scr.MenuSelShadowGC);

  if (area)
  {
    XSetClipMask(dpy, Scr.MenuSelShadowGC, None);
    XSetClipMask(dpy, Scr.MenuSelReliefGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }

  *currentGC = Scr.MenuSelGC;
}

void
DrawTopMenu_trench (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  XDrawLine (dpy, win, Scr.BlackGC, 0, 0, w - 2, 0);
  XDrawLine (dpy, win, ReliefGC, 1, 1, w - 3, 1);
}

void
DrawBottomMenu_trench (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  DrawSeparator (win, NULL, ShadowGC, Scr.BlackGC, x, y, w, h, 1);
}

void
DrawTrianglePattern_trench (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  int m, i;
  XSegment seg[10];
  GC MenuReliefGC = NULL;
  GC MenuShadowGC = NULL;

  if (state)
  {
    MenuReliefGC = Scr.MenuSelReliefGC;
    MenuShadowGC = Scr.MenuSelShadowGC;
  }
  else
  {
    MenuReliefGC = Scr.MenuReliefGC;
    MenuShadowGC = Scr.MenuShadowGC;
  }
  m = (t + b) >> 1;

  i = 0;
  seg[i].x1 = l;
  seg[i].y1 = t;
  seg[i].x2 = l;
  seg[i++].y2 = b;
  seg[i].x1 = l;
  seg[i].y1 = t;
  seg[i].x2 = r;
  seg[i++].y2 = m;
  seg[i].x1 = l + 1;
  seg[i].y1 = b - 1;
  seg[i].x2 = r - 1;
  seg[i++].y2 = m;
  XDrawSegments (dpy, w, MenuShadowGC, seg, i);
  i = 0;
  seg[i].x1 = l;
  seg[i].y1 = b;
  seg[i].x2 = r;
  seg[i++].y2 = m;
  seg[i].x1 = l + 1;
  seg[i].y1 = t + 1;
  seg[i].x2 = l + 1;
  seg[i++].y2 = b - 1;
  seg[i].x1 = l + 1;
  seg[i].y1 = t + 1;
  seg[i].x2 = r - 1;
  seg[i++].y2 = m;
  XDrawSegments (dpy, w, MenuReliefGC, seg, i);
}

void
RelieveIconTitle_trench (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
  }
}

void
RelieveIconPixmap_trench (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 0, 0, w, h, ReliefGC, ShadowGC);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
  }
}

/*
 * GTK style theme specific routines
 */

void
SetInnerBorder_gtk (XfwmWindow * t, Bool onoroff)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  if (t->bw)
  {
    valuemask = CWBorderPixel;
    if (onoroff)
      attributes.border_pixel = GetDecor (t, HiColors.back);
    else
      attributes.border_pixel = GetDecor (t, LoColors.back);
    XChangeWindowAttributes (dpy, t->Parent, valuemask, &attributes);
  }
}


void
DrawButton_gtk (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  int type = bf->style & ButtonFaceTypeMask;
  GC BackGC = NULL;
  GC HiGC = ReliefGC;
  GC LoGC = ShadowGC;

  if (w - t->boundary_width + t->bw <= 0)
    return;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  switch (type)
  {
  case PixButton:
    {
      XFillRectangle (dpy, win, BackGC, 0, 0, w, h);
      if (((stateflags & MWMDecorMaximize) && (t->flags & MAXIMIZED)) || ((stateflags & DecorSticky) && (t->flags & STICKY)) || ((stateflags & DecorShaded) && (t->flags & SHADED)))
      {
	XSetClipOrigin (dpy, BackGC, (w - 16) / 2, (h - 16) / 2);
        if (inverted)
        {
	  XCopyPlane (dpy, bf->bitmap_pressed, win, BackGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2 + 1, 1);
        }
        else
        {
	  XCopyPlane (dpy, bf->bitmap_pressed, win, BackGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
        }
      }
      else
      {
	XSetClipOrigin (dpy, BackGC, (w - 16) / 2, (h - 16) / 2);
        if (inverted)
        {
	  XCopyPlane (dpy, bf->bitmap, win, BackGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2 + 1, 1);
        }
        else
        {
	  XCopyPlane (dpy, bf->bitmap, win, BackGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
        }
      }
    }
    break;
  case SolidButton:
  case GradButton:
    {
      if (bf->u.ReliefGC)
      {
	HiGC = bf->u.ReliefGC;
      }
      if (bf->u.ShadowGC)
      {
	LoGC = bf->u.ShadowGC;
      }
    }
    break;
  default:
    ;
  }
  if (inverted)
  {
    XDrawLine (dpy, win, LoGC, 0, 0, 0, h - 1);
    XDrawLine (dpy, win, LoGC, 0, 0, w - 1, 0);
    XDrawLine (dpy, win, HiGC, 1, h - 1, w - 1, h - 1);
    XDrawLine (dpy, win, HiGC, w - 1, 1, w - 1, h - 1);
    XDrawLine (dpy, win, Scr.BlackGC, 1, 1, 1, h - 2);
    XDrawLine (dpy, win, Scr.BlackGC, 1, 1, w - 2, 1);
  }
  else
  {
    RelieveRectangleGtk (win, NULL, 0, 0, w, h, HiGC, Scr.BlackGC);
    XDrawLine (dpy, win, LoGC, 2, h - 2, w - 3, h - 2);
    XDrawLine (dpy, win, LoGC, w - 2, 2, w - 2, h - 2);
  }
}

Bool 
RedrawTitleOnButtonPress_gtk (void)
{
  return (True);
}

void
SetTitleBar_gtk (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  int hor_off, w;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC;
  Pixel Forecolor, BackColor;
  ButtonFace *bf;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
      w = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (GetDecor (t, WindowFont.xftfont)))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, GetDecor (t, WindowFont.xftfont), (XftChar8 *) t->name, strlen (t->name), &extents);
      w = extents.xOff;
    }
#endif
    else
    {
      w = XTextWidth (GetDecor (t, WindowFont.font), t->name, strlen (t->name));
    }
    if (w > t->title_width - 12)
      w = t->title_width - 4;
    if (w < 0)
      w = 0;
  }
  else
    w = 0;

  title_state = GetButtonState (t->title_w);
  tb_style = GetDecor (t, titlebar.state[title_state].style);
  tb_flags = GetDecor (t, titlebar.flags);
  hor_off = (t->title_width - w) / 2;

  if (GetDecor (t, WindowFont.font))
  {
    NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
  }
  else
  {
    NewFontAndColor (0, Forecolor, BackColor);
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
  }

  bf = &GetDecor (t, titlebar.state[title_state]);
  XSetForeground (dpy, Scr.TransMaskGC, bf->u.back);
  XFillRectangle (dpy, t->title_w, Scr.TransMaskGC, 0, 0, t->title_width, t->title_height);
  DrawButton_gtk (t, t->title_w, NULL, t->title_width, t->title_height, bf, ReliefGC, ShadowGC, (t->title_w == PressedW), 0);

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && GetDecor (t, WindowFont.xftfont))
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XftFont *xftfont;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftfont = GetDecor (t, WindowFont.xftfont);
      xftdraw = XftDrawCreate (dpy, (Drawable) t->title_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      dummyc.pixel = Forecolor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = Forecolor;

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off, GetDecor (t, WindowFont.y) + 1, (XftChar8 *) t->name, strlen (t->name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.ScratchGC3, None);
    XSetClipMask(dpy, Scr.TransMaskGC, None);
  }
}

void
RelieveWindow_gtk (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[10];
  GC BackGC = NULL;
  int i;
  int edge;

  edge = 0;
  if ((win == t->sides[0]) || (win == t->sides[1]) || (win == t->sides[2]) || (win == t->sides[3]))
    edge = -1;
  else if (win == t->corners[0])
    edge = 1;
  else if (win == t->corners[1])
    edge = 2;
  else if (win == t->corners[2])
    edge = 3;
  else if (win == t->corners[3])
    edge = 4;

  BackGC = ((Scr.Hilite == t) ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
  }

  /* window sides */
  if (edge == -1)
  {
    switch (hilite)
    {
    case LEFT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 1, y, w - 2, h + y + 1);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 2;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = x + w - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + w - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      break;

    case TOP_HILITE:
      XFillRectangle (dpy, win, BackGC, x, y + 1, w + x + 1, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + h - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + h - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      break;

    case RIGHT_HILITE:
      XFillRectangle (dpy, win, BackGC, x + 1, y, w + x - 3, h + y + 1);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case BOTTOM_HILITE:
      XFillRectangle (dpy, win, BackGC, x, y + 1, w + x + 1, h + y - 3);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }
  /* corners */
  else if (edge >= 1 && edge <= 4)
  {
    switch (edge)
    {
    case 1:
      XFillRectangle (dpy, win, BackGC, x + 1, y + 1, x + w, y + h);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y + t->boundary_width - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + t->boundary_width - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + t->boundary_width;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + t->boundary_width - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      break;

    case 2:
      XFillRectangle (dpy, win, BackGC, x, y + 1, x + w - 2, y + h);
      XDrawPoint (dpy, win, BackGC, x + w - 2, y + 1);
      i = 0;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y + t->boundary_width - 2;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      seg[i].x1 = x;
      seg[i].y1 = y + t->boundary_width - 2;
      seg[i].x2 = w + x - t->boundary_width - 1;
      seg[i++].y2 = y + t->boundary_width - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + t->boundary_width - 1;
      seg[i].x2 = w + x - t->boundary_width - 1;
      seg[i++].y2 = y + t->boundary_width - 1;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      break;

    case 3:
      XFillRectangle (dpy, win, BackGC, x + 1, y, x + w, y + h - 2);
      XDrawPoint (dpy, win, BackGC, x + 1, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x + t->boundary_width - 1;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 1;
      seg[i++].y2 = h + y - t->boundary_width - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x + 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + t->boundary_width - 2;
      seg[i].y1 = y;
      seg[i].x2 = x + t->boundary_width - 2;
      seg[i++].y2 = h + y - t->boundary_width - 1;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 4:
      XFillRectangle (dpy, win, BackGC, x, y, x + w - 1, y + h - 1);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + h - t->boundary_width;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      seg[i].x1 = x + w - t->boundary_width;
      seg[i].y1 = y;
      seg[i].x2 = x + w - t->boundary_width;
      seg[i++].y2 = y + h - t->boundary_width;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, BackGC, None);
  }
}

void
RelieveHalfRectangle_gtk (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, ReliefGC, x, y, x, h + y);
  XDrawLine (dpy, win, ShadowGC, w + x - 2, y + (Top ? 1 : 0), w + x - 2, h + y);
  XDrawLine (dpy, win, Scr.BlackGC, w + x - 1, y - 1, w + x - 1, h + y);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }
}

void
DrawSelectedEntry_gtk (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  Globalgcv.foreground = Scr.MenuSelColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);

  if (area)
  {
    XSetClipRectangles(dpy, Scr.MenuSelReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.MenuSelShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.ScratchGC1, 0, 0, area, 1, Unsorted);
  }

  XFillRectangle (dpy, win, Scr.ScratchGC1, x, y, w, h);
  RelieveRectangleGtk (win, NULL, x, y, w, h, Scr.MenuSelReliefGC, Scr.BlackGC);
  XDrawLine (dpy, win, Scr.MenuSelShadowGC, x + 2, y + h - 2, x + w - 3, y + h - 2);
  XDrawLine (dpy, win, Scr.MenuSelShadowGC, x + w - 2, y + 2, x + w - 2, y + h - 2);

  if (area)
  {
    XSetClipMask(dpy, Scr.MenuSelShadowGC, None);
    XSetClipMask(dpy, Scr.MenuSelReliefGC, None);
    XSetClipMask(dpy, Scr.ScratchGC1, None);
  }

  *currentGC = Scr.MenuSelGC;
}

void
DrawTopMenu_gtk (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  XDrawLine (dpy, win, ReliefGC, 0, 0, w - 2, 0);
}

void
DrawBottomMenu_gtk (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  DrawSeparator (win, NULL, ShadowGC, Scr.BlackGC, x, y, w, h, 2);
}

void
DrawTrianglePattern_gtk (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  int m;
  XPoint points[5];

  m = (t + b) >> 1;

  points[0].x = l + 2;
  points[0].y = t;
  points[1].x = l + 2;
  points[1].y = b;
  points[2].x = r;
  points[2].y = m;

  XFillPolygon (dpy, w, BackGC, points, 3, Convex, CoordModeOrigin);
}

void
RelieveIconTitle_gtk (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangleGtk (win, NULL, 0, 0, w, h, ReliefGC, Scr.BlackGC);
  XDrawLine (dpy, win, ShadowGC, 2, h - 2, w - 3, h - 2);
  XDrawLine (dpy, win, ShadowGC, w - 2, 2, w - 2, h - 2);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

void
RelieveIconPixmap_gtk (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangleGtk (win, NULL, 0, 0, w, h, ReliefGC, Scr.BlackGC);
  XDrawLine (dpy, win, ShadowGC, 2, h - 2, w - 3, h - 2);
  XDrawLine (dpy, win, ShadowGC, w - 2, 2, w - 2, h - 2);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

/*
 * Linea theme specific routines
 */

void
SetInnerBorder_linea (XfwmWindow * t, Bool onoroff)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  if (t->bw)
  {
    valuemask = CWBorderPixel;
    if (onoroff)
      attributes.border_pixel = GetDecor (t, HiColors.back);
    else
      attributes.border_pixel = GetDecor (t, LoColors.back);
    XChangeWindowAttributes (dpy, t->Parent, valuemask, &attributes);
  }
}

void
DrawButton_linea (XfwmWindow * t, Window win, XRectangle *area, int w, int h, ButtonFace * bf, GC ReliefGC, GC ShadowGC, Bool inverted, int stateflags)
{
  int type = bf->style & ButtonFaceTypeMask;
  Pixel Forecolor;
  GC HiGC = ReliefGC;
  GC LoGC = ShadowGC;
  GC ButtonGC = NULL;
  enum ButtonState title_state;
  ButtonFace *bftitle;
  Bool onoroff;

  onoroff = (Scr.Hilite == t);
  if (w - t->boundary_width + t->bw <= 0)
    return;

  title_state = GetButtonState (t->title_w);
  bftitle = &GetDecor (t, titlebar.state[title_state]);
  if (bftitle->u.ReliefGC)
  {
    HiGC = bftitle->u.ReliefGC;
  }
  if (bftitle->u.ShadowGC)
  {
    LoGC = bftitle->u.ShadowGC;
  }

  if (onoroff)
  {
    Forecolor = GetDecor (t, HiColors.fore);
    ButtonGC = HiGC;
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    ButtonGC = LoGC;
  }

  XSetForeground (dpy, Scr.TransMaskGC, bftitle->u.back);
  XSetBackground (dpy, Scr.TransMaskGC, Forecolor);

  if (area)
  {
    XFillRectangle (dpy, win, Scr.TransMaskGC, area->x, area->y, area->width, area->height);
  }
  else
  {
    XFillRectangle (dpy, win, Scr.TransMaskGC, 0, 0, w, h);
  }
  
  switch (type)
  {
  case SolidButton:
  case GradButton:
    /*
    XDrawLine (dpy, win, LoGC, 0, h - 2, w, h - 2);
    XDrawLine (dpy, win, HiGC, 0, h - 1, w, h - 1);
     */
    break;
  case PixButton:
    if (((stateflags & MWMDecorMaximize) && (t->flags & MAXIMIZED)) || ((stateflags & DecorSticky) && (t->flags & STICKY)) || ((stateflags & DecorShaded) && (t->flags & SHADED)))
    {
      XSetClipOrigin (dpy, Scr.TransMaskGC, (w - 16) / 2, (h - 16) / 2);
      if (inverted)
      {
        XCopyPlane (dpy, bf->bitmap_pressed, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2 + 1, 1);
      }
      else
      {
        XCopyPlane (dpy, bf->bitmap_pressed, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
      }
    }
    else
    {
      XSetClipOrigin (dpy, Scr.TransMaskGC, (w - 16) / 2, (h - 16) / 2);
      if (inverted)
      {
        XCopyPlane (dpy, bf->bitmap, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2 + 1, (h - 16) / 2 + 1, 1);
      }
      else
      {
        XCopyPlane (dpy, bf->bitmap, win, Scr.TransMaskGC, 0, 0, 15, 15, (w - 16) / 2, (h - 16) / 2, 1);
      }
    }

    if (inverted)
    {
      XDrawLine (dpy, win, LoGC, 2, 0, w - 4, 0);
      XDrawLine (dpy, win, LoGC, 0, 2, 0, h - 4);
      XDrawLine (dpy, win, LoGC, 1, 1, w - 3, 1);
      XDrawLine (dpy, win, LoGC, 1, 1, 1, h - 3);
      XDrawLine (dpy, win, LoGC, 2, 2, 3, 2);
      XDrawLine (dpy, win, LoGC, 2, 2, 2, 3);
      XDrawLine (dpy, win, HiGC, 2, h - 1, w - 3, h - 1);
      XDrawLine (dpy, win, HiGC, w - 1, 2, w - 1, h - 3);
      XDrawLine (dpy, win, HiGC, w - 4, h - 2, w - 2, h - 2);
      XDrawLine (dpy, win, HiGC, w - 2, h - 4, w - 2, h - 2);
    }
    else
    {
      XDrawLine (dpy, win, ButtonGC, 2, 1, w - 3, 1);
      XDrawLine (dpy, win, ButtonGC, 1, 2, 1, h - 3);
      XDrawLine (dpy, win, ButtonGC, 2, h - 2, w - 3, h - 2);
      XDrawLine (dpy, win, ButtonGC, w - 2, 2, w - 2, h - 3);
    }
    break;

  default:
    break;
  }
}

Bool 
RedrawTitleOnButtonPress_linea (void)
{
  return (False);
}

void
SetTitleBar_linea (XfwmWindow * t, XRectangle *area, Bool onoroff)
{
  int hor_off, w;
  enum ButtonState title_state;
  ButtonFaceStyle tb_style;
  int tb_flags;
  GC ReliefGC, ShadowGC;
  Pixel Forecolor, BackColor;
  ButtonFace *bf;

  if (!t)
    return;
  if (!(t->flags & TITLE))
    return;

  if (onoroff)
  {
    Forecolor = WhitePixel (dpy, Scr.screen);
    BackColor = GetDecor (t, HiColors.back);
    ReliefGC = GetDecor (t, HiReliefGC);
    ShadowGC = GetDecor (t, HiShadowGC);
  }
  else
  {
    Forecolor = GetDecor (t, LoColors.fore);
    BackColor = GetDecor (t, LoColors.back);
    ReliefGC = GetDecor (t, LoReliefGC);
    ShadowGC = GetDecor (t, LoShadowGC);
  }

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, t->name, strlen (t->name), &rect1, &rect2);
      w = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (GetDecor (t, WindowFont.xftfont)))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, GetDecor (t, WindowFont.xftfont), (XftChar8 *) t->name, strlen (t->name), &extents);
      w = extents.xOff;
    }
#endif
    else
    {
      w = XTextWidth (GetDecor (t, WindowFont.font), t->name, strlen (t->name));
    }
    if (w > t->title_width - 12)
      w = t->title_width - 4;
    if (w < 0)
      w = 0;
  }
  else
    w = 0;

  title_state = GetButtonState (t->title_w);
  tb_style = GetDecor (t, titlebar.state[title_state].style);
  tb_flags = GetDecor (t, titlebar.flags);
  hor_off = 5;

  bf = &GetDecor (t, titlebar.state[title_state]);
  DrawButton_linea (t, t->title_w, area, t->title_width, t->title_height, bf, ReliefGC, ShadowGC, (t->title_w == PressedW), 0);

  if (t->name != (char *) NULL)
  {
    XFontSet fontset = GetDecor (t, WindowFont.fontset);
    if (fontset)
    {
      if (onoroff)
      {
        if (GetDecor (t, WindowFont.font))
        {
          NewFontAndColor (GetDecor (t, WindowFont.font->fid), BlackPixel (dpy, Scr.screen), BackColor);
        }
        else
        {
          NewFontAndColor (0, BlackPixel (dpy, Scr.screen), BackColor);
        }

	if (area)
	{
	  XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
	}

        XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off + 1, GetDecor (t, WindowFont.y) + 2, t->name, strlen (t->name));

	if (area)
	{
	  XSetClipMask(dpy, Scr.ScratchGC3, None);
	}
      }
      if (GetDecor (t, WindowFont.font))
      {
        NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
      }
      else
      {
        NewFontAndColor (0, Forecolor, BackColor);
      }

      if (area)
      {
	XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
      }

      XmbDrawString (dpy, t->title_w, fontset, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));

      if (area)
      {
	XSetClipMask(dpy, Scr.ScratchGC3, None);
      }
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (enable_xft && GetDecor (t, WindowFont.xftfont))
    {
      XftDraw *xftdraw;
      XftColor color_fg;
      XftFont *xftfont;
      XWindowAttributes attributes;
      XColor dummyc;
      Region region = None;

      XGetWindowAttributes (dpy, Scr.Root, &attributes);

      xftfont = GetDecor (t, WindowFont.xftfont);
      xftdraw = XftDrawCreate (dpy, (Drawable) t->title_w, DefaultVisual (dpy, Scr.screen), attributes.colormap);

      if (area)
      {
	region = XCreateRegion ();
	XUnionRectWithRegion (area, region, region);
	XftDrawSetClip (xftdraw, region);
      }

      if (onoroff)
      {
        dummyc.pixel = BlackPixel (dpy, Scr.screen);
        XQueryColor (dpy, attributes.colormap, &dummyc);
        color_fg.color.red = dummyc.red;
        color_fg.color.green = dummyc.green;
        color_fg.color.blue = dummyc.blue;
        color_fg.color.alpha = 0xffff;
        color_fg.pixel = Forecolor;
        XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off + 1, GetDecor (t, WindowFont.y) + 2, (XftChar8 *) t->name, strlen (t->name));
      }
      dummyc.pixel = Forecolor;
      XQueryColor (dpy, attributes.colormap, &dummyc);
      color_fg.color.red = dummyc.red;
      color_fg.color.green = dummyc.green;
      color_fg.color.blue = dummyc.blue;
      color_fg.color.alpha = 0xffff;
      color_fg.pixel = Forecolor;
      XftDrawString8 (xftdraw, &color_fg, xftfont, hor_off, GetDecor (t, WindowFont.y) + 1, (XftChar8 *) t->name, strlen (t->name));

      if (area)
      {
        XDestroyRegion(region);
      }

      XftDrawDestroy (xftdraw);
    }
#endif
    else
    {
      if (onoroff)
      {
        if (GetDecor (t, WindowFont.font))
        {
          NewFontAndColor (GetDecor (t, WindowFont.font->fid), BlackPixel (dpy, Scr.screen), BackColor);
        }
        else
        {
          NewFontAndColor (0, BlackPixel (dpy, Scr.screen), BackColor);
        }

	if (area)
	{
	  XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
	}

        XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off + 1, GetDecor (t, WindowFont.y) + 2, t->name, strlen (t->name));

	if (area)
	{
	  XSetClipMask(dpy, Scr.ScratchGC3, None);
	}
      }
      if (GetDecor (t, WindowFont.font))
      {
        NewFontAndColor (GetDecor (t, WindowFont.font->fid), Forecolor, BackColor);
      }
      else
      {
        NewFontAndColor (0, Forecolor, BackColor);
      }

      if (area)
      {
	XSetClipRectangles(dpy, Scr.ScratchGC3, 0, 0, area, 1, Unsorted);
      }

      XDrawString (dpy, t->title_w, Scr.ScratchGC3, hor_off, GetDecor (t, WindowFont.y) + 1, t->name, strlen (t->name));

      if (area)
      {
	XSetClipMask(dpy, Scr.ScratchGC3, None);
      }
    }
  }
}

void
RelieveWindow_linea (XfwmWindow * t, Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int hilite)
{
  XSegment seg[10];
  GC BackGC = NULL;
  GC HiGC = ReliefGC;
  GC LoGC = ShadowGC;
  GC BackColorGC = NULL;
  enum ButtonState title_state;
  ButtonFace *bftitle;
  Bool onoroff;
  int i;
  int edge;

  edge = 0;
  if ((win == t->sides[0]) || (win == t->sides[1]) || (win == t->sides[2]) || (win == t->sides[3]))
    edge = -1;
  else if (win == t->corners[0])
    edge = 1;
  else if (win == t->corners[1])
    edge = 2;
  else if (win == t->corners[2])
    edge = 3;
  else if (win == t->corners[3])
    edge = 4;

  onoroff = (Scr.Hilite == t);

  BackGC = (onoroff ? GetDecor (t, HiBackGC) : GetDecor (t, LoBackGC));

  if (t->flags & TITLE)
  {
    title_state = GetButtonState (t->title_w);
    bftitle = &GetDecor (t, titlebar.state[title_state]);
    XSetForeground (dpy, Scr.TransMaskGC, bftitle->u.back);
    BackColorGC = Scr.TransMaskGC;

    if (bftitle->u.ReliefGC)
    {
      HiGC = bftitle->u.ReliefGC;
    }
    if (bftitle->u.ShadowGC)
    {
      LoGC = bftitle->u.ShadowGC;
    }
  }
  else
  {
    HiGC = ReliefGC;
    LoGC = ShadowGC;
    BackColorGC = BackGC;
  }

  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    if (HiGC != ReliefGC)
    {
      XSetClipRectangles(dpy, HiGC, 0, 0, area, 1, Unsorted);
    }
    if (LoGC != ShadowGC)
    {
      XSetClipRectangles(dpy, LoGC, 0, 0, area, 1, Unsorted);
    }
    XSetClipRectangles(dpy, LoGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, BackGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.TransMaskGC, 0, 0, area, 1, Unsorted);
  }

  /* window sides */
  if (edge == -1)
  {
    switch (hilite)
    {
    case LEFT_HILITE:
      XFillRectangle (dpy, win, BackColorGC, x + 2, y, w - 2, y + 2 * t->boundary_width);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = y + 2 * t->boundary_width;
      XDrawSegments (dpy, win, BackColorGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = y + 2 * t->boundary_width;
      XDrawSegments (dpy, win, HiGC, seg, i);

      XFillRectangle (dpy, win, BackGC, x + 2, y + 2 * t->boundary_width, w - 2, h + y + 1);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 2 * t->boundary_width;
      seg[i].x2 = x;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, BackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 2 * t->boundary_width;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      break;

    case TOP_HILITE:
      XFillRectangle (dpy, win, BackColorGC, x, y + 2, w + x + 1, y + h - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, BackColorGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      XDrawSegments (dpy, win, HiGC, seg, i);
      break;

    case RIGHT_HILITE:
      XFillRectangle (dpy, win, BackColorGC, x, y, w + x - 2, y + 2 * t->boundary_width);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y + 2 * t->boundary_width;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = y + 2 * t->boundary_width;
      XDrawSegments (dpy, win, LoGC, seg, i);

      XFillRectangle (dpy, win, BackGC, x, y + 2 * t->boundary_width, w + x - 2, h + y + 1);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y + 2 * t->boundary_width;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2 * t->boundary_width;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case BOTTOM_HILITE:
      XFillRectangle (dpy, win, BackGC, x, y, w + x + 1, h + y - 2);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }
  /* corners */
  else if (edge >= 1 && edge <= 4)
  {
    switch (edge)
    {
    case 1:
      XFillRectangle (dpy, win, BackColorGC, x + 2, y + 2, x + w, y + h);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, BackColorGC, seg, i);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y;
      seg[i].x1 = x + 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x;
      seg[i++].y2 = y + 1;
      seg[i].x1 = x + 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = x + 2;
      seg[i++].y2 = x + 4;
      seg[i].x1 = x + 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = x + 4;
      seg[i++].y2 = y + 2;
      XDrawSegments (dpy, win, HiGC, seg, i);
      break;

    case 2:
      XFillRectangle (dpy, win, BackColorGC, x, y + 2, x + w - 2, y + h);
      i = 0;
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y + 1;
      seg[i].x1 = w + x - 5;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = y + 2;
      seg[i].x1 = w + x - 3;
      seg[i].y1 = y + 3;
      seg[i].x2 = w + x - 3;
      seg[i++].y2 = y + 4;
      XDrawSegments (dpy, win, HiGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y + 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, LoGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y + 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = y;
      XDrawSegments (dpy, win, BackColorGC, seg, i);
      break;

    case 3:
      XFillRectangle (dpy, win, BackGC, x + 2, y, x + w, y + h - 2);
      i = 0;
      seg[i].x1 = x + 1;
      seg[i].y1 = y;
      seg[i].x2 = x + 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x + 2;
      seg[i].y1 = y + h - 5;
      seg[i].x2 = x + 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + 3;
      seg[i].y1 = h + y - 3;
      seg[i].x2 = x + 4;
      seg[i++].y2 = h + y - 3;
      XDrawSegments (dpy, win, ReliefGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = x;
      seg[i].y1 = y;
      seg[i].x2 = x;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, BackGC, seg, i);
      i = 0;
      seg[i].x1 = x + 2;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x;
      seg[i++].y2 = h + y - 2;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;

    case 4:
      XFillRectangle (dpy, win, BackGC, x, y, x + w - 2, y + h - 2);
      i = 0;
      seg[i].x1 = w + x - 1;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 1;
      seg[i].x2 = w + x - 1;
      seg[i++].y2 = h + y - 1;
      XDrawSegments (dpy, win, Scr.BlackGC, seg, i);
      i = 0;
      seg[i].x1 = w + x - 2;
      seg[i].y1 = y;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x;
      seg[i].y1 = h + y - 2;
      seg[i].x2 = w + x - 2;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = w + x - 3;
      seg[i].y1 = y + h - 5;
      seg[i].x2 = w + x - 3;
      seg[i++].y2 = h + y - 2;
      seg[i].x1 = x + w - 5;
      seg[i].y1 = h + y - 3;
      seg[i].x2 = w + x - 3;
      seg[i++].y2 = h + y - 3;
      XDrawSegments (dpy, win, ShadowGC, seg, i);
      break;
    }
  }

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    if (HiGC != ReliefGC)
    {
      XSetClipMask(dpy, HiGC, None);
    }
    if (LoGC != ShadowGC)
    {
      XSetClipMask(dpy, LoGC, None);
    }
    XSetClipMask(dpy, BackGC, None);
    XSetClipMask(dpy, Scr.TransMaskGC, None);
  }
}

void
RelieveHalfRectangle_linea (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC, int Top)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  XDrawLine (dpy, win, ReliefGC, x + 1, y, x + 1, h + y);

  XDrawLine (dpy, win, ShadowGC, w + x - 2, y + (Top ? 1 : 0), w + x - 2, h + y);
  XDrawLine (dpy, win, Scr.BlackGC, w + x - 1, y - 1, w + x - 1, h + y);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

void
DrawSelectedEntry_linea (Window win, XRectangle *area, int x, int y, int w, int h, GC * currentGC)
{
  Globalgcv.foreground = Scr.MenuSelColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
  if (area)
  {
    XSetClipRectangles(dpy, Scr.MenuReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.MenuShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

#if 0
  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 2, y + 2, w - 4, h - 4);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.MenuShadowGC,  Scr.MenuReliefGC);
  RelieveRectangle (win, NULL, x + 1, y + 1, w - 2, h - 2, Scr.BlackGC, Scr.MenuShadowGC);
#else
  XFillRectangle (dpy, win, Scr.ScratchGC1, x + 2, y + 2, w - 4, h - 4);
  RelieveRectangle (win, NULL, x, y, w, h, Scr.ScratchGC1,  Scr.BlackGC);
  RelieveRectangle (win, NULL, x + 1, y + 1, w - 2, h - 2, Scr.MenuSelReliefGC, Scr.MenuSelShadowGC);
#endif
  if (area)
  {
    XSetClipMask(dpy, Scr.MenuShadowGC, None);
    XSetClipMask(dpy, Scr.MenuReliefGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }

  *currentGC = Scr.MenuSelGC;
}

void
DrawTopMenu_linea (Window win, XRectangle *area, int w, GC ReliefGC, GC ShadowGC)
{
  Globalgcv.foreground = Scr.MenuColors.back;
  Globalgcm = GCForeground;
  XChangeGC (dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
  XDrawLine (dpy, win, Scr.ScratchGC1, 0, 0, w - 2, 0);
  XDrawLine (dpy, win, ReliefGC, 1, 1, w - 3, 1);
}

void
DrawBottomMenu_linea (Window win, XRectangle *area, int x, int y, int w, int h, GC ReliefGC, GC ShadowGC)
{
  DrawSeparator (win, NULL, ShadowGC, Scr.BlackGC, x, y, w, h, 1);
}

void
DrawTrianglePattern_linea (Window w, XRectangle *area, GC ReliefGC, GC ShadowGC, GC BackGC, int l, int t, int r, int b, short state)
{
  int m;
  XPoint points[3];

  m = (t + b) >> 1;

  points[0].x = l + 2;
  points[0].y = t;
  points[1].x = l + 2;
  points[1].y = b;
  points[2].x = r;
  points[2].y = m;

  XFillPolygon (dpy, w, BackGC, points, 3, Convex, CoordModeOrigin);
}

void
RelieveIconTitle_linea (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);
  XDrawLine (dpy, win, ReliefGC, 2, 2, 4, 2);
  XDrawLine (dpy, win, ReliefGC, 2, 2, 2, 4);
  XDrawLine (dpy, win, ReliefGC, w - 5, 2, w - 3, 2);
  XDrawLine (dpy, win, ReliefGC, w - 3, 2, w - 3, 4);
  XDrawLine (dpy, win, ReliefGC, 2, h - 3, 4, h - 3);
  XDrawLine (dpy, win, ReliefGC, 2, h - 3, 2, h - 5);
  XDrawLine (dpy, win, ShadowGC, w - 3, h - 3,w - 5, h - 3);
  XDrawLine (dpy, win, ShadowGC, w - 3, h - 3, w - 3, h - 5);
  XDrawLine (dpy, win, Scr.BlackGC, 0, h - 1, w - 1, h - 1);
  XDrawLine (dpy, win, Scr.BlackGC, w - 1, 0, w - 1, h - 1);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

void
RelieveIconPixmap_linea (Window win, XRectangle *area, int w, int h, GC ReliefGC, GC ShadowGC)
{
  if (area)
  {
    XSetClipRectangles(dpy, ReliefGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, ShadowGC, 0, 0, area, 1, Unsorted);
    XSetClipRectangles(dpy, Scr.BlackGC, 0, 0, area, 1, Unsorted);
  }

  RelieveRectangle (win, NULL, 1, 1, w - 2, h - 2, ReliefGC, ShadowGC);
  XDrawLine (dpy, win, ReliefGC, 2, 2, 4, 2);
  XDrawLine (dpy, win, ReliefGC, 2, 2, 2, 4);
  XDrawLine (dpy, win, ReliefGC, w - 5, 2, w - 3, 2);
  XDrawLine (dpy, win, ReliefGC, w - 3, 2, w - 3, 4);
  XDrawLine (dpy, win, ReliefGC, 2, h - 3, 4, h - 3);
  XDrawLine (dpy, win, ReliefGC, 2, h - 3, 2, h - 5);
  XDrawLine (dpy, win, ShadowGC, w - 3, h - 3, w - 5, h - 3);
  XDrawLine (dpy, win, ShadowGC, w - 3, h - 3, w - 3, h - 5);
  XDrawLine (dpy, win, Scr.BlackGC, 0, h - 1, w - 1, h - 1);
  XDrawLine (dpy, win, Scr.BlackGC, w - 1, 0, w - 1, h - 1);

  if (area)
  {
    XSetClipMask(dpy, ReliefGC, None);
    XSetClipMask(dpy, ShadowGC, None);
    XSetClipMask(dpy, Scr.BlackGC, None);
  }
}

