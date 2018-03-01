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

/****************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/


#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "utils.h"
#include "stack.h"
#include "xinerama.h"

#ifdef HAVE_IMLIB
#include <Imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#if !defined(MAX_TITLE_LEN)
#define MAX_TITLE_LEN 80
#endif

#if !defined(MAX_ICON_LEN)
#define MAX_ICON_LEN 80
#endif

#if 0
extern char *charset;
#endif
char NoName[] = "Untitled";		/* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass";		/* Class if no res_class in class hints */
char NoResource[] = "NoResource";	/* Class if no res_name in class hints */

#ifdef HAVE_IMLIB
extern ImlibData *imlib_id;
#endif

static int ButtonGrabs = 0;

int
check_existfile (char *filename)
{
  struct stat s;
  int status;

  if ((!filename) || (!strlen (filename)))
    return (0);

  status = stat (filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return (1);
  return (0);
}

char *
bound_name (char *s, int max_len)
{
  char *res;
  int length = strlen (s);
  
  if (length > max_len)
  {
    res = (char *) safemalloc (max_len + 5);
    snprintf (res, max_len, "%s", s);
    strcat (res, "...");
  }
  else
  {
    res = (char *) safemalloc (length + 1);
    strcpy (res, s);
  }
  
  return (res);
}

void
GetWMName (XfwmWindow * t)
{
  XTextProperty text_prop;
  char *cp;

  if (XGetWMName (dpy, t->w, &text_prop) != 0)
  {
    char **text_list = NULL;
    int text_list_num;
    if (XmbTextPropertyToTextList (dpy, &text_prop, &text_list, &text_list_num) == Success)
    {
      if (text_list)
      {
        t->name = bound_name (text_list[0], MAX_TITLE_LEN);
	XFreeStringList (text_list);
      }
    }
    else
    {
      t->name = bound_name ((char *) text_prop.value, MAX_TITLE_LEN);
    }
    XFree (text_prop.value);
  }
  if (!(t->name))
  {
    t->name = (char *) safemalloc (strlen (NoName) + 1);
    strcpy (t->name, NoName);
  }
  for (cp = t->name; *cp; cp++)
  {
    if ((*cp == '\n') || (*cp == '\r')) *cp = ' ';
  }
}

void
GetWMIconName (XfwmWindow * t)
{
  XTextProperty text_prop;
  char *cp;
  
  if (XGetWMIconName (dpy, t->w, &text_prop) != 0)
  {
    char **text_list = NULL;
    int text_list_num;
    if (XmbTextPropertyToTextList (dpy, &text_prop, &text_list, &text_list_num) == Success)
    {
      if (text_list)
      {
        t->icon_name = bound_name (text_list[0], MAX_ICON_LEN);
	XFreeStringList (text_list);
      }
    }
    else
    {
      t->icon_name = bound_name ((char *) text_prop.value, MAX_ICON_LEN);
    }
    XFree (text_prop.value);
  }
  if (!(t->icon_name))
  {
    if (t->name)
    {
      t->icon_name = bound_name (t->name, MAX_ICON_LEN);
    }
    else
    {
      t->icon_name = (char *) safemalloc (strlen (NoName) + 1);
      strcpy (t->icon_name, NoName);
    }
  }
  for (cp = t->icon_name; *cp; cp++)
  {
    if ((*cp == '\n') || (*cp == '\r')) *cp = ' ';
  }
}

/**************************************************************************
 * 
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void
free_window_names (XfwmWindow * tmp, Bool nukename, Bool nukeicon)
{

  if (!tmp)
  {
    xfwm_msg (WARN, "free_window_names", "Cannot unallocate");
    return;
  }

  if (nukename && nukeicon)
  {
    if ((tmp->name != NoName) && (tmp->name != NULL))
    {
      free (tmp->name);
    }
    tmp->name = NULL;
    if ((tmp->icon_name != NoName) && (tmp->icon_name != NULL))
    {
      free (tmp->icon_name);
    }
    tmp->icon_name = NULL;
  }
  else if (nukename)
  {
    if ((tmp->name != NoName) && (tmp->name != NULL))
    {
      free (tmp->name);
    }
    tmp->name = NULL;
  }
  else
  {				/* if (nukeicon) */
    if ((tmp->icon_name != NoName) && (tmp->icon_name != NULL))
    {
      free (tmp->icon_name);
    }
    tmp->icon_name = NULL;
  }

  return;
}

