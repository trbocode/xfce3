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
 * This module is based on Twm, but has been significantly modified 
 * by Rob Nation 
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 *
 * xfwm event handling
 *
 ***********************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef ISC
#include <sys/bsdtypes.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
/* Some people say that AIX and AIXV3 need 3 preceding underscores, other say
 * no. I'll do both */
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "utils.h"
#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "xfwm.h"
#include "module.h"
#include "stack.h"
#include "themes.h"
#include "xinerama.h"
#include "constant.h"
#include "session.h"

#include <X11/extensions/shape.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask);
extern int menuFromFrameOrWindowOrTitlebar;
extern int runlevel;

int Context = C_NO_CONTEXT;	/* current button press context */
int Button = 0;
XfwmWindow *ButtonWindow;	/* button press window structure */
XEvent Event;			/* the current event */
XfwmWindow *Tmp_win;		/* the current xfwm window */

static int alarmed;
static int old_x_root = 0;
static int old_y_root = 0;
#ifdef REQUIRES_STASHEVENT
Time lastTimestamp = CurrentTime;
#endif

extern int ShapeEventBase;
void HandleShapeNotify (void);

Window PressedW;

/*
 * ** LASTEvent is the number of X events defined - it should be defined
 * ** in X.h (to be like 35), but since extension (eg SHAPE) events are
 * ** numbered beyond LASTEvent, we need to use a bigger number than the
 * ** default, so let's undefine the default and use 256 instead.
 */
#undef LASTEvent
#ifndef LASTEvent
#define LASTEvent 256
#endif /* !LASTEvent */
typedef void (*PFEH) ();
PFEH EventHandlerJumpTable[LASTEvent];

void
enterAlarm (int nonsense)
{
  alarmed = True;
  signal (SIGALRM, enterAlarm);
}

#ifdef REQUIRES_STASHEVENT
/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Bool StashEventTime (XEvent * ev)
{
  Time NewTimestamp = CurrentTime;

  switch (ev->type)
  {
  case KeyPress:
  case KeyRelease:
    NewTimestamp = ev->xkey.time;
    break;
  case ButtonPress:
  case ButtonRelease:
    NewTimestamp = ev->xbutton.time;
    break;
  case MotionNotify:
    NewTimestamp = ev->xmotion.time;
    break;
  case EnterNotify:
  case LeaveNotify:
    NewTimestamp = ev->xcrossing.time;
    break;
  case PropertyNotify:
    NewTimestamp = ev->xproperty.time;
    break;
  case SelectionClear:
    NewTimestamp = ev->xselectionclear.time;
    break;
  case SelectionRequest:
    NewTimestamp = ev->xselectionrequest.time;
    break;
  case SelectionNotify:
    NewTimestamp = ev->xselection.time;
    break;
  default:
    return False;
  }
  /* Only update is the new timestamp is later than the old one, or
   * if the new one is from a time at least 30 seconds earlier than the
   * old one (in which case the system clock may have changed) */
  if ((NewTimestamp > CurrentTime) || ((CurrentTime - NewTimestamp) > 30000))
    lastTimestamp = NewTimestamp;

  return True;
}
#endif

int
flush_expose (Window w)
{
  XEvent dummy;
  int i = 0;

  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))
    i++;
  return i;
}

void
fast_process_expose (void)
{
  XEvent old_event;

  mymemcpy((char *) &old_event, (char *) &Event, sizeof(XEvent));
  while (XCheckMaskEvent (dpy, ExposureMask, &Event))
  {
    DispatchEvent ();
  }
  mymemcpy((char *) &Event, (char *) &old_event, sizeof(XEvent));
}

int discard_events(long event_mask)
{
  XEvent e;
  int count;

  for (count = 0; XCheckMaskEvent(dpy, event_mask, &e); count++)
  {
#ifdef REQUIRES_STASHEVENT
    StashEventTime(&e);
#endif
  }

  return count;
}

int discard_window_events(Window w, long event_mask)
{
  XEvent e;
  int count;

  for (count = 0; XCheckWindowEvent(dpy, w, event_mask, &e); count++)
  {
#ifdef REQUIRES_STASHEVENT
    StashEventTime(&e);
#endif
  }

  return count;
}

void
InitEventHandlerJumpTable (void)
{
  int i;

  for (i = 0; i < LASTEvent; i++)
  {
    EventHandlerJumpTable[i] = NULL;
  }
  EventHandlerJumpTable[Expose] = HandleExpose;
  EventHandlerJumpTable[DestroyNotify] = HandleDestroyNotify;
  EventHandlerJumpTable[MapRequest] = HandleMapRequest;
  EventHandlerJumpTable[MapNotify] = HandleMapNotify;
  EventHandlerJumpTable[UnmapNotify] = HandleUnmapNotify;
  EventHandlerJumpTable[ButtonPress] = HandleButtonPress;
  EventHandlerJumpTable[ButtonRelease] = HandleButtonRelease;
  EventHandlerJumpTable[EnterNotify] = HandleEnterNotify;
  EventHandlerJumpTable[LeaveNotify] = HandleLeaveNotify;
  EventHandlerJumpTable[FocusIn] = HandleFocusIn;
  EventHandlerJumpTable[FocusOut] = HandleFocusOut;
  EventHandlerJumpTable[ConfigureRequest] = HandleConfigureRequest;
  EventHandlerJumpTable[ClientMessage] = HandleClientMessage;
  EventHandlerJumpTable[PropertyNotify] = HandlePropertyNotify;
  EventHandlerJumpTable[KeyPress] = HandleKeyPress;
  EventHandlerJumpTable[VisibilityNotify] = HandleVisibilityNotify;
  EventHandlerJumpTable[ColormapNotify] = HandleColormapNotify;
  EventHandlerJumpTable[MotionNotify] = HandleMotionNotify;
  if (ShapesSupported)
    EventHandlerJumpTable[ShapeEventBase + ShapeNotify] = HandleShapeNotify;
}

/***********************************************************************
 *
 *  Procedure:
 *	DispatchEvent - handle a single X event stored in global var Event
 *
 ************************************************************************/
