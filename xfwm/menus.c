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
 * xfwm menu code
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
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "themes.h"
#include "xinerama.h"

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_X11_XFT_XFT_H
extern Bool enable_xft;
#endif

#if 0
extern char *charset;
#endif
int menu_on = 0;

MenuRoot *ActiveMenu = NULL;	/* the active menu */
MenuItem *ActiveItem = NULL;	/* the active menu item */

int menuFromFrameOrWindowOrTitlebar = False;

extern int Context, Button;
extern XfwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
int Stashed_X, Stashed_Y, MenuY = 0;

int ButtonPosition (int, XfwmWindow *);
int UpdateMenu (int sticks);
int mouse_moved = 0;
int menu_aborted = 0;

extern XContext MenuContext;
/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 * Style = 1 = sticky menu, stays up on initial button release.
 * Style = 0 = transient menu, drops on initial release.
 ***************************************************************************/
int
do_menu (MenuRoot * menu, int style)
{
#ifdef REQUIRES_STASHEVENT
  extern Time lastTimestamp;
#endif
  Time t0 = 0;
  int prevStashedX = 0, prevStashedY = 0;
  MenuRoot *PrevActiveMenu = 0;
  MenuItem *PrevActiveItem = 0;
  int retval = MENU_NOP;
  int x, y;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_win_x, dummy_win_y;
  unsigned int dummy_mask;


  /* this condition could get ugly */
  if (menu->in_use)
    return MENU_ERROR;

  /* In case we wind up with a move from a menu which is
   * from a window border, we'll return to here to start
   * the move */
  XSync (dpy, 0);
  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &x, &y, &dummy_win_x, &dummy_win_y, &dummy_mask);

  if (menu_on)
  {
    prevStashedX = Stashed_X;
    prevStashedY = Stashed_Y;

    PrevActiveMenu = ActiveMenu;
    PrevActiveItem = ActiveItem;
    if (ActiveMenu)
    {
      if ((Stashed_X + (ActiveMenu->width >> 1) + menu->width) < MyDisplayMaxX (Stashed_X, Stashed_Y))
	x = Stashed_X + (ActiveMenu->width >> 1) + (menu->width >> 1);
      else
	x = Stashed_X - (ActiveMenu->width >> 1) - (menu->width >> 1);
    }
    if (ActiveItem)
      y = ActiveItem->y_offset + MenuY + (Scr.EntryHeight >> 1);
  }
  else
  {
    mouse_moved = 0;
#ifdef REQUIRES_STASHEVENT
    t0 = lastTimestamp;
#else
    t0 = CurrentTime;
#endif
    if (!GrabEm (MENU))
    {
      XBell (dpy, Scr.screen);
      xfwm_msg (WARN, "do_menu", "Can't grab mouse/kbd\n");
      return MENU_DONE;
    }
    if ((x + menu->width + 5) < MyDisplayMaxX (x, y))
      x += ((menu->width >> 1) + 5);
    else
      x -= ((menu->width >> 1) + 5);
  }
  if (PopUpMenu (menu, x, y))
  {
    retval = UpdateMenu (style);
  }
  else
    XBell (dpy, Scr.screen);

  ActiveMenu = PrevActiveMenu;
  ActiveItem = PrevActiveItem;
  if ((ActiveItem) && (menu_on))
    ActiveItem->state = 1;
  Stashed_X = prevStashedX;
  Stashed_Y = prevStashedY;

  if (!menu_on)
  {
    UngrabEm ();
    WaitForButtonsUp ();
  }
#ifdef REQUIRES_STASHEVENT
  if (((lastTimestamp - t0) < 3 * Scr.ClickTime) && (mouse_moved == 0))
#else
  if (((CurrentTime - t0) < 3 * Scr.ClickTime) && (mouse_moved == 0))
#endif
    menu_aborted = 1;
  else
    menu_aborted = 0;
  return retval;
}

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/
void
PaintMenu (MenuRoot * mr)
{
  MenuItem *mi;

  for (mi = mr->first; mi != NULL; mi = mi->next)
    PaintEntry (mr, mi);
  return;
}

MenuRoot *PrevMenu = NULL;
MenuItem *PrevItem = NULL;
int PrevY = 0;