void
RevertFocus (XfwmWindow * tmp_win, Bool fallback_to_itself)
{
  XfwmWindow *Win;

  if ((!tmp_win) || (Scr.Focus != tmp_win))
    return;
  
#ifdef DEBUG
  fprintf (stderr, "xfwm : RevertFocus () : Entering routine\n");
#endif

  Win = tmp_win->next;
  while (Win && (!AcceptInput(Win) || (Win == tmp_win) || (Win->flags & ICONIFIED) || (tmp_win->Desk != Win->Desk)))
  {
    Win = Win->next;
  }
 
  if ((Win) && (Win != &Scr.XfwmRoot))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : RevertFocus () : Setting focus\n");
#endif
    SetFocus (Win->w, Win, True, False);
#ifdef DEBUG
    fprintf (stderr, "xfwm : RevertFocus () : Leaving routine\n");
#endif
    return;
  }

  if ((fallback_to_itself) && AcceptInput(tmp_win))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : RevertFocus () : Revert focus itself\n");
#endif
    SetFocus (tmp_win->w, tmp_win, True, False);
#ifdef DEBUG
    fprintf (stderr, "xfwm : RevertFocus () : Leaving routine\n");
#endif
    return;
  }

#ifdef DEBUG
  fprintf (stderr, "xfwm : RevertFocus () : Setting focus to NoFocusWin\n");
#endif
  SetFocus (Scr.NoFocusWin, NULL, False, False);
#ifdef DEBUG
  fprintf (stderr, "xfwm : RevertFocus () : Leaving routine\n");
#endif
}
/***************************************************************************
 *
 * Handles destruction of a window 
 *
 ****************************************************************************/
