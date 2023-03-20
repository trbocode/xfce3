/* Copyright (C) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include "utils.h"
#include "module.h"
#include "xfgnome.h"
#include "../xfwm/xfwm.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static s_list *window_list;	/* single linked list of window id's etc */
static int current_desk = 0;	/* current workspace */
static char **desk_names;	/* holds workspace names */
static int desk_count = 0;	/* number of workspaces */

/* inserts a window id into the list
 * args: list, id
 */
list_item *
s_list_insert (s_list * list, long id)
{
  list_item *item = NULL;

  item = (list_item *) safemalloc (sizeof (list_item));

  if (!item)
    return NULL;

  if (list->first == NULL)
  {
    list->first = item;
    list->last = item;
  }
  else
  {
    list->last->next = item;
  }
  item->next = NULL;
  list->last = item;
  item->id = id;
  return item;
}

/* removes an item from list by window id
 * args: list, id
 */
int
s_list_remove_by_data (s_list * list, long id)
{
  list_item *tmp1;

  tmp1 = s_list_find_by_data (list, id);

  if (tmp1 == NULL)
    return 0;

  /* this is our only list item */
  if (tmp1 == list->first && tmp1 == list->last)
  {
    free (tmp1);
    list->first = NULL;
    list->last = NULL;
    return 1;
  }
  /* the last item */
  else if (tmp1 == list->last)
  {
    list_item *tmp2;

    tmp2 = list->first;
    while (tmp2)
    {
      if (tmp2->next == list->last)
	break;
      tmp2 = tmp2->next;
    }
    free (tmp1);
    tmp2->next = NULL;
    list->last = tmp2;
    return 1;
  }
  /* the first item */
  else if (tmp1 == list->first)
  {
    list->first = tmp1->next;
    free (tmp1);
    return 1;
  }
  /*somewhere in between */
  else
  {
    list_item *tmp2;

    tmp2 = list->first;
    while (tmp2)
    {
      if (tmp2->next == tmp1)
	break;
      tmp2 = tmp2->next;
    }
    tmp2->next = tmp1->next;
    free (tmp1);
    return 1;
  }
  return 0;
}

/* searches for a window id in a list
 * args: list, id
 */
