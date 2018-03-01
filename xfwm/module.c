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
 * code for launching xfwm modules.
 *
 ***********************************************************************/

#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "stack.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

int npipes;
int *readPipes;
int *writePipes;
int *pipeOn;
char **pipeName;

unsigned long *PipeMask;
struct queue_buff_struct **pipeQueue;

int PositiveWrite (int module, unsigned long *ptr, int size);
void DeleteQueueBuff (int module);
void AddToQueue (int module, unsigned long *ptr, int size, int done);
extern void AddToModList (char *tline);

void
initModules (void)
{
  int i;

  npipes = GetFdWidth ();

  writePipes = (int *) safemalloc (sizeof (int) * npipes);
  readPipes = (int *) safemalloc (sizeof (int) * npipes);
  pipeOn = (int *) safemalloc (sizeof (int) * npipes);
  PipeMask = (unsigned long *) safemalloc (sizeof (unsigned long) * npipes);
  pipeName = (char **) safemalloc (sizeof (char *) * npipes);
  pipeQueue = (struct queue_buff_struct **) safemalloc (sizeof (struct queue_buff_struct *) * npipes);

  for (i = 0; i < npipes; i++)
  {
    writePipes[i] = -1;
    readPipes[i] = -1;
    pipeOn[i] = -1;
    PipeMask[i] = MAX_MASK;
    pipeQueue[i] = (struct queue_buff_struct *) NULL;
    pipeName[i] = NULL;
  }
}

void
ClosePipes (void)
{
  int i;
  for (i = 0; i < npipes; i++)
  {
    if (writePipes[i] > 0)
    {
      close (writePipes[i]);
      close (readPipes[i]);
    }
    if (pipeName[i] != NULL)
    {
      free (pipeName[i]);
      pipeName[i] = 0;
    }
    while (pipeQueue[i] != NULL)
    {
      DeleteQueueBuff (i);
    }
  }
  /*
     free (pipeQueue);
     free (pipeName);
     free (PipeMask);
     free (pipeOn);
     free (readPipes);
     free (writePipes);
   */
}


