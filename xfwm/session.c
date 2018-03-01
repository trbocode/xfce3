/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Initially inspired by fvwm2, enlightment and twm implementations */

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <signal.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "session.h"
#include "misc.h"
#include "screen.h"
#include "utils.h"
#include "constant.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern int master_pid;


typedef struct _match
{
  unsigned long win;
  unsigned long client_leader;
  char *client_id;
  char *res_name;
  char *res_class;
  char *window_role;
  char *wm_name;
  int wm_command_count;
  char **wm_command;
  int x, y, w, h, icon_x, icon_y;
  int desktop;
  int focusdesk;
  unsigned long flags;
  int used;
}
Match;

int sm_fd = -1;

static char *sm_client_id = NULL;
static int num_match = 0;
static Match *matches = NULL;
static Bool sent_save_done = 0;

extern Bool Restarting;

static char *last_used_filename = NULL;

/* 
   2-pass function to compute new string length,
   allocate memory and finally copy string 
   - Returned value must be freed -
 */
static char *
escape_quote (char *s)
{
  char *ns;
  char *idx1, *idx2;
  int nbquotes = 0;
  int lg = 0;

  if (!s)
    return NULL;

  lg = strlen (s);
  /* First, count quotes in string */
  idx1 = s;
  while (*idx1)
  {
    if (*(idx1++) == '"')
      nbquotes++;
  }
  /* If there is no quote in the string, return it */
  if (!nbquotes)
    return (strdup (s));
  /* Or else, allocate memory for the new string */
  ns = (char *) safemalloc (sizeof (char) * (lg + nbquotes + 1));
  /* And prepend a backslash before any quote found in string */
  idx1 = s;
  idx2 = ns;
  while (*idx1)
  {
    if (*idx1 == '"')
    {
      *(idx2++) = '\\';
      *(idx2++) = '"';
    }
    else
    {
      *(idx2++) = *idx1;
    }
    idx1++;
  }
  /* Add null char */
  *idx2 = '\0';
  return ns;
}

/* 
   single-pass function to replace backslash+quotes
   by quotes. 
   - Returned value must be freed -
 */
static char *
unescape_quote (char *s)
{
  char *ns;
  Bool backslash;
  char *idx1, *idx2;
  int lg;

  if (!s)
    return NULL;

  lg = strlen (s);
  backslash = False;
  ns = (char *) safemalloc (sizeof (char) * (lg + 1));
  idx1 = s;
  idx2 = ns;
  while (*idx1)
  {
    if (*idx1 == '\\')
    {
      *(idx2++) = *idx1;
      backslash = True;
    }
    else if ((*idx1 == '"') && backslash)
    {
      /* Move backward to override the "\" */
      *(--idx2) = *idx1;
      idx2++;
      backslash = False;
    }
    else
    {
      *(idx2++) = *idx1;
      backslash = False;
    }
    idx1++;
  }
  *idx2 = '\0';
  return ns;
}

static char *
getsubstring (char *s, int *length)
{
  char pbrk;
  char *ns;
  char *end, *idx1, *idx2, *skip;
  int lg;
  Bool finished = False, backslash = False;

  lg = *length = 0;
  if (!s)
    return NULL;

  end = skip = s;
  while ((*skip == ' ') || (*skip == '\t'))
  {
    end = ++skip;
    (*length)++;
  }
  if (*skip == '"')
  {
    pbrk = '"';
    end = ++skip;
    (*length)++;
  }
  else
  {
    pbrk = ' ';
  }

  finished = False;
  while ((!finished) && (*end))
  {
    if (*end == '\\')
    {
      backslash = True;
    }
    else if ((*end == pbrk) && backslash)
    {
      backslash = False;
    }
    else if (*end == pbrk)
    {
      finished = True;
    }
    end++;
    lg++;
    (*length)++;
  }
  ns = (char *) safemalloc (sizeof (char) * (lg + 1));
  /* Skip pbrk character */
  end--;
  idx1 = skip;
  idx2 = ns;
  do
  {
    *(idx2++) = *idx1;
  }
  while (++idx1 < end);
  *idx2 = '\0';
  return ns;
}