void
DispatchEvent ()
{
  Window w = Event.xany.window;

#ifdef REQUIRES_STASHEVENT
  StashEventTime (&Event);
#endif
  XFlush (dpy);
  if ((w == None) || (XFindContext (dpy, w, XfwmContext, (caddr_t *) &Tmp_win) == XCNOENT))
  {
    Tmp_win = NULL;
  }
  if (EventHandlerJumpTable[Event.type])
    (*EventHandlerJumpTable[Event.type]) ();

  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleEvents - handle X events
 *
 ************************************************************************/
void
HandleEvents ()
{
  while (!runlevel)
  {
    if (My_XNextEvent (dpy, &Event))
    {
      DispatchEvent ();
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	Find the Xfwm context for the Event.
 *
 ************************************************************************/
int
GetContext (XfwmWindow * t, XEvent * e, Window * w)
{
  int Context, i;

  if (!t)
    return C_ROOT;

  Context = C_NO_CONTEXT;
  *w = e->xany.window;

  if (*w == Scr.NoFocusWin)
    return C_ROOT;

  /* Since key presses and button presses are grabbed in the frame
   * when we have re-parented windows, we need to find out the real
   * window where the event occured */
  if ((e->type == KeyPress) && (e->xkey.subwindow != None))
    *w = e->xkey.subwindow;

  if ((e->type == ButtonPress) && (e->xbutton.subwindow != None) && ((e->xbutton.subwindow == t->w) || (e->xbutton.subwindow == t->Parent)))
    *w = e->xbutton.subwindow;

  if (*w == Scr.Root)
    Context = C_ROOT;
  if (t)
  {
    if (*w == t->title_w)
      Context = C_TITLE;
    if ((*w == t->w) || (*w == t->Parent))
      Context = C_WINDOW;
    if (*w == t->icon_w)
      Context = C_ICON;
    if (*w == t->icon_pixmap_w)
      Context = C_ICON;
    if (*w == t->frame)
      Context = C_SIDEBAR;
    for (i = 0; i < 4; i++)
      if (*w == t->corners[i])
      {
	Context = C_FRAME;
	Button = i;
      }
    for (i = 0; i < 4; i++)
      if (*w == t->sides[i])
      {
	Context = C_SIDEBAR;
	Button = i;
      }
    for (i = 0; i < Scr.nr_left_buttons; i++)
    {
      if (*w == t->left_w[i])
      {
	Context = (1 << i) * C_L1;
	Button = i;
      }
    }
    for (i = 0; i < Scr.nr_right_buttons; i++)
    {
      if (*w == t->right_w[i])
      {
	Context = (1 << i) * C_R1;
	Button = i;
      }
    }
  }
  return Context;
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleFocusIn - handles focus in events
 *
 ************************************************************************/
void
HandleFocusIn ()
{
  XEvent d;
  Window w;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleFocusIn ()\n");
#endif
  w = Event.xany.window;

  while (XCheckTypedEvent (dpy, FocusIn, &d))
  {
#ifdef REQUIRES_STASHEVENT
    StashEventTime (&d);
#endif
    if ((w = d.xany.window) == None) continue;
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleFocusIn () : Skipping event...\n");
    if (XFindContext (dpy, w, XfwmContext, (caddr_t *) &Tmp_win) != XCNOENT)
    {
       fprintf (stderr, "xfwm : HandleFocusIn () : ... FocusIn on \"%s\"\n", Tmp_win->name);
    }
#endif
  }

  if (w == None)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleFocusIn () : window is 'None'\n");
#endif
    return;
  }

  if (XFindContext(dpy, w, XfwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
  {
    Tmp_win = NULL;
  }

  if (!Tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleFocusIn () : Tmp_win not set\n");
#endif
    if (w != Scr.NoFocusWin)
    {
      Scr.UnknownWinFocused = w;
    }
    else
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : HandleFocusIn () : Unsetting focus from Scr.Hilite (if any)\n");
#endif
      Scr.Focus = NULL;
      SetBorder (Scr.Hilite, NULL, False, True, True, None);
      Broadcast (XFCE_M_FOCUS_CHANGE, 5, 0, 0, 0, Scr.DefaultDecor.HiColors.fore, Scr.DefaultDecor.HiColors.back, 0, 0);
      if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
      {
	if ((Scr.Hilite) && (!(Scr.Hilite->flags & ICONIFIED)))
	{
	  InstallWindowColormaps (Scr.Hilite);
	}
	else
	{
	  InstallWindowColormaps (NULL);
	}
      }
    }
  }
  else if (Tmp_win != Scr.Hilite)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleFocusIn () : Focus set to %s\n", Tmp_win->name);
#endif
    Scr.Focus = Tmp_win;
    SetBorder (Tmp_win, NULL, True, True, True, None);
    Broadcast (XFCE_M_FOCUS_CHANGE, 5, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, GetDecor (Tmp_win, HiColors.fore), GetDecor (Tmp_win, HiColors.back), 0, 0);
    if (Scr.ColormapFocus == COLORMAP_FOLLOWS_FOCUS)
    {
      if ((Scr.Hilite) && (!(Scr.Hilite->flags & ICONIFIED)))
      {
	InstallWindowColormaps (Scr.Hilite);
      }
      else
      {
	InstallWindowColormaps (NULL);
      }
    }
  }
#ifdef DEBUG
  else
  {
    fprintf (stderr, "xfwm : HandleFocusIn () : No change because Tmp_win == Scr.Hilite (ie \"%s\")\n", Tmp_win->name);
  }
#endif
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleFocusIn ()\n");
#endif
}

void
HandleFocusOut ()
{
#if 0
  Window focusBug;
  int rt;
#endif

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleFocusOut ()\n");
#endif

#if 0
  XGetInputFocus (dpy, &focusBug, &rt);
  if ((Tmp_win == NULL) && (Scr.Focus != NULL) && (focusBug == None))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleFocusOut () Forcing focus\n");
#endif
    XSetInputFocus (dpy, Scr.Focus->w, RevertToParent, CurrentTime);
  }
#endif

#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleFocusOut ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleKeyPress - key press event handler
 *
 ************************************************************************/
void
HandleKeyPress ()
{
  Binding *key;
  unsigned int modifier;
  modifier = vmod ((Event.xkey.state) & KeyMask);
  ButtonWindow = Tmp_win;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleKeyPress ()\n");
#endif

  Context = GetContext (Tmp_win, &Event, &PressedW);
  PressedW = None;

  Event.xkey.keycode = XKeysymToKeycode (dpy, XKeycodeToKeysym (dpy, Event.xkey.keycode, 0));

  for (key = Scr.AllBindings; key != NULL; key = key->NextBinding)
  {
    if ((key->Button_Key == Event.xkey.keycode) && ((vmod (key->Modifier) == modifier) || (key->Modifier == AnyModifier)) && (key->Context & Context) && (key->IsMouse == 0))
    {
      ExecuteFunction (key->Action, Tmp_win, &Event, Context, -1);
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving HandleKeyPress () -ExecuteFunction-\n");
#endif
      return;
    }
  }

  /* if we get here, no function key was bound to the key.  Send it
   * to the client if it was in a window we know about.
   */
  if (Tmp_win)
  {
    if (Event.xkey.window != Tmp_win->w)
    {
      Event.xkey.window = Tmp_win->w;
      XSendEvent (dpy, Tmp_win->w, False, KeyPressMask, &Event);
    }
  }

  ButtonWindow = NULL;
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleKeyPress ()\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandlePropertyNotify - property notify event handler
 *
 ***********************************************************************/

void
HandlePropertyNotify ()
{
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_bw, dummy_depth;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandlePropertyNotify ()\n");
#endif

  if (Event.xproperty.atom == _XA_WIN_WORKSPACE_COUNT)
  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;

    if ((XGetWindowProperty (dpy, Scr.Root, _XA_WIN_WORKSPACE_COUNT, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
    {
      if (prop != NULL)
      {
	Scr.ndesks = *(unsigned long *) prop;
	if (Scr.ndesks < 1)
	  Scr.ndesks = 1;
	if (Scr.ndesks > NBSCREENS)
	  Scr.ndesks = NBSCREENS;
	XFree (prop);
      }
    }
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandlePropertyNotify ()\n");
#endif
    return;
  }

  if ((!Tmp_win) || (XGetGeometry (dpy, Tmp_win->w, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth) == 0))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandlePropertyNotify () : Not one of our windows\n");
#endif
    return;
  }
  switch (Event.xproperty.atom)
  {
  case XA_WM_NAME:
    free_window_names (Tmp_win, True, False);
    GetWMName (Tmp_win);
    Tmp_win->flags |= WM_NAME_CHANGED;
    BroadcastName (XFCE_M_WINDOW_NAME, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, Tmp_win->name);

    /* fix the name in the title bar */
    if (!(Tmp_win->flags & ICONIFIED))
      SetTitleBar (Tmp_win, NULL, (Scr.Hilite == Tmp_win));

    /*
     * if the icon name is NoName, set the name of the icon to be
     * the same as the window 
     */
    if (!mystrncasecmp (Tmp_win->icon_name, NoName, strlen (NoName)))
    {
      free_window_names (Tmp_win, False, True);
      Tmp_win->icon_name = (char *) safemalloc (strlen (Tmp_win->name) + 1);
      strcpy (Tmp_win->icon_name, Tmp_win->name);
      BroadcastName (XFCE_M_ICON_NAME, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, Tmp_win->icon_name);
      RedoIconName (Tmp_win, NULL);
    }
    break;

  case XA_WM_ICON_NAME:
    free_window_names (Tmp_win, False, True);
    GetWMIconName (Tmp_win);
    BroadcastName (XFCE_M_ICON_NAME, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, Tmp_win->icon_name);
    RedoIconName (Tmp_win, NULL);
    break;

  case XA_WM_HINTS:
    if (Tmp_win->wmhints)
      XFree (Tmp_win->wmhints);
    Tmp_win->wmhints = XGetWMHints (dpy, Event.xany.window);

    if (Tmp_win->wmhints == NULL)
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving HandlePropertyNotify ()\n");
#endif
      return;
    }
    if ((Tmp_win->wmhints->flags & IconPixmapHint) || (Tmp_win->wmhints->flags & IconWindowHint))
      if (Tmp_win->icon_bitmap_file == Scr.DefaultIcon)
	Tmp_win->icon_bitmap_file = (char *) 0;

    if ((Tmp_win->wmhints->flags & IconPixmapHint) || (Tmp_win->wmhints->flags & IconWindowHint))
    {
      if (!(Tmp_win->flags & SUPPRESSICON))
      {
	if (Tmp_win->icon_w)
	{
	  XDeleteContext (dpy, Tmp_win->icon_w, XfwmContext);
	  XDestroyWindow (dpy, Tmp_win->icon_w);
	}
	if (Tmp_win->flags & ICON_OURS)
	{
	  if (Tmp_win->icon_pixmap_w != None)
	  {
	    XDeleteContext (dpy, Tmp_win->icon_pixmap_w, XfwmContext);
	    XDestroyWindow (dpy, Tmp_win->icon_pixmap_w);
	  }
	}
	else
	  XUnmapWindow (dpy, Tmp_win->icon_pixmap_w);
      }
      Tmp_win->icon_w = None;
      Tmp_win->icon_pixmap_w = None;
      Tmp_win->iconPixmap = (Window) NULL;
      if (Tmp_win->flags & ICONIFIED)
      {
	Tmp_win->flags &= ~ICONIFIED;
	Tmp_win->flags &= ~ICON_UNMAPPED;
	CreateIconWindow (Tmp_win, Tmp_win->icon_x_loc, Tmp_win->icon_y_loc);
	Broadcast (XFCE_M_ICONIFY, 7, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, Tmp_win->icon_x_loc, Tmp_win->icon_y_loc, Tmp_win->icon_w_width, Tmp_win->icon_w_height);
	BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, Tmp_win);

	if (!(Tmp_win->flags & SUPPRESSICON))
	{
	  if (!(Tmp_win->flags & STARTICONIC))
	    LowerWindow (Tmp_win);

	  if ((!(Tmp_win->flags & STARTICONIC)) && (!(Tmp_win->flags & ICON_MOVED) || !CheckIconPlace (Tmp_win)))
	    AutoPlace (Tmp_win, False);

	  if (Tmp_win->Desk == Scr.CurrentDesk)
	  {
	    if (Tmp_win->icon_w)
	      XMapWindow (dpy, Tmp_win->icon_w);
	    if (Tmp_win->icon_pixmap_w != None)
	      XMapWindow (dpy, Tmp_win->icon_pixmap_w);
	  }
	}
	Tmp_win->flags |= ICONIFIED;
	DrawIconWindow (Tmp_win, NULL);
	LowerWindow (Tmp_win);
      }
    }
    break;

  case XA_WM_NORMAL_HINTS:
    {
      GetWindowSizeHints (Tmp_win);
      BroadcastConfig (XFCE_M_CONFIGURE_WINDOW, Tmp_win);
    }
    break;

  default:
    if (Event.xproperty.atom == _XA_WM_PROTOCOLS)
      FetchWmProtocols (Tmp_win);
    else if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS)
    {
      FetchWmColormapWindows (Tmp_win);	/* frees old data */
      ReInstallActiveColormap ();
    }
    else if (Event.xproperty.atom == _XA_WM_STATE)
    {
      if ((Tmp_win != NULL) && AcceptInput(Tmp_win) && (Tmp_win == Scr.Focus))
      {
	SetFocus (Tmp_win->w, Tmp_win, True, True);
      }
    }
    else if (Event.xproperty.atom == _XA_WIN_LAYER)
    {
      if ((Tmp_win) && (GetWindowLayer (Tmp_win)))
      {
	RaiseWindow (Tmp_win);
      }
    }
    break;
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandlePropertyNotify ()\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleClientMessage - client message event handler
 *
 ************************************************************************/
void
HandleClientMessage ()
{
#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleClientMessage ()\n");
#endif
  if ((Event.xclient.message_type == _XA_WM_CHANGE_STATE) && (Tmp_win) && (Event.xclient.data.l[0] == IconicState) && !(Tmp_win->flags & TRANSIENT) && !(Tmp_win->flags & ICONIFIED))
  {
    XEvent button;
    /* Dummy var for XQueryPointer */
    Window dummy_root, dummy_child;
    int dummy_win_x, dummy_win_y;
    unsigned int dummy_mask;

    XQueryPointer (dpy, Scr.Root, &dummy_root, &dummy_child, &(button.xmotion.x_root), &(button.xmotion.y_root), &dummy_win_x, &dummy_win_y, &dummy_mask);
    button.type = 0;
    ExecuteFunction ("Iconify", Tmp_win, &button, C_FRAME, -1);
  }

#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleClientMessage ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleExpose - expose event handler
 *
 ***********************************************************************/
void
HandleExpose ()
{
  XEvent dummy;
  int x1 = Event.xexpose.x;
  int y1 = Event.xexpose.y;
  int x2 = x1 + Event.xexpose.width;
  int y2 = y1 + Event.xexpose.height;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleExpose ()\n");
#endif

  while (XCheckTypedWindowEvent(dpy, Event.xany.window, Expose, &dummy))
  {
    x1 = my_min(x1, dummy.xexpose.x);
    y1 = my_min(y1, dummy.xexpose.y);
    x2 = my_max(x2, dummy.xexpose.x + dummy.xexpose.width);
    y2 = my_max(y2, dummy.xexpose.y + dummy.xexpose.height);
  }

  if (Tmp_win)
  {
    XRectangle area;

    /* Retrieve clipping from event */
    area.x      = x1;
    area.y      = y1;
    area.width  = x2 - x1;
    area.height = y2 - y1;

    if ((Event.xany.window == Tmp_win->title_w))
    {
      SetTitleBar (Tmp_win, &area, (Scr.Hilite == Tmp_win));
    }
    else
    {
      SetBorder (Tmp_win, &area, (Scr.Hilite == Tmp_win), False, True, Event.xany.window);
    }
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleExpose ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleDestroyNotify - DestroyNotify event handler
 *
 ***********************************************************************/
void
HandleDestroyNotify ()
{
#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering DestroyNotify ()\n");
#endif
  if (!Tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleDestroyNotify (): Tmp_win == NULL\n");
#endif
    return;
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : HandleDestroyNotify (): Destroying %s\n", Tmp_win->name);
#endif
  Destroy (Tmp_win);
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleDestroyNotify ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleMapRequest - MapRequest event handler
 *
 ************************************************************************/
void
HandleMapRequest ()
{
  extern long isIconicState;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleMapRequest ()\n");
#endif
  Event.xany.window = Event.xmaprequest.window;

  if (Event.xany.window == None)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleMapRequest () : Mapping 'None' window !\n");
#endif
    return;
  }
  if (XFindContext (dpy, Event.xany.window, XfwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleMapRequest : Context not found, new window ?\n");
#endif
    Tmp_win = NULL;
  }

  if (!Tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleMapRequest : Calling AddWindow ()\n");
#endif
    Tmp_win = AddWindow (Event.xany.window);
    if (Tmp_win == NULL)
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving HandleMapRequest ()\n");
#endif
      return;
    }
  }

  if (!(Tmp_win->flags & ICONIFIED))
  {
    int state;

    if (Tmp_win->wmhints && (Tmp_win->wmhints->flags & StateHint))
      state = Tmp_win->wmhints->initial_state;
    else
      state = NormalState;

    if (Tmp_win->flags & STARTICONIC)
      state = IconicState;

    if (isIconicState != DontCareState)
      state = isIconicState;

    /* Force state to NormalState for transients to comply with ICCCM */
    if (Tmp_win->flags & TRANSIENT)
      state = NormalState;

    switch (state)
    {
    case IconicState:
      if (Tmp_win->wmhints)
	Iconify (Tmp_win, Tmp_win->wmhints->icon_x, Tmp_win->wmhints->icon_y, True);
      else
	Iconify (Tmp_win, 0, 0, True);
      break;
    case DontCareState:
    case NormalState:
    case InactiveState:
    default:
      if (Tmp_win->Desk == Scr.CurrentDesk)
      {
	XMapWindow (dpy, Tmp_win->frame);
        Tmp_win->flags |= MAP_PENDING;
      }
      XMapWindow (dpy, Tmp_win->Parent);
      XMapWindow (dpy, Tmp_win->w);
      SetMapStateProp (Tmp_win, NormalState);
      break;
    }
  }
  else
  {
    DeIconify (Tmp_win);
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleMapRequest ()\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleMapNotify - MapNotify event handler
 *
 ***********************************************************************/
void
HandleMapNotify ()
{
#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleMapNotify ()\n");
#endif
  if (!Tmp_win)
  {
    if ((Event.xmap.override_redirect == True) && (Event.xmap.window != Scr.NoFocusWin))
    {
#ifdef MANAGE_OVERRIDES
      Scr.overrides = AddToWindowList (Scr.overrides, Event.xmap.window);
#endif
      XSelectInput (dpy, Event.xmap.window, FocusChangeMask);
      Scr.UnknownWinFocused = Event.xmap.window;
    }
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleMapNotify ()\n");
#endif
    return;
  }

  MyXGrabServer (dpy);
  if (Tmp_win->icon_w)
    XUnmapWindow (dpy, Tmp_win->icon_w);
  if (Tmp_win->icon_pixmap_w != None)
    XUnmapWindow (dpy, Tmp_win->icon_pixmap_w);
  if (Tmp_win->Desk == Scr.CurrentDesk)
    XMapWindow (dpy, Tmp_win->frame);
  XMapWindow (dpy, Tmp_win->Parent);
  XMapWindow (dpy, Tmp_win->w);
  SetMapStateProp (Tmp_win, NormalState);
  MyXUngrabServer (dpy);
  XFlush (dpy);
  
  Tmp_win->flags |= MAPPED;
  Tmp_win->flags &= ~(MAP_PENDING | ICONIFIED | ICON_UNMAPPED);

  if (Tmp_win->flags & ICONIFIED)
    Broadcast (XFCE_M_DEICONIFY, 3, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, 0, 0, 0, 0);
  else
    Broadcast (XFCE_M_MAP, 3, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, 0, 0, 0, 0);

  if ((Tmp_win->Desk == Scr.CurrentDesk) && (Scr.Options & MapFocus) && AcceptInput (Tmp_win))
  {
    SetFocus (Tmp_win->w, Tmp_win, True, False);
  }

#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleMapNotify ()\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleUnmapNotify - UnmapNotify event handler
 *
 ************************************************************************/
void
HandleUnmapNotify ()
{
  Window w;
  XEvent dummy;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleUnmapNotify ()\n");
#endif
#ifdef MANAGE_OVERRIDES
  /* Search for the window in the list of overrides and remove it if found */
  if (SearchWindowList (Scr.overrides, Event.xunmap.window) != None)
  {
    Scr.overrides = RemoveFromWindowList (Scr.overrides, Event.xunmap.window);
  }
#endif

  /*
   * Don't ignore events as described below.
   */
  if ((Event.xunmap.event != Event.xunmap.window) && (Event.xunmap.event != Scr.Root || !Event.xunmap.send_event))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleUnmapNotify (): Event ignored\n");
#endif
    return;
  }

  if ((Event.xunmap.event == Scr.Root) && Event.xunmap.send_event)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : HandleUnmapNotify (): ICCCM2 unmap request\n");
#endif
    Event.xany.window = Event.xunmap.window;
    if ((Event.xany.window == None) || (XFindContext (dpy, Event.xany.window, XfwmContext, (caddr_t *) &Tmp_win) == XCNOENT))
    {
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving HandleUnmapNotify (): Tmp_win == NULL\n");
#endif
      return;
    }
  }

  if (!Tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleUnmapNotify (): Tmp_win == NULL\n");
#endif
    return;
  }

  if (!(Tmp_win->flags & (MAPPED | ICONIFIED)))
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleUnmapNotify (): win !mapped & !iconified\n");
#endif
    return;
  }
  w = Tmp_win->w;
  if (Tmp_win->flags & ICONIFIED)
  {
    if (Tmp_win->icon_pixmap_w != None)
      XUnmapWindow (dpy, Tmp_win->icon_pixmap_w);
    if (Tmp_win->icon_w != None)
      XUnmapWindow (dpy, Tmp_win->icon_w);
  }
  else
  {
    XUnmapWindow (dpy, Tmp_win->frame);
  }
  MyXGrabServer (dpy);
  if (!XCheckTypedWindowEvent (dpy, w, DestroyNotify, &dummy))
  {
    XEvent ev;
    Bool reparented;
    XSelectInput (dpy, w, NoEventMask);
    SetMapStateProp (Tmp_win, WithdrawnState);
    reparented = XCheckTypedWindowEvent (dpy, w, ReparentNotify, &ev);
    if (reparented)
    {
      if (Tmp_win->old_bw)
	XSetWindowBorderWidth (dpy, w, Tmp_win->old_bw);
      if ((!(Tmp_win->flags & SUPPRESSICON)) && (Tmp_win->wmhints && (Tmp_win->wmhints->flags & IconWindowHint)))
	XUnmapWindow (dpy, Tmp_win->wmhints->icon_window);
    }
    else
    {
      RestoreWithdrawnLocation (Tmp_win, False);
    }
    XRemoveFromSaveSet (dpy, w);
  }
  Destroy (Tmp_win);
  MyXUngrabServer (dpy);
  XFlush (dpy);
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleUnmapNotify ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 *
 ***********************************************************************/
void
HandleButtonPress ()
{
  unsigned int modifier;
  Binding *MouseEntry;
  Window x, eventw;
  int LocalContext;
  Bool HandledInternaly;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_bw, dummy_depth;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleButtonPress ()\n");
#endif
  if (Scr.Options & AutoRaiseWin)
    SetTimer (0);

  if (!Tmp_win && (Event.xany.window != Scr.Root))
  {
    /* event in unmanaged window or subwindow of a client */
    XSync (dpy, 0);
    XAllowEvents (dpy, ReplayPointer, CurrentTime);
    XSync (dpy, 0);
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleButtonPress ()\n");
#endif
    return;
  }

  if (Event.xbutton.subwindow != None && (Tmp_win == None || Event.xany.window != Tmp_win->w))
  {
    eventw = Event.xbutton.subwindow;
  }
  else
  {
    eventw = Event.xany.window;
  }

  if (!XGetGeometry (dpy, eventw, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth))
  {
    XSync (dpy, 0);
    XAllowEvents (dpy, ReplayPointer, CurrentTime);
    XSync (dpy, 0);
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleButtonPress ()\n");
#endif
    return;
  }

  XSync(dpy, 0);
  if (Tmp_win)
  {
    if (!(Tmp_win->triggered) || (Event.xbutton.window == Tmp_win->frame))
    {
      if ((Tmp_win != Scr.Focus) && AcceptInput(Tmp_win))
      {
	SetFocus (Tmp_win->w, Tmp_win, True, False);
      }
      Context = GetContext (Tmp_win, &Event, &PressedW);
      if ((Event.xbutton.button == 1) && ((Scr.Options & AutoRaiseWin) || (Scr.Options & ClickToFocus) || (Scr.Options & ClickRaise) || (!(Tmp_win->flags & TITLE)) || (Context != C_WINDOW)))
      {
	RaiseWindow (Tmp_win);
      }
      Tmp_win->triggered = True;
      XSync (dpy, 0);
      XAllowEvents (dpy, ReplayPointer, CurrentTime);
      XSync (dpy, 0);
#ifdef DEBUG
      fprintf (stderr, "xfwm : Leaving HandleButtonPress ()\n");
#endif
      return;
    }
  }

  if (Tmp_win)
  {
    Tmp_win->triggered = False;
  }
  XSync (dpy, 0);
  XAllowEvents (dpy, ReplayPointer, CurrentTime);
  XSync (dpy, 0);

  Context = GetContext (Tmp_win, &Event, &PressedW);
  modifier = vmod ((Event.xkey.state) & KeyMask);
  LocalContext = Context;
  x = PressedW;
  ButtonWindow = Tmp_win;

  if (Context & C_RALL)
  {
    RedrawRightButtons (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow), True, x);
  }
  else if (Context & C_LALL)
  {
    RedrawLeftButtons (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow), True, x);
  }
  else if (Context & C_TITLE)
  {
    if (RedrawTitleOnButtonPress ())
    {
      SetTitleBar (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow));
    }
  }

  /* we have to execute a function or pop up a menu */

  /* need to search for an appropriate mouse binding */
  HandledInternaly = False;
  for (MouseEntry = Scr.AllBindings; MouseEntry != NULL; MouseEntry = MouseEntry->NextBinding)
  {
    if (((MouseEntry->Button_Key == Event.xbutton.button) || (MouseEntry->Button_Key == 0)) && (MouseEntry->Context & Context) && ((vmod (MouseEntry->Modifier) == modifier) || (MouseEntry->Modifier == AnyModifier)) && (MouseEntry->IsMouse == 1))
    {
      /* got a match, now process it */
      ExecuteFunction (MouseEntry->Action, Tmp_win, &Event, Context, -1);
      HandledInternaly = True;
      break;
    }
  }

  if ((!HandledInternaly) && (Event.xany.window == Scr.Root))
  {
    /*
       If we have no function that matches the click, then pass it to the
       proxy window so some other application (gmc or other) may receive it
     */
    if (Event.type == ButtonPress)
    {
      XUngrabPointer (dpy, CurrentTime);
    }
    XSendEvent (dpy, Scr.GnomeProxyWin, False, SubstructureNotifyMask, &Event);
    Tmp_win = NULL;
    ButtonWindow = NULL;
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleButtonPress ()\n");
#endif
    return;
  }

  PressedW = None;
  if (Context & C_RALL)
  {
    RedrawRightButtons (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow), True, x);
  }
  else if (Context & C_LALL)
  {
    RedrawLeftButtons (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow), True, x);
  }
  else if (Context & C_TITLE)
  {
    if (RedrawTitleOnButtonPress ())
    {
      SetTitleBar (ButtonWindow, NULL, (Scr.Hilite == ButtonWindow));
    }
  }
  ButtonWindow = NULL;
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleButtonPress ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleButtonPress - ButtonPress event handler
 *
 ***********************************************************************/
void
HandleButtonRelease ()
{
#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleButtonRelease ()\n");
#endif
  if (Event.xany.window == Scr.Root)
  {
    XSendEvent (dpy, Scr.GnomeProxyWin, False, SubstructureNotifyMask, &Event);
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleButtonRelease ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleEnterNotify - EnterNotify event handler
 *
 ************************************************************************/
void
HandleEnterNotify ()
{
  XCrossingEvent *e;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleEnterNotify ()\n");
#endif
  e = (XCrossingEvent *) & Event;
  /* make sure its for one of our windows */
  if (!Tmp_win)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleEnterNotify ()\n");
#endif
    return;
  }
  if (AcceptInput(Tmp_win) && !(Scr.Options & ClickToFocus) && !(e->focus) && ((e->x_root != old_x_root) || (e->y_root != old_y_root) || (Scr.Options & ForceFocus)) && (e->mode == NotifyNormal))
  {
    SetFocus (Tmp_win->w, Tmp_win, True, False);

    if (((Scr.Options & AutoRaiseWin) && (Scr.AutoRaiseDelay > 0)) && (!(Tmp_win->flags & ICONIFIED) && !(Tmp_win->flags & RAISEDWIN)))
    {
      SetTimer (Scr.AutoRaiseDelay);
    }
  }
  old_x_root = e->x_root;
  old_y_root = e->y_root;
  if (Scr.ColormapFocus == COLORMAP_FOLLOWS_MOUSE)
  {
    if ((!(Tmp_win->flags & ICONIFIED)) && (Event.xany.window == Tmp_win->w))
      InstallWindowColormaps (Tmp_win);
    else
      InstallWindowColormaps (NULL);
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleEnterNotify ()\n");
#endif
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleLeaveNotify - LeaveNotify event handler
 *
 ************************************************************************/
void
HandleLeaveNotify ()
{
  /* If we leave the root window, then we're really moving
   * another screen on a multiple screen display, and we
   * need to de-focus and unhighlight to make sure that we
   * don't end up with more than one highlighted window at a time */
#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleLeaveNotify ()\n");
#endif
  if ((Event.xcrossing.window == Scr.Root) && (Scr.NumberOfScreens > 1))
  {
    if (Scr.Options & AutoRaiseWin)
      SetTimer (0);
    if (Event.xcrossing.mode == NotifyNormal)
    {
      if (Event.xcrossing.detail != NotifyInferior)
      {
	if (Scr.Focus != NULL)
	  SetFocus (Scr.NoFocusWin, NULL, False, False);
	if (Scr.Hilite != NULL)
	  SetBorder (Scr.Hilite, NULL, False, False, True, None);
      }
    }
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleLeaveNotify ()\n");
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	HandleConfigureRequest - ConfigureRequest event handler
 *
 ************************************************************************/
void
HandleConfigureRequest ()
{
  XWindowChanges xwc;
  unsigned long xwcm;
  int x, y, width, height;
  XConfigureRequestEvent *cre = &Event.xconfigurerequest;
  XEvent otherEvent;
  /*
   * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
   * be wrong
   */

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleConfigureRequest ()\n");
#endif
  Event.xany.window = cre->window;	/* mash parent field */

  if (cre->window == None)
  {
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleConfigureRequest () : Window is 'None'\n");
#endif
    return;
  }

  /* compress configure requests (Shamely stolen to kwin 2.2) */
  XSync (dpy, 0);
  while (XCheckTypedWindowEvent (dpy, cre->window, ConfigureRequest, &otherEvent) ) 
  {
    if (otherEvent.xconfigurerequest.value_mask == cre->value_mask)
    {
       cre = &otherEvent.xconfigurerequest;
#ifdef DEBUG
       fprintf (stderr, "xfwm : HandleConfigureRequest () : Skipping event\n");
#endif
    }
    else 
    {
#ifdef DEBUG
       fprintf (stderr, "xfwm : HandleConfigureRequest () : Putting back event\n");
#endif
       XPutBackEvent(dpy, &otherEvent);
       break;
    }
  }
  
  if (XFindContext (dpy, Event.xany.window, XfwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
  {
    Tmp_win = NULL;
  }
  
  if (!Tmp_win || (Tmp_win->icon_w == Event.xany.window))
  {
    xwcm = cre->value_mask & (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
    xwc.x = cre->x;
    xwc.y = cre->y;
    if ((Tmp_win) && ((Tmp_win->icon_w == Event.xany.window)))
    {
      Tmp_win->icon_xl_loc = cre->x;
      Tmp_win->icon_x_loc = cre->x + (Tmp_win->icon_w_width - Tmp_win->icon_p_width) / 2;
      Tmp_win->icon_y_loc = cre->y - Tmp_win->icon_p_height;
      if (!(Tmp_win->flags & ICON_UNMAPPED))
	Broadcast (XFCE_M_ICON_LOCATION, 7, Tmp_win->w, Tmp_win->frame, (unsigned long) Tmp_win, Tmp_win->icon_x_loc, Tmp_win->icon_y_loc, Tmp_win->icon_w_width, Tmp_win->icon_w_height + Tmp_win->icon_p_height);
    }
    xwc.width = cre->width;
    xwc.height = cre->height;
    xwc.border_width = cre->border_width;
    XConfigureWindow (dpy, Event.xany.window, xwcm, &xwc);

    if (Tmp_win)
    {
      xwc.x = Tmp_win->icon_x_loc;
      xwc.y = Tmp_win->icon_y_loc - Tmp_win->icon_p_height;
      xwcm = cre->value_mask & (CWX | CWY);
      if (Tmp_win->icon_pixmap_w != None)
	XConfigureWindow (dpy, Tmp_win->icon_pixmap_w, xwcm, &xwc);
      xwc.x = Tmp_win->icon_x_loc;
      xwc.y = Tmp_win->icon_y_loc;
      xwcm = cre->value_mask & (CWX | CWY);
      if (Tmp_win->icon_w != None)
	XConfigureWindow (dpy, Tmp_win->icon_w, xwcm, &xwc);
    }
#ifdef DEBUG
    fprintf (stderr, "xfwm : Leaving HandleConfigureRequest ()\n");
#endif
    return;
  }

  if (cre->value_mask & CWStackMode)
  {
    if ((cre->detail) == Above)
    {
      RaiseWindow (Tmp_win);
    }
    else if ((cre->detail) == Below)
    {
      LowerWindow (Tmp_win);
    }
  }

  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    XShapeQueryExtents (dpy, Tmp_win->w, &boundingShaped, &xws, &yws, &wws, &hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
    Tmp_win->wShaped = boundingShaped;
  }

  /* Don't modify frame_XXX fields before calling SetupFrame */
  x = Tmp_win->frame_x;
  y = Tmp_win->frame_y;
  width = Tmp_win->frame_width;
  height = Tmp_win->frame_height;

  if (cre->value_mask & CWBorderWidth)
  {
    Tmp_win->old_bw = cre->border_width;
  }
  if (cre->value_mask & CWX)
  {
    x = cre->x - (Tmp_win->boundary_width + Tmp_win->bw - Tmp_win->old_bw);
  }
  if (cre->value_mask & CWY)
  {
    y = cre->y - (Tmp_win->boundary_width + Tmp_win->bw - Tmp_win->old_bw) - Tmp_win->title_height;
  }
  if (cre->value_mask & CWWidth)
  {
    width = cre->width + 2 * (Tmp_win->boundary_width + Tmp_win->bw);
  }
  if (cre->value_mask & CWHeight)
  {
    height = cre->height + Tmp_win->title_height + 2 * (Tmp_win->boundary_width + Tmp_win->bw);
  }
  /* Remove the MAXIMIZED flag */
  if (Tmp_win->flags & MAXIMIZED)
  {
    Tmp_win->flags &= ~MAXIMIZED;
    SetBorder (Tmp_win, NULL, Scr.Hilite == Tmp_win, True, True, None);
  }

  if (cre->value_mask & (CWX | CWY | CWWidth | CWHeight))
  {
    /* 
       Not sure if we should send an event in all cases when
       responding to a configureRequest event. I guess, by reading
       the ICCCM 2.x that we should not, however, some apps seem
       to be waiting for that event, so we send it... Thus the 
       "True" in 6th field of the SetupFrame call.
     */
    SetupFrame (Tmp_win, x, y, width, height, True, True);
  }
  else
  {
    sendclient_event (Tmp_win, x, y, width, height);
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleConfigureRequest ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *      HandleShapeNotify - shape notification event handler
 *
 ***********************************************************************/
void
HandleShapeNotify (void)
{
  if (ShapesSupported)
  {
    XShapeEvent *sev = (XShapeEvent *) & Event;

    if (!Tmp_win)
      return;
    if (sev->kind != ShapeBounding)
      return;
    Tmp_win->wShaped = sev->shaped;
    SetShape (Tmp_win, Tmp_win->frame_width);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleVisibilityNotify - record fully visible windows for
 *      use in the RaiseLower function.
 *
 ************************************************************************/
void
HandleVisibilityNotify ()
{
  XVisibilityEvent *vevent = (XVisibilityEvent *) & Event;

#ifdef DEBUG
  fprintf (stderr, "xfwm : Entering HandleVisibilityNotify ()\n");
#endif
  if (Tmp_win)
  {
    if (vevent->state == VisibilityUnobscured)
      Tmp_win->flags |= VISIBLE;
    else
      Tmp_win->flags &= ~VISIBLE;
  }
#ifdef DEBUG
  fprintf (stderr, "xfwm : Leaving HandleVisibilityNotify ()\n");
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *     HandleRootMotionNotify - Root-MotionNotify event handler
 *
 ************************************************************************/
int
_xfwm_deskwrap (int horz, int vert)
{
  int ndesks = Scr.ndesks;
  int desk = Scr.CurrentDesk;

  if (ndesks < 2)
    return 0;

  /*assumes that the desk-layout is alway 2 rows in height -gud */

  if (horz)
  {
    desk += horz * 2;
    if (desk >= ndesks)
    {
      desk -= ndesks;
    }
    if (desk < 0)
    {
      desk += ndesks;
    }
  }
  if (vert)
  {
    desk ^= 1;
  }

  return desk;
}

void
sendclient_event (XfwmWindow * tmp_win, int x, int y, int w, int h)
{
  XEvent client_event;

  client_event.type = ConfigureNotify;
  client_event.xconfigure.display = dpy;
  client_event.xconfigure.event = tmp_win->w;
  client_event.xconfigure.window = tmp_win->w;

  client_event.xconfigure.x = x + tmp_win->boundary_width + tmp_win->bw;
  client_event.xconfigure.y = y + tmp_win->title_height + tmp_win->boundary_width + tmp_win->bw;
  client_event.xconfigure.width = w - 2 * (tmp_win->boundary_width + tmp_win->bw);
  client_event.xconfigure.height = h - 2 * (tmp_win->boundary_width + tmp_win->bw) - tmp_win->title_height;

  client_event.xconfigure.border_width = 0;
  client_event.xconfigure.above = tmp_win->frame;
  client_event.xconfigure.override_redirect = False;
  XSendEvent (dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
}

void
HandleRootMotionNotify ()
{

  int evx, evy;
  int warpx, warpy;

  if (Scr.SnapSize < MINRESISTANCE)
  {
    return;
  }
  else
  {
    /* check if the pointer is pushed onto the screen border,
       if so, increment the corresponding X or Y counter (until Snapsize),
       otherwise reset them to null.
     */
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
    if (Scr.EdgeScrollX <= (-EDGEFACTOR * Scr.SnapSize))
    {				/* move desktop left */
      changeDesks (0, _xfwm_deskwrap (-1, 0), 1, 1, 1);
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = Scr.MyDisplayWidth;
      warpy = evy;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx) - Scr.SnapSize, my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy));
      return;
    }
    if (Scr.EdgeScrollX >= (EDGEFACTOR * Scr.SnapSize))
    {				/* move desktop right */
      changeDesks (0, _xfwm_deskwrap (+1, 0), 1, 1, 1);
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = 0;
      warpy = evy;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayX (warpx, warpy), warpx) + Scr.SnapSize, my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy));
      return;
    }
    if (Scr.EdgeScrollY <= (-EDGEFACTOR * Scr.SnapSize))
    {				/* move desktop upper */
      changeDesks (0, _xfwm_deskwrap (0, -1), 1, 1, 1);
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = evx;
      warpy = Scr.MyDisplayHeight;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx), my_min (MyDisplayMaxY (warpx, warpy) - 1, warpy) - Scr.SnapSize);
      return;
    }
    if (Scr.EdgeScrollY >= (EDGEFACTOR * Scr.SnapSize))
    {				/* move desktop lower */
      changeDesks (0, _xfwm_deskwrap (0, +1), 1, 1, 1);
      Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
      warpx = evx;
      warpy = 0;
      XWarpPointer (dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, my_min (MyDisplayMaxX (warpx, warpy) - 1, warpx), my_min (MyDisplayY (warpx, warpy), warpy) + Scr.SnapSize);
      return;
    }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	HandleMotionNotify - MotionNotify event handler
 *
 ************************************************************************/
void
HandleMotionNotify ()
{
  XMotionEvent *e;

  e = (XMotionEvent *) & Event;

  if ((e->x_root != old_x_root) || (e->y_root != old_y_root))
  {
    old_x_root = e->x_root;
    old_y_root = e->y_root;
  }

  /* multi screen? */
  if (Event.xany.window == Scr.Root)
  {
    HandleRootMotionNotify ();
  }
  return;
}

/***************************************************************************
 *
 * Waits for next X event, or for an auto-raise timeout.
 *
 ***************************************************************************/
int
My_XNextEvent (Display * dpy, XEvent * event)
{
  extern int fd_width, x_fd;
  fd_set in_fdset, out_fdset;
  Window targetWindow;
  int i, count;
  int retval;

  FD_ZERO (&in_fdset);
  FD_SET (x_fd, &in_fdset);
  FD_ZERO (&out_fdset);
#ifdef HAVE_SESSION
  if (sm_fd >= 0)
  {
    FD_SET (sm_fd, &in_fdset);
  }
#endif
  for (i = 0; i < npipes; i++)
  {
    if (readPipes[i] >= 0)
    {
      FD_SET (readPipes[i], &in_fdset);
    }
    if (pipeQueue[i] != NULL)
    {
      FD_SET (writePipes[i], &out_fdset);
    }
  }

  if (alarmed)
  {
    SetTimer (0);
    alarmed = False;
    if (Scr.Focus != NULL)
    {
      RaiseWindow (Scr.Focus);
    }
    return 0;
  }

  ReapChildren ();

  if (XPending (dpy))
  {
    XNextEvent (dpy, event);
#ifdef REQUIRES_STASHEVENT
    StashEventTime (event);
#endif
    return 1;
  }

#ifdef __hpux
  retval = select (fd_width, (fd_set *) & in_fdset, (fd_set *) & out_fdset, 0, NULL);
#else
  retval = select (fd_width, &in_fdset, &out_fdset, 0, NULL);
#endif

  /* Check for module input. */
  for (i = 0; i < npipes; i++)
  {
    if (readPipes[i] >= 0)
    {
      if ((retval > 0) && (FD_ISSET (readPipes[i], &in_fdset)))
      {
	if ((count = read (readPipes[i], &targetWindow, sizeof (Window))) > 0)
	{
	  HandleModuleInput (targetWindow, i);
	}
	if (count <= 0)
	{
	  KillModule (i, 10);
	}
      }
    }
    if (writePipes[i] >= 0)
    {
      if ((retval > 0) && (FD_ISSET (writePipes[i], &out_fdset)))
      {
	FlushQueue (i);
      }
    }
  }
#ifdef HAVE_SESSION
  /* Weird : If alarm is set, ProcessICEMsgs () get stuck */
  if ((!alarmed) && (sm_fd >= 0) && (FD_ISSET (sm_fd, &in_fdset)))
    ProcessICEMsgs ();
#endif

  return 0;
}
