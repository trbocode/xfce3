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

#include "my_intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <pwd.h>
#include <sys/types.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "module.h"
#include "constant.h"
#include "session.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
/* need to get prototype for XrmUniqueQuark for XUniqueContext call */
#include <X11/Xresource.h>
#include <X11/extensions/shape.h>

#if defined (sparc) && defined (SVR4)
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#ifdef  ENABLE_NLS
#include "locale.h"
#endif

#ifdef HAVE_IMLIB
#include <Imlib.h>
#include <fcntl.h>
#endif

#ifndef HAVE_GETHOSTNAME_PROTO
extern int gethostname (char *, size_t);
#endif

#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#include <X11/extensions/Xinerama.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "close.xbm"
#include "iconify.xbm"
#include "maximize.xbm"
#include "maximize_pressed.xbm"
#include "menu.xbm"
#include "shade.xbm"
#include "shade_pressed.xbm"
#include "stick.xbm"
#include "stick_pressed.xbm"

#define MAXHOSTNAME 255

#ifdef EMULATE_XINERAMA
/* NOTE : Look in xfwm.h to define EMULATE_XINERAMA
 * (do not do it here)
 * 
 * We simulate 2 screens side by side on a 1024x768 screen :
 * Left screen is 512x768 pixels wide
 * Right screen is 512x600 pixels wide with 84 unused pixels 
 *   on top and bottom of screen (this is to simulate the
 *   black holes as show below :
 *
 *  +-----------------+//////////////////
 *  |                 |///BLACK HOLE/////
 *  |                 |//////////////////
 *  |                 |-----------------+
 *  |                 |                 |
 *  |                 |                 |
 *  |      First      |      Second     |
 *  |     Screen      |      Screen     |
 *  |                 |                 |
 *  |                 |                 |
 *  |                 |-----------------+
 *  |                 |//////////////////
 *  |                 |///BLACK HOLE/////
 *  +-----------------+//////////////////
 *
 */

static XineramaScreenInfo emulate_infos[2] = {
  {0, 0, 0, 512, 768},
  {0, 512, 84, 512, 600}
};
static int emulate_heads = 2;
#endif

#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H) || defined(EMULATE_XINERAMA)
XineramaScreenInfo *xinerama_infos;
int xinerama_heads;
Bool enable_xinerama;
#endif

#ifdef HAVE_X11_XFT_XFT_H
Bool enable_xft;
#endif

int runlevel;
int master_pid;			/* process number of 1st xfwm process */

ScreenInfo Scr;			/* structures for the screen */
Display *dpy;			/* which display are we talking to */

#ifdef HAVE_IMLIB
ImlibData *imlib_id;
#endif


Window BlackoutWin = None;	/* window to hide window captures */

char *default_config_command = "ReadCfg xfwmrc";
char *user_menu_command = "Read xfwm.user_menu";

extern char *ModulePath;

#define MAX_CFG_CMDS 10
static char *config_commands[MAX_CFG_CMDS];
static int num_config_commands = 0;

char *output_file = NULL;

static XErrorHandler XfwmErrorHandler (Display *, XErrorEvent *);
static XIOErrorHandler CatchFatal (Display *);
static XErrorHandler CatchRedirectError (Display *, XErrorEvent *);
static void newhandler (int sig);
static RETSIGTYPE on_signal (int sig_num);
static void SaveDesktopState (void);
static void SetMWM_INFO (Window window);
static void SetRCDefaults (void);
static void InternUsefulAtoms (void);
static void InitVariables (void);
void CreateCursors (void);
void ChildDied (int nonsense);
void StartupStuff (void);
void initModifiers (void);

static void usage (void);
static void version (void);

XContext XfwmContext;		/* context for xfwm windows */
XContext MenuContext;		/* context for xfwm menus */

Bool PPosOverride, Blackout = False;

unsigned int KeyMask;
unsigned int ButtonMask;
unsigned int ButtonKeyMask;
unsigned int AltMask;
unsigned int MetaMask;
unsigned int NumLockMask;
unsigned int ScrollLockMask;
unsigned int CapsLockMask;
unsigned int SuperMask;
unsigned int HyperMask;

#if 0
char *charset = NULL;
#endif
char **g_argv;
int g_argc;

Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_DESKTOP;
Atom _XA_MwmAtom;
Atom _XA_MOTIF_WM;

Atom _XA_OL_WIN_ATTR;
Atom _XA_OL_WT_BASE;
Atom _XA_OL_WT_CMD;
Atom _XA_OL_WT_HELP;
Atom _XA_OL_WT_NOTICE;
Atom _XA_OL_WT_OTHER;
Atom _XA_OL_DECOR_ADD;
Atom _XA_OL_DECOR_DEL;
Atom _XA_OL_DECOR_CLOSE;
Atom _XA_OL_DECOR_RESIZE;
Atom _XA_OL_DECOR_HEADER;
Atom _XA_OL_DECOR_ICON_NAME;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_LAYER;
Atom _XA_WIN_STATE;

Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_SM_CLIENT_ID;
Atom _XA_XFWM_FLAGS;
Atom _XA_XFWM_ICONPOS_X;
Atom _XA_XFWM_ICONPOS_Y;
Atom _WIN_DESKTOP_BUTTON_PROXY;

int ShapeEventBase, ShapeErrorBase;
Bool ShapesSupported = False;

long isIconicState = 0;
extern XEvent Event;
Bool Restarting = False;
int fd_width, x_fd;
char *display_name = NULL;
char *client_id = NULL;

char restore_filename[MAXSTRLEN + 1];

/***********************************************************************
 *
 *  Procedure:
 *	main - start of xfwm
 *
 ***********************************************************************
 */
int
main (int argc, char **argv)
{
  XSetWindowAttributes attributes;	/* attributes for create windows */
  /* XIconSize *iconsize; */
  void enterAlarm (int);
  int i;
  extern int x_fd;
  int len;
  char *display_string;
  char message[255];
  Bool single = False;
  Bool option_error = False;
  Bool custom_filename = False;
  Bool launch_xfce = True;
  char *temp_path = NULL;
  struct passwd *pwd;
/* ??? patch 3.8.12c
  int ruid, euid, suid;
 ??? patch 3.8.12c */

#if defined(HAVE_X11_EXTENSIONS_XINERAMA_H)
  int xinerama_major, xinerama_minor;
#endif

#ifdef HAVE_X11_XFT_XFT_H
  Bool use_xft = True;
#endif

#ifdef  ENABLE_NLS
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, XFCE_LOCALE_DIR);
  textdomain (PACKAGE);

  g_argv = argv;
  g_argc = argc;

/* ??? patch 3.8.12c
  getresuid(&ruid, &euid, &suid);
  fprintf(stderr, "%d %d %d\n", ruid, euid, suid);
 ??? patch 3.8.12c */
  
  temp_path = getenv ("SM_SAVE_DIR");
  if (!temp_path)
  {
    pwd = getpwuid (getuid ());
    temp_path = pwd->pw_dir;
  }
  snprintf (restore_filename, MAXSTRLEN, "%s/.xfce/xfwm-session", temp_path);
  custom_filename = False;

  for (i = 1; i < argc; i++)
  {
    if (mystrncasecmp (argv[i], "-s", 2) == 0)
    {
      single = True;
    }
    else if (mystrncasecmp (argv[i], "-noxfce", 7) == 0)
    {
      launch_xfce = False;
    }
#ifdef HAVE_SESSION
    else if (strncasecmp (argv[i], "-clientId", 9) == 0)
    {
      if (++i >= argc)
	usage ();
      client_id = argv[i];
    }
#endif
    else if (strncasecmp (argv[i], "-restore", 8) == 0)
    {
      if (++i >= argc)
	usage ();
      snprintf (restore_filename, MAXSTRLEN, "%s", argv[i]);
      custom_filename = True;
    }
    else if (mystrncasecmp (argv[i], "-d", 2) == 0)
    {
      if (++i >= argc)
	usage ();
      display_name = argv[i];
    }
    else if (mystrncasecmp (argv[i], "-f", 2) == 0)
    {
      if (++i >= argc)
	usage ();
      if (num_config_commands < MAX_CFG_CMDS)
      {
	config_commands[num_config_commands] = (char *) safemalloc (9 + strlen (argv[i]));
	strcpy (config_commands[num_config_commands], "ReadCfg ");
	strcat (config_commands[num_config_commands], argv[i]);
	num_config_commands++;
      }
      else
      {
	xfwm_msg (ERR, "main", "only %d -f and -cmd parms allowed!", MAX_CFG_CMDS);
      }
    }
    else if (mystrncasecmp (argv[i], "-cmd", 4) == 0)
    {
      if (++i >= argc)
	usage ();
      if (num_config_commands < MAX_CFG_CMDS)
      {
	config_commands[num_config_commands] = strdup (argv[i]);
	num_config_commands++;
      }
      else
      {
	xfwm_msg (ERR, "main", "only %d -f and -cmd parms allowed!", MAX_CFG_CMDS);
      }
    }
    else if (mystrncasecmp (argv[i], "-outfile", 8) == 0)
    {
      if (++i >= argc)
	usage ();
      output_file = argv[i];
    }
    else if (mystrncasecmp (argv[i], "-h", 2) == 0)
    {
      usage ();
      exit (0);
    }
    else if (mystrncasecmp (argv[i], "-blackout", 9) == 0)
    {
      Blackout = True;
    }
#ifdef HAVE_X11_XFT_XFT_H
    else if (mystrncasecmp (argv[i], "-noxft", 6) == 0)
    {
      use_xft = False;
    }
#endif
    else if (mystrncasecmp (argv[i], "-version", 8) == 0)
    {
      version ();
      exit (0);
    }
    else
    {
      xfwm_msg (ERR, "main", "Unknown option:  `%s'\n", argv[i]);
      option_error = True;
    }
  }

  if (option_error)
  {
    usage ();
  }
  ReapChildren ();

  newhandler (SIGINT);
  newhandler (SIGHUP);
  newhandler (SIGQUIT);
  newhandler (SIGTERM);