list_item *
s_list_find_by_data (s_list * list, long id)
{
  list_item *tmp;

  tmp = list->first;

  if (list->first == NULL && list->last == NULL)
    return NULL;

  while (tmp)
  {
    if (tmp->id == id)
      return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

/* creates a new single linked list */
s_list *
s_list_new ()
{
  s_list *list;

  list = (s_list *) safemalloc (sizeof (s_list));

  if (!list)
    return NULL;

  list->first = list->last = NULL;
  return list;
}

/* destroys a single linked list
 * args: list
 */
int
s_list_free (s_list * list)
{
  list_item *tmp1, *tmp2;

  tmp1 = list->first;
  while (tmp1)
  {
    tmp2 = tmp1->next;
    free (tmp1);
    tmp1 = tmp2;
  }
  free (list);
  return 0;
}

/* returns the count of items in a list, skips windowlistskip
 * windows
 * args: list
 */
int
s_list_count (s_list * list)
{
  list_item *tmp;
  int count = 0;

  tmp = list->first;

  while (tmp)
  {
    if (!(tmp->flags & WINDOWLISTSKIP))
      count++;
    tmp = tmp->next;
  }
  return count;
}

/* sets up the properties and the supported protocols list */
static void
gnome_compliance_init ()
{
  Atom supported_list[12];
  int count;

  root_win = RootWindow (dpy, screen);

  /* supporting WM check */
  _XA_WIN_SUPPORTING_WM_CHECK = XInternAtom (dpy, "_WIN_SUPPORTING_WM_CHECK", False);
  _XA_WIN_PROTOCOLS = XInternAtom (dpy, "_WIN_PROTOCOLS", False);
  _XA_WIN_STATE = XInternAtom (dpy, "_WIN_STATE", False);
  _XA_WIN_HINTS = XInternAtom (dpy, "_WIN_HINTS", False);
  _XA_WIN_APP_STATE = XInternAtom (dpy, "_WIN_APP_STATE", False);
    /*_XA_WIN_EXPANDED_SIZE = XInternAtom(dpy, "_WIN_EXPANDED_SIZE", False);*/
  _XA_WIN_ICONS = XInternAtom (dpy, "_WIN_ICONS", False);
  _XA_WIN_WORKSPACE = XInternAtom (dpy, "_WIN_WORKSPACE", False);
  _XA_WIN_WORKSPACE_COUNT = XInternAtom (dpy, "_WIN_WORKSPACE_COUNT", False);
  _XA_WIN_WORKSPACE_NAMES = XInternAtom (dpy, "_WIN_WORKSPACE_NAMES", False);
  _XA_WIN_CLIENT_LIST = XInternAtom (dpy, "_WIN_CLIENT_LIST", False);
  _XA_WIN_DESKTOP_BUTTON_PROXY = XInternAtom (dpy, "_WIN_DESKTOP_BUTTON_PROXY", False);

  /* create the GNOME window */
  gnome_win = XCreateSimpleWindow (dpy, root_win, 0, 0, 5, 5, 0, 0, 0);

  /* supported WM check */
  XChangeProperty (dpy, root_win, _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);
  XChangeProperty (dpy, gnome_win, _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &gnome_win, 1);

  /* supported protocols */
  count = 0;

  supported_list[count++] = _XA_WIN_STATE;	/* done */
  supported_list[count++] = _XA_WIN_HINTS;	/* done */
  supported_list[count++] = _XA_WIN_APP_STATE;	/* ???? */
  supported_list[count++] = _XA_WIN_ICONS;	/* ???? */
  supported_list[count++] = _XA_WIN_WORKSPACE;	/* done */
  supported_list[count++] = _XA_WIN_WORKSPACE_COUNT;	/* done */
  supported_list[count++] = _XA_WIN_WORKSPACE_NAMES;	/* done */
  supported_list[count++] = _XA_WIN_CLIENT_LIST;	/* done */
  XChangeProperty (dpy, root_win, _XA_WIN_PROTOCOLS, XA_ATOM, 32, PropModeReplace, (unsigned char *) supported_list, count);

}

/* sets the current workspace */
static void
gnome_set_current_workspace (int current_desk)
{

  XChangeProperty (dpy, root_win, _XA_WIN_WORKSPACE, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &current_desk, 1);
}

/* sets the number of workspaces */
static void
gnome_set_workspace_count (int workspaces)
{

  XChangeProperty (dpy, root_win, _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &workspaces, 1);

}

/* sets up the 'workspace names' property */
static void
gnome_set_workspace_names (int count, char **names)
{
  XTextProperty text;

  if (XStringListToTextProperty (names, count, &text))
  {
    XSetTextProperty (dpy, root_win, &text, _XA_WIN_WORKSPACE_NAMES);
    XFree (text.value);
  }
}

/* sets the window list property, skips winlistskip windows */
static void
gnome_set_client_list (s_list * list)
{
  Window *windows = NULL;
  int windows_count = 0, count;
  list_item *tmp;

  windows_count = s_list_count (list);

  if (windows_count != 0)
  {
    windows = (Window *) safemalloc (sizeof (Window) * windows_count);

    if (!windows)
    {
      fprintf (stderr, "gnome_set_client_list: malloc failed\n");
      return;
    }
    tmp = list->first;
    count = 0;
    while (tmp)
    {
      if (!(tmp->flags & WINDOWLISTSKIP))
	windows[count++] = tmp->id;
      tmp = tmp->next;
    }
    XChangeProperty (dpy, root_win, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) windows, windows_count);
    free (windows);
  }
}

/* translates xfwm window flags to GNOME state properties */
static void
gnome_set_win_hints (list_item * xfwm_window)
{
  unsigned long flags = 0L;

  if (xfwm_window->flags & STICKY)
  {
    flags = WIN_STATE_STICKY;
  }

  if (xfwm_window->flags & ICONIFIED)
  {
    flags |= WIN_STATE_MINIMIZED;
  }

  if (xfwm_window->flags & SHADED)
  {
    flags |= WIN_STATE_SHADED;
  }

  if (!(xfwm_window->flags & STICKY) && (xfwm_window->workspace != current_desk))
  {
    flags |= WIN_STATE_HID_WORKSPACE;
  }

  XChangeProperty (dpy, (Window) xfwm_window->id, _XA_WIN_WORKSPACE, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &xfwm_window->workspace, 1);

  XChangeProperty (dpy, (Window) xfwm_window->id, _XA_WIN_STATE, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &flags, 1);
}

/* state requests, GNOME -> xfwm */
static void
gnome_handle_win_state (XClientMessageEvent * event, Window id, long mask)
{
  long new_members, change_mask;
  list_item *xfwm_window;

  if (event)
  {
    xfwm_window = s_list_find_by_data (window_list, (long) event->window);
    change_mask = event->data.l[0];
    new_members = event->data.l[1];
  }
  else
  {
    xfwm_window = s_list_find_by_data (window_list, id);
    new_members = mask;
    change_mask = 0;
  }
  if (!xfwm_window)
    return;

  /* stick or unstick */
  if (new_members & WIN_STATE_STICKY)
  {
    if (!(xfwm_window->flags & STICKY))
    {
      SendInfo (fd, "Stick on", xfwm_window->id);
    }
  }
  else if (change_mask & WIN_STATE_STICKY)
  {
    if (xfwm_window->flags & STICKY)
    {
      SendInfo (fd, "Stick off", xfwm_window->id);
    }
  }

  if ((new_members & WIN_STATE_MAXIMIZED_VERT) && (new_members & WIN_STATE_MAXIMIZED_HORIZ))
  {
    SendInfo (fd, "Maximize 100% 100%", xfwm_window->id);
  }
  else
  {
    /* maximize verticaly */
    if (new_members & WIN_STATE_MAXIMIZED_VERT)
    {
      SendInfo (fd, "Maximize 0% 100%", xfwm_window->id);
    }

    /* maximize horizontaly */
    if (new_members & WIN_STATE_MAXIMIZED_HORIZ)
    {
      SendInfo (fd, "Maximize 100% 0%", xfwm_window->id);
    }
  }
  /* shade or unshade */
  if (new_members & WIN_STATE_SHADED)
  {
    if (!(xfwm_window->flags & SHADED))
    {
      SendInfo (fd, "Shade", xfwm_window->id);
    }
  }
  else if (change_mask & WIN_STATE_SHADED)
  {
    if (xfwm_window->flags & SHADED)
    {
      SendInfo (fd, "Shade", xfwm_window->id);
    }
  }

  if (event)
    gnome_get_prop_workspace (xfwm_window->id);
}

/* initial app state, get GNOME state from windows that set them */
static void
gnome_get_prop_state (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems, bytes_after;
  long flags, *data = 0;

  if (XGetWindowProperty (dpy, id, _XA_WIN_STATE, 0, 1, False, XA_CARDINAL, &ret_type, &fmt, &nitems, &bytes_after, (unsigned char **) &data) == Success && data)
  {
    flags = *data;
    gnome_handle_win_state (NULL, id, flags);
    XFree (data);
  }

}

/* initial app workspace, get it from the window if set */
static void
gnome_get_prop_workspace (Window id)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems;
  unsigned long bytes_after;
  long val, *data = 0;


  if (XGetWindowProperty (dpy, id, _XA_WIN_WORKSPACE, 0, 1, False, XA_CARDINAL, &ret_type, &fmt, &nitems, &bytes_after, (unsigned char **) &data) == Success && data)
  {
    val = *data;
    XFree (data);
    if (val != current_desk)
    {
      char msg[50];
      sprintf (msg, "WindowsDesk 0 %d\n", (int) val);
      SendInfo (fd, msg, id);
    }

  }
}