void
executeModule (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int xfwm_to_app[2], app_to_xfwm[2];
  int i, val, nargs = 0;
  char *cptr;
  char *args[20];
  char *arg1 = NULL;
  char arg2[20];
  char arg3[20];
  char arg5[20];
  char arg6[20];
  extern char *ModulePath;
  extern char *xfwm_file;
  Window win;

  if (action == NULL)
    return;

  if (tmp_win)
    win = tmp_win->w;
  else
    win = None;

  /* If we execute a module, don't wait for buttons to come up,
   * that way, a pop-up menu could be implemented */
  *Module = 0;

  action = GetNextToken (action, &cptr);

  arg1 = findIconFile (cptr, ModulePath, X_OK);
  if (arg1 == NULL)
  {
    if (cptr != NULL)
    {
      xfwm_msg (ERR, "executeModule", "No such module '%s' in ModulePath '%s'", cptr, ModulePath);
      free (cptr);
    }
    return;
  }

  /* Look for an available pipe slot */
  i = 0;
  while ((i < npipes) && (writePipes[i] >= 0))
    i++;
  if (i >= npipes)
  {
    xfwm_msg (ERR, "executeModule", "Too many Accessories!");
    if (cptr != NULL)
      free (cptr);
    if (arg1 != NULL)
      free (arg1);
    return;
  }

  /* I want one-ended pipes, so I open two two-ended pipes,
   * and close one end of each. I need one ended pipes so that
   * I can detect when the module crashes/malfunctions */
  if (pipe (xfwm_to_app) != 0)
  {
    xfwm_msg (ERR, "executeModule", "Failed to open pipe");
    if (cptr != NULL)
      free (cptr);
    if (arg1 != NULL)
      free (arg1);
    return;
  }
  if (pipe (app_to_xfwm) != 0)
  {
    xfwm_msg (ERR, "executeModule", "Failed to open pipe2");
    if (cptr != NULL)
      free (cptr);
    if (arg1 != NULL)
      free (arg1);
    close (xfwm_to_app[0]);
    close (xfwm_to_app[1]);
    return;
  }

  pipeName[i] = stripcpy (cptr);
  sprintf (arg2, "%d", app_to_xfwm[1]);
  sprintf (arg3, "%d", xfwm_to_app[0]);
  sprintf (arg5, "%lx", (unsigned long) win);
  sprintf (arg6, "%lx", (unsigned long) context);
  args[0] = arg1;
  args[1] = arg2;
  args[2] = arg3;
  if (xfwm_file != NULL)
    args[3] = xfwm_file;
  else
    args[3] = "none";
  args[4] = arg5;
  args[5] = arg6;
  args[6] = "XFwm";
  nargs = 7;
  while ((action != NULL) && (nargs < 20) && (strlen (args[nargs - 1]) > 0))
  {
    args[nargs] = 0;
    action = GetNextToken (action, &args[nargs]);
    nargs++;
  }
  if (strlen (args[nargs - 1]) <= 0)
  {
    free (args[nargs - 1]);
    nargs--;
  }

  args[nargs] = 0;
  val = fork ();
  if (val > 0)
  {
    /* This fork remains running xfwm */
    /* close appropriate descriptors from each pipe so
     * that xfwm will be able to tell when the app dies */
    close (app_to_xfwm[1]);
    close (xfwm_to_app[0]);

    /* add these pipes to xfwm's active pipe list */
    writePipes[i] = xfwm_to_app[1];
    readPipes[i] = app_to_xfwm[0];
    pipeOn[i] = -1;
    PipeMask[i] = MAX_MASK;
    pipeQueue[i] = NULL;

    /* make the PositiveWrite pipe non-blocking. Don't want to jam up
     * xfwm because of an uncooperative module */
    fcntl (writePipes[i], F_SETFL, O_NONBLOCK);	/* POSIX, better behavior */
    /* Mark the pipes close-on exec so other programs
     * won`t inherit them */
    if (fcntl (readPipes[i], F_SETFD, 1) == -1)
      xfwm_msg (ERR, "executeModule", "module close-on-exec failed");
    if (fcntl (writePipes[i], F_SETFD, 1) == -1)
      xfwm_msg (ERR, "executeModule", "module close-on-exec failed");

    if (cptr != NULL)
      free (cptr);
    if (arg1 != NULL)
      free (arg1);
    for (i = 7; i < nargs; i++)
      free (args[i]);
  }
  else if (val == 0)
  {
    /* this is  the child */
    /* this fork execs the module */
    close (xfwm_to_app[1]);
    close (app_to_xfwm[0]);
    execvp (arg1, args);
    xfwm_msg (ERR, "executeModule", "Execution of module failed: %s", arg1);
    perror ("");
    close (app_to_xfwm[1]);
    close (xfwm_to_app[0]);
    exit (1);
  }
  else
  {
    xfwm_msg (ERR, "executeModule", "Fork failed");
    for (i = 7; i < nargs; i++)
      free (args[i]);
    free (arg1);
  }
  return;
}

void
HandleModuleInput (Window w, int channel)
{
  char text[256];
  int size;
  int cont, n;

  /* Already read a (possibly NULL) window id from the pipe,
   * Now read an xfwm bultin command line */
  n = read (readPipes[channel], &size, sizeof (size));
  if (n < sizeof (size))
  {
    KillModule (channel, 1);
    return;
  }

  if (size > 255)
  {
    xfwm_msg (ERR, "HandleModuleInput", "Module command is too big (%d)", (void *) ((long) size));
    size = 255;
  }

  pipeOn[channel] = 1;

  n = read (readPipes[channel], text, size);
  if (n < size)
  {
    KillModule (channel, 2);
    return;
  }

  text[n] = 0;
  n = read (readPipes[channel], &cont, sizeof (cont));
  if (n < sizeof (cont))
  {
    KillModule (channel, 3);
    return;
  }
  if (cont == 0)
  {
    KillModule (channel, 4);
  }
  if (strlen (text) > 0)
  {
    extern int Context;
    XfwmWindow *tmp_win;

    /* first, check to see if module config line */
    if (text[0] == '*')
    {
      AddToModList (text);
      return;
    }

    /* perhaps the module would like us to kill it? */
    if (mystrncasecmp (text, "KillMe", 6) == 0)
    {
      KillModule (channel, 12);
      return;
    }

    /* If a module does XUngrabPointer(), it can now get proper Popups */
    if (mystrncasecmp (text, "popup", 5) == 0)
      Event.xany.type = ButtonPress;
    else
      Event.xany.type = ButtonRelease;
    Event.xany.window = w;

    if (XFindContext (dpy, w, XfwmContext, (caddr_t *) & tmp_win) == XCNOENT)
    {
      tmp_win = NULL;
      w = None;
    }
    if (tmp_win)
    {
      Event.xbutton.button = 1;
      Event.xbutton.x_root = tmp_win->frame_x;
      Event.xbutton.y_root = tmp_win->frame_y;
      Event.xbutton.x = 0;
      Event.xbutton.y = 0;
      Event.xbutton.subwindow = None;
    }
    else
    {
      Event.xbutton.button = 1;
      Event.xbutton.x_root = 0;
      Event.xbutton.y_root = 0;
      Event.xbutton.x = 0;
      Event.xbutton.y = 0;
      Event.xbutton.subwindow = None;
    }
    Context = GetContext (tmp_win, &Event, &w);
    ExecuteFunction (text, tmp_win, &Event, Context, channel);
  }
  return;
}