void
Destroy (XfwmWindow * tmp_win)
{
  int i;
  extern XfwmWindow *ButtonWindow;
  extern XfwmWindow *colormap_win;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Destroy () : Entering routine\n");
#endif

  if (!tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Destroy () : tmp_win == NULL : Leaving routine\n");
#endif
    return;
  }

  XDeleteContext (dpy, tmp_win->w,      XfwmContext);
  XDeleteContext (dpy, tmp_win->Parent, XfwmContext);
  XDeleteContext (dpy, tmp_win->frame,  XfwmContext);
  
  Broadcast (XFCE_M_DESTROY_WINDOW, 3, tmp_win->w, tmp_win->frame, (unsigned long) tmp_win, 0, 0, 0, 0);

  /* Removing window from internal window stack */
  if (tmp_win->prev != NULL)
    tmp_win->prev->next = tmp_win->next;
  if (tmp_win->next != NULL)
    tmp_win->next->prev = tmp_win->prev;
  Scr.stacklist = RemoveFromXfwmWindowList (Scr.stacklist, tmp_win);

  if (Scr.LastWindowRaised == tmp_win)
  {
    Scr.LastWindowRaised = NULL;
  }

  if (Scr.LastWindowLowered == tmp_win)
  {
    Scr.LastWindowLowered = NULL;
  }

  if (Scr.Hilite == tmp_win)
  {
    Scr.Hilite = NULL;
  }

  if (Scr.PreviousFocus == tmp_win)
  {
    Scr.PreviousFocus = NULL;
  }

  if (ButtonWindow == tmp_win)
  {
    ButtonWindow = NULL;
  }

  if (Scr.pushed_window == tmp_win)
  {
    Scr.pushed_window = NULL;
  }

  if (tmp_win == colormap_win)
  {
    colormap_win = NULL;
  }

  if (Scr.Focus == tmp_win)
  {
    RevertFocus (tmp_win, False);
  }

  if (tmp_win->icon_pixmap_w != None)
  {
    XDeleteContext (dpy, tmp_win->icon_pixmap_w, XfwmContext);
    if (tmp_win->flags & ICON_OURS)
    {
      XDestroyWindow (dpy, tmp_win->icon_pixmap_w);
    }
  }
  if ((tmp_win->icon_w) && (tmp_win->flags & PIXMAP_OURS))
  {
#ifdef HAVE_IMLIB
    Imlib_free_pixmap (imlib_id, tmp_win->iconPixmap);
#else
    XFreePixmap (dpy, tmp_win->iconPixmap);
#endif
  }
  if (tmp_win->icon_w != None)
  {
    XDeleteContext (dpy, tmp_win->icon_w, XfwmContext);
    XDestroyWindow (dpy, tmp_win->icon_w);
  }
  if (tmp_win->flags & TITLE)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Destroy () : Destroying title_w\n");
#endif
    XDeleteContext (dpy, tmp_win->title_w, XfwmContext);
    for (i = 0; i < Scr.nr_left_buttons; i++)
      if (tmp_win->left_w[i] != None)
      {
#ifdef DEBUG
        fprintf (stderr, "xfwm : Destroy () : Destroying left_w[%i]\n", i);
#endif
	XDeleteContext (dpy, tmp_win->left_w[i], XfwmContext);
	tmp_win->left_w[i] = None;
      }
    for (i = 0; i < Scr.nr_right_buttons; i++)
      if (tmp_win->right_w[i] != None)
      {
#ifdef DEBUG
        fprintf (stderr, "xfwm : Destroy () : Destroying right_w[%i]\n", i);
#endif
	XDeleteContext (dpy, tmp_win->right_w[i], XfwmContext);
	tmp_win->right_w[i] = None;
      }
  }
  if (tmp_win->flags & BORDER)
  {
    for (i = 0; i < 4; i++)
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : Destroy () : Destroying sides[%i]\n", i);
#endif
      XDeleteContext (dpy, tmp_win->sides[i], XfwmContext);
      tmp_win->sides[i] = None;

#ifdef DEBUG
      fprintf (stderr, "xfwm : Destroy () : Destroying corners[%i]\n", i);
#endif
      XDeleteContext (dpy, tmp_win->corners[i], XfwmContext);
      tmp_win->corners[i] = None;
    }
  }
  free_window_names (tmp_win, True, True);
  if (tmp_win->wmhints)
    XFree (tmp_win->wmhints);
  if (tmp_win->class.res_name && tmp_win->class.res_name != NoResource)
    XFree (tmp_win->class.res_name);
  if (tmp_win->class.res_class && tmp_win->class.res_class != NoClass)
    XFree (tmp_win->class.res_class);
  if (tmp_win->mwm_hints)
    XFree (tmp_win->mwm_hints);
  if (tmp_win->cmap_windows)
    XFree (tmp_win->cmap_windows);
#ifdef DEBUG
  fprintf (stderr, "xfwm : Destroy () : Destroying frame\n");
#endif
  XDestroyWindow (dpy, tmp_win->frame);
#ifdef DEBUG
  fprintf (stderr, "xfwm : Destroy () : Freeing data\n");
#endif
  mymemset ((char *) tmp_win, 0, sizeof(XfwmWindow));
  free (tmp_win);
#ifdef DEBUG
  fprintf (stderr, "xfwm : Destroy () : Leaving routine\n");
#endif
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 * 
 *  Puts windows back where they were before xfwm took over 
 *
 ************************************************************************/