#ifdef HAVE_SESSION
#include <X11/SM/SMlib.h>

extern char **g_argv;
extern int g_argc;

SmcConn sm_conn = NULL;

static void set_sm_properties (SmcConn sm_conn, char *filename, char hint);

static int save_session_state (SmcConn sm_conn, char *filename, FILE * f);

#endif /* HAVE_SESSION */

static char *
GetWindowRole (Window window)
{
  XTextProperty tp;

  if (XGetTextProperty (dpy, window, &tp, _XA_WM_WINDOW_ROLE))
  {
    if (tp.encoding == XA_STRING && tp.format == 8 && tp.nitems != 0)
      return ((char *) tp.value);
  }

  return NULL;
}

static Window
GetClientLeader (Window window)
{
  Window client_leader = 0;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty (dpy, window, _XA_WM_CLIENT_LEADER, 0L, 1L, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success)
  {
    if (actual_type == XA_WINDOW && actual_format == 32 && nitems == 1 && bytes_after == 0)
      client_leader = *((Window *) prop);
    XFree (prop);
  }
  return client_leader;
}

static char *
GetClientID (Window window)
{
  char *client_id = NULL;
  Window client_leader;
  XTextProperty tp;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty (dpy, window, _XA_WM_CLIENT_LEADER, 0L, 1L, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success)
  {
    if (actual_type == XA_WINDOW && actual_format == 32 && nitems == 1 && bytes_after == 0)
    {
      client_leader = *((Window *) prop);

      if (XGetTextProperty (dpy, client_leader, &tp, _XA_SM_CLIENT_ID))
      {
	if (tp.encoding == XA_STRING && tp.format == 8 && tp.nitems != 0)
	  client_id = (char *) tp.value;
      }
    }

    if (prop)
      XFree (prop);
  }

  return client_id;
}

/* The following tries to get the command line from the XfwmWindow */
static int
GetWindowCommand (XfwmWindow * tmp_win, char ***argv, int *argc)
{
  int id;

  if (XGetCommand (dpy, tmp_win->w, argv, argc) && (*argc > 0))
    return 1;
  if ((id = GetClientLeader (tmp_win->w)))
  {
    if (XGetCommand (dpy, id, argv, argc) && (*argc > 0))
      return 1;
  }
  return 0;
}

