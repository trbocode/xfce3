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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "xfwm.h"
#include "misc.h"
#include "screen.h"
#include "stack.h"
#include "module.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


static int
count_wins (void)
{
  XfwmWindowList *t;
  int count = 0;

  for (t = Scr.stacklist; t != NULL; t = t->next)
  {
    count++;
    if (((t->win)->flags & ICONIFIED) && (!((t->win)->flags & (SUPPRESSICON | ICON_UNMAPPED))))
    {
      count += 2;
    }
  }
  return (count);
}

#ifdef MANAGE_OVERRIDES
static int
count_overrides (void)
{
  WindowList *t;
  int count = 0;

  for (t = Scr.overrides; t != NULL; t = t->next)
  {
    count++;
  }
  return (count);
}
#endif

static XfwmWindowList *
PushStackList (XfwmWindowList * stack, Window * wins, XfwmWindow * t, int *index)
{
  XfwmWindowList *newstack = NULL;
  wins[(*index)++] = t->frame;
  newstack = AddToXfwmWindowList (stack, t);
  if ((t->flags & ICONIFIED) && (!(t->flags & (SUPPRESSICON | ICON_UNMAPPED))))
  {
    wins[(*index)++] = t->icon_w;
    if (t->icon_pixmap_w)
      wins[(*index)++] = t->icon_pixmap_w;
  }
  return (newstack);
}

static void
RestackWindow (XfwmWindow * t, Bool top)
{
#ifdef MANAGE_OVERRIDES
  WindowList *t1;
#endif
  XfwmWindowList *t2;
  XfwmWindowList *newstack = NULL;
  int count, i, j;
  Window *wins;

  /* Count windows */
#ifdef MANAGE_OVERRIDES
  count = count_wins () + count_overrides () + 1;
#else
  count = count_wins () + 1;
#endif
  i = 0;
  /* Allocate memory for stack */
  wins = (Window *) safemalloc (count * sizeof (Window));

#ifdef MANAGE_OVERRIDES
  for (t1 = Scr.overrides; t1 != NULL; t1 = t1->next)
  {
    wins[i++] = t1->w;
  }
#endif

  for (j = MAX_LAYERS; j >= 0; j--)
  {
    if ((t->layer == j) && (top == True))
    {
      /* Raise transients first */
      for (t2 = Scr.stacklist; t2 != NULL; t2 = t2->next)
      {
	if (((t2->win)->flags & TRANSIENT) && ((t2->win)->transientfor == t->w) && (t2->win != t))
	{
	  newstack = PushStackList (newstack, wins, t2->win, &i);
	}
      }
      /* and place the raised window in first position for the given layer */
      newstack = PushStackList (newstack, wins, t, &i);
    }
    /* Other windows */
    for (t2 = Scr.stacklist; t2 != NULL; t2 = t2->next)
    {
      if ((t2->win != t) && !(((t2->win)->flags & TRANSIENT) && ((t2->win)->transientfor == t->w)) && ((t2->win)->layer == j))
      {
	newstack = PushStackList (newstack, wins, t2->win, &i);
      }
    }
    if ((t->layer == j) && (top == False))
    {
      /* Raise transients first */
      for (t2 = Scr.stacklist; t2 != NULL; t2 = t2->next)
      {
	if (((t2->win)->flags & TRANSIENT) && ((t2->win)->transientfor == t->w) && (t2->win != t))
	{
	  newstack = PushStackList (newstack, wins, t2->win, &i);
	}
      }
      /* and place the raised window in first position for the given layer */
      newstack = PushStackList (newstack, wins, t, &i);
    }
  }

  FreeXfwmWindowList (Scr.stacklist);
  Scr.stacklist = newstack;

  if (i > 0)
  {
    XRaiseWindow (dpy, wins[0]);
    XRestackWindows (dpy, wins, i);
  }
  free (wins);
  Broadcast (XFCE_M_RAISE_WINDOW, 3, t->w, t->frame, (unsigned long) t, 0, 0, 0, 0);
  Broadcast (XFCE_M_RESTACK, 0, 0, 0, 0, 0, 0, 0, 0);
}

