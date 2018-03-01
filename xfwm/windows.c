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
 * This module from original code
 * by Rob Nation 
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * xfwm window-list popup code
 *
 ***********************************************************************/


#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <limits.h>

#include "utils.h"

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "stack.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define SHOW_GEOMETRY (1<<0)
#define SHOW_ALLDESKS (1<<1)
#define SHOW_NORMAL   (1<<2)
#define SHOW_ICONIC   (1<<3)
#define SHOW_STICKY   (1<<4)
#define SHOW_ICONNAME (1<<7)
#define SHOW_ALPHABETIC (1<<8)
#define SHOW_EVERYTHING (SHOW_GEOMETRY | SHOW_ALLDESKS | SHOW_NORMAL | SHOW_ICONIC | SHOW_STICKY)

/* Function to compare window title names
 */
static int globalFlags;
int
winCompare (const XfwmWindow ** a, const XfwmWindow ** b)
{
  if (globalFlags & SHOW_ICONNAME)
    return strcasecmp ((*a)->icon_name, (*b)->icon_name);
  else
    return strcasecmp ((*a)->name, (*b)->name);
}


/*
 * Change by PRB (pete@tecc.co.uk), 31/10/93.  Prepend a hot key
 * specifier to each item in the list.  This means allocating the
 * memory for each item (& freeing it) rather than just using the window
 * title directly.  */
void
do_windowList (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  MenuRoot *mr;
  XfwmWindowList *wl, *u;
  char *tname = NULL;
  char *name = NULL;
  char *func = NULL;
  char tlabel[51] = "";
  char *t_hot = NULL;		/* Menu label with hotkey added */
  char scut = '0';		/* Current short cut key */
  char *line = NULL, *tok = NULL;
  int dwidth, dheight;
  int desk = Scr.CurrentDesk;
  int flags = SHOW_EVERYTHING;
  int ln;

  if (action && *action)
  {
    /* local copy */
    line = strdup (action);
    /* parse args */
    while (line && *line)
    {
      tok = GetToken (&line);
      if (!tok)
      {
	break;
      }
      if (StrEquals (tok, "Function"))
      {
	func = GetToken (&line);
      }
      else if (StrEquals (tok, "Desk"))
      {
	char *pc = GetToken (&line);
	if (pc)
	{
	  desk = atoi (pc);
	  free (pc);
	  flags &= ~SHOW_ALLDESKS;
	}
      }
      else if (StrEquals (tok, "CurrentDesk"))
      {
	desk = Scr.CurrentDesk;
	flags &= ~SHOW_ALLDESKS;
      }
      else if (StrEquals (tok, "UseIconName"))
	flags |= SHOW_ICONNAME;
      else if (StrEquals (tok, "NoGeometry"))
	flags &= ~SHOW_GEOMETRY;
      else if (StrEquals (tok, "Geometry"))
	flags |= SHOW_GEOMETRY;
      else if (StrEquals (tok, "NoIcons"))
	flags &= ~SHOW_ICONIC;
      else if (StrEquals (tok, "Icons"))
	flags |= SHOW_ICONIC;
      else if (StrEquals (tok, "OnlyIcons"))
	flags = SHOW_ICONIC;
      else if (StrEquals (tok, "NoNormal"))
	flags &= ~SHOW_NORMAL;
      else if (StrEquals (tok, "Normal"))
	flags |= SHOW_NORMAL;
      else if (StrEquals (tok, "OnlyNormal"))
	flags = SHOW_NORMAL;
      else if (StrEquals (tok, "NoSticky"))
	flags &= ~SHOW_STICKY;
      else if (StrEquals (tok, "Sticky"))
	flags |= SHOW_STICKY;
      else if (StrEquals (tok, "OnlySticky"))
	flags = SHOW_STICKY;
      else
      {
	xfwm_msg (ERR, "WindowList", "Unknown option '%s'", tok);
      }
      free (tok);
    }
  }

  globalFlags = flags;
  if (flags & SHOW_GEOMETRY)
  {
    snprintf (tlabel, 50, "Desk: %d\tGeometry", desk);
  }
  else
  {
    snprintf (tlabel, 50, "Desk: %d", desk);
  }
  mr = NewMenuRoot (tlabel, 0);

  wl = WindowSorted ();
  for (u = wl; u != NULL; u = u->next)
  {
    if (((u->win->Desk == desk) || (flags & SHOW_ALLDESKS)) && (!(u->win->flags & WINDOWLISTSKIP)))
    {
      if (!(AcceptInput (u->win)))
      {
	continue;
      }
      if (!(flags & SHOW_ICONIC) && (u->win->flags & ICONIFIED))
      {
	continue;		/* don't want icons - skip */
      }
      if (!(flags & SHOW_STICKY) && (u->win->flags & STICKY))
      {
	continue;		/* don't want sticky ones - skip */
      }
      if (!(flags & SHOW_NORMAL) && !((u->win->flags & ICONIFIED) || (u->win->flags & STICKY)))
      {
	continue;		/* don't want "normal" ones - skip */
      }
      if (scut > 0)
      {
	scut++;
	if (scut == ('9' + 1))
	{
	  scut = 'a';
	}
	else if (scut == ('z' + 1))
	{
	  scut = 'A';
	}
	else if (scut == ('Z' + 1))
	{
	  scut = 0;
	}
      }
      if (flags & SHOW_ICONNAME)
      {
	name = u->win->icon_name;
      }
      else
      {
	name = u->win->name;
      }
      tname = safemalloc (81);
      tname[0] = '\0';
      if (flags & SHOW_GEOMETRY)
      {
	dheight = u->win->frame_height - u->win->title_height - 2 * u->win->boundary_width;
	dwidth = u->win->frame_width - 2 * u->win->boundary_width;

	dwidth -= u->win->hints.base_width;
	dheight -= u->win->hints.base_height;

	dwidth /= u->win->hints.width_inc;
	dheight /= u->win->hints.height_inc;

	snprintf (tname, 80, "\t[%d] %dx%d%+d%+d", u->win->Desk + 1, dwidth, dheight, u->win->frame_x, u->win->frame_y);
      }
      ln = strlen (name) + strlen (tname) + 12;
      t_hot = safemalloc (ln + 1);
      if (u->win->flags & ICONIFIED)
      {
	if (scut > 0)
	{
	  snprintf (t_hot, ln, "&%c. (%s)%s", scut, name, tname);
	}
	else
	{
	  snprintf (t_hot, ln, "     (%s)%s", name, tname);
	}
      }
      else
      {
	if (scut > 0)
	{
	  snprintf (t_hot, ln, "&%c.  %s %s", scut, name, tname);
	}
	else
	{
	  snprintf (t_hot, ln, "      %s %s", name, tname);
	}
      }
      if (func)
      {
	snprintf (tlabel, 50, "%s %ld", func, u->win->w);
	free (func);
	func = NULL;
      }
      else
      {
	snprintf (tlabel, 50, "WindowListFunc %ld", u->win->w);
      }
      AddToMenu (mr, t_hot, tlabel);
      if (t_hot)
      {
	free (t_hot);
	t_hot = NULL;
      }
      if (tname)
      {
	free (tname);
	tname = NULL;
      }
    }
  }
  FreeXfwmWindowList (wl);
  MakeMenu (mr);

  /* and1000@cam.ac.uk, 27/6/96 */
  do_menu (mr, eventp->type == ButtonPress);

  DestroyMenu (mr);
}