static int
SaveWindowStates (FILE * f)
{
  char *client_id;
  char *window_role;
  char **wm_command;
  int wm_command_count;
  unsigned long client_leader;
  XfwmWindow *tmp_win;
  int i;

  for (tmp_win = Scr.XfwmRoot.next; tmp_win; tmp_win = tmp_win->next)
  {
    /*
     * Skip transient windows
    if ((tmp_win->flags & TRANSIENT) && (tmp_win->transientfor != tmp_win->w))
      continue;
     */

    fprintf (f, "[CLIENT] %lx\n", tmp_win->w);

    client_id = GetClientID (tmp_win->w);
    if (client_id)
    {
      fprintf (f, "  [CLIENT_ID] %s\n", client_id);
      XFree (client_id);
    }

    client_leader = GetClientLeader (tmp_win->w);
    if (client_leader)
      fprintf (f, "  [CLIENT_LEADER] %li\n", client_leader);

    window_role = GetWindowRole (tmp_win->w);
    if (window_role)
    {
      fprintf (f, "  [WINDOW_ROLE] %s\n", window_role);
      XFree (window_role);
    }

    if (tmp_win->class.res_class)
      fprintf (f, "  [RES_NAME] %s\n", tmp_win->class.res_name);

    if (tmp_win->class.res_name)
      fprintf (f, "  [RES_CLASS] %s\n", tmp_win->class.res_class);

    if (tmp_win->name)
      fprintf (f, "  [WM_NAME] %s\n", tmp_win->name);

    if (GetWindowCommand (tmp_win, &wm_command, &wm_command_count) && (wm_command_count > 0))
    {
      fprintf (f, "  [WM_COMMAND] %i", wm_command_count);
      for (i = 0; i < wm_command_count; i++)
      {
	char *escaped_string;
	escaped_string = escape_quote (wm_command[i]);
	fprintf (f, " \"%s\"", escaped_string);
	free (escaped_string);
      }
      fprintf (f, "\n");
      XFreeStringList (wm_command);
    }

    fprintf (f, "  [GEOMETRY] %i %i %i %i %i %i\n", tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd - 2 * (tmp_win->boundary_width + tmp_win->bw), tmp_win->orig_ht - 2 * (tmp_win->boundary_width + tmp_win->bw) - tmp_win->title_height, tmp_win->icon_x_loc, tmp_win->icon_y_loc);
    fprintf (f, "  [FOCUSDESK] %i\n", tmp_win->FocusDesk);
    fprintf (f, "  [DESK] %i\n", ((tmp_win->flags & (STICKY)) ? 0 : tmp_win->Desk));
    fprintf (f, "  [FLAGS] %li\n", (tmp_win->flags & (STICKY | ICONIFIED | ICON_MOVED | SHADED | WM_NAME_CHANGED)));
  }
  return 1;
}

void
LoadWindowStates (char *filename)
{
  FILE *f;
  char s[4096], s1[4096];
  int i, pos, pos1;
  unsigned long w;

  /* We just ignore session management if restarting xfwm */
  if (Restarting)
    return;

  if (filename && (f = fopen (filename, "r")))
  {
    while (fgets (s, sizeof (s), f))
    {
      sscanf (s, "%4000s", s1);
      if (!strcmp (s1, "[CLIENT]"))
      {
	sscanf (s, "%*s %lx", &w);
	num_match++;
	matches = realloc (matches, sizeof (Match) * num_match);
	matches[num_match - 1].win = w;
	matches[num_match - 1].client_id = NULL;
	matches[num_match - 1].res_name = NULL;
	matches[num_match - 1].res_class = NULL;
	matches[num_match - 1].window_role = NULL;
	matches[num_match - 1].wm_name = NULL;
	matches[num_match - 1].wm_command_count = 0;
	matches[num_match - 1].wm_command = NULL;
	matches[num_match - 1].x = 0;
	matches[num_match - 1].y = 0;
	matches[num_match - 1].w = 100;
	matches[num_match - 1].h = 100;
	matches[num_match - 1].icon_x = 0;
	matches[num_match - 1].icon_y = 0;
	matches[num_match - 1].desktop = 0;
	matches[num_match - 1].focusdesk = 0;
	matches[num_match - 1].used = 0;
	matches[num_match - 1].flags = 0;
      }
      else if (!strcmp (s1, "[GEOMETRY]"))
      {
	sscanf (s, "%*s %i %i %i %i %i %i", &(matches[num_match - 1].x), &(matches[num_match - 1].y), &(matches[num_match - 1].w), &(matches[num_match - 1].h), &(matches[num_match - 1].icon_x), &(matches[num_match - 1].icon_y));
      }
      else if (!strcmp (s1, "[DESK]"))
      {
	sscanf (s, "%*s %i", &(matches[num_match - 1].desktop));
      }
      else if (!strcmp (s1, "[FOCUSDESK]"))
      {
	sscanf (s, "%*s %i", &(matches[num_match - 1].focusdesk));
      }
      else if (!strcmp (s1, "[CLIENT_LEADER]"))
      {
	sscanf (s, "%*s %li", &(matches[num_match - 1].client_leader));
      }
      else if (!strcmp (s1, "[FLAGS]"))
      {
	sscanf (s, "%*s %li", &(matches[num_match - 1].flags));
      }
      else if (!strcmp (s1, "[CLIENT_ID]"))
      {
	sscanf (s, "%*s %[^\n]", s1);
	matches[num_match - 1].client_id = strdup (s1);
      }
      else if (!strcmp (s1, "[WINDOW_ROLE]"))
      {
	sscanf (s, "%*s %[^\n]", s1);
	matches[num_match - 1].window_role = strdup (s1);
      }
      else if (!strcmp (s1, "[RES_NAME]"))
      {
	sscanf (s, "%*s %[^\n]", s1);
	matches[num_match - 1].res_name = strdup (s1);
      }
      else if (!strcmp (s1, "[RES_CLASS]"))
      {
	sscanf (s, "%*s %[^\n]", s1);
	matches[num_match - 1].res_class = strdup (s1);
      }
      else if (!strcmp (s1, "[WM_NAME]"))
      {
	sscanf (s, "%*s %[^\n]", s1);
	matches[num_match - 1].wm_name = strdup (s1);
      }
      else if (!strcmp (s1, "[WM_COMMAND]"))
      {
	sscanf (s, "%*s %i%n", &matches[num_match - 1].wm_command_count, &pos);
	matches[num_match - 1].wm_command = (char **) safemalloc ((matches[num_match - 1].wm_command_count + 1) * sizeof (char *));
	for (i = 0; i < matches[num_match - 1].wm_command_count; i++)
	{
	  char *substring;
	  substring = getsubstring (s + pos, &pos1);
	  pos += pos1;
	  matches[num_match - 1].wm_command[i] = unescape_quote (substring);
	  free (substring);
	}
	matches[num_match - 1].wm_command[matches[num_match - 1].wm_command_count] = NULL;
      }

    }
    fclose (f);
  }
}