void
RaiseWindow (XfwmWindow * t)
{
  if (t != Scr.LastWindowRaised)
  {
    RestackWindow (t, True);
  }
  if (t == Scr.LastWindowLowered)
  {
    Scr.LastWindowLowered = NULL;
  }
  Scr.LastWindowRaised = t;
}

void
LowerWindow (XfwmWindow * t)
{
  if (t != Scr.LastWindowLowered)
  {
    RestackWindow (t, False);
  }
  if (t == Scr.LastWindowRaised)
  {
    Scr.LastWindowRaised = NULL;
  }
  Scr.LastWindowLowered = t;
}

void
LowerIcons (void)
{
#ifdef MANAGE_OVERRIDES
  WindowList *t1;
#endif
  XfwmWindowList *t;
  XfwmWindowList *newstack = NULL;
  int count, i, j;
  Window *wins;

  /* Count windows */
#ifdef MANAGE_OVERRIDES
  count = count_wins () + count_overrides () + 1;
#else
  count = count_wins () + 1;
#endif
  i = 0;
  /* Allocate memory for stack */
  wins = (Window *) safemalloc (count * sizeof (Window));

#ifdef MANAGE_OVERRIDES
  for (t1 = Scr.overrides; t1 != NULL; t1 = t1->next)
  {
    wins[i++] = t1->w;
  }
#endif

  Scr.LastWindowLowered = NULL;
  for (j = MAX_LAYERS; j >= 0; j--)
  {
    /* Put regular windows on top */
    for (t = Scr.stacklist; t != NULL; t = t->next)
    {
      if ((!((t->win)->flags & ICONIFIED) || ((t->win)->flags & (SUPPRESSICON | ICON_UNMAPPED))) && ((t->win)->layer == j))
      {
        Scr.LastWindowRaised  = t->win;
	wins[i++] = (t->win)->frame;
	newstack = AddToXfwmWindowList (newstack, t->win);
      }
    }
    /* And icon on bottom */
    for (t = Scr.stacklist; t != NULL; t = t->next)
    {
      if (((t->win)->flags & ICONIFIED) && (!((t->win)->flags & (SUPPRESSICON | ICON_UNMAPPED))) && ((t->win)->layer == j))
      {
	newstack = AddToXfwmWindowList (newstack, t->win);
	wins[i++] = (t->win)->icon_w;
        if (!Scr.LastWindowLowered)
	  Scr.LastWindowLowered = t->win;
	if ((t->win)->icon_pixmap_w)
	  wins[i++] = (t->win)->icon_pixmap_w;
      }
    }
  }

  FreeXfwmWindowList (Scr.stacklist);
  Scr.stacklist = newstack;

  if (i > 0)
  {
    XRaiseWindow (dpy, wins[0]);
    XRestackWindows (dpy, wins, i);
  }
  free (wins);
  
  Broadcast (XFCE_M_RESTACK, 0, 0, 0, 0, 0, 0, 0, 0);
}

Bool
GetWindowLayer (XfwmWindow * tmp)
{
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;
  if ((XGetWindowProperty (dpy, tmp->w, _XA_WIN_LAYER, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      tmp->layer = *(unsigned long *) prop;
      if (tmp->layer < 0)
	tmp->layer = 0;
      if (tmp->layer > MAX_LAYERS)
	tmp->layer = MAX_LAYERS;
      XFree (prop);
      return (True);
    }
  }
  return (False);
}

XfwmWindowList *
WindowSorted (void)
{
  XfwmWindowList *wl, *i, *prev, *start;
  XfwmWindow *t;

  start = NULL;

  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    i = start;
    prev = NULL;
    while ((i != NULL) && (mystrcasecmp (t->name, i->win->name) > 0))
    {
      prev = i;
      i = i->next;
    }
    /* Create new element */
    wl = (XfwmWindowList *) safemalloc (sizeof (XfwmWindowList));
    wl->win = t;
    if (i == NULL)		/* Insert after prev */
    {
      if (prev == NULL)
      {
	start = wl;
	wl->next = NULL;
	wl->prev = NULL;
      }
      else
      {
	prev->next = wl;
	wl->prev = prev;
	wl->next = NULL;
      }
    }
    else			/* Insert before i */
    {
      if (prev == NULL)
      {
	start = wl;
	wl->prev = NULL;
	wl->next = i;
	i->prev = wl;
      }
      else
      {
	prev->next = wl;
	wl->prev = prev;
	wl->next = i;
      }
    }
  }
  return (start);
}