void
RestoreWithdrawnLocation (XfwmWindow * tmp, Bool restart)
{
  int x, y;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  unsigned int dummy_width, dummy_height, dummy_depth, dummy_bw;

  if (!tmp)
    return;

  if (XGetGeometry (dpy, tmp->w, &dummy_root, &x, &y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth))
  {
    if ((tmp->flags & ICONIFIED) && (!(tmp->flags & SUPPRESSICON)))
    {
      if (tmp->icon_w)
	XUnmapWindow (dpy, tmp->icon_w);
      if (tmp->icon_pixmap_w)
	XUnmapWindow (dpy, tmp->icon_pixmap_w);
    }

    GetGravityOffsets (tmp);
    XReparentWindow (dpy, tmp->w, Scr.Root, tmp->frame_x - tmp->gravx, tmp->frame_y - tmp->gravy);
    XSetWindowBorderWidth (dpy, tmp->w, tmp->old_bw);
    XSync (dpy, 0);
  }
}

/***************************************************************************
 *
 * Start/Stops the auto-raise timer
 *
 ****************************************************************************/
void
SetTimer (int delay)
{
  struct itimerval value;

  value.it_value.tv_usec = 1000 * (delay % 1000);
  value.it_value.tv_sec = delay / 1000;
  value.it_interval.tv_usec = 0;
  value.it_interval.tv_sec = 0;
  setitimer (ITIMER_REAL, &value, NULL);
}

int
GetTwoArguments (char *action, int *val1, int *val2, int *val1_unit, int *val2_unit, int x, int y)
{
  char c1, c2;
  int n;

  *val1 = 0;
  *val2 = 0;
  *val1_unit = MyDisplayWidth (x, y);
  *val2_unit = MyDisplayHeight (x, y);

  n = sscanf (action, "%d %d", val1, val2);
  if (n == 2)
    return 2;

  c1 = 's';
  c2 = 's';
  n = sscanf (action, "%d%c %d%c", val1, &c1, val2, &c2);

  if (n != 4)
    return 0;

  if ((c1 == 'p') || (c1 == 'P'))
    *val1_unit = 100;

  if ((c2 == 'p') || (c2 == 'P'))
    *val2_unit = 100;

  return 2;
}


int
GetOneArgument (char *action, long *val1, int *val1_unit, int x, int y)
{
  char c1;
  int n;

  *val1 = 0;
  *val1_unit = MyDisplayWidth (x, y);

  n = sscanf (action, "%ld", val1);
  if (n == 1)
    return 1;

  c1 = '%';
  n = sscanf (action, "%ld%c", val1, &c1);

  if (n != 2)
    return 0;

  if ((c1 == 'p') || (c1 == 'P'))
    *val1_unit = 100;

  return 1;
}

/*
 * A handy function to grab a button along with its modifier, ignoring other "fake"
 * modifiers
 */