/* initial hints, check for GNOME hints on a window */
static void
gnome_check_client_hints (Window id)
{
  /* state */
  gnome_get_prop_state (id);

  /* workspace */
  gnome_get_prop_workspace (id);
}

/* signal handler, also called by xfwm when it wants to kill the module */
void
DeadPipe (int sig)
{
  switch (sig)
  {
  case SIGSEGV:
    fprintf (stderr, "Segmentation fault in %s\n", MyName);
    exit (-1);
  case SIGINT:
    fprintf (stderr, "User abort, exiting\n");
    exit (-1);
  default:
    break;
  }
  exit (0);
}

/* process xfwm events */
static void
process_message (unsigned long type, int elements, unsigned long *body)
{
  int status = 0;

  switch (type)
  {
  case XFCE_M_CONFIGURE_WINDOW:
    status = list_configure (body);
    break;
  case XFCE_M_ADD_WINDOW:
    status = list_add_window (body);
    break;
  case XFCE_M_DESTROY_WINDOW:
    status = list_destroy_window (body);
    break;
  case XFCE_M_ICONIFY:
    status = list_iconify (body);
    break;
  case XFCE_M_DEICONIFY:
    status = list_deiconify (body);
    break;
  case XFCE_M_WINDOW_NAME:
    status = list_window_name (body);
    break;
  case XFCE_M_ICON_NAME:
    status = list_icon_name (body);
    break;
  case XFCE_M_NEW_DESK:
    status = list_desk_change (body);
    break;
  case XFCE_M_END_WINDOWLIST:
    status = list_end ();
    break;
  default:
    break;
  }
}