void
DeadPipe (int nonsense)
{
  signal (SIGPIPE, DeadPipe);
}


void
KillModule (int channel, int place)
{
  close (readPipes[channel]);
  close (writePipes[channel]);

  readPipes[channel] = -1;
  writePipes[channel] = -1;
  pipeOn[channel] = -1;
  while (pipeQueue[channel] != NULL)
  {
    DeleteQueueBuff (channel);
  }
  if (pipeName[channel] != NULL)
  {
    free (pipeName[channel]);
    pipeName[channel] = NULL;
  }

  return;
}

void
KillModuleByName (char *name)
{
  int i = 0;

  if (name == NULL)
    return;

  while (i < npipes)
  {
    if ((pipeName[i] != NULL) && (matchWildcards (name, pipeName[i])))
    {
      KillModule (i, 10);
    }
    i++;
  }
  return;
}


void
SendPacket (int module, unsigned long event_type, unsigned long num_datum, unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4, unsigned long data5, unsigned long data6, unsigned long data7)
{
#ifdef REQUIRES_STASHEVENT
  extern Time lastTimestamp;
#endif
  unsigned long body[11];

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = num_datum + HEADER_SIZE;
#ifdef REQUIRES_STASHEVENT
  body[3] = lastTimestamp;
#else
  body[3] = CurrentTime;
#endif
  if (num_datum > 0)
    body[HEADER_SIZE] = data1;
  if (num_datum > 1)
    body[HEADER_SIZE + 1] = data2;
  if (num_datum > 2)
    body[HEADER_SIZE + 2] = data3;
  if (num_datum > 3)
    body[HEADER_SIZE + 3] = data4;
  if (num_datum > 4)
    body[HEADER_SIZE + 4] = data5;
  if (num_datum > 5)
    body[HEADER_SIZE + 5] = data6;
  if (num_datum > 6)
    body[HEADER_SIZE + 6] = data7;
  PositiveWrite (module, body, (num_datum + 4) * sizeof (body[0]));
}

void
Broadcast (unsigned long event_type, unsigned long num_datum, unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4, unsigned long data5, unsigned long data6, unsigned long data7)
{
  int i;

  for (i = 0; i < npipes; i++)
  {
    SendPacket (i, event_type, num_datum, data1, data2, data3, data4, data5, data6, data7);
  }
}

