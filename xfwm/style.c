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
 * code for parsing the xfwm style command
 *
 ***********************************************************************/
#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
ProcessNewStyle (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *text, int *Module)
{
  char *name, *line, *restofline, *tmp;
  char *icon_name = NULL;
  char *forecolor = NULL;
  char *backcolor = NULL;
  unsigned long off_buttons = 0L;
  unsigned long on_buttons = 0L;

  int desknumber = 0, bw = 0, len = 0, is_quoted = 0, layer = DEFAULT_LAYER;
  unsigned long off_flags = 0L;
  unsigned long on_flags = 0L;

  restofline = GetNextToken (text, &name);
  /* in case there was no argument! */
  if (name == NULL)
    return;

  if (restofline == NULL)
  {
    free (name);
    return;
  }

  off_flags |= MWM_DECOR_FLAG;
  off_flags |= MWM_FUNCTIONS_FLAG;

  while (isspace (*restofline) && (*restofline != 0))
    restofline++;
  line = restofline;

  if (restofline == NULL)
  {
    free (name);
    return;
  }


  while ((*restofline != 0) && (*restofline != '\n'))
  {
    while (isspace (*restofline))
      restofline++;
    switch (tolower (restofline[0]))
    {
    case 'a':
      if (mystrncasecmp (restofline, "allowfreemove", 13) == 0)
      {
	restofline += 13;
	off_flags |= ALLOWFREEMOVE_FLAG;
      }
      else if (mystrncasecmp (restofline, "authorize_translate", 19) == 0)
      {
	restofline += 19;
	off_flags |= TRANSLATE_FLAG;
      }
      break;
    case 'b':
      if (mystrncasecmp (restofline, "borderwidth", 11) == 0)
      {
	restofline += 11;
	off_flags |= BW_FLAG;
	sscanf (restofline, "%d", &bw);
	/* Avoid border width = 1, 2 or 3, use 0 or > 3 */
	if ((bw > 0) && (bw < 4))
	  bw = 4;
	while (isspace (*restofline))
	  restofline++;
	while ((!isspace (*restofline)) && (*restofline != 0) && (*restofline != ',') && (*restofline != '\n'))
	  restofline++;
	while (isspace (*restofline))
	  restofline++;
      }
      break;
    case 'c':
      if (mystrncasecmp (restofline, "circulateskipicon", 17) == 0)
      {
	restofline += 17;
	off_flags |= CIRCULATE_SKIP_ICON_FLAG;
      }
      else if (mystrncasecmp (restofline, "circulateskip", 13) == 0)
      {
	restofline += 13;
	off_flags |= CIRCULATESKIP_FLAG;
      }
      else if (mystrncasecmp (restofline, "circulateskipicon", 17) == 0)
      {
	restofline += 17;
	off_flags |= CIRCULATE_SKIP_ICON_FLAG;
      }
      else if (mystrncasecmp (restofline, "circulatehiticon", 16) == 0)
      {
	restofline += 16;
	on_flags |= CIRCULATE_SKIP_ICON_FLAG;
      }
      break;
    case 'd':
      break;
    case 'e':
      break;
    case 'f':
      break;
    case 'g':
      break;
    case 'h':
      break;
    case 'i':
      if (mystrncasecmp (restofline, "icon", 4) == 0)
      {
	restofline += 4;
	while (isspace (*restofline))
	  restofline++;
	len = 0;
	tmp = restofline;
	is_quoted = 0;
	if (*restofline == '"')
	{
	  is_quoted = 1;
	  ++restofline;
	}
	while ((tmp != NULL) && (*tmp != 0) && (((*tmp != ',') && (*tmp != '\n')) || ((is_quoted && (*tmp != '\n') && (*tmp != '"')))))
	{
	  if (!(is_quoted && (*tmp == '"')))
	    len++;
	  tmp++;
	}
	if (len > 0)
	{
	  icon_name = safemalloc (len + 1);
	  strncpy (icon_name, restofline, len);
	  icon_name[len] = 0;
	  off_flags |= ICON_FLAG;
	  if (is_quoted)
	    tmp++;
	}
	restofline = tmp;
      }
      break;
    case 'j':
      break;
    case 'k':
      break;
    case 'l':
      if (mystrncasecmp (restofline, "layer", 5) == 0)
      {
	restofline += 5;
	sscanf (restofline, "%d", &layer);
	if (layer > MAX_LAYERS)
	  layer = MAX_LAYERS;
	while (isspace (*restofline))
	  restofline++;
	while ((!isspace (*restofline)) && (*restofline != 0) && (*restofline != ',') && (*restofline != '\n'))
	  restofline++;
	while (isspace (*restofline))
	  restofline++;
      }
      break;
    case 'm':
      break;
    case 'n':
      if (mystrncasecmp (restofline, "noborder", 8) == 0)
      {
	restofline += 8;
	off_flags |= NOBORDER_FLAG;
      }
      else if (mystrncasecmp (restofline, "noicon", 6) == 0)
      {
	restofline += 6;
	off_flags |= SUPPRESSICON_FLAG;
      }
      else if (mystrncasecmp (restofline, "notitle", 7) == 0)
      {
	restofline += 7;
	off_flags |= NOTITLE_FLAG;
      }
      break;
    case 'o':
      break;
    case 'p':
      break;
    case 'q':
      break;
    case 'r':
      break;
    case 's':
      if (mystrncasecmp (restofline, "stickyicon", 10) == 0)
      {
	restofline += 10;
	off_flags |= STICKY_ICON_FLAG;
      }
      else if (mystrncasecmp (restofline, "starticonic", 11) == 0)
      {
	restofline += 11;
	off_flags |= START_ICONIC_FLAG;
      }
      else if (mystrncasecmp (restofline, "stayonbottom", 12) == 0)
      {
	/* This one is kept for backward compatibility */
	restofline += 12;
	layer = 0;
      }
      else if (mystrncasecmp (restofline, "stayontop", 9) == 0)
      {
	/* This one is kept for backward compatibility */
	restofline += 9;
	layer = MAX_LAYERS;
      }
      else if (mystrncasecmp (restofline, "sticky", 6) == 0)
      {
	off_flags |= STICKY_FLAG;
	restofline += 6;
      }
      else if (mystrncasecmp (restofline, "startondesk", 11) == 0)
      {
	restofline += 11;
	off_flags |= STARTONDESK_FLAG;
	sscanf (restofline, "%d", &desknumber);
	while (isspace (*restofline))
	  restofline++;
	while ((!isspace (*restofline)) && (*restofline != 0) && (*restofline != ',') && (*restofline != '\n'))
	  restofline++;
	while (isspace (*restofline))
	  restofline++;
      }
      break;
    case 't':
      if (mystrncasecmp (restofline, "title", 5) == 0)
      {
	restofline += 5;
	on_flags |= NOTITLE_FLAG;
      }
      break;
    case 'u':
      break;
    case 'v':
      break;
    case 'w':
      if (mystrncasecmp (restofline, "windowlistskip", 14) == 0)
      {
	restofline += 14;
	off_flags |= LISTSKIP_FLAG;
      }
      else if (mystrncasecmp (restofline, "windowlisthit", 13) == 0)
      {
	restofline += 13;
	on_flags |= LISTSKIP_FLAG;
      }
      break;
    case 'x':
      break;
    case 'y':
      break;
    case 'z':
      break;
    default:
      break;
    }

    while (isspace (*restofline))
      restofline++;
    if (*restofline == ',')
      restofline++;
    else if ((*restofline != 0) && (*restofline != '\n'))
    {
      xfwm_msg (ERR, "ProcessNewStyle", "bad style command: %s", restofline);
      return;
    }
  }

  /* capture default icons */
  if (strcmp (name, "*") == 0)
  {
    if (off_flags & ICON_FLAG)
      Scr.DefaultIcon = icon_name;
    off_flags &= ~ICON_FLAG;
    icon_name = NULL;
  }
  AddToStyleList (name, icon_name, off_flags, on_flags, desknumber, bw, layer, forecolor, backcolor, off_buttons, on_buttons);
}