/* list_configure:
 * pipe configure events
 */

int
list_configure (unsigned long *body)
{
  list_item *item;

  if (body[0] == gnome_win)
    return 0;

  if (body[0] == None)
    return 0;

  if ((item = s_list_find_by_data (window_list, body[0])) != NULL)
  {
    Bool update = False;

    if (item->flags != body[8])
    {
      item->flags = body[8];
      update = True;
    }
    if (item->workspace != body[7])
    {
      if (!(item->flags & ICONIFIED))
      {
	item->workspace = body[7];
	update = True;
      }
    }

    if (update)
    {
      gnome_set_win_hints (item);
      gnome_set_client_list (window_list);
      return 1;
    }
    return 0;
  }
  return (list_add_window (body));
}

/* list_add_window:
 * pipe add window events
 */

int
list_add_window (unsigned long *body)
{
  list_item *item;


  if (body[0] == None)
    return 0;

  if (s_list_find_by_data (window_list, body[0]))
  {
    return 0;
  }
  if (body[0] == gnome_win)
    return 0;

  item = s_list_insert (window_list, body[0]);
  item->flags = body[8];
  item->workspace = body[7];
  gnome_check_client_hints (item->id);
  gnome_set_win_hints (item);
  gnome_set_client_list (window_list);
  return 1;
}

/* list_window_name:
 * pipe chage of window name events
 */

int
list_window_name (unsigned long *body)
{
  return 0;
}

/* list_icon_name:
 * pipe change of icon name events
 */

int
list_icon_name (unsigned long *body)
{
  return 0;
}

/* list_destroy_window:
 * pipe destroyed windows event
 */

int
list_destroy_window (unsigned long *body)
{
  if (s_list_find_by_data (window_list, body[0]))
  {
    s_list_remove_by_data (window_list, body[0]);
    gnome_set_client_list (window_list);
    /* Perform some cleanup */
    XDeleteProperty (dpy, body[0], _XA_WIN_WORKSPACE);
    XDeleteProperty (dpy, body[0], _XA_WIN_STATE);
    return 1;
  }
  return 0;
}

/* list_end
 * end of window list from pipe
 */

static int
list_end (void)
{
  gnome_set_current_workspace (current_desk);
  gnome_set_client_list (window_list);
  return 0;
}

/* list_deiconify:
 * window deiconified from the pipe
 */

int
list_deiconify (unsigned long *body)
{
  return 0;
}

/* list_iconify:
 * window iconified from the pipe
 */

int
list_iconify (unsigned long *body)
{
  return 0;
}

int
list_desk_change (unsigned long *body)
{
  current_desk = (unsigned int) body[0];
  gnome_set_current_workspace (current_desk);
  return 0;
}

static void
event_handler ()
{
  XEvent Event;
  char tmp[255];

  while (XPending (dpy))
  {
    XNextEvent (dpy, &Event);
    switch (Event.type)
    {
    case PropertyNotify:
      if (Event.xproperty.window == gnome_win)
      {
	gnome_check_client_hints (Event.xclient.window);
      }
      break;
    case ClientMessage:
      /* change workspace request */
      if (Event.xclient.message_type == _XA_WIN_WORKSPACE)
      {
	int desk;
	desk = Event.xclient.data.l[0];
	sprintf (tmp, "Desk 0 %d\n", desk);
	SendInfo (fd, tmp, None);
      }
      /* state requests */
      else if (Event.xclient.message_type == _XA_WIN_STATE)
      {
	gnome_handle_win_state (&Event.xclient, None, 0);
      }
      break;
    default:
      break;
    }
  }
}