#ifndef DEBUG
  signal (SIGABRT, on_signal);
  signal (SIGBUS, on_signal);
  signal (SIGSEGV, on_signal);
  signal (SIGFPE, on_signal);
#endif
  signal (SIGPIPE, DeadPipe);
  signal (SIGALRM, enterAlarm);
  signal (SIGUSR1, Restart);
  signal (SIGUSR2, Restart);

  if (!(dpy = XOpenDisplay (display_name)))
  {
    xfwm_msg (ERR, "main", "can't open display %s", XDisplayName (display_name));
    exit (1);
  }
  Scr.screen = DefaultScreen (dpy);
  Scr.NumberOfScreens = ScreenCount (dpy);

  initModifiers ();
  master_pid = getpid ();

  if (!single)
  {
    int myscreen = 0;
    char *cp;

    strcpy (message, XDisplayString (dpy));

    for (i = 0; i < Scr.NumberOfScreens; i++)
    {
      if ((i != Scr.screen) && (fork () == 0))
      {
	myscreen = i;

	/*
	 * Truncate the string 'whatever:n.n' to 'whatever:n',
	 * and then append the screen number.
	 */
	cp = strchr (message, ':');
	if (cp != NULL)
	{
	  cp = strchr (cp, '.');
	  if (cp != NULL)
	    *cp = '\0';		/* truncate at display part */
	}
	sprintf (message + strlen (message), ".%d", myscreen);
	dpy = XOpenDisplay (message);
	Scr.screen = myscreen;
	Scr.NumberOfScreens = ScreenCount (dpy);
	if ((!custom_filename) && (myscreen != 0))
	{
	  snprintf (restore_filename, MAXSTRLEN, "%s/.xfce/xfwm-session-%i", temp_path, (int) Scr.screen);
	}

	break;
      }
    }
  }

#ifdef EMULATE_XINERAMA
  xinerama_infos = emulate_infos;
  xinerama_heads = emulate_heads;
  enable_xinerama = True;
#else
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
  if (!XineramaQueryExtension (dpy, &xinerama_major, &xinerama_minor))
  {
    enable_xinerama = False;
    xfwm_msg (INFO, "main", "Xinerama extension disabled\n");
  }
  else
  {
    enable_xinerama = True;
    xinerama_infos = XineramaQueryScreens (dpy, &xinerama_heads);
  }
#endif
#endif

#ifdef HAVE_X11_XFT_XFT_H
  enable_xft = (use_xft & XftDefaultHasRender (dpy));
  if (!enable_xft)
  {
    xfwm_msg (INFO, "main", "Render (and Xft)  extension disabled\n");
  }
#endif
  x_fd = XConnectionNumber (dpy);
  fd_width = GetFdWidth ();

  if (fcntl (x_fd, F_SETFD, 1) == -1)
  {
    xfwm_msg (ERR, "main", "close-on-exec failed");
    exit (1);
  }

  /* Add a DISPLAY entry to the environment, in case we were started
   * with xfwm -display term:0.0
   */
  len = strlen (XDisplayString (dpy));
  display_string = safemalloc (len + 10);
  sprintf (display_string, "DISPLAY=%s", XDisplayString (dpy));
  putenv (display_string);
  /* Add a HOSTDISPLAY environment variable, which is the same as
   * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
   * host name will be used for ease in networking . */
  /* Note: Can't free the rdisplay_string after putenv, because it
   * becomes part of the environment! */
  if (strncmp (display_string, "DISPLAY=:", 9) == 0)
  {
    char client[MAXHOSTNAME], *rdisplay_string;
    mygethostname (client, MAXHOSTNAME);
    rdisplay_string = safemalloc (len + 14 + strlen (client));
    sprintf (rdisplay_string, "HOSTDISPLAY=%s:%s", client, &display_string[9]);
    putenv (rdisplay_string);
  }
  else if (strncmp (display_string, "DISPLAY=unix:", 13) == 0)
  {
    char client[MAXHOSTNAME], *rdisplay_string;
    mygethostname (client, MAXHOSTNAME);
    rdisplay_string = safemalloc (len + 14 + strlen (client));
    sprintf (rdisplay_string, "HOSTDISPLAY=%s:%s", client, &display_string[13]);
    putenv (rdisplay_string);
  }
  else
  {
    char *rdisplay_string;
    rdisplay_string = safemalloc (len + 14);
    sprintf (rdisplay_string, "HOSTDISPLAY=%s", XDisplayString (dpy));
    putenv (rdisplay_string);
  }

  Scr.Root = RootWindow (dpy, Scr.screen);
  if (Scr.Root == None)
  {
    xfwm_msg (ERR, "main", "Screen %d is not a valid screen", (char *) Scr.screen);
    exit (1);
  }

#ifdef HAVE_IMLIB
  imlib_id = Imlib_init (dpy);
#endif

  ShapesSupported = XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);

  InternUsefulAtoms ();

  /* Make sure property priority colors is empty */
  XChangeProperty (dpy, Scr.Root, _XA_MIT_PRIORITY_COLORS, XA_CARDINAL, 32, PropModeReplace, NULL, 0);

  XSetErrorHandler ((XErrorHandler) CatchRedirectError);
  XSetIOErrorHandler ((XIOErrorHandler) CatchFatal);
  XSelectInput (dpy, Scr.Root, LeaveWindowMask | 
                               EnterWindowMask | 
			       PropertyChangeMask | 
			       SubstructureRedirectMask | 
			       SubstructureNotifyMask | 
			       KeyPressMask | KeyReleaseMask | 
			       PointerMotionMask |	/* AutoDeskSwitch */
		               ButtonPressMask | ButtonReleaseMask | 
			       ColormapChangeMask);
  XSync (dpy, 0);
  XSetErrorHandler ((XErrorHandler) XfwmErrorHandler);
  BlackoutScreen ();		/* if they want to hide the capture/startup */

  /* create a window which will accept the keyboard focus when no other
   * windows have it */
  attributes.event_mask = KeyPressMask | FocusChangeMask | KeyReleaseMask;
  attributes.override_redirect = True;
  Scr.NoFocusWin = XCreateWindow (dpy, Scr.Root, -10, -10, 10, 10, 0, 0, InputOnly, CopyFromParent, CWEventMask | CWOverrideRedirect, &attributes);
  XMapWindow (dpy, Scr.NoFocusWin);

  SetMWM_INFO (Scr.NoFocusWin);

  XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);

  attributes.event_mask = NoEventMask;
  attributes.override_redirect = True;
  Scr.GnomeProxyWin = XCreateWindow (dpy, Scr.Root, -10, -10, 10, 10, 0, 0, InputOnly, CopyFromParent, CWEventMask | CWOverrideRedirect, &attributes);
  XMapWindow (dpy, Scr.GnomeProxyWin);
  XChangeProperty (dpy, Scr.Root, _WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &Scr.GnomeProxyWin, 1);
  XChangeProperty (dpy, Scr.GnomeProxyWin, _WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &Scr.GnomeProxyWin, 1);

  /* Set prefered icons size for apps that can handle it
  if ((iconsize = XAllocIconSize()) != NULL)
  {
    iconsize->min_width  = 48;
    iconsize->max_width  = 50;
    iconsize->min_height = 48;
    iconsize->max_height = 50;
    iconsize->width_inc  = 1;
    iconsize->height_inc = 1;
    XSetIconSizes (dpy, Scr.Root, iconsize, 1);
    XFree (iconsize);
  } 
   */
  CreateCursors ();
  InitVariables ();
  InitEventHandlerJumpTable ();
  initModules ();

  XDefineCursor (dpy, Scr.Root, Scr.XfwmCursors[DEFAULT]);

  SetRCDefaults ();

  if (num_config_commands > 0)
  {
    int i;
    for (i = 0; i < num_config_commands; i++)
    {
      ExecuteFunction (config_commands[i], NULL, &Event, C_ROOT, -1);
      free (config_commands[i]);
    }
  }
  else
  {
    ExecuteFunction (user_menu_command, NULL, &Event, C_ROOT, -1);
    ExecuteFunction (default_config_command, NULL, &Event, C_ROOT, -1);
  }
  StartupStuff ();

  if (launch_xfce)
  {
    ExecuteFunction ("Module xfce", NULL, &Event, C_ROOT, -1);
  }
  UnBlackoutScreen ();
  if (Scr.Options & SessionMgt)
  {
    LoadWindowStates (restore_filename);
  }
  SessionInit (client_id);
  SetRunLevel (XFWM_RUNNING);
  /* The restart procedure is now over, no we can safely turn Restarting to False */
  Restarting = False;
  XSync (dpy, 0);
  HandleEvents ();
  switch (runlevel)
  {
  case XFWM_RESTART:
    Done (XFWM_RESTART, *g_argv);
    break;
  case XFWM_HALT:
  case XFWM_END:
  default:
    Done (XFWM_HALT, NULL);
    break;
  }
  return (0);
}


/*
 * ** StartupStuff
 * **
 * ** Does initial window captures and runs init/restart function
 */
