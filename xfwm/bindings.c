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
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define Shift  1
#define Ctrl   2
#define Alt    4
#define Meta   8
#define Super  16
#define Hyper  32

struct charstring
{
  char key;
  int value;
};

/* The keys musat be in lower case! */
struct charstring win_contexts[] = {
  {'w', C_WINDOW},
  {'t', C_TITLE},
  {'i', C_ICON},
  {'r', C_ROOT},
  {'f', C_FRAME},
  {'s', C_SIDEBAR},
  {'1', C_L1},
  {'2', C_R1},
  {'3', C_L2},
  {'4', C_R2},
  {'5', C_L3},
  {'6', C_R3},
  {'a', C_WINDOW | C_TITLE | C_ICON | C_ROOT | C_FRAME | C_SIDEBAR | C_L1 | C_L2 | C_L3 | C_R1 | C_R2 | C_R3},
  {0, 0}
};

/* The keys musat be in lower case! */
struct charstring key_modifiers[] = {
  {'s', ShiftMask},
  {'c', ControlMask},
  {'m', Mod1Mask},
  {'1', Mod1Mask},
  {'2', Mod2Mask},
  {'3', Mod3Mask},
  {'4', Mod4Mask},
  {'5', Mod5Mask},
  {'a', AnyModifier},
  {'n', 0},
  {0, 0}
};

unsigned int
vmod (int m)
{
  int vm = 0;

  if (m & ShiftMask)
    vm |= Shift;
  if (m & ControlMask)
    vm |= Ctrl;
  if (m & AltMask)
    vm |= Alt;
  if (m & MetaMask)
    vm |= Meta;
  if (m & SuperMask)
    vm |= Super;
  if (m & HyperMask)
    vm |= Hyper;

  if ((vm & (Ctrl | Alt | Meta)) == Meta)
    vm = (vm & ~Meta) | Ctrl | Alt;

  return vm;
}

void find_context (char *string, int *output, struct charstring *table, char *tline);

/*
 * ** to remove a binding from the global list (probably needs more processing
 * ** for mouse binding lines though, like when context is a title bar button).
 */
void
remove_binding (int contexts, int mods, int button, KeySym keysym, int mouse_binding)
{
  Binding *temp = Scr.AllBindings, *temp2, *prev = NULL;
  KeyCode keycode = 0;
  XfwmWindow *t;

  if (!mouse_binding)
    keycode = XKeysymToKeycode (dpy, keysym);

  while (temp)
  {
    temp2 = temp->NextBinding;
    if (temp->IsMouse == mouse_binding)
    {
      if ((temp->Button_Key == ((mouse_binding) ? (button) : (keycode))) && (temp->Context == contexts) && (temp->Modifier == mods))
      {
	/* we found it, remove it from list */
	if ((temp->Action != NULL) && (temp->Context & C_WINDOW) && (temp->IsMouse == 1))
	{
	  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
	  {
	    if (temp->Button_Key > 0)
	      MyXUngrabButton (dpy, temp->Button_Key, temp->Modifier, t->w);
	    else
	      MyXUngrabButton (dpy, AnyButton, temp->Modifier, t->w);
	  }
	}
	if (prev)		/* middle of list */
	{
	  prev->NextBinding = temp2;
	}
	else
	  /* must have been first one, set new start */
	{
	  Scr.AllBindings = temp2;
	}
	free (temp);
	temp = NULL;
      }
    }
    if (temp)
      prev = temp;
    temp = temp2;
  }
}

/****************************************************************************
 * 
 *  Parses a mouse binding - probably should combine w/ ParseKeyEntry to save
 *  some more memory (CKH)
 *
 ****************************************************************************/