/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 ***********************************************************************/
int
FindEntry (void)
{
  MenuItem *mi;
  MenuRoot *actual_mr;
  int retval = MENU_NOP;
  MenuRoot *PrevPrevMenu;
  MenuItem *PrevPrevItem;
  int PrevPrevY;
  int x, y, ChildY;
  Window Child;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_x;
  unsigned int dummy_mask;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &Child, &dummy_x, &ChildY, &x, &y, &dummy_mask);
  XQueryPointer (dpy, ActiveMenu->w, &dummy_root, &dummy_child, &dummy_x, &ChildY, &x, &y, &dummy_mask);

  /* look for the entry that the mouse is in */
  for (mi = ActiveMenu->first; mi; mi = mi->next)
    if (y >= mi->y_offset && y < mi->y_offset + mi->y_height)
      break;
  if (x < 0 || x > ActiveMenu->width)
    mi = NULL;


  /* if we weren't on the active entry, let's turn the old active one off */
  if ((ActiveItem) && (mi != ActiveItem))
  {
    ActiveItem->state = 0;
    PaintEntry (ActiveMenu, ActiveItem);
  }
  /* if we weren't on the active item, change the active item and turn it on */
  if ((mi != ActiveItem) && (mi != NULL))
  {
    mi->state = 1;
    PaintEntry (ActiveMenu, mi);
  }
  ActiveItem = mi;

  if (ActiveItem)
  {
    MenuRoot *menu;
    /* create a new sub-menu */
    if (ActiveItem->func_type == F_POPUP)
    {
      menu = FindPopup (&ActiveItem->action[5]);
      if (menu != NULL)
      {
	PrevPrevMenu = PrevMenu;
	PrevPrevItem = PrevItem;
	PrevPrevY = PrevY;
	PrevY = MenuY;
	PrevMenu = ActiveMenu;
	PrevItem = ActiveItem;
	retval = do_menu (menu, 0);
	flush_expose (ActiveMenu->w);
	for (mi = ActiveMenu->first; mi != NULL; mi = mi->next)
	{
	  PaintEntry (ActiveMenu, mi);
	}
	MenuY = PrevY;
	PrevMenu = PrevPrevMenu;
	PrevItem = PrevPrevItem;
	PrevY = PrevPrevY;
      }
    }
  }
  /* end a sub-menu */
  if (XFindContext (dpy, Child, MenuContext, (caddr_t *) & actual_mr) == XCNOENT)
  {
    return retval;
  }

  if (actual_mr != ActiveMenu)
  {
    if (actual_mr == PrevMenu)
    {
      if ((PrevItem->y_offset + PrevY > ChildY) || (PrevItem->y_offset + PrevItem->y_height + PrevY < ChildY))
      {
	return SUBMENU_DONE;
      }
    }
    else
    {
      return SUBMENU_DONE;
    }
  }
  return retval;
}

/***********************************************************************
 * Procedure
 * 	menuShortcuts() - Menu keyboard processing (pete@tecc.co.uk)
 *
 * Function called from UpdateMenu instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 ***********************************************************************/