/* Hey, don't forget to clean up your stuff before you leave ! */
void
FreeWindowStates (void)
{
  int i;

  for (i = 0; i < num_match; i++)
  {
    if (matches[i].client_id)
      free (matches[i].client_id);
    if (matches[i].res_name)
      free (matches[i].res_name);
    if (matches[i].res_class)
      free (matches[i].res_class);
    if (matches[i].window_role)
      free (matches[i].window_role);
    if (matches[i].wm_name)
      free (matches[i].wm_name);
    if (matches[i].wm_command_count)
      XFreeStringList (matches[i].wm_command);
  }
}

/* This complicated logic is from twm, where it is explained */

#define xstreq(a,b) ((!a && !b) || (a && b && (strcmp(a,b)==0)))

static Bool
matchWin (XfwmWindow * w, Match * m)
{
  char *client_id = NULL;
  char *window_role = NULL;
  char **wm_command = NULL;
  int wm_command_count = 0, i;
  int found;

  found = 0;

  client_id = GetClientID (w->w);

  if (xstreq (client_id, m->client_id))
  {

    /* client_id's match */

    window_role = GetWindowRole (w->w);

    if (window_role || m->window_role)
    {
      /* We have or had a window role, base decision on it */

      found = xstreq (window_role, m->window_role);

    }
    else
    {
      /* Compare res_class, res_name and WM_NAME, unless the
       * WM_NAME has changed
       */
      if (xstreq (w->class.res_name, m->res_name) &&
	  /* Some GNOME applications do not report the following correctly */
	  /* xstreq(w->class.res_class, m->res_class) && */
	  ((w->flags & WM_NAME_CHANGED) || (m->flags & WM_NAME_CHANGED) || xstreq (w->name, m->wm_name)))

      {
	if (client_id)
	{
	  /* If we have a client_id, we don't compare
	     WM_COMMAND, since it will be different. */
	  found = 1;
	}
	else
	{
	  /* for non-SM-aware clients we also compare WM_COMMAND */
	  GetWindowCommand (w, &wm_command, &wm_command_count);
	  if (wm_command_count == m->wm_command_count)
	  {
	    for (i = 0; i < wm_command_count; i++)
	    {
	      if (strcmp (wm_command[i], m->wm_command[i]) != 0)
		break;
	    }

	    if ((i == wm_command_count) && (wm_command_count))
	    {
	      found = 1;
	    }
	  }			/* if (wm_command_count ==... */
	  /* We have to deal with a now-SM-aware client, it means that it won't probably
	   * restore its state in a proper manner.
	   * Thus, we also mark all other instances of this application as used, to avoid
	   * dummy side effects in case we found a matching entry.
	   */
	  if (found)
	    for (i = 0; i < num_match; i++)
	    {
	      if (!(matches[i].used) && !(&matches[i] == m) && (m->client_leader) && (matches[i].client_leader == m->client_leader))
		matches[i].used = 1;
	    }
	}
      }
    }
  }
  if (client_id)
    XFree (client_id);

  if (window_role)
    XFree (window_role);

  if (wm_command)
    XFreeStringList (wm_command);

  return found;
}

