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
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * xfwm icon code
 *
 ***********************************************************************/

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

#ifdef NeXT
#include <fcntl.h>
#endif

#ifdef HAVE_IMLIB
#include <Imlib.h>
#endif

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "stack.h"
#include "xinerama.h"
#include "themes.h"
#include "default_icon.h"

#include <X11/extensions/shape.h>

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_IMLIB
extern ImlibData *imlib_id;
#endif

#ifdef HAVE_X11_XFT_XFT_H
extern Bool enable_xft;
#endif

void GrabIconButtons (XfwmWindow *, Window);
void GrabIconKeys (XfwmWindow *, Window);

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
void
CreateIconWindow (XfwmWindow * tmp_win, int def_x, int def_y)
{
  int final_x, final_y;
  unsigned long valuemask;	/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */

  tmp_win->flags |= ICON_OURS;
  tmp_win->flags &= ~PIXMAP_OURS;
  tmp_win->flags &= ~SHAPED_ICON;
  tmp_win->icon_pixmap_w = None;
  tmp_win->iconPixmap = None;
  tmp_win->iconDepth = 0;

  if (tmp_win->flags & SUPPRESSICON)
    return;

  tmp_win->icon_p_height = 0;
  tmp_win->icon_p_width = 0;

  /* First, check for a color pixmap */
  if (tmp_win->icon_bitmap_file != NULL)
  {
    GetXPMFile (tmp_win);
  }

  /* Next, check for a monochrome bitmap */
  if ((tmp_win->icon_bitmap_file != NULL) && (tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0))
  {
    GetBitmapFile (tmp_win);
  }

  /* See if the app supplies its own icon window */
  if ((tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0) && (tmp_win->wmhints) && (tmp_win->wmhints->flags & IconWindowHint))
  {
    GetIconWindow (tmp_win);
  }

  /* Finally, try to get icon bitmap from the application */
  if ((tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0) && (tmp_win->wmhints) && (tmp_win->wmhints->flags & IconPixmapHint))
  {
    GetIconBitmap (tmp_win);
  }

  if ((tmp_win->icon_p_height == 0) && (tmp_win->icon_p_width == 0))
  {
    SetDefaultXPMIcon (tmp_win);
  }

  /* figure out the icon window size */
  if (!(tmp_win->flags & SUPPRESSICON) || (tmp_win->icon_p_height == 0))
  {
    XFontSet fontset = Scr.IconFont.fontset;
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, tmp_win->icon_name, strlen (tmp_win->icon_name), &rect1, &rect2);
      tmp_win->icon_t_width = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (Scr.IconFont.xftfont))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, Scr.IconFont.xftfont, (XftChar8 *) tmp_win->icon_name, strlen (tmp_win->icon_name), &extents);
      tmp_win->icon_t_width = extents.xOff;
    }