void
menuShortcuts (XEvent * ev)
{
  MenuItem *mi;
  char kpressed[2];
  KeySym keysym;
  XComposeStatus status;

  XLookupString (&ev->xkey, &kpressed[0], 1, &keysym, &status);

  /* Try to match hot keys */
  if (((kpressed[0] >= 'a') && (kpressed[0] <= 'z')) ||	/* Only consider alphabetic */
      ((kpressed[0] >= 'A') && (kpressed[0] <= 'Z')) || ((kpressed[0] >= '0') && (kpressed[0] <= '9')))	/* ...or numeric keys   */
  {
    /* Search menu for matching hotkey */
    for (mi = ActiveMenu->first; mi; mi = mi->next)
    {
      char key;
      if (mi->hotkey == 0)
	continue;		/* Item has no hotkey   */
      key = (mi->hotkey > 0) ?	/* Extract hot character */
	mi->item[mi->hotkey - 1] : mi->item2[-1 - mi->hotkey];
      if (kpressed[0] == key)
      {
	ActiveItem = mi;	/* Make this the active item    */
	ev->type = ButtonRelease;	/* Force a menu exit            */
	return;
      }
    }
  }

  switch (keysym)		/* Other special keyboard handling      */
  {
  case XK_Escape:		/* Escape key pressed. Abort            */
    if (ActiveItem)
    {
      ActiveItem->state = 0;
      PaintEntry (ActiveMenu, ActiveItem);
    }
    ActiveItem = NULL;		/* No selection                         */
    ev->type = ButtonRelease;	/* Make the menu exit                   */
    break;

    /* Nothing special --- Allow other shortcuts (cursor movement)    */
  default:
    Keyboard_shortcuts (ev, ButtonRelease);
    break;
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 * 
 *  Input
 *      sticks = 0, transient style menu, drops on button release
 *      sticks = 1, sticky style, stays up if initial release is close to initial press.
 *  Returns:
 *      0 on error condition
 *      1 on return from submenu to parent menu
 *      2 on button release return
 *
 ***********************************************************************/
int
UpdateMenu (int sticks)
{
  int done = 0;
  int retval;
  MenuItem *InitialMI;
  MenuRoot *actual_mr;
  int x_init, y_init, x, y;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_x, dummy_y;
  unsigned int dummy_mask;

  XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &dummy_x, &dummy_y, &x_init, &y_init, &dummy_mask);

  FindEntry ();
  InitialMI = ActiveItem;

  while (True)
  {
    /* block until there is an event */
    XMaskEvent (dpy, ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | VisibilityChangeMask | ButtonMotionMask, &Event);
#ifdef REQUIRES_STASHEVENT
    StashEventTime (&Event);
#endif
    done = 0;
    if (Event.type == MotionNotify)
    {
      /* discard any extra motion events before a release */
      while ((XCheckMaskEvent (dpy, ButtonMotionMask | ButtonReleaseMask, &Event)) && (Event.type != ButtonRelease));
      {
#ifdef REQUIRES_STASHEVENT
	StashEventTime (&Event);
#endif
      }
    }
    /* Handle a limited number of key press events to allow mouseless
     * operation */
    if (Event.type == KeyPress)
      menuShortcuts (&Event);

    switch (Event.type)
    {
    case ButtonRelease:
      /* The following lines holds the menu when the button is released */
      if (sticks)
      {
	sticks = 0;
	break;
      }
      if (!ActiveItem && (menu_on > 1))
      {
	int x, y;
	XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &dummy_x, &dummy_y, &x, &y, &dummy_mask);
	if ((dummy_child != None) && (XFindContext (dpy, dummy_child, MenuContext, (caddr_t *) &actual_mr) != XCNOENT) && (actual_mr != ActiveMenu))
	{
	  done = 1;
	  break;
	}
      }
      PopDownMenu ();
      if (ActiveItem)
      {
	done = 1;
	ExecuteFunction (ActiveItem->action, ButtonWindow, &Event, Context, -1);
      }
      ActiveItem = NULL;
      ActiveMenu = NULL;
      menuFromFrameOrWindowOrTitlebar = False;
      return MENU_DONE;

    case KeyPress:
    case VisibilityNotify:
    case ButtonPress:
      done = 1;
      break;

    case MotionNotify:
      XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &dummy_x, &dummy_y, &x, &y, &dummy_mask);
      if (((x - x_init > 3) || (x_init - x > 3)) && ((y - y_init > 3) || (y_init - y > 3)))
	mouse_moved = 1;
      done = 1;

      retval = FindEntry ();
      if (ActiveItem != InitialMI)
	sticks = 0;
      if ((retval == MENU_DONE) || (retval == SUBMENU_DONE))
      {
	PopDownMenu ();
	ActiveItem = NULL;
	ActiveMenu = NULL;
	menuFromFrameOrWindowOrTitlebar = False;
      }

      if (retval == MENU_DONE)
	return MENU_DONE;
      else if (retval == SUBMENU_DONE)
	return MENU_NOP;
      break;

    case Expose:
      /* grab our expose events, let the rest go through */
      if ((XFindContext (dpy, Event.xany.window, MenuContext, (caddr_t *) & actual_mr) != XCNOENT))
      {
	PaintMenu (actual_mr);
	done = 1;
      }
      break;

    default:
      break;
    }

    if (!done)
      DispatchEvent ();
  }
}


/***********************************************************************
 *
 *  Procedure:
 *	PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *      center	- whether or not to center horizontally over position
 *
 ***********************************************************************/