void
ParseMouseEntry (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long junk, char *tline, int *Module)
{
  char context[20], modifiers[20], *ptr, *action, *token;
  Binding *temp;
  int button, i, j;
  int n1 = 0, n2 = 0, n3 = 0;
  int contexts;
  int mods;

  /* tline points after the key word "Mouse" */
  ptr = tline;
  ptr = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n1 = sscanf (token, "%d", &button);
    free (token);
  }

  ptr = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n2 = sscanf (token, "%19s", context);
    free (token);
  }

  action = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n3 = sscanf (token, "%19s", modifiers);
    free (token);
  }
  if ((n1 != 1) || (n2 != 1) || (n3 != 1))
  {
    xfwm_msg (ERR, "ParseMouseEntry", "Mouse binding: Syntax error in line %s", tline);
    return;
  }

  find_context (context, &contexts, win_contexts, tline);
  find_context (modifiers, &mods, key_modifiers, tline);

  /*
   * ** strip leading whitespace from action if necessary
   */
  while (*action && (*action == ' ' || *action == '\t'))
    action++;

  /*
   * ** is this an unbind request?
   */
  if (!action || action[0] == '-')
  {
    remove_binding (contexts, mods, button, 0, 1);
    return;
  }

  if ((contexts != C_ALL) && (contexts & C_LALL))
  {
    /* check for nr_left_buttons */
    i = 0;
    j = (contexts & C_LALL) / C_L1;
    while (j > 0)
    {
      i++;
      j = j >> 1;
    }
    if ((Scr.nr_left_buttons < i) && (i < 4))
      Scr.nr_left_buttons = i;
  }
  if ((contexts != C_ALL) && (contexts & C_RALL))
  {
    /* check for nr_right_buttons */
    i = 0;
    j = (contexts & C_RALL) / C_R1;
    while (j > 0)
    {
      i++;
      j = j >> 1;
    }
    if ((Scr.nr_right_buttons < i) && (i < 4))
      Scr.nr_right_buttons = i;
  }

  if ((mods & AnyModifier) && (mods & (~AnyModifier)))
  {
    xfwm_msg (WARN, "ParseMouseEntry", "Binding specified AnyModifier and other modifers too. Excess modifiers will be ignored.");

  }

  temp = Scr.AllBindings;
  Scr.AllBindings = (Binding *) safemalloc (sizeof (Binding));
  Scr.AllBindings->IsMouse = 1;
  Scr.AllBindings->Button_Key = button;
  Scr.AllBindings->key_name = NULL;
  Scr.AllBindings->Context = contexts;
  Scr.AllBindings->Modifier = mods;
  Scr.AllBindings->Action = stripcpy (action);
  Scr.AllBindings->NextBinding = temp;
  return;
}

/****************************************************************************
 * 
 *  Processes a line with a key binding
 *
 ****************************************************************************/
void
ParseKeyEntry (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long junk, char *tline, int *Module)
{
  char *action, context[20], modifiers[20], key[20], *ptr, *token;
  Binding *temp;
  int i, min, max;
  int n1 = 0, n2 = 0, n3 = 0;
  KeySym keysym;
  int contexts;
  int mods;

  /* tline points after the key word "key" */
  ptr = tline;

  ptr = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n1 = sscanf (token, "%19s", key);
    free (token);
  }

  ptr = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n2 = sscanf (token, "%19s", context);
    free (token);
  }
  action = GetNextToken (ptr, &token);
  if (token != NULL)
  {
    n3 = sscanf (token, "%19s", modifiers);
    free (token);
  }

  if ((n1 != 1) || (n2 != 1) || (n3 != 1))
  {
    xfwm_msg (ERR, "ParseKeyEntry", "Syntax error in line %s", tline);
    return;
  }

  find_context (context, &contexts, win_contexts, tline);
  find_context (modifiers, &mods, key_modifiers, tline);

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym (key)) == NoSymbol || (XKeysymToKeycode (dpy, keysym)) == 0)
    return;

  /*
   * ** strip leading whitespace from action if necessary
   */
  while (*action && (*action == ' ' || *action == '\t'))
    action++;

  /*
   * ** is this an unbind request?
   */
  if (!action || action[0] == '-')
  {
    remove_binding (contexts, mods, 0, keysym, 0);
    return;
  }

  if ((mods & AnyModifier) && (mods & (~AnyModifier)))
  {
    xfwm_msg (WARN, "ParseKeyEntry", "Binding specified AnyModifier and other modifers too. Excess modifiers will be ignored.");
  }


  /*
   * ** why wasn't XKeysymToKeycode used instead of this for loop?
   */
  XDisplayKeycodes (dpy, &min, &max);
  for (i = min; i <= max; i++)
    if (XKeycodeToKeysym (dpy, i, 0) == keysym)
    {
      temp = Scr.AllBindings;
      Scr.AllBindings = (Binding *) safemalloc (sizeof (Binding));
      Scr.AllBindings->IsMouse = 0;
      Scr.AllBindings->Button_Key = i;
      Scr.AllBindings->key_name = stripcpy (key);;
      Scr.AllBindings->Context = contexts;
      Scr.AllBindings->Modifier = mods;
      Scr.AllBindings->Action = stripcpy (action);
      Scr.AllBindings->NextBinding = temp;
    }
  return;
}

/****************************************************************************
 * 
 * Turns a  string context of context or modifier values into an array of 
 * true/false values (bits)
 *
 ****************************************************************************/
void
find_context (char *string, int *output, struct charstring *table, char *tline)
{
  int i = 0, j = 0;
  Bool matched;
  char tmp1;

  *output = 0;
  i = 0;
  while (i < strlen (string))
  {
    j = 0;
    matched = False;
    while ((!matched) && (table[j].key != 0))
    {
      /* in some BSD implementations, tolower(c) is not defined
       * unless isupper(c) is true */
      tmp1 = string[i];
      if (isupper (tmp1))
	tmp1 = tolower (tmp1);
      /* end of ugly BSD patch */

      if (tmp1 == table[j].key)
      {
	*output |= table[j].value;
	matched = True;
      }
      j++;
    }
    if (!matched)
    {
      xfwm_msg (ERR, "find_context", "bad context in line %s", tline);
    }
    i++;
  }
  return;
}