void
StartupStuff (void)
{
  MenuRoot *mr;

  CaptureAllWindows ();
  MakeMenus ();

  if (Restarting)
  {
    mr = FindPopup ("RestartFunction");
    if (mr != NULL)
      ExecuteFunction ("Function RestartFunction", NULL, &Event, C_ROOT, -1);
  }
  else
  {
    mr = FindPopup ("InitFunction");
    if (mr != NULL)
      ExecuteFunction ("Function InitFunction", NULL, &Event, C_ROOT, -1);
  }
}

/***********************************************************************
 *
 *  Procedure:
 *      CaptureAllWindows
 *
 *   Decorates all windows at start-up and during recaptures
 *
 ***********************************************************************/

void
CaptureAllWindows (void)
{
  int i, j;
  unsigned int nchildren;
  Window root, parent, *children;
  XfwmWindow *tmp, *next;	/* temp xfwm window structure */
  Window w;
  unsigned long data[1];
  unsigned char *prop;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;

  MyXGrabServer (dpy);
  if (!XQueryTree (dpy, Scr.Root, &root, &parent, &children, &nchildren))
  {
    MyXUngrabServer (dpy);
    return;
  }

  PPosOverride = True;

  if (!(Scr.flags & WindowsCaptured))	/* initial capture? */
  {
    /*
     * ** weed out icon windows
     */
    for (i = 0; i < nchildren; i++)
    {
      if (children[i])
      {
	XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
	if (wmhintsp)
	{
	  if (wmhintsp->flags & IconWindowHint)
	  {
	    for (j = 0; j < nchildren; j++)
	    {
	      if (children[j] == wmhintsp->icon_window)
	      {
		children[j] = None;
		break;
	      }
	    }
	  }
	  XFree (wmhintsp);
	}
      }
    }
    /*
     * ** map all of the non-override, non-icon windows
     */
    for (i = 0; i < nchildren; i++)
    {
      if (children[i] && MappedNotOverride (children[i]))
      {
	XUnmapWindow (dpy, children[i]);
	Event.xmaprequest.window = children[i];
	HandleMapRequest ();
      }
    }
    Scr.flags |= WindowsCaptured;
  }
  else
    /* must be recapture */
  {
    /* reborder all windows */
    tmp = Scr.XfwmRoot.next;
    for (i = 0; i < nchildren; i++)
    {
      if (XFindContext (dpy, children[i], XfwmContext, (caddr_t *) & tmp) != XCNOENT)
      {
	isIconicState = DontCareState;
	if (XGetWindowProperty (dpy, tmp->w, _XA_WM_STATE, 0L, 3L, False, _XA_WM_STATE, &atype, &aformat, &nitems, &bytes_remain, &prop) == Success)
	{
	  if (prop != NULL)
	  {
	    isIconicState = *(long *) prop;
	    XFree (prop);
	  }
	}
	next = tmp->next;
	data[0] = (unsigned long) tmp->Desk;
	XChangeProperty (dpy, tmp->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32, PropModeReplace, (unsigned char *) data, 1);

	data[0] = (unsigned long) (tmp->flags & (STICKY | ICON_MOVED | SHADED));
	XChangeProperty (dpy, tmp->w, _XA_XFWM_FLAGS, _XA_XFWM_FLAGS, 32, PropModeReplace, (unsigned char *) data, 1);
	data[0] = (unsigned long) tmp->icon_x_loc;
	XChangeProperty (dpy, tmp->w, _XA_XFWM_ICONPOS_X, _XA_XFWM_ICONPOS_X, 32, PropModeReplace, (unsigned char *) data, 1);
	data[0] = (unsigned long) tmp->icon_y_loc;
	XChangeProperty (dpy, tmp->w, _XA_XFWM_ICONPOS_Y, _XA_XFWM_ICONPOS_Y, 32, PropModeReplace, (unsigned char *) data, 1);
	XSelectInput (dpy, tmp->w, NoEventMask);
	w = tmp->w;
	XUnmapWindow (dpy, tmp->w);
	XUnmapWindow (dpy, tmp->frame);
	RestoreWithdrawnLocation (tmp, True);
	Destroy (tmp);
	Event.xmaprequest.window = w;
	HandleMapRequest ();
	tmp = next;
      }
    }
  }

  isIconicState = DontCareState;

  if (nchildren > 0)
    XFree (children);

  /* after the windows already on the screen are in place,
   * don't use PPosition */
  PPosOverride = False;
  MyXUngrabServer (dpy);
  XSync (dpy, 0);
}

/*
 * ** SetRCDefaults
 * **
 * ** Sets some initial style values & such
 */
static void
SetRCDefaults ()
{
  /* set up default colors, fonts, etc */
  char *defaults[] = {
    "XORValue 0",
    "ModulePath $PATH",
    "ActiveColor white #DCDCDC",
    "InactiveColor black #DCDCDC",
    "TitleStyle Active Solid #0A5F89",
    "TitleStyle Inactive Solid #C8C8C8",
    "MenuColor black #DCDCDC black #F2F2F2",
    "CursorColor #000000 #FFFFFF",
    "FocusMode Click",
    "AnimateWin Off",
    "OpaqueMove On",
    "OpaqueResize On",
    "AutoRaise Off",
    "AutoRaiseDelay 250",
    "SnapSize 10",
    "SessionManagement On",
    "WaitSession 10",
    "IconFont fixed",
    "IconGrid 10",
    "IconSpacing 5",
    "IconPos Right",
    "Style \"*\" Title, BorderWidth 4",
    "Style \"XFce*\" Sticky",
    "Style \"XFbd*\" Sticky",
    "Style \"Popup*\" Sticky",

    "AddToMenu \"__builtin_exit_menu__\" \"" N_("&Restart") "\" Restart xfwm",
    "AddToMenu \"__builtin_exit_menu__\" \"" N_("&Quit") "\" Quit",

    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 1\" WindowsDesk 0 0",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 2\" WindowsDesk 0 1",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 3\" WindowsDesk 0 2",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 4\" WindowsDesk 0 3",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 5\" WindowsDesk 0 4",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 6\" WindowsDesk 0 5",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 7\" WindowsDesk 0 6",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 8\" WindowsDesk 0 7",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 9\" WindowsDesk 0 8",
    "AddToMenu \"__builtin_sendtodesk_menu__\" \"" N_("Workspace") " 10\" WindowsDesk 0 9",

    "AddToMenu \"__builtin_window_menu__\" \"" N_("&Move") "\" Move",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("&Resize") "\" Resize",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("(De)&Iconify") "\" Iconify",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("(Un)&Stick") "\" Stick",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("(Un)S&hade") "\" Shade",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("(Un)Ma&ximize") "\" Maximize 100 100",
    "AddToMenu \"__builtin_window_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("Se&nd to...") "\" Popup \"__builtin_sendtodesk_menu__\"",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("S&witch to...") "\" WindowList",
    "AddToMenu \"__builtin_window_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_window_menu__\" \"" N_("&Close") "\" Delete",

    "AddToMenu \"user_menu\" \"" N_("User Menu") "\" Title",
    "AddToMenu \"user_menu\" \"" N_("Edit menu...") "\" Exec xfumed",
    "AddToMenu \"user_menu\" \"\" Nop",

    "AddToMenu \"__settings_menu__\" \"" N_("Settings") "\" Title",
    "AddToMenu \"__settings_menu__\" \"" N_("&Mouse...") "\" Exec xfmouse -i",
    "AddToMenu \"__settings_menu__\" \"" N_("&Backdrop...") "\" Exec xfbd -i",
    "AddToMenu \"__settings_menu__\" \"" N_("&Sound...") "\" Exec xfsound",

    "AddToMenu \"__builtin_root_menu__\" \"" N_("&New window") "\" Exec xfterm",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&User Menu") "\" popup user_menu",
    "AddToMenu \"__builtin_root_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("Run program ...") "\" Exec xfrun",
    "AddToMenu \"__builtin_root_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&Arrange icons") "\" ArrangeIcons",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&Iconify all") "\" IconifyAll",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&Refresh") "\" Refresh",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("Shuffle &Up") "\" Next [*] focus",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("Shuffle &Down") "\"  Prev [*] focus",
    "AddToMenu \"__builtin_root_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&Settings") "\" popup __settings_menu__",
    "AddToMenu \"__builtin_root_menu__\" \"\" Nop",
    "AddToMenu \"__builtin_root_menu__\" \"" N_("&Quit XFwm") "\" popup \"__builtin_exit_menu__\"",

    "AddToMenu \"__Maximize_menu__\" \"" N_("&Full Screen") "\" Maximize 100 100",
    "AddToMenu \"__Maximize_menu__\" \"" N_("&Vertical") "\" Maximize 0   100",
    "AddToMenu \"__Maximize_menu__\" \"" N_("&Horizontal") "\" Maximize 100   0",

    "AddToFunc WindowListFunc \"I\" WindowId $0 Iconify -1",
    "+ \"I\" WindowId $0 Raise",
    "+ \"I\" WindowId $0 Focus",

    "AddToFunc \"InitFunction\"",
    "+ \"I\" Desk 0 0",

    "AddToFunc \"RestartFunction\"",
    "+ \"I\" Desk 0 0",

    "AddToFunc \"__Iconfunc__\"",
    "+ \"M\" Move",
    "+ \"D\" Iconify",

    "AddToFunc \"__Move__\"",
    "+ \"M\" Move",
    "+ \"D\" Maximize",

    "AddToFunc \"__Resize__\"",
    "+ \"M\" Resize",

    "AddToFunc \"__builtin_showmemouse_or_move__\"",
    "+ \"M\" Move",
    "+ \"C\" ShowMeMouse",

    "AddToFunc \"__builtin_windowops_or_die__\"",
    "+ \"M\" PopUp \"__builtin_window_menu__\"",
    "+ \"C\" PopUp \"__builtin_window_menu__\"",
    "+ \"D\" Close",

    "AddToFunc \"__builtin_Focus_Prev__\"",
    "+ \"I\" Prev [!Iconic CurrentDesk] Focus",

    "AddToFunc \"__builtin_focus_raise__\"",
    "+ \"I\" Raise",
    "+ \"I\" Focus",

    "AddToFunc \"__builtin_iconify_and_focus__" "+ \"I\" Iconify",
    "+ \"I\" Current [Iconic] __builtin_Focus_Prev__",

    "AddToFunc \"__builtin_maximize_function__\"",
    "+ \"C\" Maximize 100 100",
    "+ \"M\" PopUp \"__Maximize_menu__\"",
    "Mouse 0 1 A __builtin_windowops_or_die__",
    "Mouse 0 4 A __builtin_maximize_function__",
    "Mouse 0 3 A Stick",
    "Mouse 0 6 A Iconify",
    "Mouse 0 5 A Shade",
    "Mouse 0 2 A Close",
    "Mouse 0 2 S Destroy",
    "Mouse 0 FS A Function __Resize__",
    "Mouse 0 I A Function __Iconfunc__",
    "Mouse 1 T A Function __Move__",
    "Mouse 1 FS S Move",
    "Mouse 1 FS M Move",
    "Mouse 1 R A Menu __builtin_root_menu__",
    "Mouse 1 W M __builtin_showmemouse_or_move__",
    "Mouse 1 R M ShowMeMouse",
    "Mouse 2 T A RaiseLower",
    "Mouse 2 T C Iconify",
    "Mouse 2 T M Iconify",
    "Mouse 2 T S Iconify",
    "Mouse 2 FS A Move",
    "Mouse 2 R A Menu __builtin_window_menu__",
    "Mouse 3 T A Shade",
    "Mouse 3 FS S Move",
    "Mouse 3 I A Menu __builtin_window_menu__",
    "Mouse 3 R A WindowList CurrentDesk",
    "Key Tab A M Switch",
    "Key Tab A SM Next [CurrentDesk] __builtin_focus_raise__",
    "Key Escape A S __builtin_iconify_and_focus__",
    "Key Escape A C WindowList",
    "Key Left A SC CursorMove -1  0",
    "Key Right A SC CursorMove +1 +0",
    "Key Up A SC CursorMove +0 -1",
    "Key Down A SC  CursorMove +0 +1",
    "Key Left A SM  CursorMove -10 +0",
    "Key Right A SM  CursorMove +10 +0",
    "Key Up A SM  CursorMove +0 -10",
    "Key Down A SM  CursorMove +0 +10",
    "Key F1 A M Popup __builtin_window_menu__",
    "Key F2 A M Popup __builtin_root_menu__",
    "Key F3 A M Lower",
    "Key F4 A M Close",
    "Key F5 A M Next [*] focus",
    "Key F6 A M Prev [*] focus",
    "Key F7 A M Move",
    "Key F8 A M Resize",
    "Key F9 A M Iconify",
    "Key F10 A M Maximize",
    "Key F11 A M Shade",
    "Key F12 A M Exec xfrun",
    "Key F1 A C Desk 0 0",
    "Key F2 A C Desk 0 1",
    "Key F3 A C Desk 0 2",
    "Key F4 A C Desk 0 3",
    "Key F5 A C Desk 0 4",
    "Key F6 A C Desk 0 5",
    "Key F7 A C Desk 0 6",
    "Key F8 A C Desk 0 7",
    "Key F9 A C Desk 0 8",
    "Key F10 A C Desk 0 9",
    "Key Left A CM Desk -1",
    "Key Right A CM Desk 1",
    "Key Delete A CM exec xflock",
    "Key L5 IW N RaiseLower",
    "Key L5 IW S Lower",
    "Key L5 IW C Raise",
    "Key L7 IW A Iconify",
    "WindowFont " XFWM_TITLEFONT,
    "MenuFont " XFWM_MENUFONT,
    "IconFont " XFWM_ICONFONT,
    NULL
  };
  int i = 0;

  while (defaults[i])
  {
    ExecuteFunction (defaults[i], NULL, &Event, C_ROOT, -1);
    i++;
  }
}				/* SetRCDefaults */