Bool PopUpMenu (MenuRoot * menu, int x, int y)
{

  if ((!menu) || (menu->w == None) || (menu->items == 0) || (menu->in_use))
    return False;

  menu_on++;
  InstallRootColormap ();

  /* pop up the menu */
  ActiveMenu = menu;
  ActiveItem = NULL;

  Stashed_X = x;
  Stashed_Y = y;

  x -= (menu->width >> 1);
  y -= (Scr.EntryHeight >> 1);

  if ((Tmp_win) && (menu_on == 1) && (Context & (C_LALL | C_RALL | C_TITLE)))
  {
    y = Tmp_win->frame_y + Tmp_win->boundary_width + Tmp_win->title_height;
  }

  /* clip to screen */
  if (x + menu->width > MyDisplayMaxX (x, y))
  {
    x = MyDisplayMaxX (x, y) - menu->width;
  }
  if (x < 0)
  {
    x = 0;
  }
  if (y + menu->height > MyDisplayMaxY (x, y))
  {
    y = MyDisplayMaxY (x, y) - menu->height;
  }
  if (y < 0)
  {
    y = 0;
  }
  MenuY = y;
  XMoveWindow (dpy, menu->w, x, y);
  XMapRaised (dpy, menu->w);
  menu->in_use = True;
  UninstallRootColormap ();
  return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************/
void
PopDownMenu ()
{
  if (ActiveMenu == NULL)
    return;

  menu_on--;

  if (ActiveItem)
    ActiveItem->state = 0;

  XUnmapWindow (dpy, ActiveMenu->w);
  if (!menu_on)
  {
    UninstallRootColormap ();
    UngrabEm ();
    WaitForButtonsUp ();
    XFlush (dpy);
  }
  discard_events (EnterWindowMask | LeaveWindowMask);  
  if (Context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    menuFromFrameOrWindowOrTitlebar = True;
  else
    menuFromFrameOrWindowOrTitlebar = False;
  ActiveMenu->in_use = False;
}

/***************************************************************************
 * 
 * Wait for all mouse buttons to be released 
 * This can ease some confusion on the part of the user sometimes 
 * 
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void
WaitForButtonsUp ()
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;
  /* Dummy var for XQueryPointer */
  Window dummy_root, dummy_child;
  int dummy_x, dummy_y, dummy_win_x, dummy_win_y;

  while (!AllUp)
  {
    XAllowEvents (dpy, ReplayPointer, CurrentTime);
    XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &dummy_x, &dummy_y, &dummy_win_x, &dummy_win_y, &mask);

    if ((mask & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) == 0)
      AllUp = True;
  }
  XSync (dpy, 0);
  while (XCheckMaskEvent (dpy, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, &JunkEvent))
  {
#ifdef REQUIRES_STASHEVENT
    StashEventTime (&JunkEvent);
#endif
    XAllowEvents (dpy, ReplayPointer, CurrentTime);
  }
}

void
DestroyMenu (MenuRoot * mr)
{
  MenuItem *mi, *tmp2;
  MenuRoot *tmp, *prev;

  if (mr == NULL)
    return;

  tmp = Scr.AllMenus;
  prev = NULL;
  while ((tmp != NULL) && (tmp != mr))
  {
    prev = tmp;
    tmp = tmp->next;
  }
  if (tmp != mr)
    return;

  if (prev == NULL)
    Scr.AllMenus = mr->next;
  else
    prev->next = mr->next;

  XDestroyWindow (dpy, mr->w);
  XDeleteContext (dpy, mr->w, MenuContext);
  if (mr->name)
    free (mr->name);

  /* need to free the window list ? */
  mi = mr->first;
  while (mi != NULL)
  {
    tmp2 = mi->next;
    if (mi->item != NULL)
      free (mi->item);
    if (mi->item2 != NULL)
      free (mi->item2);
    if (mi->action != NULL)
      free (mi->action);
    free (mi);
    mi = tmp2;
  }
  free (mr);
}

/****************************************************************************
 * 
 * Generates the windows for all menus
 *
 ****************************************************************************/
void
MakeMenus (void)
{
  MenuRoot *mr;

  mr = Scr.AllMenus;
  while (mr != NULL)
  {
    MakeMenu (mr);
    mr = mr->next;
  }
}

/****************************************************************************
 * 
 * Generates the window for a menu
 *
 ****************************************************************************/