void
SendConfig (int module, unsigned long event_type, XfwmWindow * t)
{
#ifdef REQUIRES_STASHEVENT
  extern Time lastTimestamp;
#endif
  unsigned long body[MAX_BODY_SIZE + HEADER_SIZE];

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = MAX_PACKET_SIZE;
#ifdef REQUIRES_STASHEVENT
  body[3] = lastTimestamp;
#else
  body[3] = CurrentTime;
#endif
  body[HEADER_SIZE] = t->w;
  body[HEADER_SIZE + 1] = t->frame;
  body[HEADER_SIZE + 2] = (unsigned long) t;
  body[HEADER_SIZE + 3] = t->frame_x;
  body[HEADER_SIZE + 4] = t->frame_y;
  body[HEADER_SIZE + 5] = t->frame_width;
  body[HEADER_SIZE + 6] = t->frame_height;
  body[HEADER_SIZE + 7] = t->Desk;
  body[HEADER_SIZE + 8] = t->flags;
  body[HEADER_SIZE + 9] = t->title_height;
  body[HEADER_SIZE + 10] = t->boundary_width;
  body[HEADER_SIZE + 11] = (t->hints.flags & PBaseSize) ? t->hints.base_width : 0;
  body[HEADER_SIZE + 12] = (t->hints.flags & PBaseSize) ? t->hints.base_height : 0;
  body[HEADER_SIZE + 13] = (t->hints.flags & PResizeInc) ? t->hints.width_inc : 1;
  body[HEADER_SIZE + 14] = (t->hints.flags & PResizeInc) ? t->hints.height_inc : 1;
  body[HEADER_SIZE + 15] = t->hints.min_width;
  body[HEADER_SIZE + 16] = t->hints.min_height;
  body[HEADER_SIZE + 17] = t->hints.max_width;
  body[HEADER_SIZE + 18] = t->hints.max_height;
  body[HEADER_SIZE + 19] = t->icon_w;
  body[HEADER_SIZE + 20] = t->icon_pixmap_w;
  body[HEADER_SIZE + 21] = t->hints.win_gravity;
  body[HEADER_SIZE + 22] = t->TextPixel;
  body[HEADER_SIZE + 23] = t->BackPixel;
  body[HEADER_SIZE + 24] = t->shade_x;
  body[HEADER_SIZE + 25] = t->shade_y;
  body[HEADER_SIZE + 26] = t->shade_width;
  body[HEADER_SIZE + 27] = t->shade_height;

  PositiveWrite (module, body, (MAX_PACKET_SIZE) * sizeof (body[0]));
}


void
BroadcastConfig (unsigned long event_type, XfwmWindow * t)
{
  int i;

  for (i = 0; i < npipes; i++)
  {
    SendConfig (i, event_type, t);
  }
}

void
SendName (int module, unsigned long event_type, unsigned long data1, unsigned long data2, unsigned long data3, char *name)
{
#ifdef REQUIRES_STASHEVENT
  extern Time lastTimestamp;
#endif
  int l;
  unsigned long *body;

  if (name == NULL)
    return;
  l = strlen (name) / (sizeof (unsigned long)) + HEADER_SIZE + 4;
  body = (unsigned long *) safemalloc (l * sizeof (unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = l;
#ifdef REQUIRES_STASHEVENT
  body[3] = lastTimestamp;
#else
  body[3] = CurrentTime;
#endif

  body[HEADER_SIZE] = data1;
  body[HEADER_SIZE + 1] = data2;
  body[HEADER_SIZE + 2] = data3;
  strcpy ((char *) &body[HEADER_SIZE + 3], name);

  PositiveWrite (module, (unsigned long *) body, l * sizeof (unsigned long));

  free (body);
}

void
BroadcastName (unsigned long event_type, unsigned long data1, unsigned long data2, unsigned long data3, char *name)
{
  int i;

  for (i = 0; i < npipes; i++)
    SendName (i, event_type, data1, data2, data3, name);
}

/*
 * ** send an arbitrary string to all instances of a module
 */
void
SendStrToModule (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  char *module, *str;
  int i;

  GetNextToken (action, &module);
  str = strdup (action + strlen (module) + 1);

  for (i = 0; i < npipes; i++)
  {
    if ((pipeName[i] != NULL) && (matchWildcards (module, pipeName[i])))
    {
      SendName (i, XFCE_M_STRING, 0, 0, 0, str);
    }
  }

  free (module);
  free (str);
}



int
PositiveWrite (int module, unsigned long *ptr, int size)
{
  if ((pipeOn[module] < 0) || (!((PipeMask[module]) & ptr[1])))
    return -1;

  AddToQueue (module, ptr, size, 0);
  return size;
}


void
AddToQueue (int module, unsigned long *ptr, int size, int done)
{
  struct queue_buff_struct *c, *e;
  unsigned long *d;

  c = (struct queue_buff_struct *) safemalloc (sizeof (struct queue_buff_struct));
  c->next = NULL;
  c->size = size;
  c->done = done;
  d = (unsigned long *) safemalloc (size);
  c->data = d;
  memcpy (d, ptr, size);

  e = pipeQueue[module];
  if (e == NULL)
  {
    pipeQueue[module] = c;
    return;
  }
  while (e->next != NULL)
    e = e->next;
  e->next = c;
}

void
DeleteQueueBuff (int module)
{
  struct queue_buff_struct *a;

  if (pipeQueue[module] == NULL)
    return;
  a = pipeQueue[module];
  pipeQueue[module] = a->next;
  free (a->data);
  free (a);
  return;
}

void
FlushQueue (int module)
{
  char *dptr;
  struct queue_buff_struct *d;
  int a;
  extern int errno;

  if ((pipeOn[module] <= 0) || (pipeQueue[module] == NULL))
    return;

  while (pipeQueue[module] != NULL)
  {
    d = pipeQueue[module];
    dptr = (char *) d->data;
    while (d->done < d->size)
    {
      a = write (writePipes[module], &dptr[d->done], d->size - d->done);
      if (a >= 0)
	d->done += a;
      /* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
       * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
       * returns EAGAIN under these conditions. Hopefully other OSes
       * return one of these values too. Solaris 2 doesn't seem to have
       * a man page for write(2) (!) */
      else if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR))
      {
	return;
      }
      else
      {
	KillModule (module, 123);
	return;
      }
    }
    DeleteQueueBuff (module);
  }
}