void
MatchWinToSM (XfwmWindow * tmp_win)
{
  int i;

  for (i = 0; i < num_match; i++)
  {
    if (!matches[i].used && matchWin (tmp_win, &matches[i]))
    {
      matches[i].used = 1;

      tmp_win->attr.x = matches[i].x;
      tmp_win->attr.y = matches[i].y;

      tmp_win->attr.width = matches[i].w;
      tmp_win->attr.height = matches[i].h;

      tmp_win->icon_x_loc = matches[i].icon_x;
      tmp_win->icon_y_loc = matches[i].icon_y;
      tmp_win->Desk = matches[i].desktop;
      tmp_win->FocusDesk = matches[i].focusdesk;
      tmp_win->flags |= (matches[i].flags & (STICKY | ICON_MOVED | SHADED));
      if (matches[i].flags & ICONIFIED)
      {
	tmp_win->flags |= ICON_MOVED;
	tmp_win->flags |= STARTICONIC;
      }
      return;
    }
  }
}

void
RestartInSession (char *filename)
{
  FILE *f;

#ifdef HAVE_SESSION
  if (sm_conn)
  {
    if (Scr.Options & SessionMgt)
    {
      f = fopen (filename, "w");
      if (last_used_filename)
	fprintf (f, "[SESSION_STATE] %s\n", last_used_filename);
      save_session_state (sm_conn, filename, f);
      fclose (f);
    }
    set_sm_properties (sm_conn, filename, SmRestartImmediately);

    XSelectInput (dpy, Scr.Root, 0);
    XCloseDisplay (dpy);

    SmcCloseConnection (sm_conn, 0, NULL);

    exit (0);			/* let the SM restart us */
  }
#endif
  if (Scr.Options & SessionMgt)
  {
    f = fopen (filename, "w");
    SaveWindowStates (f);
    fclose (f);			/* return and let Done restart us */
  }
}

#ifdef HAVE_SESSION
static void
callback_save_yourself2 (SmcConn sm_conn, SmPointer client_data)
{
  char *path = NULL;
  char *filename = NULL;
  FILE *f = NULL;
  Bool success = True;
  struct passwd *pwd;

  path = getenv ("SM_SAVE_DIR");
  if (!path)
  {
    pwd = getpwuid (getuid ());
    path = pwd->pw_dir;
  }

  filename = safemalloc (MAXSTRLEN * sizeof (char));
  snprintf (filename, MAXSTRLEN - 1, "%s/.fs-XXXXXX", path);

  if ((Scr.Options & SessionMgt) && (f = fdopen (mkstemp (filename), "r+")))
  {
    success = save_session_state (sm_conn, filename, f);
    fclose (f);
  }
  if (!success)
    xfwm_msg (WARN, "SaveSession", "Can't save session\n");
  /* also save session for builtin session mgt, so user will get
   * back to his environmnent even if he doesn't use ICE
   */
  builtin_save_session ();
  SmcSaveYourselfDone (sm_conn, success);
  sent_save_done = 1;
  free (filename);
}