void
MakeMenu (MenuRoot * mr)
{
  MenuItem *cur;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  XFontSet fontset = Scr.StdFont.fontset;
  int y, width;

  if ((mr->func != F_POPUP) || (!Scr.flags & WindowsCaptured))
    return;

  /* allow two pixels for top border */
  mr->width = 0;
  mr->width2 = 0;
  for (cur = mr->first; cur != NULL; cur = cur->next)
  {
    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, cur->item, cur->strlen, &rect1, &rect2);
      width = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (Scr.StdFont.xftfont))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) cur->item, cur->strlen, &extents);
      width = extents.xOff;
    }
#endif
    else
    {
      width = XTextWidth (Scr.StdFont.font, cur->item, cur->strlen);
    }
    if (cur->func_type == F_POPUP)
      width += 15;
    if (width <= 0)
      width = 1;
    if (width > mr->width)
      mr->width = width;

    if (fontset)
    {
      XRectangle rect1, rect2;
      XmbTextExtents (fontset, cur->item2, cur->strlen2, &rect1, &rect2);
      width = rect2.width;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if ((enable_xft) && (Scr.StdFont.xftfont))
    {
      XGlyphInfo extents;

      XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) cur->item2, cur->strlen2, &extents);
      width = extents.xOff;
    }
#endif
    else
    {
      width = XTextWidth (Scr.StdFont.font, cur->item2, cur->strlen2);
    }
    if (width < 0)
      width = 0;
    if (width > mr->width2)
      mr->width2 = width;
    if ((width == 0) && (cur->strlen2 > 0))
      mr->width2 = 1;
  }

  /* lets first size the window accordingly */
  mr->width += 10;
  if (mr->width2 > 0)
    mr->width += 5;
  /* Force even width */
  mr->width += (mr->width & 1);

  for (y = 2, cur = mr->first; cur != NULL; cur = cur->next)
  {
    cur->y_offset = y;
    cur->x = 5;
    if (cur->func_type == F_TITLE)
    {
      /* Title */
      if (cur->strlen2 == 0)
      {
	int width;
	if (fontset)
	{
	  XRectangle rect1, rect2;
	  XmbTextExtents (fontset, cur->item, cur->strlen, &rect1, &rect2);
	  width = rect2.width;
	}
#ifdef HAVE_X11_XFT_XFT_H
	else if ((enable_xft) && (Scr.StdFont.xftfont))
	{
	  XGlyphInfo extents;

	  XftTextExtents8 (dpy, Scr.StdFont.xftfont, (XftChar8 *) cur->item, cur->strlen, &extents);
	  width = extents.xOff;
	}
#endif
	else
	{
	  width = XTextWidth (Scr.StdFont.font, cur->item, cur->strlen);
	}
	cur->x = (mr->width + mr->width2 - width) >> 1;
      }
      if ((cur->strlen > 0) || (cur->strlen2 > 0))
	cur->y_height = Scr.EntryHeight + HEIGHT_EXTRA_TITLE;
      else
	cur->y_height = HEIGHT_SEPARATOR;
    }
    else if ((cur->strlen == 0) && (cur->strlen2 == 0))
      /* Separator */
      cur->y_height = HEIGHT_SEPARATOR;
    else
      /* Normal text entry */
      cur->y_height = Scr.EntryHeight;
    y += cur->y_height;
    if (mr->width2 == 0)
    {
      cur->x2 = cur->x;
    }
    else
    {
      cur->x2 = mr->width - 5;
    }
  }
  mr->in_use = 0;
  mr->height = y + 2;

  valuemask = (CWBackPixel | CWEventMask | CWCursor | CWSaveUnder);
  attributes.background_pixel = Scr.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.XfwmCursors[MENU];
  attributes.save_under = True;
  if (mr->w != None)
  {
    XDestroyWindow (dpy, mr->w);
    XDeleteContext (dpy, mr->w, MenuContext);
  }
  mr->width = mr->width + mr->width2;
  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) (mr->width), (unsigned int) mr->height, (unsigned int) 0, CopyFromParent, (unsigned int) InputOutput, (Visual *) CopyFromParent, valuemask, &attributes);
  XSaveContext (dpy, mr->w, MenuContext, (caddr_t) mr);
  return;
}

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a MenuItem
 * 							(pete@tecc.co.uk)
 * 
 * Inputs:
 *	it	- MenuItem to scan
 * 	which 	- +1 to look in it->item1 and -1 to look in it->item2.
 *
 ***********************************************************************/