/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a xfwm frame on the window
 *
 *  Returned Value:
 *	True	- go ahead and frame the window
 *	False	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

int
MappedNotOverride (Window w)
{
  XWindowAttributes wa;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;

  isIconicState = DontCareState;

  if ((w == Scr.NoFocusWin) || (!XGetWindowAttributes (dpy, w, &wa)))
    return False;

  if (XGetWindowProperty (dpy, w, _XA_WM_STATE, 0L, 3L, False, _XA_WM_STATE, &atype, &aformat, &nitems, &bytes_remain, &prop) == Success)
  {
    if (prop != NULL)
    {
      isIconicState = *(long *) prop;
      XFree (prop);
    }
  }
  if (wa.override_redirect == True)
  {
    XSelectInput (dpy, w, FocusChangeMask);
  }
  return (((isIconicState == IconicState) || (wa.map_state != IsUnmapped)) && (wa.override_redirect != True));
}

static KeyCode
sym2code (KeySym k)
{
  return XKeysymToKeycode (dpy, k);
}

void
initModifiers ()
{
  XModifierKeymap *xmk = XGetModifierMapping (dpy);
  int m, k;

  AltMask = MetaMask = NumLockMask = ScrollLockMask = CapsLockMask = SuperMask = HyperMask = 0;
  if (xmk)
  {
    KeyCode *c = xmk->modifiermap;
    KeyCode numLockKeyCode;
    KeyCode scrollLockKeyCode;
    KeyCode capsLockKeyCode;
    KeyCode altKeyCode;
    KeyCode metaKeyCode;
    KeyCode superKeyCode;
    KeyCode hyperKeyCode;

    numLockKeyCode = sym2code (XK_Num_Lock);
    scrollLockKeyCode = sym2code (XK_Scroll_Lock);
    capsLockKeyCode = sym2code (XK_Caps_Lock);
    altKeyCode = sym2code (XK_Alt_L);
    metaKeyCode = sym2code (XK_Meta_L);
    superKeyCode = sym2code (XK_Super_L);
    hyperKeyCode = sym2code (XK_Hyper_L);

    if (!altKeyCode)
      altKeyCode = sym2code (XK_Alt_R);
    if (!metaKeyCode)
      metaKeyCode = sym2code (XK_Meta_R);
    if (!superKeyCode)
      superKeyCode = sym2code (XK_Super_R);
    if (!hyperKeyCode)
      hyperKeyCode = sym2code (XK_Hyper_R);


    for (m = 0; m < 8; m++)
      for (k = 0; k < xmk->max_keypermod; k++, c++)
      {
	if (*c == NoSymbol)
	  continue;
	if (*c == numLockKeyCode)
	  NumLockMask = (1 << m);
	if (*c == scrollLockKeyCode)
	  ScrollLockMask = (1 << m);
	if (*c == capsLockKeyCode)
	  CapsLockMask = (1 << m);
	if (*c == altKeyCode)
	  AltMask = (1 << m);
	if (*c == metaKeyCode)
	  MetaMask = (1 << m);
	if (*c == superKeyCode)
	  SuperMask = (1 << m);
	if (*c == hyperKeyCode)
	  HyperMask = (1 << m);
      }
    XFreeModifiermap (xmk);
  }
  if (MetaMask == AltMask)
    MetaMask = 0;

  if ((AltMask != 0) && (MetaMask == Mod1Mask))
  {
    MetaMask = AltMask;
    AltMask = Mod1Mask;
  }

  if ((AltMask == 0) && (MetaMask != 0))
  {
    if (MetaMask != Mod1Mask)
    {
      AltMask = Mod1Mask;
    }
    else
    {
      AltMask = MetaMask;
      MetaMask = 0;
    }
  }

  if (AltMask == 0)
    AltMask = Mod1Mask;

  KeyMask = ControlMask | ShiftMask | AltMask | MetaMask | SuperMask | HyperMask;

  ButtonMask = Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask;

  ButtonKeyMask = KeyMask | ButtonMask;
}