XfwmWindowList *
LastXfwmWindowList (XfwmWindowList * start)
{
  XfwmWindowList *i, *last;

  i = start;
  last = NULL;
  while (i != NULL)
  {
    last = i;
    i = i->next;
  }
  return (last);
}

XfwmWindow *
SearchXfwmWindowList (XfwmWindowList * start, Window t)
{
  XfwmWindowList *i;

  i = start;
  while ((i != NULL) && ((i->win)->w != t))
  {
    i = i->next;
  }

  if (i != NULL)
  {
    return (i->win);
  }
  return (NULL);
}

XfwmWindowList *
RemoveFromXfwmWindowList (XfwmWindowList * start, XfwmWindow * t)
{
  XfwmWindowList *i, *new_start;

  i = start;
  new_start = start;
  while ((i != NULL) && (i->win != t))
  {
    i = i->next;
  }

  if (i != NULL)
  {
    if (i->prev != NULL)
    {
      i->prev->next = i->next;
    }
    if (i->next != NULL)
    {
      i->next->prev = i->prev;
    }
    if (i == start)
    {
      new_start = i->next;
    }
    free (i);
  }
  return (new_start);
}

XfwmWindowList *
AddToXfwmWindowList (XfwmWindowList * start, XfwmWindow * t)
{
  XfwmWindowList *i, *prev, *new, *new_start;

  if (!t)
    return (start);

  prev = NULL;
  i = start;
  new_start = start;
  while (i != NULL)
  {
    prev = i;
    i = i->next;
  }

  new = (XfwmWindowList *) safemalloc (sizeof (XfwmWindowList));
  new->next = NULL;
  new->prev = prev;
  new->win = t;
  if (prev != NULL)
    prev->next = new;
  if (start == NULL)
  {
    new_start = new;
  }
  return (new_start);
}

void
FreeXfwmWindowList (XfwmWindowList * start)
{
  XfwmWindowList *i, *next;

  i = start;
  while (i != NULL)
  {
    next = i->next;
    free (i);
    i = next;
  }
  start = NULL;
}

#ifdef MANAGE_OVERRIDES
XfwmWindowList *
LastWindowList (XfwmWindowList * start)
{
  XfwmWindowList *i, *last;

  i = start;
  last = NULL;
  while (i != NULL)
  {
    last = i;
    i = i->next;
  }
  return (last);
}

Window SearchWindowList (WindowList * start, Window t)
{
  WindowList *i;

  i = start;
  while ((i != NULL) && (i->w != t))
  {
    i = i->next;
  }

  if (i != NULL)
  {
    return (i->w);
  }
  return (None);
}

WindowList *
RemoveFromWindowList (WindowList * start, Window t)
{
  WindowList *i, *prev, *new_start;

  i = start;
  prev = NULL;
  new_start = start;
  while ((i != NULL) && (i->w != t))
  {
    prev = i;
    i = i->next;
  }

  if (i != NULL)
  {
    if (prev != NULL)
    {
      prev->next = i->next;
    }
    if (i == start)
    {
      new_start = i->next;
    }
    free (i);
    i = NULL;
  }
  return (new_start);
}

WindowList *
AddToWindowList (WindowList * start, Window t)
{
  WindowList *i, *prev, *new, *new_start;

  prev = NULL;
  i = start;
  new_start = start;
  while (i != NULL)
  {
    prev = i;
    i = i->next;
  }

  new = (WindowList *) safemalloc (sizeof (WindowList));
  new->next = NULL;
  new->w = t;
  if (prev != NULL)
    prev->next = new;
  if (start == NULL)
  {
    new_start = new;
  }
  return (new_start);
}

void
FreeWindowList (WindowList * start)
{
  WindowList *i, *next;

  i = start;
  while (i != NULL)
  {
    next = i->next;
    free (i);
    i = next;
  }
  start = NULL;
}
#endif