void
scanForHotkeys (MenuItem * it, int which)
{
  char *start, *txt;

  start = (which > 0) ? it->item : it->item2;	/* Get start of string  */
  for (txt = start; *txt != '\0'; txt++)
  {
    /* Scan whole string      */
    if (*txt == '&')
    {				/* A hotkey marker?                     */
      if (txt[1] == '&')
      {				/* Just an escaped &                    */
	char *tmp;		/* Copy the string down over it */
	for (tmp = txt; *tmp != '\0'; tmp++)
	  tmp[0] = tmp[1];
	continue;		/* ...And skip to the key char          */
      }
      /* It's a hot key marker - work out the offset value          */
      it->hotkey = (1 + (txt - start)) * which;
      for (; *txt != '\0'; txt++)
	txt[0] = txt[1];	/* Copy down..  */
      return;			/* Only one hotkey per item...  */
    }
  }
  it->hotkey = 0;		/* No hotkey found.  Set offset to zero */
}

/***********************************************************************
 *
 *  Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *	(MenuItem *)
 *
 *  Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *	func	- the numeric function
 *
 ***********************************************************************/
void
AddToMenu (MenuRoot * menu, char *item, char *action)
{
  MenuItem *tmp;
  char *start, *end;

  if (item == NULL)
    return;

  tmp = (MenuItem *) safemalloc (sizeof (MenuItem));
  if (menu->first == NULL)
  {
    menu->first = tmp;
    tmp->prev = NULL;
  }
  else
  {
    menu->last->next = tmp;
    tmp->prev = menu->last;
  }
  menu->last = tmp;

  start = item;
  end = item;
  while ((*end != '\t') && (*end != 0))
    end++;
  tmp->item = safemalloc (end - start + 1);
  strncpy (tmp->item, start, end - start);
  tmp->item[end - start] = 0;
  tmp->item2 = NULL;
  if (*end == '\t')
  {
    start = end + 1;
    while (*end != 0)
      end++;
    if (end - start != 0)
    {
      tmp->item2 = safemalloc (end - start + 1);
      strncpy (tmp->item2, start, end - start);
      tmp->item2[end - start] = 0;
    }
  }
  if (item != (char *) 0)
  {
    scanForHotkeys (tmp, 1);	/* pete@tecc.co.uk */
    tmp->strlen = strlen (tmp->item);
  }
  else
    tmp->strlen = 0;

  if (tmp->item2 != (char *) 0)
  {
    if (tmp->hotkey == 0)
      scanForHotkeys (tmp, -1);	/* pete@tecc.co.uk */
    tmp->strlen2 = strlen (tmp->item2);
  }
  else
    tmp->strlen2 = 0;

  tmp->action = stripcpy (action);
  tmp->next = NULL;
  tmp->state = 0;
  tmp->func_type = find_func_type (tmp->action);
  tmp->item_num = menu->items++;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************/
MenuRoot *
NewMenuRoot (char *name, int junk)
{
  MenuRoot *tmp;

  tmp = (MenuRoot *) safemalloc (sizeof (MenuRoot));
  if (junk == 0)
    tmp->func = F_POPUP;
  else
    tmp->func = F_FUNCTION;

  tmp->name = stripcpy (name);
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->w = None;
  tmp->next = Scr.AllMenus;
  Scr.AllMenus = tmp;
  return (tmp);
}



/***********************************************************************
 * change by KitS@bartley.demon.co.uk to correct popups off title buttons
 *
 *  Procedure:
 *ButtonPosition - find the actual position of the button
 *                 since some buttons may be disabled
 *
 *  Returned Value:
 *The button count from left or right taking in to account
 *that some buttons may not be enabled for this window
 *
 *  Inputs:
 *      context - context as per the global Context
 *      t       - the window (XfwmWindow) to test against
 *
 ***********************************************************************/
int
ButtonPosition (int context, XfwmWindow * t)
{
  int i;
  int buttons = -1;

  if (context & C_RALL)
  {
    for (i = 0; i < Scr.nr_right_buttons; i++)
    {
      if (t->right_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1 << i) * C_R1) & context)
	return (buttons);
    }
  }
  else
  {
    for (i = 0; i < Scr.nr_left_buttons; i++)
    {
      if (t->left_w[i])
      {
	buttons++;
      }
      /* is this the button ? */
      if (((1 << i) * C_L1) & context)
	return (buttons);
    }
  }
  /* you never know... */
  return 0;
}