static void
set_sm_properties (SmcConn sm_conn, char *filename, char hint)
{
  SmProp prop1, prop2, prop3, prop4, prop5, prop6, *props[6];
  SmPropValue prop1val, prop2val, prop3val, prop4val;
#ifdef XSM_BUGGY_DISCARD_COMMAND
  SmPropValue prop6val;
#endif
  struct passwd *pwd;
  char *user_id;
  int numVals, i, priority = 30;

  if (!sm_conn)
    return;

  pwd = getpwuid (getuid ());
  user_id = pwd->pw_name;

  prop1.name = SmProgram;
  prop1.type = SmARRAY8;
  prop1.num_vals = 1;
  prop1.vals = &prop1val;
  prop1val.value = g_argv[0];
  prop1val.length = strlen (g_argv[0]);

  prop2.name = SmUserID;
  prop2.type = SmARRAY8;
  prop2.num_vals = 1;
  prop2.vals = &prop2val;
  prop2val.value = (SmPointer) user_id;
  prop2val.length = strlen (user_id);

  prop3.name = SmRestartStyleHint;
  prop3.type = SmCARD8;
  prop3.num_vals = 1;
  prop3.vals = &prop3val;
  prop3val.value = (SmPointer) & hint;
  prop3val.length = 1;

  prop4.name = "_GSM_Priority";
  prop4.type = SmCARD8;
  prop4.num_vals = 1;
  prop4.vals = &prop4val;
  prop4val.value = (SmPointer) & priority;
  prop4val.length = 1;

  prop5.name = SmRestartCommand;
  prop5.type = SmLISTofARRAY8;

  prop5.vals = (SmPropValue *) safemalloc ((g_argc + 7) * sizeof (SmPropValue));

  numVals = 0;

  for (i = 0; i < g_argc; i++)
  {
    if (strcmp (g_argv[i], "-clientId") == 0 || strcmp (g_argv[i], "-restore") == 0 || strcmp (g_argv[i], "-d") == 0)
    {
      i++;
    }
    else if (strcmp (g_argv[i], "-s") != 0)
    {
      prop5.vals[numVals].value = (SmPointer) g_argv[i];
      prop5.vals[numVals++].length = strlen (g_argv[i]);
    }
  }

  prop5.vals[numVals].value = (SmPointer) "-d";
  prop5.vals[numVals++].length = 2;

  prop5.vals[numVals].value = (SmPointer) XDisplayString (dpy);
  prop5.vals[numVals++].length = strlen (XDisplayString (dpy));

  prop5.vals[numVals].value = (SmPointer) "-s";
  prop5.vals[numVals++].length = 2;

  prop5.vals[numVals].value = (SmPointer) "-clientId";
  prop5.vals[numVals++].length = 9;

  prop5.vals[numVals].value = (SmPointer) sm_client_id;
  prop5.vals[numVals++].length = strlen (sm_client_id);

  prop5.vals[numVals].value = (SmPointer) "-restore";
  prop5.vals[numVals++].length = 8;

  prop5.vals[numVals].value = (SmPointer) filename;
  prop5.vals[numVals++].length = strlen (filename);

  prop5.num_vals = numVals;

  prop6.name = SmDiscardCommand;
#ifdef XSM_BUGGY_DISCARD_COMMAND
  /* the protocol spec says that the discard command
     should be LISTofARRAY8 on posix systems, but xsm
     demands that it be ARRAY8.
   */
  sprintf (discardCommand, "rm -f \"%s\"", filename);
  prop6.type = SmARRAY8;
  prop6.num_vals = 1;
  prop6.vals = &prop6val;
  prop6val.value = (SmPointer) discardCommand;
  prop6val.length = strlen (discardCommand);
#else
  prop6.type = SmLISTofARRAY8;
  prop6.num_vals = 3;
  prop6.vals = (SmPropValue *) safemalloc (3 * sizeof (SmPropValue));
  prop6.vals[0].value = "rm";
  prop6.vals[0].length = 2;
  prop6.vals[1].value = "-f";
  prop6.vals[1].length = 2;
  prop6.vals[2].value = filename;
  prop6.vals[2].length = strlen (filename);
#endif

  props[0] = &prop1;
  props[1] = &prop2;
  props[2] = &prop3;
  props[3] = &prop4;
  props[4] = &prop5;
  props[5] = &prop6;

  SmcSetProperties (sm_conn, 6, props);

  free (prop5.vals);
#ifndef XSM_BUGGY_DISCARD_COMMAND
  free (prop6.vals);
#endif
}

