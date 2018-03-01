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
 * This module is based on Twm, but has been siginificantly modified 
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


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "xfwm.h"
#include "misc.h"
#include "screen.h"
#include "module.h"
#include "session.h"
#include "stack.h"
#include "constant.h"

#include <X11/extensions/shape.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static XrmDatabase db;
static XrmOptionDescRec table[] = {
  /* Want to accept "-workspace N" or -xrm "xfwm*desk:N" as options
   * to specify the desktop. I have to include dummy options that
   * are meaningless since Xrm seems to allow -w to match -workspace
   * if there would be no ambiguity. */
  {"-workspacf", "*junk", XrmoptionSepArg, (caddr_t) NULL},
  {"-workspace", "*desk", XrmoptionSepArg, (caddr_t) NULL},
  {"-xrn", NULL, XrmoptionResArg, (caddr_t) NULL},
  {"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
};

extern void GetMwmHints (XfwmWindow *);
extern void GetExtHints (XfwmWindow *);
extern void GetKDEHints (XfwmWindow *);

XfwmWindow *
AddWindow (Window w)
{
  XfwmWindow *tmp_win;		/* new xfwm window structure */
  unsigned long valuemask;	/* mask for create windows */
  unsigned long tflag = 0L;
  unsigned long nitems, bytes_remain;
  unsigned long state;
  int aformat;
  int i, x, y, width, height;
  int Desk, border_width, layer;
  int client_argc;
  char *value;
  char *forecolor = NULL, *backcolor = NULL;
  char **client_argv = NULL, *str_type;
  unsigned char *prop;
  XSetWindowAttributes attributes;	/* attributes for create windows */
  Bool status;
  XrmValue rm_value;
  Atom atype;
  extern XfwmWindow *colormap_win;
  /* Dummy var for XGetGeometry */
  Window dummy_root;
  int dummy_x, dummy_y;
  unsigned int dummy_width, dummy_height, dummy_depth, dummy_bw;


  /* allocate space for the xfwm window */
  tmp_win = (XfwmWindow *) safemalloc (sizeof (XfwmWindow));
  if (!tmp_win)
  {
    return NULL;
  }

  XSelectInput(dpy, tmp_win->w, PropertyChangeMask);
  XSync (dpy, 0);
  XGetWindowAttributes (dpy, w, &(tmp_win->attr));
  XSetWindowBorderWidth (dpy, w, 0);

  tmp_win->flags = 0;
  tmp_win->old_bw = tmp_win->attr.border_width;
  tmp_win->w = w;
  tmp_win->icon_arranged = False;
  tmp_win->triggered = False;
  tmp_win->deleted = False;
  tmp_win->mwm_hints = NULL;
  tmp_win->cmap_windows = (Window *) NULL;
  tmp_win->name = NULL;

  GetWMName (tmp_win);
  tmp_win->icon_name = NULL;
  GetWMIconName (tmp_win);
  tmp_win->class.res_name = NoResource;
  tmp_win->class.res_class = NoClass;
  XGetClassHint (dpy, w, &tmp_win->class);
  if (tmp_win->class.res_name == NULL)
    tmp_win->class.res_name = NoResource;
  if (tmp_win->class.res_class == NULL)
    tmp_win->class.res_class = NoClass;
  tmp_win->wmhints = XGetWMHints (dpy, w);

  if (XGetTransientForHint (dpy, w, &tmp_win->transientfor))
    tmp_win->flags |= TRANSIENT;
  else
    tmp_win->flags &= ~TRANSIENT;

  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    XShapeSelectInput (dpy, w, ShapeNotifyMask);
    XShapeQueryExtents (dpy, w, &boundingShaped, &xws, &yws, &wws, &hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
    tmp_win->wShaped = boundingShaped;
  }

  tflag = LookInStyleList (tmp_win->name, &tmp_win->class, &value, &Desk, &border_width, &layer, &forecolor, &backcolor, &tmp_win->buttons);
  tmp_win->layer = layer;
  tmp_win->title_height = GetDecor (tmp_win, TitleHeight);

  /*
     By default, transient windows reside in the same layer as their master window 
   */

  if ((tmp_win->flags & TRANSIENT))
  {
    XfwmWindow *transientfor = SearchXfwmWindowList (Scr.stacklist, tmp_win->transientfor);

    if (transientfor)
    {
      tmp_win->layer = transientfor->layer;
    }
  }
  /* Unless the application uses the atom _WIN_LAYER to force it... */
  GetWindowLayer (tmp_win);

  /* Check GNOME hint for desk or STICKY */
  if (GetWindowState (tmp_win, &state))
  {
    if (state & WIN_STATE_STICKY)
    {
      tflag |= STICKY;
    }
  }

  /* find a suitable icon pixmap */
  if (tflag & ICON_FLAG)
  {
    /* an icon was specified */
    tmp_win->icon_bitmap_file = value;
  }
  else if ((tmp_win->wmhints) && (tmp_win->wmhints->flags & (IconWindowHint | IconPixmapHint)))
  {
    /* window has its own icon */
    tmp_win->icon_bitmap_file = NULL;
  }
  else
  {
    /* use default icon */
    tmp_win->icon_bitmap_file = Scr.DefaultIcon;
  }

  GetMwmHints (tmp_win);
  GetExtHints (tmp_win);
  GetKDEHints (tmp_win);
  SelectDecor (tmp_win, tflag, border_width);

  GetWindowSizeHints (tmp_win);

  /* Find out if the client requested a specific desk on the command line. */
  if (XGetCommand (dpy, w, &client_argv, &client_argc))
  {
    XrmParseCommand (&db, table, 4, "xfwm", &client_argc, client_argv);
    status = XrmGetResource (db, "xfwm.desk", "Xfwm.Desk", &str_type, &rm_value);
    if ((status == True) && (rm_value.size != 0))
    {
      Desk = atoi (rm_value.addr);
      Desk = ((Desk > 0) ? Desk - 1 : 0);
      tflag |= STARTONDESK_FLAG;
    }
    XrmDestroyDatabase (db);
    db = NULL;
    XFreeStringList (client_argv);
  }
  if (tflag & ALLOWFREEMOVE_FLAG)
  {
      tmp_win->flags |= FREEMOVE;
  }

  tmp_win->flags |= tflag & ALL_COMMON_FLAGS;
  /* If restarting, retrieve parameters saved as Atoms */
  if ((XGetWindowProperty (dpy, w, _XA_XFWM_FLAGS, 0L, 1L, True, _XA_XFWM_FLAGS, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      tmp_win->flags |= (*(unsigned long *) prop) & (STICKY | ICON_MOVED | SHADED | RECAPTURE);
      XFree (prop);
    }
  }

  if ((XGetWindowProperty (dpy, w, _XA_XFWM_ICONPOS_X, 0L, 1L, True, _XA_XFWM_ICONPOS_X, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      tmp_win->icon_x_loc = *(int *) prop;
      XFree (prop);
    }
  }

  if ((XGetWindowProperty (dpy, w, _XA_XFWM_ICONPOS_Y, 0L, 1L, True, _XA_XFWM_ICONPOS_Y, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      tmp_win->icon_y_loc = *(int *) prop;
      XFree (prop);
    }
  }

  PlaceWindow (tmp_win, tflag, Desk);
  /* Once placement is computed, set recapture to false */
  tmp_win->flags &= ~RECAPTURE;

  /* Load Session Management saved values */
  MatchWinToSM (tmp_win);

  /* Tentative size estimate */
  tmp_win->frame_x = tmp_win->attr.x;
  tmp_win->frame_y = tmp_win->attr.y;

  tmp_win->frame_width = tmp_win->attr.width + 2 * (tmp_win->boundary_width + tmp_win->bw);
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height + 2 * (tmp_win->boundary_width + tmp_win->bw);
  tmp_win->shade_x = tmp_win->frame_x;
  tmp_win->shade_y = tmp_win->frame_y;
  tmp_win->shade_width = tmp_win->frame_width;
  tmp_win->shade_height = tmp_win->title_height + 2 * tmp_win->boundary_width;

  ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

  tmp_win->flags &= ~ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  tmp_win->flags &= ~MAXIMIZED;

  tmp_win->TextPixel = Scr.MenuColors.fore;

  if (forecolor != NULL)
  {
    XColor color;

    if ((XParseColor (dpy, Scr.XfwmRoot.attr.colormap, forecolor, &color)) && (XAllocColor (dpy, Scr.XfwmRoot.attr.colormap, &color)))
    {
      tmp_win->TextPixel = color.pixel;
    }
  }
  tmp_win->BackPixel = GetDecor (tmp_win, LoColors.back);
  attributes.background_pixel = tmp_win->BackPixel;

  /* Grab the server so the window doesn't go away while we're capturing it */
  MyXGrabServer (dpy);
  if (XGetGeometry (dpy, w, &dummy_root, &dummy_x, &dummy_y, &dummy_width, &dummy_height, &dummy_bw, &dummy_depth) == 0)
  {
    free (tmp_win);
    free_window_names (tmp_win, True, True);
    if (tmp_win->wmhints)
      XFree (tmp_win->wmhints);
    if (tmp_win->class.res_name && tmp_win->class.res_name != NoResource)
      XFree (tmp_win->class.res_name);
    if (tmp_win->class.res_class && tmp_win->class.res_class != NoClass)
      XFree (tmp_win->class.res_class);
    if (tmp_win->mwm_hints)
      XFree (tmp_win->mwm_hints);
    MyXUngrabServer (dpy);
    return NULL;
  }

  /* create windows */

  valuemask = CWCursor | CWEventMask;
  attributes.cursor = Scr.XfwmCursors[DEFAULT];
  attributes.event_mask = (SubstructureRedirectMask |
			   VisibilityChangeMask | 
			   EnterWindowMask | LeaveWindowMask | 
			   ExposureMask);
  attributes.win_gravity = StaticGravity;
  attributes.bit_gravity = StaticGravity;
  
  tmp_win->frame = XCreateWindow (dpy, Scr.Root, tmp_win->frame_x, tmp_win->frame_y, tmp_win->frame_width, tmp_win->frame_height, 0, CopyFromParent, InputOutput, CopyFromParent, CWWinGravity | CWBitGravity | valuemask, &attributes);

  attributes.cursor = Scr.XfwmCursors[DEFAULT];
#ifndef OLD_STYLE
  attributes.border_pixel = GetDecor (t, LoRelief.back);
#endif
  attributes.event_mask = SubstructureRedirectMask;
  tmp_win->Parent = XCreateWindow (dpy, tmp_win->frame, tmp_win->boundary_width, tmp_win->boundary_width + tmp_win->title_height, tmp_win->frame_width - 2 * (tmp_win->boundary_width + tmp_win->bw), tmp_win->frame_height - 2 * (tmp_win->boundary_width + tmp_win->bw) - tmp_win->title_height, tmp_win->bw, CopyFromParent, InputOutput, CopyFromParent,
#ifndef OLD_STYLE
				   CWBorderPixel | valuemask,
#else
				   valuemask,
#endif
				   &attributes);

  attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask);

  tmp_win->title_x = tmp_win->title_y = 0;
  tmp_win->title_w = None;
  tmp_win->title_width = tmp_win->frame_width - 2 * tmp_win->corner_width;
  if (tmp_win->title_width < 1)
    tmp_win->title_width = 1;
  if (tmp_win->flags & BORDER)
  {
    for (i = 0; i < 4; i++)
    {
      attributes.cursor = Scr.XfwmCursors[TOP + i];
      tmp_win->sides[i] = XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->boundary_width, tmp_win->boundary_width, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    }
    /* Just dump the windows any old place and left SetupFrame take
     * care of the mess */
    for (i = 3; i >= 0; i--)
    {
      attributes.cursor = Scr.XfwmCursors[TOP_LEFT + i];
      tmp_win->corners[i] = XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->corner_width, tmp_win->corner_width, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    }
  }

  if (tmp_win->flags & TITLE)
  {
    tmp_win->title_x = tmp_win->boundary_width + tmp_win->title_height + 1;
    tmp_win->title_y = tmp_win->boundary_width;
    attributes.cursor = Scr.XfwmCursors[TITLE_CURSOR];
    tmp_win->title_w = XCreateWindow (dpy, tmp_win->frame, tmp_win->title_x, tmp_win->title_y, tmp_win->title_width, tmp_win->title_height, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    attributes.cursor = Scr.XfwmCursors[SYS];
    for (i = 2; i >= 0; i--)
    {
      if ((i < Scr.nr_left_buttons) && (tmp_win->left_w[i] > 0))
      {
	tmp_win->left_w[i] = XCreateWindow (dpy, tmp_win->frame, tmp_win->title_height * i, 0, tmp_win->title_height, tmp_win->title_height, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
      }
      else
	tmp_win->left_w[i] = None;

      if ((i < Scr.nr_right_buttons) && (tmp_win->right_w[i] > 0))
      {
	tmp_win->right_w[i] = XCreateWindow (dpy, tmp_win->frame, tmp_win->title_width - tmp_win->title_height * (i + 1), 0, tmp_win->title_height, tmp_win->title_height, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
      }
      else
	tmp_win->right_w[i] = None;
    }
  }

  XRaiseWindow (dpy, tmp_win->Parent);
  XUnmapWindow (dpy, w);
  XReparentWindow (dpy, w, tmp_win->Parent, 0, 0);
  valuemask = (CWEventMask | CWDontPropagate);
  attributes.event_mask = (StructureNotifyMask | PropertyChangeMask | ColormapChangeMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask);

  attributes.do_not_propagate_mask = (ButtonPressMask | ButtonReleaseMask);

  XChangeWindowAttributes (dpy, w, valuemask, &attributes);
  XAddToSaveSet (dpy, w);

  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to False to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
   * again in HandleMapNotify.
   */
  tmp_win->flags &= ~MAPPED;

  /* add the window into the xfwm list */
  if (tmp_win != Scr.XfwmRoot.next)
  {
    tmp_win->next = Scr.XfwmRoot.next;
    if (Scr.XfwmRoot.next != NULL)
      Scr.XfwmRoot.next->prev = tmp_win;
    tmp_win->prev = &Scr.XfwmRoot;
    Scr.XfwmRoot.next = tmp_win;
  }
  Scr.stacklist = AddToXfwmWindowList (Scr.stacklist, tmp_win);

  RaiseWindow (tmp_win);

  tmp_win->icon_w = None;
  GrabButtons (tmp_win);
  GrabKeys (tmp_win);
  XSaveContext (dpy, w, XfwmContext, (caddr_t) tmp_win);
  XSaveContext (dpy, tmp_win->frame, XfwmContext, (caddr_t) tmp_win);
  XSaveContext (dpy, tmp_win->Parent, XfwmContext, (caddr_t) tmp_win);
  if (tmp_win->flags & TITLE)
  {
    XSaveContext (dpy, tmp_win->title_w, XfwmContext, (caddr_t) tmp_win);
    for (i = 0; i < Scr.nr_left_buttons; i++)
      XSaveContext (dpy, tmp_win->left_w[i], XfwmContext, (caddr_t) tmp_win);
    for (i = 0; i < Scr.nr_right_buttons; i++)
      if (tmp_win->right_w[i] != None)
	XSaveContext (dpy, tmp_win->right_w[i], XfwmContext, (caddr_t) tmp_win);
  }
  if (tmp_win->flags & BORDER)
  {
    for (i = 0; i < 4; i++)
    {
      XSaveContext (dpy, tmp_win->sides[i], XfwmContext, (caddr_t) tmp_win);
      XSaveContext (dpy, tmp_win->corners[i], XfwmContext, (caddr_t) tmp_win);
    }
  }

  MyXGrabButton (dpy, AnyButton, 0, tmp_win->frame, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  MyXGrabButton (dpy, AnyButton, AnyModifier, tmp_win->frame, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if (tmp_win->attr.colormap == None)
    tmp_win->attr.colormap = Scr.XfwmRoot.attr.colormap;
  InstallWindowColormaps (colormap_win);

  /* When we're all clear, map window */
  XMapSubwindows (dpy, tmp_win->frame);

  x = tmp_win->frame_x;
  tmp_win->frame_x = 1;
  y = tmp_win->frame_y;
  tmp_win->frame_y = 1;
  width = tmp_win->frame_width;
  tmp_win->frame_width = 0;
  height = tmp_win->frame_height;
  tmp_win->frame_height = 0;
  SetupFrame (tmp_win, x, y, width, height, True, True);

  MyXUngrabServer (dpy);
  
  BroadcastConfig (XFCE_M_ADD_WINDOW, tmp_win);

  BroadcastName (XFCE_M_WINDOW_NAME, w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->name);
  BroadcastName (XFCE_M_ICON_NAME, w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->icon_name);
  if (tmp_win->icon_bitmap_file != NULL && tmp_win->icon_bitmap_file != Scr.DefaultIcon)
    BroadcastName (XFCE_M_ICON_FILE, w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->icon_bitmap_file);
  BroadcastName (XFCE_M_RES_CLASS, w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->class.res_class);
  BroadcastName (XFCE_M_RES_NAME, w, tmp_win->frame, (unsigned long) tmp_win, tmp_win->class.res_name);

  return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the xfwm window structure to use
 *
 ***********************************************************************/
void
GrabButtons (XfwmWindow * tmp_win)
{
  Binding *MouseEntry;

  if (!tmp_win)
    return;

  MouseEntry = Scr.AllBindings;
  while (MouseEntry != (Binding *) 0)
  {
    if ((MouseEntry->Action != NULL) && (MouseEntry->Context & C_WINDOW) && (MouseEntry->IsMouse == 1))
    {
      if (MouseEntry->Button_Key > 0)
      {
	MyXGrabButton (dpy, MouseEntry->Button_Key, MouseEntry->Modifier, tmp_win->w, True, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
      }
      else
      {
	MyXGrabButton (dpy, AnyButton, MouseEntry->Modifier, tmp_win->w, True, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
      }
    }
    MouseEntry = MouseEntry->NextBinding;
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the xfwm window structure to use
 *
 ***********************************************************************/
void
GrabKeys (XfwmWindow * tmp_win)
{
  Binding *tmp;

  if (!tmp_win)
    return;

  for (tmp = Scr.AllBindings; tmp != NULL; tmp = tmp->NextBinding)
  {
    if ((tmp->Context & (C_WINDOW | C_TITLE | C_RALL | C_LALL | C_SIDEBAR)) && (tmp->IsMouse == 0))
    {
      MyXGrabKey (dpy, tmp->Button_Key, tmp->Modifier, tmp_win->frame, True, GrabModeAsync, GrabModeAsync);
    }
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the xfwm window structure to use
 *
 ***********************************************************************/
void
FetchWmProtocols (XfwmWindow * tmp)
{
  unsigned long flags = 0L;
  Atom *protocols = NULL, *ap;
  int i, n;
  Atom atype;
  int aformat;
  unsigned long bytes_remain, nitems;

  if (!tmp)
    return;

  /* First, try the Xlib function to read the protocols.
   * This is what Twm uses. */
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n))
  {
    for (i = 0, ap = protocols; i < n; i++, ap++)
    {
      if (*ap == (Atom) _XA_WM_TAKE_FOCUS)
	flags |= DoesWmTakeFocus;
      if (*ap == (Atom) _XA_WM_DELETE_WINDOW)
	flags |= DoesWmDeleteWindow;
    }
    if (protocols)
      XFree (protocols);
  }
  else
  {
    /* Next, read it the hard way. mosaic from Coreldraw needs to
     * be read in this way. */
    if ((XGetWindowProperty (dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False, _XA_WM_PROTOCOLS, &atype, &aformat, &nitems, &bytes_remain, (unsigned char **) &protocols)) == Success)
    {
      for (i = 0, ap = protocols; i < nitems; i++, ap++)
      {
	if (*ap == (Atom) _XA_WM_TAKE_FOCUS)
	  flags |= DoesWmTakeFocus;
	if (*ap == (Atom) _XA_WM_DELETE_WINDOW)
	  flags |= DoesWmDeleteWindow;
      }
      if (protocols)
	XFree (protocols);
    }
  }
  tmp->flags |= flags;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the xfwm window structure to use
 *
 ***********************************************************************/
void
GetWindowSizeHints (XfwmWindow * tmp)
{
  long supplied = 0;

  if (!tmp)
    return;

  if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
  {
    tmp->hints.flags = 0;
  }
  /* Beat up our copy of the hints, so that all important field are
   * filled in! */
  if (tmp->hints.flags & PResizeInc)
  {
    if (tmp->hints.width_inc == 0)
      tmp->hints.width_inc = 1;
    if (tmp->hints.height_inc == 0)
      tmp->hints.height_inc = 1;
  }
  else
  {
    tmp->hints.width_inc = 1;
    tmp->hints.height_inc = 1;
  }

  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */

  if (!(tmp->hints.flags & PBaseSize))
  {
    if (tmp->hints.flags & PMinSize)
    {
      tmp->hints.base_width = tmp->hints.min_width;
      tmp->hints.base_height = tmp->hints.min_height;
    }
    else
    {
      tmp->hints.base_width = 0;
      tmp->hints.base_height = 0;
    }
  }
  if (!(tmp->hints.flags & PMinSize))
  {
    tmp->hints.min_width = tmp->hints.base_width;
    tmp->hints.min_height = tmp->hints.base_height;
  }
  if (!(tmp->hints.flags & PMaxSize))
  {
    tmp->hints.max_width = MAX_WINDOW_WIDTH;
    tmp->hints.max_height = MAX_WINDOW_HEIGHT;
  }
  if (tmp->hints.max_width < tmp->hints.min_width)
    tmp->hints.max_width = MAX_WINDOW_WIDTH;
  if (tmp->hints.max_height < tmp->hints.min_height)
    tmp->hints.max_height = MAX_WINDOW_HEIGHT;

  /* Zero width/height windows are bad news! */
  if (tmp->hints.min_height <= 0)
    tmp->hints.min_height = 1;
  if (tmp->hints.min_width <= 0)
    tmp->hints.min_width = 1;

  if (!(tmp->hints.flags & PWinGravity))
  {
    tmp->hints.win_gravity = NorthWestGravity;
    tmp->hints.flags |= PWinGravity;
  }
}

Bool
GetWindowState (XfwmWindow * tmp, unsigned long *state)
{
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;

  if (!tmp)
    return (False);

  if ((XGetWindowProperty (dpy, tmp->w, _XA_WIN_STATE, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      *state = (unsigned long) *prop;
      XFree (prop);
      return (True);
    }
  }
  *state = 0L;
  return (False);
}

void
SetWindowState (XfwmWindow * tmp)
{
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;
  unsigned long oldstate = 0L;
  unsigned long newstate = 0L;

  if (!tmp)
    return;

  if ((XGetWindowProperty (dpy, tmp->w, _XA_WIN_STATE, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      oldstate = newstate = (unsigned long) *prop;
      XFree (prop);
      /* For the moment, we just manage sticky property */
      /* Other properties are handled by xfgnome (external module) */
      if (tmp->flags & STICKY)
      {
	newstate |= WIN_STATE_STICKY;
      }
      else
      {
	newstate &= ~WIN_STATE_STICKY;
      }
      if (newstate != oldstate)
      {
	XChangeProperty (dpy, (Window) tmp->w, _XA_WIN_STATE, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &newstate, 1);
      }
    }
  }
}