static void
InternUsefulAtoms (void)
{
  /*
   * Create priority colors if necessary.
   */
  _XA_MIT_PRIORITY_COLORS = XInternAtom (dpy, "_MIT_PRIORITY_COLORS", False);
  _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_WM_DESKTOP = XInternAtom (dpy, "WM_DESKTOP", False);
  _XA_XFWM_FLAGS = XInternAtom (dpy, "XFWM_FLAGS", False);
  _XA_XFWM_ICONPOS_X = XInternAtom (dpy, "XFWM_ICONPOS_X", False);
  _XA_XFWM_ICONPOS_Y = XInternAtom (dpy, "XFWM_ICONPOS_Y", False);
  _XA_MwmAtom = XInternAtom (dpy, "_MOTIF_WM_HINTS", False);
  _XA_MOTIF_WM = XInternAtom (dpy, "_MOTIF_WM_INFO", False);

  _XA_OL_WIN_ATTR = XInternAtom (dpy, "_OL_WIN_ATTR", False);
  _XA_OL_WT_BASE = XInternAtom (dpy, "_OL_WT_BASE", False);
  _XA_OL_WT_CMD = XInternAtom (dpy, "_OL_WT_CMD", False);
  _XA_OL_WT_HELP = XInternAtom (dpy, "_OL_WT_HELP", False);
  _XA_OL_WT_NOTICE = XInternAtom (dpy, "_OL_WT_NOTICE", False);
  _XA_OL_WT_OTHER = XInternAtom (dpy, "_OL_WT_OTHER", False);
  _XA_OL_DECOR_ADD = XInternAtom (dpy, "_OL_DECOR_ADD", False);
  _XA_OL_DECOR_DEL = XInternAtom (dpy, "_OL_DECOR_DEL", False);
  _XA_OL_DECOR_CLOSE = XInternAtom (dpy, "_OL_DECOR_CLOSE", False);
  _XA_OL_DECOR_RESIZE = XInternAtom (dpy, "_OL_DECOR_RESIZE", False);
  _XA_OL_DECOR_HEADER = XInternAtom (dpy, "_OL_DECOR_HEADER", False);
  _XA_OL_DECOR_ICON_NAME = XInternAtom (dpy, "_OL_DECOR_ICON_NAME", False);

  _XA_WIN_WORKSPACE_COUNT = XInternAtom (dpy, "_WIN_WORKSPACE_COUNT", False);
  _XA_WIN_WORKSPACE = XInternAtom (dpy, "_WIN_WORKSPACE", False);
  _XA_WIN_LAYER = XInternAtom (dpy, "_WIN_LAYER", False);
  _XA_WIN_STATE = XInternAtom (dpy, "_WIN_STATE", False);

  _XA_WM_WINDOW_ROLE = XInternAtom (dpy, "WM_WINDOW_ROLE", False);
  _XA_WM_CLIENT_LEADER = XInternAtom (dpy, "WM_CLIENT_LEADER", False);
  _XA_SM_CLIENT_ID = XInternAtom (dpy, "SM_CLIENT_ID", False);
  _WIN_DESKTOP_BUTTON_PROXY = XInternAtom (dpy, "_WIN_DESKTOP_BUTTON_PROXY", False);

  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	newhandler: Installs new signal handler
 *
 ************************************************************************/
static void
newhandler (int sig)
{
  if (signal (sig, SIG_IGN) != SIG_IGN)
    signal (sig, SigDone);
}

static void
terminate (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf (_("xfwm terminated: "));
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  exit (1);
}

static RETSIGTYPE
on_signal (int sig_num)
{
  fprintf (stderr, "Xfwm has crashed due to a ");
  switch (sig_num)
  {
  case SIGABRT:
    fprintf (stderr, "sigabrt");
    break;
  case SIGBUS:
    fprintf (stderr, "sigbus");
    break;
  case SIGSEGV:
    fprintf (stderr, "sigsegv");
    break;
  case SIGFPE:
    fprintf (stderr, "sigfpe");
    break;
  default:
    fprintf (stderr, "other");
    break;
  }
  if ((!Restarting) && (runlevel != XFWM_RESTART))
  {
    fprintf (stderr, " signal\n");
    fprintf (stderr, "Please report the problem to the Xfce devel list\n");
    fprintf (stderr, "(See http://www.xfce.org for more informations)\n");
    SetRunLevel (XFWM_RESTART);
    Done (XFWM_RESTART, *g_argv);
  }
  terminate ("Restart failed !");
  SIGNAL_RETURN;
}

/*************************************************************************
 * Restart on a signal
 ************************************************************************/
RETSIGTYPE Restart (int nonsense)
{
  xfwm_msg (INFO, "Restart", "Restart signal caught\n");
  SetRunLevel (XFWM_RESTART);
  SIGNAL_RETURN;
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads xfwm cursors
 *
 ***********************************************************************
 */
void
CreateCursors (void)
{
  /* define cursors */
  Scr.XfwmCursors[POSITION] = XCreateFontCursor (dpy, XC_top_left_corner);
  Scr.XfwmCursors[DEFAULT] = XCreateFontCursor (dpy, XFCE_CURS);
  Scr.XfwmCursors[SYS] = XCreateFontCursor (dpy, XFCE_CURS);
  Scr.XfwmCursors[TITLE_CURSOR] = XCreateFontCursor (dpy, XFCE_CURS);
  Scr.XfwmCursors[MOVE] = XCreateFontCursor (dpy, XC_fleur);
  Scr.XfwmCursors[MENU] = XCreateFontCursor (dpy, MENU_CURS);
  Scr.XfwmCursors[WAIT] = XCreateFontCursor (dpy, XC_watch);
  Scr.XfwmCursors[SELECT] = XCreateFontCursor (dpy, XC_center_ptr);
  Scr.XfwmCursors[DESTROY] = XCreateFontCursor (dpy, XC_center_ptr);
  Scr.XfwmCursors[LEFT] = XCreateFontCursor (dpy, XC_left_side);
  Scr.XfwmCursors[RIGHT] = XCreateFontCursor (dpy, XC_right_side);
  Scr.XfwmCursors[TOP] = XCreateFontCursor (dpy, XC_top_side);
  Scr.XfwmCursors[BOTTOM] = XCreateFontCursor (dpy, XC_bottom_side);
  Scr.XfwmCursors[TOP_LEFT] = XCreateFontCursor (dpy, XC_top_left_corner);
  Scr.XfwmCursors[TOP_RIGHT] = XCreateFontCursor (dpy, XC_top_right_corner);
  Scr.XfwmCursors[BOTTOM_LEFT] = XCreateFontCursor (dpy, XC_bottom_left_corner);
  Scr.XfwmCursors[BOTTOM_RIGHT] = XCreateFontCursor (dpy, XC_bottom_right_corner);
}

/***********************************************************************
 *
 *  LoadDefaultLeftButton -- loads default left button # into 
 *		assumes associated button memory is already free
 * 
 ************************************************************************/
void
LoadDefaultLeftButton (ButtonFace * bf, int i)
{
  bf->u.ShadowGC = (GC) NULL;
  bf->u.ReliefGC = (GC) NULL;
  bf->style = PixButton;
  switch (i % 3)
  {
  case 0:
    bf->vector.x[0] = 20;
    bf->vector.y[0] = 42;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 80;
    bf->vector.y[1] = 42;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 80;
    bf->vector.y[2] = 58;
    bf->vector.line_style[2] = 0;
    bf->vector.x[3] = 20;
    bf->vector.y[3] = 58;
    bf->vector.line_style[3] = 0;
    bf->vector.x[4] = 20;
    bf->vector.y[4] = 42;
    bf->vector.line_style[4] = 1;
    bf->vector.num = 5;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) menu_bits, menu_width, menu_height);
    bf->bitmap_pressed = None;
    break;
  case 1:
    bf->vector.x[0] = 30;
    bf->vector.y[0] = 25;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 70;
    bf->vector.y[1] = 25;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 70;
    bf->vector.y[2] = 40;
    bf->vector.line_style[2] = 0;
    bf->vector.x[3] = 55;
    bf->vector.y[3] = 40;
    bf->vector.line_style[3] = 0;
    bf->vector.x[4] = 55;
    bf->vector.y[4] = 75;
    bf->vector.line_style[4] = 0;
    bf->vector.x[5] = 45;
    bf->vector.y[5] = 75;
    bf->vector.line_style[5] = 0;
    bf->vector.x[6] = 45;
    bf->vector.y[6] = 40;
    bf->vector.line_style[6] = 1;
    bf->vector.x[7] = 30;
    bf->vector.y[7] = 40;
    bf->vector.line_style[7] = 0;
    bf->vector.x[8] = 30;
    bf->vector.y[8] = 25;
    bf->vector.line_style[8] = 1;
    bf->vector.num = 9;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) stick_bits, stick_width, stick_height);
    bf->bitmap_pressed = XCreateBitmapFromData (dpy, Scr.Root, (const char *) stick_pressed_bits, stick_pressed_width, stick_pressed_height);
    break;
  case 2:
    bf->vector.x[0] = 20;
    bf->vector.y[0] = 36;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 20;
    bf->vector.y[1] = 25;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 80;
    bf->vector.y[2] = 25;
    bf->vector.line_style[2] = 1;
    bf->vector.x[3] = 80;
    bf->vector.y[3] = 36;
    bf->vector.line_style[3] = 0;
    bf->vector.x[4] = 50;
    bf->vector.y[4] = 36;
    bf->vector.line_style[4] = 0;
    bf->vector.x[5] = 70;
    bf->vector.y[5] = 56;
    bf->vector.line_style[5] = 0;
    bf->vector.x[6] = 58;
    bf->vector.y[6] = 56;
    bf->vector.line_style[6] = 0;
    bf->vector.x[7] = 58;
    bf->vector.y[7] = 75;
    bf->vector.line_style[7] = 0;
    bf->vector.x[8] = 42;
    bf->vector.y[8] = 75;
    bf->vector.line_style[8] = 0;
    bf->vector.x[9] = 42;
    bf->vector.y[9] = 56;
    bf->vector.line_style[9] = 1;
    bf->vector.x[10] = 30;
    bf->vector.y[10] = 56;
    bf->vector.line_style[10] = 0;
    bf->vector.x[11] = 50;
    bf->vector.y[11] = 36;
    bf->vector.line_style[11] = 1;
    bf->vector.x[12] = 20;
    bf->vector.y[12] = 36;
    bf->vector.line_style[12] = 0;
    bf->vector.num = 13;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) shade_bits, shade_width, shade_height);
    bf->bitmap_pressed = XCreateBitmapFromData (dpy, Scr.Root, (const char *) shade_pressed_bits, shade_pressed_width, shade_pressed_height);
    break;
  default:
    break;
  }
}

/***********************************************************************
 *
 *  LoadDefaultRightButton -- loads default left button # into
 *		assumes associated button memory is already free
 * 
 ************************************************************************/