/******************************************************************************
  Loop -  Read and redraw until we get killed, blocking when can't read
******************************************************************************/
static void
Loop (int *fd)
{
  fd_set readset;
  struct timeval tv;
  unsigned long header[3], *body;

  while (1)
  {
    FD_ZERO (&readset);
    FD_SET (fd[1], &readset);
    FD_SET (x_fd, &readset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
#ifdef __hpux
    if (!select (fd_width, (int *) &readset, NULL, NULL, &tv))
    {
      XPending (dpy);
      FD_ZERO (&readset);
      FD_SET (fd[1], &readset);
      FD_SET (x_fd, &readset);
      select (fd_width, (int *) &readset, NULL, NULL, NULL);
    }
#else
    if (!select (fd_width, &readset, NULL, NULL, &tv))
    {
      XPending (dpy);
      FD_ZERO (&readset);
      FD_SET (fd[1], &readset);
      FD_SET (x_fd, &readset);
      select (fd_width, &readset, NULL, NULL, NULL);
    }
#endif

    if (FD_ISSET (x_fd, &readset))
      event_handler ();
    if (FD_ISSET (fd[1], &readset))
      if (ReadXfwmPacket (fd[1], header, &body) > 0)
      {
	process_message (header[1], header[2], body);
	free (body);
      }
  }
}

static void
default_config ()
{
  int i;

  desk_names = (char **) safemalloc (sizeof (char *));

  for (i = 0; i < NBSCREENS; i++)
  {
    if (i != 0)
      desk_names = realloc (desk_names, sizeof (char *) + i * sizeof (char *));
    desk_names[i] = (char *) safemalloc (4);
    sprintf (desk_names[i], "%i", i + 1);
  }
}

XErrorHandler
ErrorHandler (Display * dpy, XErrorEvent * event)
{
  if ((event->error_code == BadWindow) || (event->error_code == BadDrawable) || (event->request_code == X_GetGeometry) || (event->request_code == X_ChangeProperty) || (event->request_code == X_SetInputFocus) || (event->request_code == X_ChangeWindowAttributes) || (event->request_code == X_GrabButton) || (event->request_code == X_ChangeWindowAttributes) || (event->request_code == X_InstallColormap))
    return (0);

  fprintf (stderr, "xfgnome: Fatal XLib internal error\n");
  exit (1);
  return (0);
}

int
main (int argc, char **argv)
{
  char *temp, *s;
  char *display_name = NULL;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;

  /* Save our program  name - for error messages */
  temp = argv[0];
  s = strrchr (argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc (strlen (temp) + 2);
  strcpy (MyName, temp);

  if (argc < 7)
  {
    fprintf (stderr, "%s Version %s should only be executed by xfwm!\n", MyName, VERSION);
    exit (1);
  }

  /* Dead pipe == Fvwm died */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi (argv[1]);
  fd[1] = atoi (argv[2]);

  /* Initialize X connection */
  if (!(dpy = XOpenDisplay (display_name)))
  {
    fprintf (stderr, "%s: can't open display %s", MyName, XDisplayName (display_name));
    exit (1);
  }

  x_fd = XConnectionNumber (dpy);
  fd_width = GetFdWidth ();
  screen = DefaultScreen (dpy);
  XSetErrorHandler ((XErrorHandler) ErrorHandler);

  gnome_compliance_init ();

  if (argc > 7)
  {
    desk_count = atoi (argv[7]);
    if (desk_count < 1)
      desk_count = 1;
    else if (desk_count > 32)
      desk_count = 32;
  }
  else if ((XGetWindowProperty (dpy, root_win, _XA_WIN_WORKSPACE_COUNT, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
  {
    if (prop != NULL)
    {
      desk_count = *(unsigned long *) prop;
      if (desk_count > NBSCREENS)
	desk_count = NBSCREENS;
      XFree (prop);
    }
    else
      desk_count = NBSCREENS;
  }
  else
    desk_count = NBSCREENS;

  default_config ();
  gnome_set_workspace_names (NBSCREENS, desk_names);
  gnome_set_workspace_count (desk_count);

  SetMessageMask (fd, XFCE_M_ADD_WINDOW | XFCE_M_CONFIGURE_WINDOW | XFCE_M_DESTROY_WINDOW | XFCE_M_NEW_DESK | XFCE_M_ICONIFY | XFCE_M_DEICONIFY | XFCE_M_WINDOW_NAME | XFCE_M_ICON_NAME | XFCE_M_END_WINDOWLIST);

  window_list = s_list_new ();

  XSelectInput (dpy, root_win, PropertyChangeMask | SubstructureNotifyMask);
  XSelectInput (dpy, gnome_win, PropertyChangeMask);

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo (fd, "Send_WindowList", 0);

  Loop (fd);

  return (0);
}