void
AddToStyleList (char *name, char *icon_name, unsigned long off_flags, unsigned long on_flags, int desk, int bw, int layer, char *forecolor, char *backcolor, unsigned long off_buttons, unsigned long on_buttons)
{
  name_list *nptr, *lastptr = NULL;

  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next)
  {
    lastptr = nptr;
  }

  nptr = (name_list *) safemalloc (sizeof (name_list));
  nptr->next = NULL;
  nptr->name = name;
  nptr->on_flags = on_flags;
  nptr->off_flags = off_flags;
  nptr->value = icon_name;
  nptr->Desk = desk;
  nptr->border_width = bw;
  nptr->layer = layer;
  nptr->ForeColor = forecolor;
  nptr->BackColor = backcolor;
  nptr->off_buttons = off_buttons;
  nptr->on_buttons = on_buttons;

  if (lastptr != NULL)
    lastptr->next = nptr;
  else
    Scr.TheList = nptr;
}

void
FreeStyleList (void)
{
  name_list *nptr, *nextptr = NULL;

  nptr = Scr.TheList;
  while (nptr)
  {
    nextptr = nptr->next;
    if (nptr->name)
    {
      free (nptr->name);
    }
    if (nptr->value)
    {
      if (Scr.DefaultIcon == nptr->value)
      {
	Scr.DefaultIcon = NULL;
      }
      free (nptr->value);
    }
    free (nptr);
    nptr = nextptr;
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	LookInList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	the ptr field of the list structure or NULL if the name 
 *	or class was not found in the list
 *
 *  Inputs:
 *	list	- a pointer to the head of a list
 *	name	- a pointer to the name to look for
 *	class	- a pointer to the class to look for
 *
 ***********************************************************************/
unsigned long
LookInStyleList (char *name, XClassHint * class, char **value, int *Desk, int *border_width, int *layer, char **forecolor, char **backcolor, unsigned long *buttons)
{
  name_list *nptr;
  unsigned long retval = 0;

  *value = NULL;
  *forecolor = NULL;
  *backcolor = NULL;
  *Desk = 0;
  *buttons = 0L;
  *border_width = 0;
  *layer = DEFAULT_LAYER;

  /* look for the name first */
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next)
  {
    if (class)
    {
      /* first look for the res_class  (lowest priority) */
      if (matchWildcards (nptr->name, class->res_class) == True)
      {
	if (nptr->value != NULL)
	  *value = nptr->value;
	if (nptr->off_flags & STARTONDESK_FLAG)
	  *Desk = ((nptr->Desk > 0) ? nptr->Desk - 1 : 0);
	if (nptr->off_flags & BW_FLAG)
	  *border_width = nptr->border_width;
	if (nptr->off_flags & FORE_COLOR_FLAG)
	  *forecolor = nptr->ForeColor;
	if (nptr->off_flags & BACK_COLOR_FLAG)
	  *backcolor = nptr->BackColor;
	retval |= nptr->off_flags;
	retval &= ~(nptr->on_flags);
	*buttons |= nptr->off_buttons;
	*buttons &= ~(nptr->on_buttons);
	*layer = nptr->layer;
      }

      /* look for the res_name next */
      if (matchWildcards (nptr->name, class->res_name) == True)
      {
	if (nptr->value != NULL)
	  *value = nptr->value;
	if (nptr->off_flags & STARTONDESK_FLAG)
	  *Desk = ((nptr->Desk > 0) ? nptr->Desk - 1 : 0);
	if (nptr->off_flags & FORE_COLOR_FLAG)
	  *forecolor = nptr->ForeColor;
	if (nptr->off_flags & BACK_COLOR_FLAG)
	  *backcolor = nptr->BackColor;
	if (nptr->off_flags & BW_FLAG)
	  *border_width = nptr->border_width;
	retval |= nptr->off_flags;
	retval &= ~(nptr->on_flags);
	*buttons |= nptr->off_buttons;
	*buttons &= ~(nptr->on_buttons);
	*layer = nptr->layer;
      }
    }
    /* finally, look for name matches */
    if (matchWildcards (nptr->name, name) == True)
    {
      if (nptr->value != NULL)
	*value = nptr->value;
      if (nptr->off_flags & STARTONDESK_FLAG)
	*Desk = ((nptr->Desk > 0) ? nptr->Desk - 1 : 0);
      if (nptr->off_flags & FORE_COLOR_FLAG)
	*forecolor = nptr->ForeColor;
      if (nptr->off_flags & BACK_COLOR_FLAG)
	*backcolor = nptr->BackColor;
      if (nptr->off_flags & BW_FLAG)
	*border_width = nptr->border_width;
      retval |= nptr->off_flags;
      retval &= ~(nptr->on_flags);
      *buttons |= nptr->off_buttons;
      *buttons &= ~(nptr->on_buttons);
      *layer = nptr->layer;
    }
  }
  return retval;
}