#endif
    else
    {
      tmp_win->icon_t_width = XTextWidth (Scr.IconFont.font, tmp_win->icon_name, strlen (tmp_win->icon_name));
    }
    tmp_win->icon_w_height = ICON_HEIGHT;
  }
  else
  {
    tmp_win->icon_t_width = 0;
    tmp_win->icon_w_height = 0;
  }
  if ((tmp_win->flags & ICON_OURS) && (tmp_win->icon_p_height > 0))
  {
    tmp_win->icon_p_width += 4;
    tmp_win->icon_p_height += 4;
  }

  if (tmp_win->icon_p_width == 0)
    tmp_win->icon_p_width = tmp_win->icon_t_width + 6;
  tmp_win->icon_w_width = tmp_win->icon_p_width;

  final_x = def_x;
  final_y = def_y;
  if (final_x < MyDisplayX (final_x, final_y))
    final_x = MyDisplayX (final_x, final_y);
  if (final_y < MyDisplayY (final_x, final_y))
    final_y = MyDisplayY (final_x, final_y);

  if (final_x + tmp_win->icon_w_width >= MyDisplayMaxX (final_x, final_y))
    final_x = MyDisplayMaxX (final_x, final_y) - tmp_win->icon_w_width - 1;
  if (final_y + tmp_win->icon_w_height >= MyDisplayMaxY (final_x, final_y))
    final_y = MyDisplayMaxY (final_x, final_y) - tmp_win->icon_w_height - 1;

  tmp_win->icon_x_loc = final_x;
  tmp_win->icon_xl_loc = final_x;
  tmp_win->icon_y_loc = final_y;

  /* clip to fit on screen */
  attributes.background_pixel = GetDecor (Tmp_win, LoColors.back);
  valuemask = CWCursor | CWEventMask | CWDontPropagate | CWBackPixel;
  attributes.cursor = Scr.XfwmCursors[DEFAULT];
  attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | VisibilityChangeMask | ExposureMask | KeyPressMask | EnterWindowMask | FocusChangeMask);
  attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
  if (!(tmp_win->flags & SUPPRESSICON) || (tmp_win->icon_p_height == 0))
    tmp_win->icon_w = XCreateWindow (dpy, Scr.Root, final_x, final_y + tmp_win->icon_p_height, tmp_win->icon_w_width, tmp_win->icon_w_height, 0, CopyFromParent, CopyFromParent, CopyFromParent, valuemask, &attributes);

  if ((tmp_win->flags & ICON_OURS) && (tmp_win->icon_p_width > 0) && (tmp_win->icon_p_height > 0))
  {
    tmp_win->icon_pixmap_w = XCreateWindow (dpy, Scr.Root, final_x, final_y, tmp_win->icon_p_width, tmp_win->icon_p_height, 0, CopyFromParent, CopyFromParent, CopyFromParent, valuemask, &attributes);
  }
  else
  {
    valuemask = CWEventMask | CWDontPropagate;
    attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | VisibilityChangeMask | KeyPressMask | EnterWindowMask | FocusChangeMask | LeaveWindowMask);
    attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
    XChangeWindowAttributes (dpy, tmp_win->icon_pixmap_w, valuemask, &attributes);
  }

  if (ShapesSupported && (tmp_win->flags & SHAPED_ICON) && (Scr.Options & UseShapedIcons))
  {
    XShapeCombineMask (dpy, tmp_win->icon_pixmap_w, ShapeBounding, 2, 2, tmp_win->icon_maskPixmap, ShapeSet);
  }

  if (tmp_win->icon_w != None)
  {
    XSaveContext (dpy, tmp_win->icon_w, XfwmContext, (caddr_t) tmp_win);
    XDefineCursor (dpy, tmp_win->icon_w, Scr.XfwmCursors[DEFAULT]);
    GrabIconButtons (tmp_win, tmp_win->icon_w);
    GrabIconKeys (tmp_win, tmp_win->icon_w);
  }
  if ((tmp_win->icon_pixmap_w != None) && (tmp_win->flags & ICON_OURS))
  {
    XSaveContext (dpy, tmp_win->icon_pixmap_w, XfwmContext, (caddr_t) tmp_win);
    XDefineCursor (dpy, tmp_win->icon_pixmap_w, Scr.XfwmCursors[DEFAULT]);
    GrabIconButtons (tmp_win, tmp_win->icon_pixmap_w);
    GrabIconKeys (tmp_win, tmp_win->icon_pixmap_w);
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	AutoPlace - Find a home for an icon
 *
 ************************************************************************/

#define TEST_1 \
      (((Scr.iconbox == 0) || (Scr.iconbox == 2)) \
      ? \
          ((test_y + temp_h) < (MyDisplayMaxY(base_x, base_y) + base_y)) \
      : \
          ((test_x + temp_w) < (MyDisplayMaxX(base_x, base_y) + base_x)))

#define TEST_2 \
      (((Scr.iconbox == 0) || (Scr.iconbox == 2)) \
      ? \
          ((test_x + temp_w) < (MyDisplayMaxX(base_x, base_y) + base_x)) \
      : \
          ((test_y + temp_h) < (MyDisplayMaxY(base_x, base_y) + base_y)))

/* A macro to determine if 2 areas are overlapping */
#define OVERLAP(x1, y1, x2, y2, x3, y3, x4, y4) \
   (((x1 >= x3) && (x1 <= x4) && (y1 >= y3) && (y1 <= y4)) || \
    ((x2 >= x3) && (x2 <= x4) && (y2 >= y3) && (y2 <= y4)) || \
    ((x1 >= x3) && (x1 <= x4) && (y2 >= y3) && (y2 <= y4)) || \
    ((x2 >= x3) && (x2 <= x4) && (y1 >= y3) && (y1 <= y4)) || \
    ((x3 >= x1) && (x3 <= x2) && (y3 >= y1) && (y3 <= y2)) || \
    ((x4 >= x1) && (x4 <= x2) && (y4 >= y1) && (y4 <= y2)) || \
    ((x3 >= x1) && (x3 <= x2) && (y4 >= y1) && (y4 <= y2)) || \
    ((x4 >= x1) && (x4 <= x2) && (y3 >= y1) && (y3 <= y2)))

Bool CheckIconPlace (XfwmWindow * t)
{
  XfwmWindow *tw;
  Bool loc_ok = True;

  tw = Scr.XfwmRoot.next;
  while ((tw) && (loc_ok == True))
  {
    if (tw->Desk == t->Desk)
    {
      if ((tw->flags & ICONIFIED) && (tw->icon_w || tw->icon_pixmap_w) && (tw != t))
      {
	if (OVERLAP (t->icon_x_loc - Scr.iconspacing, t->icon_y_loc - Scr.iconspacing, t->icon_x_loc + t->icon_p_width + Scr.iconspacing, t->icon_y_loc + t->icon_w_height + t->icon_p_height + Scr.iconspacing, tw->icon_x_loc, tw->icon_y_loc, tw->icon_x_loc + tw->icon_p_width, tw->icon_y_loc + tw->icon_w_height + tw->icon_p_height))
	{
	  loc_ok = False;
	}
      }
    }
    tw = tw->next;
  }
  return (loc_ok);
}

void
AutoPlace (XfwmWindow * t, Bool rearrange)
{
  int test_x = 0, test_y = 0, tw, th, tx, ty, temp_h, temp_w;
  int base_x, base_y;
  int width, height;
  XfwmWindow *test_window;
  Bool loc_ok;
  int real_x = 10, real_y = 10;
  int step_x, step_y;
  int center_x, center_y;

  width = t->icon_p_width;
  height = t->icon_w_height + t->icon_p_height;

  center_x = t->frame_x + (t->frame_width / 2);
  center_y = t->frame_y + (t->frame_height / 2);

  switch (Scr.iconbox)
  {
  case 1:
    base_x = MyDisplayX (center_x, center_y);
    base_y = MyDisplayY (center_x, center_y);
    step_x = Scr.icongrid;
    step_y = Scr.icongrid;
    break;
  case 2:
    base_x = MyDisplayX (center_x, center_y);
    base_y = ((int) (MyDisplayMaxY (center_x, center_y) - height) / Scr.icongrid) * Scr.icongrid;
    step_x = Scr.icongrid;
    step_y = -Scr.icongrid;
    break;
  case 3:
    base_x = ((int) (MyDisplayMaxX (center_x, center_y) - width) / Scr.icongrid) * Scr.icongrid;
    base_y = MyDisplayY (center_x, center_y);
    step_x = -Scr.icongrid;
    step_y = Scr.icongrid;
    break;
  default:
    base_x = MyDisplayX (center_x, center_y);
    base_y = MyDisplayY (center_x, center_y);
    step_x = Scr.icongrid;
    step_y = Scr.icongrid;
    break;
  }
  if (t->flags & ICON_MOVED)
  {
    /* just make sure the icon is on this screen */
    t->icon_x_loc = t->icon_x_loc % MyDisplayMaxX (t->icon_x_loc, t->icon_y_loc) + base_x;
    t->icon_y_loc = t->icon_y_loc % MyDisplayMaxY (t->icon_x_loc, t->icon_y_loc) + base_y;
    if (t->icon_x_loc < MyDisplayX (t->icon_x_loc, t->icon_y_loc))
      t->icon_x_loc += MyDisplayX (t->icon_x_loc, t->icon_y_loc);
    if (t->icon_y_loc < MyDisplayY (t->icon_x_loc, t->icon_y_loc))
      t->icon_y_loc += MyDisplayY (t->icon_x_loc, t->icon_y_loc);
    if (t->icon_x_loc > MyDisplayMaxX (t->icon_x_loc, t->icon_y_loc))
      t->icon_x_loc -= MyDisplayMaxX (t->icon_x_loc, t->icon_y_loc);
    if (t->icon_y_loc > MyDisplayMaxY (t->icon_x_loc, t->icon_y_loc))
      t->icon_y_loc -= MyDisplayMaxY (t->icon_x_loc, t->icon_y_loc);
  }
  loc_ok = False;

  /* check all boxes in order */
  if ((Scr.iconbox == 0) || (Scr.iconbox == 2))
    test_y = base_y;
  else
    test_x = base_x;

  temp_h = height;
  temp_w = width;
  while (TEST_1 && (!loc_ok))
  {
    if ((Scr.iconbox == 0) || (Scr.iconbox == 2))
      test_x = base_x;
    else
      test_y = base_y;
    while (TEST_2 && (!loc_ok))
    {
      real_x = test_x;
      real_y = test_y;
      loc_ok = True;
      test_window = Scr.XfwmRoot.next;
      while ((test_window != (XfwmWindow *) 0) && (loc_ok == True))
      {
	if (test_window->Desk == t->Desk)
	{
	  if ((test_window->flags & ICONIFIED) && (test_window->icon_w || test_window->icon_pixmap_w) && (test_window != t))
	  {
	    tw = test_window->icon_p_width;
	    th = test_window->icon_p_height + test_window->icon_w_height;
	    tx = test_window->icon_x_loc;
	    ty = test_window->icon_y_loc;

	    if (OVERLAP (tx - Scr.iconspacing, ty - Scr.iconspacing, tx + tw + Scr.iconspacing, ty + th + Scr.iconspacing, real_x, real_y, real_x + width, real_y + height) && !(rearrange && !(test_window->icon_arranged) && !(test_window->flags & ICON_MOVED)))
	    {
	      loc_ok = False;
	    }
	  }
	}
	test_window = test_window->next;
      }
      if ((Scr.iconbox == 0) || (Scr.iconbox == 2))
	test_x += step_x;
      else
	test_y += step_y;
    }
    if ((Scr.iconbox == 0) || (Scr.iconbox == 2))
      test_y += step_y;
    else
      test_x += step_x;
  }
  if (loc_ok == False)
    return;
  t->icon_x_loc = real_x;
  t->icon_y_loc = real_y;

  if ((t->icon_pixmap_w) && (t->Desk == Scr.CurrentDesk))
    XMoveWindow (dpy, t->icon_pixmap_w, t->icon_x_loc, t->icon_y_loc);

  t->icon_w_width = t->icon_p_width;
  t->icon_xl_loc = t->icon_x_loc;

  if ((t->icon_w != None) && (t->Desk == Scr.CurrentDesk))
    XMoveResizeWindow (dpy, t->icon_w, t->icon_xl_loc, t->icon_y_loc + t->icon_p_height, t->icon_w_width, ICON_HEIGHT);
  Broadcast (XFCE_M_ICON_LOCATION, 7, t->w, t->frame, (unsigned long) t, t->icon_x_loc, t->icon_y_loc, t->icon_w_width, t->icon_w_height + t->icon_p_height);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabIconButtons - grab needed buttons for the icon window
 *
 *  Inputs:
 *	tmp_win - the xfwm window structure to use
 *
 ***********************************************************************/
void
GrabIconButtons (XfwmWindow * tmp_win, Window w)
{
  MyXGrabButton (dpy, AnyButton, 0, w, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  MyXGrabButton (dpy, AnyButton, AnyModifier, w, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabIconKeys - grab needed keys for the icon window
 *
 *  Inputs:
 *	tmp_win - the xfwm window structure to use
 *
 ***********************************************************************/
void
GrabIconKeys (XfwmWindow * tmp_win, Window w)
{
  Binding *tmp;
  for (tmp = Scr.AllBindings; tmp != NULL; tmp = tmp->NextBinding)
  {
    if ((tmp->Context & C_ICON) && (tmp->IsMouse == 0))
      MyXGrabKey (dpy, tmp->Button_Key, tmp->Modifier, w, True, GrabModeAsync, GrabModeAsync);
  }
  return;
}

/****************************************************************************
 *
 * Looks for a monochrome icon bitmap file
 *
 ****************************************************************************/
void
GetBitmapFile (XfwmWindow * tmp_win)
{
  int HotX, HotY;
  int res;

  res = 0;
  if (check_existfile (tmp_win->icon_bitmap_file))
    res = (XReadBitmapFile (dpy, Scr.Root, tmp_win->icon_bitmap_file, (unsigned int *) &tmp_win->icon_p_width, (unsigned int *) &tmp_win->icon_p_height, &tmp_win->iconPixmap, &HotX, &HotY) != BitmapSuccess);

  if (!res)
  {
    tmp_win->icon_p_width = 0;
    tmp_win->icon_p_height = 0;
  }
}

/****************************************************************************
 *
 * Set default XPM icon file
 *
 ****************************************************************************/
void
SetDefaultXPMIcon (XfwmWindow * tmp_win)
{
  int res;
#ifdef HAVE_IMLIB
  ImlibImage *im;

  res = 0;
  im = Imlib_create_image_from_xpm_data (imlib_id, default_icon);
  if (im)
  {

    if ((im->rgb_width > 48) || (im->rgb_height > 48))
    {
      res = Imlib_render (imlib_id, im, 48, 48);
      tmp_win->icon_p_width = tmp_win->icon_p_height = 48;
    }
    else
    {
      res = Imlib_render (imlib_id, im, im->rgb_width, im->rgb_height);
      tmp_win->icon_p_width = im->rgb_width;
      tmp_win->icon_p_height = im->rgb_height;
    }

    tmp_win->iconPixmap = Imlib_move_image (imlib_id, im);
    tmp_win->icon_maskPixmap = Imlib_move_mask (imlib_id, im);
    Imlib_kill_image (imlib_id, im);
  }
#else
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
 
  XGetWindowAttributes (dpy, Scr.Root, &root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.depth = Scr.d_depth;
  xpm_attributes.closeness = 65535;
  xpm_attributes.valuemask = XpmSize | XpmColormap | XpmCloseness | XpmDepth;

  res = (XpmCreatePixmapFromData (dpy, Scr.Root, default_icon, &tmp_win->iconPixmap, &tmp_win->icon_maskPixmap, &xpm_attributes) == XpmSuccess);

  if (res)
  {
    tmp_win->icon_p_width = xpm_attributes.width;
    tmp_win->icon_p_height = xpm_attributes.height;
  }
#endif
  if (res)
  {
    tmp_win->flags |= PIXMAP_OURS;
    tmp_win->iconDepth = Scr.d_depth;
    if (ShapesSupported && tmp_win->icon_maskPixmap)
      tmp_win->flags |= SHAPED_ICON;
  }
}

/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void
GetXPMFile (XfwmWindow * tmp_win)
{
  int res;
#ifdef HAVE_IMLIB
  ImlibImage *im;

  res = 0;
  im = Imlib_load_image (imlib_id, tmp_win->icon_bitmap_file);
  if (!im)
  {
    im = Imlib_create_image_from_xpm_data (imlib_id, default_icon);
  }
  if (im)
  {

    if ((im->rgb_width > 48) || (im->rgb_height > 48))
    {
      res = Imlib_render (imlib_id, im, 48, 48);
      tmp_win->icon_p_width = tmp_win->icon_p_height = 48;
    }
    else
    {
      res = Imlib_render (imlib_id, im, im->rgb_width, im->rgb_height);
      tmp_win->icon_p_width = im->rgb_width;
      tmp_win->icon_p_height = im->rgb_height;
    }

    tmp_win->iconPixmap = Imlib_move_image (imlib_id, im);
    tmp_win->icon_maskPixmap = Imlib_move_mask (imlib_id, im);
    Imlib_kill_image (imlib_id, im);
  }
#else
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  static XpmColorSymbol none_color = { NULL, "None", (Pixel) 0 };

  XGetWindowAttributes (dpy, Scr.Root, &root_attr);
  xpm_attributes.colormap = root_attr.colormap;
  xpm_attributes.depth = Scr.d_depth;
  xpm_attributes.closeness = 40000;	/* Allow for "similar" colors */
  xpm_attributes.colorsymbols = &none_color;
  xpm_attributes.numsymbols = 1;
  xpm_attributes.valuemask = XpmSize | XpmColormap | XpmCloseness | XpmColorSymbols | XpmDepth;

  res = 0;
  if (check_existfile (tmp_win->icon_bitmap_file))
    res = (XpmReadFileToPixmap (dpy, Scr.Root, tmp_win->icon_bitmap_file, &tmp_win->iconPixmap, &tmp_win->icon_maskPixmap, &xpm_attributes) == XpmSuccess);
  if (!res)
    res = (XpmCreatePixmapFromData (dpy, Scr.Root, default_icon, &tmp_win->iconPixmap, &tmp_win->icon_maskPixmap, &xpm_attributes) == XpmSuccess);

  if (res)
  {
    tmp_win->icon_p_width = xpm_attributes.width;
    tmp_win->icon_p_height = xpm_attributes.height;
  }
#endif
  if (res)
  {
    tmp_win->flags |= PIXMAP_OURS;
    tmp_win->iconDepth = Scr.d_depth;
    if (ShapesSupported && tmp_win->icon_maskPixmap)
      tmp_win->flags |= SHAPED_ICON;
  }
}

/****************************************************************************
 *
 * Looks for an application supplied icon window
 *
 ****************************************************************************/
void
GetIconWindow (XfwmWindow * tmp_win)
{
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_bw, dummy_depth;

  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  if (XGetGeometry (dpy, tmp_win->wmhints->icon_window, &dummy_root, &dummy_x, &dummy_y, (unsigned int *) &tmp_win->icon_p_width, (unsigned int *) &tmp_win->icon_p_height, &dummy_bw, &dummy_depth) == 0)
  {
    xfwm_msg (ERR, "GetIconWindow", "Help! Bad Icon Window!");
  }
  tmp_win->icon_p_width += dummy_bw << 1;
  tmp_win->icon_p_height += dummy_bw << 1;
  /*
   * Now make the new window the icon window for this window,
   * and set it up to work as such (select for key presses
   * and button presses/releases, set up the contexts for it,
   * and define the cursor for it).
   */
  tmp_win->icon_pixmap_w = tmp_win->wmhints->icon_window;
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      tmp_win->flags |= SHAPED_ICON;
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
  /* Make sure that the window is a child of the root window ! */
  /* Olwais screws this up, maybe others do too! */
  XReparentWindow (dpy, tmp_win->icon_pixmap_w, Scr.Root, 0, 0);
  tmp_win->flags &= ~ICON_OURS;
}


/****************************************************************************
 *
 * Looks for an application supplied bitmap or pixmap
 *
 ****************************************************************************/
void
GetIconBitmap (XfwmWindow * tmp_win)
{
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_bw, dummy_depth;

  /* We are guaranteed that wmhints is non-null when calling this
   * routine */
  XGetGeometry (dpy, tmp_win->wmhints->icon_pixmap, &dummy_root, &dummy_x, &dummy_y, (unsigned int *) &tmp_win->icon_p_width, (unsigned int *) &tmp_win->icon_p_height, &dummy_bw, &dummy_depth);
  tmp_win->iconPixmap = tmp_win->wmhints->icon_pixmap;
  tmp_win->iconDepth = dummy_depth;
  if (ShapesSupported)
  {
    if (tmp_win->wmhints->flags & IconMaskHint)
    {
      tmp_win->flags |= SHAPED_ICON;
      tmp_win->icon_maskPixmap = tmp_win->wmhints->icon_mask;
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	DeIconify a window
 *
 ***********************************************************************/
void
DeIconify (XfwmWindow * tmp_win)
{
  XfwmWindow *t;
  XWindowAttributes winattrs;
  unsigned long eventMask;

  if (!tmp_win)
    return;

  if ((tmp_win->Desk == Scr.CurrentDesk) && !(tmp_win->flags & SUPPRESSICON))
    Animate (tmp_win->icon_x_loc, tmp_win->icon_y_loc, tmp_win->icon_w_width, tmp_win->icon_w_height, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->flags & SHADED ? tmp_win->title_height + 2 * tmp_win->boundary_width : tmp_win->frame_height);
  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    if ((t == tmp_win) || ((t->flags & TRANSIENT) && (t->transientfor == tmp_win->w)))
    {
      XGetWindowAttributes (dpy, t->w, &winattrs);
      eventMask = winattrs.your_event_mask;
      MyXGrabServer (dpy);
      XSelectInput (dpy, t->w, (eventMask & ~StructureNotifyMask));
      if (t->Desk == Scr.CurrentDesk)
      {
	XMapWindow (dpy, t->frame);
      }
      XMapWindow (dpy, t->Parent);
      XMapWindow (dpy, t->w);
      SetMapStateProp (t, NormalState);
      XSelectInput (dpy, t->w, eventMask);
      MyXUngrabServer (dpy);
      t->flags &= ~(ICONIFIED | ICON_UNMAPPED | STARTICONIC);
      t->flags |= MAPPED;
      if (t->icon_w)
	XUnmapWindow (dpy, t->icon_w);
      if (t->icon_pixmap_w)
	XUnmapWindow (dpy, t->icon_pixmap_w);
      Broadcast (XFCE_M_DEICONIFY, 3, t->w, t->frame, (unsigned long) t, 0, 0, 0, 0);
    }
  }
  RaiseWindow (tmp_win);
  if ((tmp_win->Desk == Scr.CurrentDesk) && AcceptInput (tmp_win))
  {
    SetFocus (tmp_win->w, tmp_win, True, False);
  }
  XSync (dpy, 0);
  
  return;
}


/****************************************************************************
 *
 * Iconifies the selected window
 *
 ****************************************************************************/
void
Iconify (XfwmWindow * tmp_win, int def_x, int def_y, Bool stackit)
{
  XfwmWindow *t;
  XWindowAttributes winattrs;
  unsigned long eventMask;

  if (!tmp_win)
    return;

  /* Transient can't be iconified separately */
  if ((tmp_win->flags & TRANSIENT))
    return;

  /* Now create icons first so it looks faster */
  if (tmp_win->icon_w == None)
  {
    if (tmp_win->flags & ICON_MOVED)
      CreateIconWindow (tmp_win, tmp_win->icon_x_loc, tmp_win->icon_y_loc);
    else
      CreateIconWindow (tmp_win, def_x, def_y);
  }
  tmp_win->icon_arranged = False;
  /* if no pixmap we want icon width to change to text width every iconify */
  if ((tmp_win->icon_w != None) && (tmp_win->icon_pixmap_w == None))
  {
    XFontSet fontset = Scr.IconFont.fontset;
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, tmp_win->icon_name, strlen (tmp_win->icon_name), &rect1, &rect2);
      tmp_win->icon_t_width = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (Scr.IconFont.xftfont))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, Scr.IconFont.xftfont, (XftChar8 *) tmp_win->icon_name, strlen (tmp_win->icon_name), &extents);
      tmp_win->icon_t_width = extents.xOff;
    }
#endif
    else
    {
      tmp_win->icon_t_width = XTextWidth (Scr.IconFont.font, tmp_win->icon_name, strlen (tmp_win->icon_name));
    }
    tmp_win->icon_p_width = tmp_win->icon_t_width + 6;
    tmp_win->icon_w_width = tmp_win->icon_p_width;
  }

  if ((!(tmp_win->flags & STARTICONIC)) && (!(tmp_win->flags & ICON_MOVED) || !CheckIconPlace (tmp_win)))
    AutoPlace (tmp_win, False);

  /* iconify transients first */
  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    if ((t == tmp_win) || ((t->flags & TRANSIENT) && (t->transientfor == tmp_win->w)))
    {
      /*
       * Prevent the receipt of an UnmapNotify, since that would
       * cause a transition to the Withdrawn state.
       */
      t->flags &= ~MAPPED;
      XGetWindowAttributes (dpy, t->w, &winattrs);
      eventMask = winattrs.your_event_mask;
      MyXGrabServer (dpy);
      XSelectInput (dpy, t->w, eventMask & ~StructureNotifyMask);
      XUnmapWindow (dpy, t->w);
      XUnmapWindow (dpy, t->frame);
      XSelectInput (dpy, t->w, eventMask);
      MyXUngrabServer (dpy);
      t->DeIconifyDesk = t->Desk;
      if (t->icon_w)
	XUnmapWindow (dpy, t->icon_w);
      if (t->icon_pixmap_w)
	XUnmapWindow (dpy, t->icon_pixmap_w);
      SetMapStateProp (t, IconicState);
      SetBorder (t, NULL, False, False, False, None);
      if (t != tmp_win)
      {
	t->flags |= ICONIFIED | ICON_UNMAPPED;

	Broadcast (XFCE_M_ICONIFY, 7, t->w, t->frame, (unsigned long) t, -10000, -10000, t->icon_w_width, t->icon_w_height + t->icon_p_height);
	BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, t);
      }
    }
  }
  tmp_win->flags |= ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  Broadcast (XFCE_M_ICONIFY, 7, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->icon_x_loc, tmp_win->icon_y_loc, tmp_win->icon_w_width, tmp_win->icon_w_height + tmp_win->icon_p_height);
  BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, tmp_win);

  if (tmp_win->Desk == Scr.CurrentDesk)
  {
    if (stackit)
      LowerWindow (tmp_win);
    if (!(tmp_win->flags & SUPPRESSICON))
      Animate (tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->flags & SHADED ? tmp_win->title_height + 2 * tmp_win->boundary_width : tmp_win->frame_height, tmp_win->icon_x_loc, tmp_win->icon_y_loc, tmp_win->icon_w_width, tmp_win->icon_w_height);

    if (tmp_win->icon_w != None)
      XMapWindow (dpy, tmp_win->icon_w);

    if (tmp_win->icon_pixmap_w != None)
      XMapWindow (dpy, tmp_win->icon_pixmap_w);

    if ((tmp_win->Desk == Scr.CurrentDesk) && (tmp_win == Scr.Focus))
    {
      if (Scr.PreviousFocus == tmp_win)
	Scr.PreviousFocus = NULL;
      RevertFocus (tmp_win, True);
    }
  }
  return;
}

/****************************************************************************
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 ****************************************************************************/
void
SetMapStateProp (XfwmWindow * tmp_win, int state)
{
  unsigned long data[2];	/* "suggested" by ICCCM version 1 */

  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_w;
  /*  data[2] = (unsigned long) tmp_win->icon_pixmap_w; */

  XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32, PropModeReplace, (unsigned char *) data, 2);
  return;
}