void
MyXGrabButton (Display * dpi, unsigned int button, unsigned int modifiers, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor)
{
  if ((modifiers == AnyModifier) || (modifiers == 0))
  {
    XGrabButton (dpi, button, modifiers, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
  }
  else
  {
    XGrabButton (dpi, button, modifiers, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | ScrollLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | NumLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | CapsLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | ScrollLockMask | NumLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | ScrollLockMask | CapsLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | CapsLockMask | NumLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    XGrabButton (dpi, button, modifiers | ScrollLockMask | CapsLockMask | NumLockMask, grab_window, owner_events, event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
  }
}

void
MyXUngrabButton (Display * dpi, unsigned int button, unsigned int modifiers, Window grab_window)
{
  if ((modifiers == AnyModifier) || (modifiers == 0))
  {
    XUngrabButton (dpi, button, modifiers, grab_window);
  }
  else
  {
    XUngrabButton (dpi, button, modifiers, grab_window);
    XUngrabButton (dpi, button, modifiers | ScrollLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | NumLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | CapsLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | ScrollLockMask | NumLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | ScrollLockMask | CapsLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | CapsLockMask | NumLockMask, grab_window);
    XUngrabButton (dpi, button, modifiers | ScrollLockMask | CapsLockMask | NumLockMask, grab_window);
  }
}

/*
 * Idem for keys
 */
void
MyXGrabKey (Display * dpi, int keycode, unsigned int modifiers, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode)
{
  if ((modifiers == AnyModifier) || (modifiers == 0))
  {
    XGrabKey (dpi, keycode, modifiers, grab_window, owner_events, pointer_mode, keyboard_mode);
  }
  else
  {
    XGrabKey (dpi, keycode, modifiers, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | ScrollLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | NumLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | CapsLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | ScrollLockMask | NumLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | ScrollLockMask | CapsLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | CapsLockMask | NumLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
    XGrabKey (dpi, keycode, modifiers | ScrollLockMask | CapsLockMask | NumLockMask, grab_window, owner_events, pointer_mode, keyboard_mode);
  }
}

void
MyXUngrabKey (Display * dpi, int keycode, unsigned int modifiers, Window grab_window)
{
  if ((modifiers == AnyModifier) || (modifiers == 0))
  {
    XUngrabKey (dpi, keycode, modifiers, grab_window);
  }
  else
  {
    XUngrabKey (dpi, keycode, modifiers, grab_window);
    XUngrabKey (dpi, keycode, modifiers | ScrollLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | NumLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | CapsLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | ScrollLockMask | NumLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | ScrollLockMask | CapsLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | CapsLockMask | NumLockMask, grab_window);
    XUngrabKey (dpi, keycode, modifiers | ScrollLockMask | CapsLockMask | NumLockMask, grab_window);
  }
}

/*****************************************************************************
 *
 * Grab the pointer and keyboard
 *
 ****************************************************************************/
Bool GrabEm (int cursor)
{
  int i = 0, val = 0;
  unsigned int mask = (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask);
  Cursor vs = None;

#ifdef DEBUG
  fprintf (stderr, "xfwm : GrabEm () : Entering routine\n");
#endif
  vs = ((cursor >= 0) ? Scr.XfwmCursors[cursor] : None);
  if (ButtonGrabs > 0)
  {
    ButtonGrabs++;
#ifdef DEBUG
    fprintf (stderr, "xfwm : GrabEm () : Already grabbed\n");
    fprintf (stderr, "xfwm : GrabEm () : Grab #%i\n", ButtonGrabs);
#endif
    XChangeActivePointerGrab (dpy, mask, vs, CurrentTime);
#ifdef DEBUG
    fprintf (stderr, "xfwm : GrabEm () : Leaving routine\n");
#endif
    return True;
  }
  XSync (dpy, 0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if (Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
#ifdef REQUIRES_STASHEVENT
  XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, lastTimestamp);
#else
  XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);
#endif
  XSync (dpy, 0);
  while ((i < 1000) && (val = XGrabPointer (dpy, Scr.Root, True, mask, GrabModeAsync, GrabModeAsync, Scr.Root, vs, CurrentTime) != GrabSuccess))
  {
    i++;
    /* If you go too fast, other windows may not get a change to release
     * any grab that they have. */
    sleep_a_little (1000);
  }

  /* If we fall out of the loop without grabbing the pointer, its
   * time to give up */
  if (val != GrabSuccess)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : GrabEm () : Cannot grab\n");
    fprintf (stderr, "xfwm : GrabEm () : Leaving routine\n");
#endif
    return False;
  }

  ButtonGrabs++;
#ifdef DEBUG
  fprintf (stderr, "xfwm : GrabEm () : Grab ok (#%i)\n", ButtonGrabs);
  fprintf (stderr, "xfwm : GrabEm () : Leaving routine\n");
#endif
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void
UngrabEm (void)
{
  Window w;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_bw, dummy_depth;

#ifdef DEBUG
  fprintf (stderr, "xfwm : UngrabEm () : Entering routine\n");
#endif
  if (--ButtonGrabs < 0)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : UngrabEm () : Error too many UngrabEm () calls\n");
#endif
    ButtonGrabs = 0;
  }
  if (ButtonGrabs > 0)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : UngrabEm () : Grab stack not empty\n");
    fprintf (stderr, "xfwm : UngrabEm () : Leaving routine\n");
#endif
    return;
  }
  XSync (dpy, 0);
  XUngrabPointer (dpy, CurrentTime);
  XSync (dpy, 0);
  if (Scr.PreviousFocus != NULL)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : UngrabEm () : Scr.PreviousFocus set\n");
#endif
    MyXGrabServer (dpy);
    if ((Scr.PreviousFocus->flags & ICONIFIED) && (Scr.PreviousFocus->icon_w))
    {
      w = Scr.PreviousFocus->icon_w;
    }
    else
    {
      w = Scr.PreviousFocus->w;
    }
    if (XGetGeometry (dpy, w, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth))
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : UngrabEm () : Calling SetFocus on %s\n", Scr.PreviousFocus->name);
#endif
#ifdef REQUIRES_STASHEVENT
      XSetInputFocus (dpy, w, RevertToParent, lastTimestamp);
#else
      XSetInputFocus (dpy, w, RevertToParent, CurrentTime);
#endif
      Scr.PreviousFocus = NULL;
    }
    MyXUngrabServer (dpy);
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : UngrabEm () : Leaving routine\n");
#endif
}

/**************************************************************************
 * 
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void
UnmapIt (XfwmWindow * t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;

  if (!t)
    return;

  XGetWindowAttributes (dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput (dpy, t->w, eventMask & ~StructureNotifyMask);
  XFlush (dpy);
  if (t->flags & ICONIFIED)
  {
    if (t->icon_pixmap_w != None)
      XUnmapWindow (dpy, t->icon_pixmap_w);
    if (t->icon_w != None)
      XUnmapWindow (dpy, t->icon_w);
  }
  else if (t->flags & (MAPPED | MAP_PENDING))
  {
    XUnmapWindow (dpy, t->w);
    XUnmapWindow (dpy, t->frame);
  }
  XSelectInput (dpy, t->w, eventMask);
  XFlush (dpy);
}

/**************************************************************************
 * 
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void
MapIt (XfwmWindow * t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;

  if (!t)
    return;

  XGetWindowAttributes (dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput (dpy, t->w, (eventMask & ~StructureNotifyMask));
  XFlush (dpy);
  if (t->flags & ICONIFIED)
  {
    if (t->icon_pixmap_w != None)
      XMapWindow (dpy, t->icon_pixmap_w);
    if (t->icon_w != None)
      XMapWindow (dpy, t->icon_w);
  }
  else if (t->flags & MAPPED)
  {
    XMapWindow (dpy, t->frame);
    XMapWindow (dpy, t->Parent);
    XMapWindow (dpy, t->w);
  }
  XSelectInput (dpy, t->w, eventMask);
  XFlush (dpy);
}

/*
 * ** xfwm_msg: used to send output from xfwm to files and or stderr/stdout
 * **
 * ** type -> DBG == Debug, ERR == Error, INFO == Information, WARN == Warning
 * ** id -> name of function, or other identifier
 */
void
xfwm_msg (int type, char *id, char *msg, ...)
{
  char *typestr;
  va_list args;

  switch (type)
  {
  case DBG:
    typestr = "Debug =";
    break;
  case ERR:
    typestr = "Error =";
    break;
  case WARN:
    typestr = "Warning =";
    break;
  case INFO:
  default:
    typestr = "";
    break;
  }

  va_start (args, msg);

  fprintf (stderr, "xfwm message (type %s): %s ", id, typestr);
  vfprintf (stderr, msg, args);
  fprintf (stderr, "\n");

  if (type == ERR)
  {
    char tmp[1024];		/* I hate to use a fixed length but this will do for now */
    sprintf (tmp, "xfwm message (type %s): %s ", id, typestr);
    vsprintf (tmp + strlen (tmp), msg, args);
    tmp[strlen (tmp) + 1] = '\0';
    tmp[strlen (tmp)] = '\n';
    BroadcastName (XFCE_M_ERROR, 0, 0, 0, tmp);
  }

  va_end (args);
}				/* xfwm_msg */