void
LoadDefaultRightButton (ButtonFace * bf, int i)
{
  bf->u.ShadowGC = (GC) NULL;
  bf->u.ReliefGC = (GC) NULL;
  bf->style = PixButton;
  switch (i % 3)
  {
  case 0:
    bf->vector.x[0] = 15;
    bf->vector.y[0] = 25;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 40;
    bf->vector.y[1] = 25;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 50;
    bf->vector.y[2] = 40;
    bf->vector.line_style[2] = 0;
    bf->vector.x[3] = 60;
    bf->vector.y[3] = 25;
    bf->vector.line_style[3] = 1;
    bf->vector.x[4] = 85;
    bf->vector.y[4] = 25;
    bf->vector.line_style[4] = 1;
    bf->vector.x[5] = 65;
    bf->vector.y[5] = 50;
    bf->vector.line_style[5] = 0;
    bf->vector.x[6] = 85;
    bf->vector.y[6] = 75;
    bf->vector.line_style[6] = 0;
    bf->vector.x[7] = 60;
    bf->vector.y[7] = 75;
    bf->vector.line_style[7] = 0;
    bf->vector.x[8] = 50;
    bf->vector.y[8] = 60;
    bf->vector.line_style[8] = 1;
    bf->vector.x[9] = 40;
    bf->vector.y[9] = 75;
    bf->vector.line_style[9] = 0;
    bf->vector.x[10] = 15;
    bf->vector.y[10] = 75;
    bf->vector.line_style[10] = 0;
    bf->vector.x[11] = 35;
    bf->vector.y[11] = 50;
    bf->vector.line_style[11] = 1;
    bf->vector.x[12] = 15;
    bf->vector.y[12] = 25;
    bf->vector.line_style[12] = 1;
    bf->vector.num = 13;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) close_bits, close_width, close_height);
    bf->bitmap_pressed = None;
    break;
  case 1:
    bf->vector.x[0] = 25;
    bf->vector.y[0] = 25;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 75;
    bf->vector.y[1] = 25;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 75;
    bf->vector.y[2] = 75;
    bf->vector.line_style[2] = 0;
    bf->vector.x[3] = 25;
    bf->vector.y[3] = 75;
    bf->vector.line_style[3] = 0;
    bf->vector.x[4] = 25;
    bf->vector.y[4] = 25;
    bf->vector.line_style[4] = 1;
    bf->vector.num = 5;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) maximize_bits, maximize_width, maximize_height);
    bf->bitmap_pressed = XCreateBitmapFromData (dpy, Scr.Root, (const char *) maximize_pressed_bits, maximize_pressed_width, maximize_pressed_height);
    break;
  case 2:
    bf->vector.x[0] = 42;
    bf->vector.y[0] = 42;
    bf->vector.line_style[0] = 1;
    bf->vector.x[1] = 58;
    bf->vector.y[1] = 42;
    bf->vector.line_style[1] = 1;
    bf->vector.x[2] = 58;
    bf->vector.y[2] = 58;
    bf->vector.line_style[2] = 0;
    bf->vector.x[3] = 42;
    bf->vector.y[3] = 58;
    bf->vector.line_style[3] = 0;
    bf->vector.x[4] = 42;
    bf->vector.y[4] = 42;
    bf->vector.line_style[4] = 1;
    bf->vector.num = 5;
    bf->bitmap = XCreateBitmapFromData (dpy, Scr.Root, (const char *) iconify_bits, iconify_width, iconify_height);
    bf->bitmap_pressed = None;
    break;
  default:
    break;
  }
}

/***********************************************************************
 *
 *  LoadDefaultButton -- loads default button # into button structure
 *		assumes associated button memory is already free
 * 
 ************************************************************************/
void
LoadDefaultButton (ButtonFace * bf, int i)
{
  bf->bitmap = None;
  bf->bitmap_pressed = None;
  if (i & 1)
    LoadDefaultRightButton (bf, i / 2);
  else
    LoadDefaultLeftButton (bf, i / 2);
}

extern void FreeButtonFace (Display * dpy, ButtonFace * bf);

/***********************************************************************
 *
 *  ResetAllButtons -- resets all buttons to defaults
 *                 destroys existing buttons
 * 
 ************************************************************************/
void
ResetAllButtons (XfwmDecor * fl)
{
  int i = 0;
  for (; i < 3; ++i)
  {
    int j;

    FreeButtonFace (dpy, &fl->left_buttons[i].state[0]);
    FreeButtonFace (dpy, &fl->right_buttons[i].state[0]);

    LoadDefaultLeftButton (&fl->left_buttons[i].state[0], i);
    LoadDefaultRightButton (&fl->right_buttons[i].state[0], i);

    for (j = 1; j < MaxButtonState; ++j)
    {
      FreeButtonFace (dpy, &fl->left_buttons[i].state[j]);
      FreeButtonFace (dpy, &fl->right_buttons[i].state[j]);

      fl->left_buttons[i].state[j] = fl->left_buttons[i].state[0];
      fl->right_buttons[i].state[j] = fl->right_buttons[i].state[0];
    }
  }
  fl->left_buttons[0].flags |= MWMDecorMenu;
  fl->left_buttons[1].flags |= DecorSticky;
  fl->left_buttons[2].flags |= DecorShaded;
  /* Treat "close" button similar to menu button for Motif hints
     fl->right_buttons[0].flags |= MWMDecorMenu;
   */
  fl->right_buttons[1].flags |= MWMDecorMaximize;
  fl->right_buttons[2].flags |= MWMDecorMinimize;
}

/***********************************************************************
 *
 *  DestroyXfwmDecor -- frees all memory assocated with an XfwmDecor
 *	structure, but does not free the XfwmDecor itself
 * 
 ************************************************************************/
void
DestroyXfwmDecor (XfwmDecor * fl)
{
  int i;
  for (i = 0; i < 3; ++i)
  {
    int j = 0;
    for (; j < MaxButtonState; ++j)
      FreeButtonFace (dpy, &fl->titlebar.state[i]);
  }
  FreeButtonFace (dpy, &fl->BorderStyle.active);
  FreeButtonFace (dpy, &fl->BorderStyle.inactive);
  if (fl->LoReliefGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->LoReliefGC);
    fl->LoReliefGC = (GC) NULL;
  }
  if (fl->LoShadowGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->LoShadowGC);
    fl->LoShadowGC = (GC) NULL;
  }
  if (fl->HiReliefGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->HiReliefGC);
    fl->HiReliefGC = (GC) NULL;
  }
  if (fl->HiShadowGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->HiShadowGC);
    fl->HiShadowGC = (GC) NULL;
  }
  if (fl->HiBackGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->HiBackGC);
    fl->HiBackGC = (GC) NULL;
  }
  if (fl->LoBackGC != (GC) NULL)
  {
    XFreeGC (dpy, fl->LoBackGC);
    fl->LoBackGC = (GC) NULL;
  }
}

/***********************************************************************
 *
 *  InitXfwmDecor -- initializes an XfwmDecor structure to defaults
 * 
 ************************************************************************/