void
send_list_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  XfwmWindowList *t;

  if (*Module >= 0)
  {
    SendPacket (*Module, XFCE_M_NEW_DESK, 1, Scr.CurrentDesk, 0, 0, 0, 0, 0, 0);
    if (Scr.Hilite != NULL)
      SendPacket (*Module, XFCE_M_FOCUS_CHANGE, 5, Scr.Hilite->w, Scr.Hilite->frame, (unsigned long) Scr.Hilite, Scr.DefaultDecor.HiColors.fore, Scr.DefaultDecor.HiColors.back, 0, 0);
    else
      SendPacket (*Module, XFCE_M_FOCUS_CHANGE, 5, 0, 0, 0, Scr.DefaultDecor.HiColors.fore, Scr.DefaultDecor.HiColors.back, 0, 0);
    if (Scr.DefaultIcon != NULL)
      SendName (*Module, XFCE_M_DEFAULTICON, 0, 0, 0, Scr.DefaultIcon);
    for (t = LastXfwmWindowList (Scr.stacklist); t != NULL; t = t->prev)
    {
      SendConfig (*Module, XFCE_M_CONFIGURE_WINDOW, (t->win));
      SendName (*Module, XFCE_M_WINDOW_NAME, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->name);
      SendName (*Module, XFCE_M_ICON_NAME, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->icon_name);
      if ((t->win)->icon_bitmap_file != NULL && (t->win)->icon_bitmap_file != Scr.DefaultIcon)
	SendName (*Module, XFCE_M_ICON_FILE, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->icon_bitmap_file);
      SendName (*Module, XFCE_M_RES_CLASS, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->class.res_class);
      SendName (*Module, XFCE_M_RES_NAME, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->class.res_name);

      if ((t->win)->flags & ICONIFIED)
      {
	if (!((t->win)->flags & ICON_UNMAPPED))
	  SendPacket (*Module, XFCE_M_ICONIFY, 7, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), (t->win)->icon_x_loc, (t->win)->icon_y_loc, (t->win)->icon_w_width, (t->win)->icon_w_height + (t->win)->icon_p_height);
	else
	  SendPacket (*Module, XFCE_M_ICONIFY, 7, (t->win)->w, (t->win)->frame, (unsigned long) (t->win), 0, 0, 0, 0);
      }
    }
    if (Scr.Hilite == NULL)
    {
      Broadcast (XFCE_M_FOCUS_CHANGE, 5, 0, 0, 0, Scr.DefaultDecor.HiColors.fore, Scr.DefaultDecor.HiColors.back, 0, 0);
    }
    else
    {
      Broadcast (XFCE_M_FOCUS_CHANGE, 5, Scr.Hilite->w, Scr.Hilite->frame, (unsigned long) Scr.Hilite, Scr.DefaultDecor.HiColors.fore, Scr.DefaultDecor.HiColors.back, 0, 0);
    }
    SendPacket (*Module, XFCE_M_END_WINDOWLIST, 0, 0, 0, 0, 0, 0, 0, 0);
  }
}
void
set_mask_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int n, val1_unit;
  int val1;
  n = GetOneArgument (action, (long *) &val1, &val1_unit, 0, 0);
  PipeMask[*Module] = (unsigned long) val1;
}
