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


#ifndef __STACK_H__
#define __STACK_H__

#include <X11/Xlib.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "xfwm.h"
#include "screen.h"

void RaiseWindow (XfwmWindow *);
void LowerWindow (XfwmWindow *);
void LowerIcons (void);
Bool GetWindowLayer (XfwmWindow *);
XfwmWindowList *WindowSorted (void);
XfwmWindowList *LastXfwmWindowList (XfwmWindowList *);
XfwmWindow *SearchXfwmWindowList (XfwmWindowList *, Window);
XfwmWindowList *RemoveFromXfwmWindowList (XfwmWindowList *, XfwmWindow *);
XfwmWindowList *AddToXfwmWindowList (XfwmWindowList *, XfwmWindow *);
void FreeXfwmWindowList (XfwmWindowList *);
#ifdef MANAGE_OVERRIDES
XfwmWindowList *LastWindowList (XfwmWindowList *);
Window SearchWindowList (WindowList *, Window);
WindowList *RemoveFromWindowList (WindowList *, Window);
WindowList *AddToWindowList (WindowList *, Window);
void FreeWindowList (WindowList *);
#endif

#endif