static int
save_session_state (SmcConn sm_conn, char *filename, FILE * cfg_file)
{
  Bool success = 0;
  if (!filename || !cfg_file)
    return 0;

  success = (SaveWindowStates (cfg_file));

  set_sm_properties (sm_conn, filename, SmRestartIfRunning);
  if (last_used_filename)
    free (last_used_filename);
  last_used_filename = filename;

  return success;
}

static void
callback_save_yourself (SmcConn sm_conn, SmPointer client_data, int save_style, Bool shutdown, int interact_style, Bool fast)
{
  if (!SmcRequestSaveYourselfPhase2 (sm_conn, callback_save_yourself2, NULL))
  {
    SmcSaveYourselfDone (sm_conn, False);
    sent_save_done = 1;
  }
  else
    sent_save_done = 0;
}

static void
callback_die (SmcConn sm_conn, SmPointer client_data)
{
  SmcCloseConnection (sm_conn, 0, NULL);
  sm_fd = -1;

  if (master_pid != getpid ())
    kill (master_pid, SIGTERM);
  Done (XFWM_END, NULL);
}

static void
callback_save_complete (SmcConn sm_conn, SmPointer client_data)
{
}

static void
callback_shutdown_cancelled (SmcConn sm_conn, SmPointer client_data)
{
  if (!sent_save_done)
  {
    SmcSaveYourselfDone (sm_conn, False);
    sent_save_done = 1;
  }
}

/* the following is taken from xsm */

static IceIOErrorHandler prev_handler;

static void
MyIoErrorHandler (IceConn ice_conn)
{
  if (prev_handler)
    (*prev_handler) (ice_conn);
}

static void
InstallIOErrorHandler (void)
{
  IceIOErrorHandler default_handler;

  prev_handler = IceSetIOErrorHandler (NULL);
  default_handler = IceSetIOErrorHandler (MyIoErrorHandler);
  if (prev_handler == default_handler)
    prev_handler = NULL;
}

#endif /* HAVE_SESSION */

void
SessionInit (char *previous_client_id)
{
#ifdef HAVE_SESSION
  char error_string_ret[4096] = "";
  static SmPointer context;
  SmcCallbacks callbacks;

  InstallIOErrorHandler ();

  callbacks.save_yourself.callback = callback_save_yourself;
  callbacks.die.callback = callback_die;
  callbacks.save_complete.callback = callback_save_complete;
  callbacks.shutdown_cancelled.callback = callback_shutdown_cancelled;

  callbacks.save_yourself.client_data = callbacks.die.client_data = callbacks.save_complete.client_data = callbacks.shutdown_cancelled.client_data = (SmPointer) NULL;

  sm_conn = SmcOpenConnection (NULL, &context, SmProtoMajor, SmProtoMinor, SmcSaveYourselfProcMask | SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask, &callbacks, previous_client_id, &sm_client_id, 4096, error_string_ret);

  if (!sm_conn)
  {
    xfwm_msg (INFO, "SessionInit", "X Session Manager not available\n" "Using builtin session management instead\n");
    if (previous_client_id)
    {
      xfwm_msg (WARN, "SessionInit", "While connecting to session manager:\n%s.", error_string_ret);
    }
    sm_fd = -1;
  }
  else
  {
    xfwm_msg (INFO, "SessionInit", "Connected to session manager.\n");
    sm_fd = IceConnectionNumber (SmcGetIceConnection (sm_conn));
  }
#endif
  if (sm_fd < 0)
    builtin_session_startup ();
}