void
InitXfwmDecor (XfwmDecor * fl)
{
  int i;
  ButtonFace tmpbf;

  fl->WindowFont.font = NULL;
  fl->WindowFont.fontset = NULL;
#ifdef HAVE_X11_XFT_XFT_H
  fl->WindowFont.xftfont = NULL;
#endif
  fl->HiReliefGC = (GC) NULL;
  fl->HiShadowGC = (GC) NULL;
  fl->LoReliefGC = (GC) NULL;
  fl->LoShadowGC = (GC) NULL;
  fl->LoBackGC = (GC) NULL;
  fl->LoBackGC = (GC) NULL;

  /* initialize title-bar button styles */
  tmpbf.style = SimpleButton;
  tmpbf.bitmap = None;
  tmpbf.bitmap_pressed = None;
  tmpbf.u.ReliefGC = (GC) NULL;
  tmpbf.u.ShadowGC = (GC) NULL;
  tmpbf.next = NULL;
  for (i = 0; i < 2; ++i)
  {
    int j = 0;
    for (; j < MaxButtonState; ++j)
    {
      fl->left_buttons[i].state[j] = fl->right_buttons[i].state[j] = tmpbf;
    }
  }


  /* initialize title-bar styles */
  fl->titlebar.flags = 0;

  for (i = 0; i < MaxButtonState; ++i)
  {
    fl->titlebar.state[i].style = SimpleButton;
    fl->titlebar.state[i].next = NULL;
    fl->titlebar.state[i].bitmap = None;
    fl->titlebar.state[i].bitmap_pressed = None;
    fl->titlebar.state[i].style = SimpleButton;
    fl->titlebar.state[i].u.ReliefGC = (GC) NULL;
    fl->titlebar.state[i].u.ShadowGC = (GC) NULL;
  }

  /* initialize border texture styles */
  fl->BorderStyle.active.style = SimpleButton;
  fl->BorderStyle.inactive.style = SimpleButton;
  fl->BorderStyle.active.bitmap = None;
  fl->BorderStyle.active.bitmap_pressed = None;
  fl->BorderStyle.inactive.bitmap = None;
  fl->BorderStyle.inactive.bitmap_pressed = None;
  fl->BorderStyle.active.next = NULL;
  fl->BorderStyle.inactive.next = NULL;
  /* reset to default button set */
  ResetAllButtons (fl);
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize xfwm variables
 *
 ************************************************************************/
static void
InitVariables (void)
{
  XfwmContext = XUniqueContext ();
  MenuContext = XUniqueContext ();

  /* initialize some lists */
  Scr.AllBindings = NULL;
  Scr.AllMenus = NULL;
  Scr.TheList = NULL;

  Scr.DefaultIcon = NULL;
  Scr.TransMaskGC = (GC) NULL;
  Scr.DrawGC = (GC) NULL;
  Scr.MenuGC = (GC) NULL;
  Scr.MenuReliefGC = (GC) NULL;
  Scr.MenuShadowGC = (GC) NULL;
  Scr.MenuSelGC = (GC) NULL;
  Scr.MenuSelReliefGC = (GC) NULL;
  Scr.MenuSelShadowGC = (GC) NULL;
  Scr.ScratchGC1 = (GC) NULL;
  Scr.ScratchGC3 = (GC) NULL;
  Scr.BlackGC = (GC) NULL;
  Scr.HintsGC = (GC) NULL;

  /* create graphics contexts */
  CreateGCs ();

  Scr.d_depth = DefaultDepth (dpy, Scr.screen);
  Scr.XfwmRoot.w = Scr.Root;
  Scr.stacklist = NULL;
#ifdef MANAGE_OVERRIDES
  Scr.overrides = NULL;
#endif
  Scr.LastWindowRaised = NULL;
  Scr.LastWindowLowered = NULL;
  Scr.XfwmRoot.next = 0;
  XGetWindowAttributes (dpy, Scr.Root, &(Scr.XfwmRoot.attr));
  Scr.root_pushes = 0;
  Scr.pushed_window = &Scr.XfwmRoot;
  Scr.XfwmRoot.number_cmap_windows = 0;

  Scr.MyDisplayWidth = DisplayWidth (dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight (dpy, Scr.screen);

  Scr.NoBoundaryWidth = 0;
  Scr.BoundaryWidth = BOUNDARY_WIDTH;
  Scr.CornerWidth = CORNER_WIDTH;
  Scr.Hilite = NULL;
  Scr.Focus = NULL;
  Scr.Options = (ClickToFocus | AnimateWin | ForceFocus | MapFocus | HonorWMFocusHint);
  Scr.engine = XFCE_ENGINE;
  Scr.AutoRaiseDelay = 250;
  Scr.iconbox = 0;
  Scr.StdFont.font = NULL;
  Scr.StdFont.fontset = NULL;
#ifdef HAVE_X11_XFT_XFT_H
  Scr.StdFont.xftfont = NULL;
#endif
  Scr.WindowFont.font = NULL;
  Scr.WindowFont.fontset = NULL;
#ifdef HAVE_X11_XFT_XFT_H
  Scr.WindowFont.xftfont = NULL;
#endif
  Scr.IconFont.font = NULL;
  Scr.IconFont.fontset = NULL;
#ifdef HAVE_X11_XFT_XFT_H
  Scr.IconFont.xftfont = NULL;
#endif
  Scr.icongrid = 10;
  Scr.iconspacing = 5;
  Scr.ndesks = 1;
  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;

    Scr.CurrentDesk = 0;
    if ((XGetWindowProperty (dpy, Scr.Root, _XA_WM_DESKTOP, 0L, 1L, True, _XA_WM_DESKTOP, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
    {
      if (prop != NULL)
      {
	Restarting = True;
	Scr.CurrentDesk = *(unsigned long *) prop;
	XFree (prop);
      }
    }
    /* The gnome-compatible properties shall override others -gud */
    if ((XGetWindowProperty (dpy, Scr.Root, _XA_WIN_WORKSPACE, 0L, 1L, False, XA_CARDINAL, &atype, &aformat, &nitems, &bytes_remain, &prop)) == Success)
    {
      if (prop != NULL)
      {
	Scr.CurrentDesk = *(unsigned long *) prop;
	if (Scr.CurrentDesk > NBSCREENS)
	  Scr.CurrentDesk = 1;
	XFree (prop);
      }
    }
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
      else
	Scr.ndesks = NBSCREENS;
    }
    else
      Scr.ndesks = NBSCREENS;
  }

  Scr.EdgeScrollX = Scr.EdgeScrollY = 0;
  Scr.ClickTime = 300;
  Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;

  InitXfwmDecor (&Scr.DefaultDecor);

  Scr.SnapSize = 10;
  Scr.sessionwait = 10;

  /* Not the right place for this, should only be called once somewhere .. */
  InitPictureCMap (dpy, Scr.Root);

  {
    int i;

    for (i = 0; i < 3; i++)
    {
      Scr.Margin[i] = 0;
    }
  }
#if 0
  charset = get_charset_from_lang();
  xfwm_msg (INFO, "Init", "Charset set to \"%s\"", charset);
#endif
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes xfwm border windows
 *
 ************************************************************************/
void
Reborder (void)
{
  XfwmWindow *tmp;		/* temp xfwm window structure */

  /* put a border back around all windows */
  MyXGrabServer (dpy);

  InstallWindowColormaps (&Scr.XfwmRoot);	/* force reinstall */
  for (tmp = Scr.XfwmRoot.next; tmp != NULL; tmp = tmp->next)
  {
    XSelectInput (dpy, tmp->w, NoEventMask);
    RestoreWithdrawnLocation (tmp, True);
    XUnmapWindow (dpy, tmp->frame);
    XDestroyWindow (dpy, tmp->frame);
  }

  MyXUngrabServer (dpy);
  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

/***********************************************************************
 *
 *  Procedure:
 *	Done - cleanup and exit xfwm
 *
 ***********************************************************************
 */
RETSIGTYPE SigDone (int nonsense)
{
  SetRunLevel (XFWM_END);
  SIGNAL_RETURN;
}

void
FreeWindowName (void)
{
  XfwmWindow *t;

  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    free_window_names (t, True, True);
  }
}


void
Done (int restart, char *command)
{
  MenuRoot *mr;

  mr = FindPopup ("ExitFunction");
  if (mr != NULL)
    ExecuteFunction ("Function ExitFunction", NULL, &Event, C_ROOT, -1);

  if ((restart == XFWM_END) || (restart == XFWM_HALT))
  {
    /* LogoutICEConn(); */
    CloseICEConn ();
  }

  /* Close all my pipes */
  ClosePipes ();

  Reborder ();

  /* Cleanup stuff (avoid memory leak) */

  if (ModulePath)
  {
    free (ModulePath);
  }
  if (Scr.TransMaskGC)
  {
    XFreeGC (dpy, Scr.TransMaskGC);
    Scr.TransMaskGC = (GC) NULL;
  }
  if (Scr.DrawGC)
  {
    XFreeGC (dpy, Scr.DrawGC);
    Scr.DrawGC = (GC) NULL;
  }
  if (Scr.MenuGC)
  {
    XFreeGC (dpy, Scr.MenuGC);
    Scr.MenuGC = (GC) NULL;
  }
  if (Scr.MenuReliefGC)
  {
    XFreeGC (dpy, Scr.MenuReliefGC);
    Scr.MenuReliefGC = (GC) NULL;
  }
  if (Scr.MenuShadowGC)
  {
    XFreeGC (dpy, Scr.MenuShadowGC);
    Scr.MenuShadowGC = (GC) NULL;
  }
  if (Scr.MenuSelGC)
  {
    XFreeGC (dpy, Scr.MenuSelGC);
    Scr.MenuSelGC = (GC) NULL;
  }
  if (Scr.MenuSelReliefGC)
  {
    XFreeGC (dpy, Scr.MenuSelReliefGC);
    Scr.MenuSelReliefGC = (GC) NULL;
  }
  if (Scr.MenuSelShadowGC)
  {
    XFreeGC (dpy, Scr.MenuSelShadowGC);
    Scr.MenuSelShadowGC = (GC) NULL;
  }
  if (Scr.ScratchGC1)
  {
    XFreeGC (dpy, Scr.ScratchGC1);
    Scr.ScratchGC1 = (GC) NULL;
  }
  if (Scr.ScratchGC3)
  {
    XFreeGC (dpy, Scr.ScratchGC3);
    Scr.ScratchGC3 = (GC) NULL;
  }
  if (Scr.BlackGC)
  {
    XFreeGC (dpy, Scr.BlackGC);
    Scr.BlackGC = (GC) NULL;
  }
  if (Scr.HintsGC)
  {
    XFreeGC (dpy, Scr.HintsGC);
    Scr.HintsGC = (GC) NULL;
  }
#ifndef EMULATE_XINERAMA
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
  XFree (xinerama_infos);
#endif
#endif

  mr = Scr.AllMenus;
  while (mr)
  {
    DestroyMenu (mr);
    mr = mr->next;
  }

  if (Scr.StdFont.font)
  {
    XFreeFont (dpy, Scr.StdFont.font);
    Scr.StdFont.font = NULL;
  }
  if (Scr.StdFont.fontset)
  {
    XFreeFontSet (dpy, Scr.StdFont.fontset);
    Scr.StdFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (Scr.StdFont.xftfont)
  {
    XftFontClose (dpy, Scr.StdFont.xftfont);
    Scr.StdFont.xftfont = NULL;
  }
#endif
  if (Scr.IconFont.font)
  {
    XFreeFont (dpy, Scr.IconFont.font);
    Scr.IconFont.font = NULL;
  }
  if (Scr.IconFont.fontset)
  {
    XFreeFontSet (dpy, Scr.IconFont.fontset);
    Scr.IconFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (Scr.IconFont.xftfont)
  {
    XftFontClose (dpy, Scr.IconFont.xftfont);
    Scr.IconFont.xftfont = NULL;
  }
#endif
  if (Scr.WindowFont.font)
  {
    XFreeFont (dpy, Scr.WindowFont.font);
    Scr.WindowFont.font = NULL;
  }
  if (Scr.WindowFont.fontset)
  {
    XFreeFontSet (dpy, Scr.WindowFont.fontset);
    Scr.WindowFont.fontset = NULL;
  }
#ifdef HAVE_X11_XFT_XFT_H
  if (Scr.WindowFont.xftfont)
  {
    XftFontClose (dpy, Scr.WindowFont.xftfont);
    Scr.WindowFont.xftfont = NULL;
  }
#endif

  /* Take time to save the current session (if ICE is not available) */

  if (!(restart == XFWM_END) && (Scr.Options & SessionMgt))
    builtin_save_session ();

  /* Free up structure allocated by session management */
  FreeWindowStates ();

  DestroyXfwmDecor (&Scr.DefaultDecor);
  FreeWindowName ();

  FreeStyleList ();

  if (restart == XFWM_RESTART)
  {
    char *my_argv[10];
    int i, done, j;
    SaveDesktopState ();

    /* Really make sure that the connection is closed and cleared! */
    XSelectInput (dpy, Scr.Root, 0);
    XCloseDisplay (dpy);
    i = 0;
    j = 0;
    done = 0;
    while ((g_argv[j] != NULL) && (i < 8))
    {
      if (strcmp (g_argv[j], "-s") != 0)
      {
	my_argv[i] = g_argv[j];
	i++;
	j++;
      }
      else
	j++;
    }
    if (strstr (command, "xfwm") != NULL)
      my_argv[i++] = "-s";
    while (i < 10)
      my_argv[i++] = NULL;

    /* really need to destroy all windows, explicitly,
     * not sleep, but this is adequate for now */
    sleep (1);
    ReapChildren ();
    execvp (command, my_argv);
    xfwm_msg (ERR, "Done", "Call of '%s' failed!!!! (restarting '%s' instead)", command, g_argv[0]);
    execvp (g_argv[0], g_argv);	/* that _should_ work */
    xfwm_msg (ERR, "Done", "Call of '%s' failed!!!!", g_argv[0]);
    /* We definitely should not get there ... */
    XCloseDisplay (dpy);
    exit (0);
  }
  else
  {
    XCloseDisplay (dpy);
    exit (0);
  }
}

static XErrorHandler
CatchRedirectError (Display * dpy, XErrorEvent * event)
{
  fprintf (stderr, "Cannot start xfwm, another window manager is already ");
  fprintf (stderr, "running on screen %s\n", getenv ("DISPLAY"));
  exit (1);
}

/***********************************************************************
 *
 *  Procedure:
 *	CatchFatal - Shuts down if the server connection is lost
 *
 ************************************************************************/
static XIOErrorHandler
CatchFatal (Display * dpy)
{
  /* No action is taken because usually this action is caused by someone
   * using "xlogout" to be able to switch between multiple window managers
   */
  ClosePipes ();
  exit (1);
}

/***********************************************************************
 *
 *  Procedure:
 *	XfwmErrorHandler - displays info on internal errors
 *
 ************************************************************************/
static XErrorHandler
XfwmErrorHandler (Display * dpy, XErrorEvent * event)
{
  char buf[64];

#ifdef DEBUG
  fprintf (stderr, "XfwmErrorHandler (): Entering routine\n");
#else
  /* some errors are acceptable, mostly they're caused by
   * trying to update a lost window */
  if ((event->error_code == BadWindow) || (event->request_code == X_GetGeometry) || (event->error_code == BadDrawable) || (event->request_code == X_SetInputFocus) || (event->request_code == X_GrabButton) || (event->request_code == X_ChangeWindowAttributes) || (event->request_code == X_InstallColormap))
    return 0;
#endif

  XGetErrorText (dpy, event->error_code, buf, 63);
  xfwm_msg (ERR, "XfwmErrorHandler", "*** internal error ***");
  xfwm_msg (ERR, "XfwmErrorHandler", "%s", buf);
  xfwm_msg (ERR, "XfwmErrorHandler", "Request %d, Error %d", event->request_code, event->error_code);
#ifdef DEBUG
  fprintf (stderr, "XfwmErrorHandler (): Leaving routine\n");
#endif
  return 0;
}

static void
usage (void)
{
  fprintf (stderr, "\nXfwm Version %s Usage:\n\n", VERSION);
  fprintf (stderr, "  %s [-d dpy] [-f config_cmd] [-s] [-blackout] [-version] [-h]\n" "  [ -noxfce ] [ -sync ] [ -clientId n] [ -restore ]\n\n", g_argv[0]);
  fprintf (stderr, "  -blackout Blackout screen at startup\n");
  fprintf (stderr, "  -cmd      Specify the command to run as startup\n");
  fprintf (stderr, "  -d        Specify the display to connect\n");
  fprintf (stderr, "  -f        Specify the configuration file to load\n");
  fprintf (stderr, "  -h        Display this text\n");
  fprintf (stderr, "  -noxfce   Do not start xfce panel automatically\n");
  fprintf (stderr, "  -s        Forces xfwm to run on a single screen\n");
  fprintf (stderr, "            (when multi-screen available)\n");
  fprintf (stderr, "  -sync     Run in synchronized mode\n");
#ifdef HAVE_X11_XFT_XFT_H
  fprintf (stderr, "  -noxft    Disable use of Xft even if available\n");
#endif
  fprintf (stderr, "  -version  Dispay xfwm version and exit\n\n");
}

static void
version (void)
{
  fprintf (stderr, "\nXfwm Version %s compiled on %s at %s\n\n", VERSION, __DATE__, __TIME__);
  fprintf (stderr, "  Build options\n");
  fprintf (stderr, "  =============\n");
  fprintf (stderr, "  Xft support........: ");
#ifdef HAVE_X11_XFT_XFT_H
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Xinerama support...: ");
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  XSM support........: ");
#ifdef HAVE_SESSION
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Imlib support......: ");
#ifdef HAVE_IMLIB
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  gdk-pixbuf support.: ");
#ifdef HAVE_GDK_PIXBUF
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Audiofile support..: ");
#ifdef HAVE_AUDIOFILE
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  aRts support.......: ");
#ifdef HAVE_ARTS
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Old style..........: ");
#ifdef OLD_STYLE
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Debugging info.....: ");
#ifdef DEBUG
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "  Stash event time...: ");
#ifdef REQUIRES_STASHEVENT
  fprintf (stderr, "enabled\n");
#else
  fprintf (stderr, "disabled\n");
#endif
  fprintf (stderr, "\n");
}

/****************************************************************************
 *
 * Save Desktop State
 *
 ****************************************************************************/
static void
SaveDesktopState ()
{
  XfwmWindow *t;
  unsigned long data[1];

  for (t = Scr.XfwmRoot.next; t != NULL; t = t->next)
  {
    data[0] = (unsigned long) t->Desk;
    XChangeProperty (dpy, t->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32, PropModeReplace, (unsigned char *) data, 1);
    t->flags |= RECAPTURE;
    data[0] = (unsigned long) (t->flags & (STICKY | ICONIFIED | ICON_MOVED | SHADED | WM_NAME_CHANGED | RECAPTURE));
    XChangeProperty (dpy, t->w, _XA_XFWM_FLAGS, _XA_XFWM_FLAGS, 32, PropModeReplace, (unsigned char *) data, 1);
    data[0] = (unsigned long) t->icon_x_loc;
    XChangeProperty (dpy, t->w, _XA_XFWM_ICONPOS_X, _XA_XFWM_ICONPOS_X, 32, PropModeReplace, (unsigned char *) data, 1);
    data[0] = (unsigned long) t->icon_y_loc;
    XChangeProperty (dpy, t->w, _XA_XFWM_ICONPOS_Y, _XA_XFWM_ICONPOS_Y, 32, PropModeReplace, (unsigned char *) data, 1);
  }

  data[0] = (unsigned long) Scr.CurrentDesk;
  XChangeProperty (dpy, Scr.Root, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32, PropModeReplace, (unsigned char *) data, 1);
}


static void
SetMWM_INFO (Window window)
{
  struct mwminfo
  {
    long flags;
    Window win;
  }
  motif_wm_info;

  motif_wm_info.flags = 2;
  motif_wm_info.win = window;

  XChangeProperty (dpy, Scr.Root, _XA_MOTIF_WM, _XA_MOTIF_WM, 32, PropModeReplace, (unsigned char *) &motif_wm_info, 2);
}

void
BlackoutScreen ()
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  if (Blackout && (BlackoutWin == None))
  {
    /* blackout screen */
    attributes.border_pixel = BlackPixel (dpy, Scr.screen);
    attributes.background_pixel = BlackPixel (dpy, Scr.screen);
    attributes.bit_gravity = NorthWestGravity;
    attributes.override_redirect = True;	/* is override redirect needed? */
    valuemask = CWBorderPixel | CWBackPixel | CWBitGravity | CWOverrideRedirect;
    BlackoutWin = XCreateWindow (dpy, Scr.Root, 0, 0, DisplayWidth (dpy, Scr.screen), DisplayHeight (dpy, Scr.screen), 0, CopyFromParent, CopyFromParent, CopyFromParent, valuemask, &attributes);
    XMapWindow (dpy, BlackoutWin);
  }
}				/* BlackoutScreen */

void
UnBlackoutScreen ()
{
  if (Blackout && (BlackoutWin != None))
  {
    XDestroyWindow (dpy, BlackoutWin);	/* unblacken the screen */
    BlackoutWin = None;
  }
}				/* UnBlackoutScreen */

void
SetRunLevel (int level)
{
  runlevel = level;
}