void
ProcessICEMsgs (void)
{
#ifdef HAVE_SESSION
  IceProcessMessagesStatus status;

  if ((sm_fd < 0) || (!sm_conn))
    return;
  status = IceProcessMessages (SmcGetIceConnection (sm_conn), NULL, NULL);
  if (status == IceProcessMessagesIOError)
  {
    xfwm_msg (WARN, "ProcessICEMSGS", "Connection to session manager lost\n");
    sm_conn = NULL;
    sm_fd = -1;
  }
#endif
}

void
CloseICEConn (void)
{
#ifdef HAVE_SESSION
  if ((sm_fd < 0) || (!sm_conn))
    return;
  IceCloseConnection (SmcGetIceConnection (sm_conn));
#endif
}

void
LogoutICEConn (void)
{
#ifdef HAVE_SESSION
  if ((sm_fd < 0) || (!sm_conn))
    return;
  SmcRequestSaveYourself (sm_conn, SmSaveBoth, 1, SmInteractStyleAny, 0, 1);
#endif
}

void
builtin_session_startup (void)
{
  int i, j;
  Bool found;

  /* Ignore session management if restarting xfwm */
  if (Restarting)
    return;

  for (i = 0; i < num_match; i++)
  {
    if (matches[i].wm_command_count)
    {
      /* Do not restart internal modules */
      if ((matches[i].wm_command_count > 6) && (!mystrncasecmp (matches[i].wm_command[6], "XFwm", 4)))
	continue;
      /* Determine whether or not an action as already been perfored for a given client
       * It's based on the client leader (do not restart several instances of the same
       * client leader
       */
      found = False;
      for (j = 0; j < i; j++)
	if ((matches[i].client_leader) && (matches[j].client_leader == matches[i].client_leader))

	{
	  found = True;
	  break;
	}
      if (found)
	continue;
      /* Execute command line */
      switch (fork ())
      {
      case 0:			/* Child process */
	/*
	 * We shall have a rest before starting the actual command,
	 * that will give time for other modules to complete and
	 * it will definitely decrease the server load at startup
	 */
	sleep_a_little (Scr.sessionwait * 1000000);
	if (execvp (matches[i].wm_command[0], matches[i].wm_command) == -1)
	{
	  xfwm_msg (WARN, "builtin_session_startup", "Startup of \"%s\" failed", matches[i].wm_command[0]);
	  exit (100);
	}
	break;
      case -1:
	xfwm_msg (WARN, "builtin_session_startup", "Cannot fork process");
	break;
      default:
	break;
      }
    }
  }
}

int
builtin_save_session (void)
{
  char *path = NULL;
  char *filename = NULL;
  FILE *f = NULL;
  Bool success = 0;
  struct passwd *pwd;

  path = getenv ("SM_SAVE_DIR");
  if (!path)
  {
    pwd = getpwuid (getuid ());
    path = pwd->pw_dir;
  }

  filename = (char *) safemalloc (sizeof (char) * (MAXSTRLEN + 1));

  /* If we have a multiheaded screen, save/load session from different
   * filenames.
   */
  if (Scr.screen != 0)
  {
    snprintf (filename, MAXSTRLEN, "%s/.xfce/xfwm-session-%i", path, (int) Scr.screen);
  }
  else
  {
    snprintf (filename, MAXSTRLEN, "%s/.xfce/xfwm-session", path);
  }

  f = fopen (filename, "w");
  if (!f)
  {
    free (filename);
    return 0;
  }
  success = SaveWindowStates (f);
  fclose (f);

  free (filename);

  return success;
}
