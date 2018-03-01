/****************************************************************************
 * This module is all new
 * by Rob Nation 
 ****************************************************************************/
/***********************************************************************
 *
 * xfwm pager handling code
 *
 ***********************************************************************/

#include "configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "utils.h"
#include "module.h"
#include "../xfwm/xfwm.h"
#include "XfwmPager.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void HandleExpose (XEvent * Event);

extern ScreenInfo Scr;
extern Display *dpy;

Pixel back_pix, fore_pix, hi_pix;
Pixel focus_pix;
Pixel focus_fore_pix;
extern Pixel win_back_pix, win_fore_pix, win_hi_back_pix, win_hi_fore_pix;
extern int window_w, window_h, window_x, window_y, usposition, uselabel, xneg, yneg;
extern int StartIconic;
extern int icon_w, icon_h, icon_x, icon_y;
XFontStruct *font, *windowFont;

GC NormalGC, DashedGC, HiliteGC, rvGC;
GC StdGC;
GC MiniIconGC;

extern PagerWindow *Start;
extern PagerWindow *FocusWin;
static Atom wm_del_win;

extern char *MyName;

extern int desk2, ndesks;
extern int Rows, Columns;
extern int fd[2];

int desk_w = 0;
int desk_h = 0;
int label_h = 0;

DeskInfo *Desks;
int Wait = 0;
XErrorHandler XfwmErrorHandler (Display *, XErrorEvent *);


/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = { 0x02, 0x01 };

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = { 0x08, 0x02 };

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] = { 0x01, 0x02, 0x04, 0x08 };


Window icon_win;		/* icon window */

/***********************************************************************
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 ***********************************************************************/
char *pager_name = "Xfwm Pager";
XSizeHints sizehints = {
  (PSize | PMinSize),
  0, 0, 100, 100,		/* x, y, width and height */
  0, 0,				/* Min width and height */
  0, 0,				/* Max width and height */
  1, 1,				/* Width and height increments */
  {1, 1}, {1, 1},		/* Aspect ratio - not used */
  0, 0,				/* base size */
  (NorthWestGravity)		/* gravity */
};

void
initialize_pager (void)
{
  XWMHints wmhints;
  XClassHint class1;

  XTextProperty name;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  extern char *PagerFore, *PagerBack, *HilightC;
  extern char *WindowBack, *WindowFore, *WindowHiBack, *WindowHiFore;
  extern char *font_string, *smallFont;
  int w, h, i, x, y;
  XGCValues gcv;
  unsigned long gcm;

  wm_del_win = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  
  /* load the font */
  if (!uselabel || ((font = XLoadQueryFont (dpy, font_string)) == NULL))
  {
    if ((font = XLoadQueryFont (dpy, "fixed")) == NULL)
    {
      fprintf (stderr, "%s: No fonts available\n", MyName);
      exit (1);
    }
  };
  if (uselabel)
    label_h = font->ascent + font->descent + 2;
  else
    label_h = 0;


  if (smallFont != NULL)
  {
    windowFont = XLoadQueryFont (dpy, smallFont);
  }
  else
    windowFont = NULL;

  /* Load the colors */
  fore_pix = GetColor (PagerFore);
  back_pix = GetColor (PagerBack);
  hi_pix = GetColor (HilightC);

  if (WindowBack && WindowFore && WindowHiBack && WindowHiFore)
  {
    win_back_pix = GetColor (WindowBack);
    win_fore_pix = GetColor (WindowFore);
    win_hi_back_pix = GetColor (WindowHiBack);
    win_hi_fore_pix = GetColor (WindowHiFore);
  }
  /* Load pixmaps for mono use */
  if (Scr.d_depth < 2)
  {
    Scr.gray_pixmap = XCreatePixmapFromBitmapData (dpy, Scr.Root, g_bits, g_width, g_height, fore_pix, back_pix, Scr.d_depth);
    Scr.light_gray_pixmap = XCreatePixmapFromBitmapData (dpy, Scr.Root, l_g_bits, l_g_width, l_g_height, fore_pix, back_pix, Scr.d_depth);
    Scr.sticky_gray_pixmap = XCreatePixmapFromBitmapData (dpy, Scr.Root, s_g_bits, s_g_width, s_g_height, fore_pix, back_pix, Scr.d_depth);
  }


  /* Size the window */
  if (Rows < 0)
  {
    if (Columns < 0)
    {
      Columns = ndesks;
      Rows = 1;
    }
    else
    {
      Rows = ndesks / Columns;
      if (Rows * Columns < ndesks)
	Rows++;
    }
  }
  if (Columns < 0)
  {
    Columns = ndesks / Rows;
    if (Rows * Columns < ndesks)
      Columns++;
  }

  if (Rows * Columns < ndesks)
    Rows = ndesks / Columns;
  if (window_w > 0)
  {
    Scr.VScale = Columns * Scr.MyDisplayWidth / (window_w - Columns + 1);
  }
  if (window_h > 0)
  {
    Scr.VScale = Rows * Scr.MyDisplayHeight / (window_h + 2 - Rows * (label_h - 1));
  }
  if (window_w < 30 * ((desk2 + 1) / Rows))
  {
    window_w = 30 * ((desk2 + 1) / Rows);
  }
  if (window_h < 30 * (Rows))
  {
    window_h = 30 * (Rows);
  }
  if (xneg)
  {
    window_x += Scr.MyDisplayWidth - window_w;
  }

  if (yneg)
  {
    window_y += Scr.MyDisplayHeight - window_h;
  }

  if (usposition)
    sizehints.flags = USPosition | PWinGravity;

  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
  attributes.background_pixel = back_pix;
  attributes.border_pixel = fore_pix;
  attributes.event_mask = (StructureNotifyMask);
  sizehints.width = sizehints.base_width = window_w;
  sizehints.height = sizehints.base_height = window_h;
  sizehints.x = window_x;
  sizehints.y = window_y;
  sizehints.min_width = 30 * ((desk2 + 1) / Rows);
  sizehints.min_height = 30 * (Rows);
  if ((xneg) && (!yneg))
    sizehints.win_gravity = NorthEastGravity;
  else if ((xneg) && (yneg))
    sizehints.win_gravity = SouthEastGravity;
  else if ((!xneg) && (yneg))
    sizehints.win_gravity = SouthWestGravity;
  else
    sizehints.win_gravity = NorthWestGravity;

  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, window_w, window_h, (unsigned int) 1, CopyFromParent, InputOutput, (Visual *) CopyFromParent, valuemask, &attributes);
  XSetWMNormalHints (dpy, Scr.Pager_w, &sizehints);
  XSetWMProtocols (dpy, Scr.Pager_w, &wm_del_win, 1);

  if ((desk2 == 0) && (Desks[0].label != NULL))
    XStringListToTextProperty (&Desks[0].label, 1, &name);
  else
    XStringListToTextProperty (&pager_name, 1, &name);

  attributes.event_mask = (StructureNotifyMask | ExposureMask);

  if (icon_w < 30)
    icon_w = 30;
  if (icon_h < 30)
    icon_h = 30;

  icon_win = XCreateWindow (dpy, Scr.Root, window_x, window_y, icon_w, icon_h, (unsigned int) 1, CopyFromParent, InputOutput, (Visual *) CopyFromParent, valuemask, &attributes);
  XGrabButton (dpy, 1, AnyModifier, icon_win, True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton (dpy, 2, AnyModifier, icon_win, True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton (dpy, 3, AnyModifier, icon_win, True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  if (!StartIconic)
    wmhints.initial_state = NormalState;
  else
    wmhints.initial_state = IconicState;
  wmhints.icon_window = icon_win;
  wmhints.input = False;

  wmhints.flags = InputHint | StateHint | IconWindowHint;

  class1.res_name = MyName;
  class1.res_class = "XfwmModule";

  XSetWMProperties (dpy, Scr.Pager_w, &name, &name, NULL, 0, &sizehints, &wmhints, &class1);
  XFree ((char *) name.value);

  for (i = 0; i < ndesks; i++)
  {
    w = window_w / ndesks;
    h = window_h;
    x = w * i;
    y = 0;

    valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
    attributes.background_pixel = GetColor (Desks[i].Dcolor);
    attributes.border_pixel = fore_pix;
    attributes.event_mask = (ExposureMask | ButtonReleaseMask);
    Desks[i].title_w = XCreateWindow (dpy, Scr.Pager_w, x, y, w, h, 1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    attributes.event_mask = (ExposureMask | ButtonReleaseMask | ButtonPressMask | ButtonMotionMask);
    desk_h = window_h - label_h;
    Desks[i].w = XCreateWindow (dpy, Desks[i].title_w, x, y, w, desk_h, 1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);


    attributes.event_mask = 0;
    attributes.background_pixel = hi_pix;

    w = window_w;
    h = window_h - label_h;
    Desks[i].CPagerWin = XCreateWindow (dpy, Desks[i].w, -1000, -1000, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    XMapRaised (dpy, Desks[i].CPagerWin);
    XMapRaised (dpy, Desks[i].w);
    XMapRaised (dpy, Desks[i].title_w);

  }
  XMapRaised (dpy, Scr.Pager_w);

  gcm = GCForeground | GCBackground | GCFont;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;

  gcv.font = font->fid;
  NormalGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  MiniIconGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = hi_pix;
  if (Scr.d_depth < 2)
  {
    gcv.foreground = fore_pix;
    gcv.background = back_pix;
  }
  HiliteGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

  if ((Scr.d_depth < 2) || (fore_pix == hi_pix))
    gcv.foreground = back_pix;
  else
    gcv.foreground = fore_pix;
  rvGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

  if (windowFont != NULL)
  {
    /* Create GC's for doing window labels */
    gcv.foreground = focus_fore_pix;
    gcv.background = focus_pix;
    gcv.font = windowFont->fid;
    StdGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  }

  gcm = gcm | GCLineStyle;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.line_style = LineOnOffDash;
  DashedGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
}



/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/
Pixel GetColor (char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes (dpy, Scr.Root, &attributes);
  color.pixel = 0;
  if (!XParseColor (dpy, attributes.colormap, name, &color))
  {
    nocolor ("parse", name);
  }
  else if (!XAllocColor (dpy, attributes.colormap, &color))
  {
    nocolor ("alloc", name);
  }
  return color.pixel;
}


void
nocolor (char *a, char *b)
{
  fprintf (stderr, "%s: can't %s %s\n", MyName, a, b);
}

/****************************************************************************
 *
 * Decide what to do about received X events
 *
 ****************************************************************************/
void
DispatchEvent (XEvent * Event)
{
  int i, x, y;
  Window JunkRoot, JunkChild;
  int JunkX, JunkY;
  unsigned JunkMask;

  switch (Event->xany.type)
  {
  case ConfigureNotify:
    ReConfigure ();
    break;
  case Expose:
    HandleExpose (Event);
    break;
  case ButtonRelease:
    if (Event->xbutton.button == 1)
    {
      for (i = 0; i < ndesks; i++)
      {
	if (Event->xany.window == Desks[i].w)
	  SwitchToDeskAndPage (i, Event);
	if (Event->xany.window == Desks[i].title_w)
	  SwitchToDesk (i);
      }
      if (Event->xany.window == icon_win)
      {
	IconSwitchPage (Event);
      }
    }
    break;
  case ButtonPress:
    if (((Event->xbutton.button == 2) || (Event->xbutton.button == 3)) && (Event->xbutton.subwindow != None))
    {
      MoveWindow (Event);
    }
    break;
  case MotionNotify:
    while (XCheckMaskEvent (dpy, PointerMotionMask | ButtonMotionMask, Event));

    if (Event->xmotion.state == Button3MotionMask)
    {
      for (i = 0; i < ndesks; i++)
      {
	if (Event->xany.window == Desks[i].w)
	{
	  XQueryPointer (dpy, Desks[i].w, &JunkRoot, &JunkChild, &JunkX, &JunkY, &x, &y, &JunkMask);
	  Scroll (i, x, y);
	}
      }
      if (Event->xany.window == icon_win)
      {
	XQueryPointer (dpy, icon_win, &JunkRoot, &JunkChild, &JunkX, &JunkY, &x, &y, &JunkMask);
	IconScroll (x, y);
      }

    }
    break;

  case ClientMessage:
    if ((Event->xclient.format == 32) && (Event->xclient.data.l[0] == wm_del_win))
    {
      exit (0);
    }
    break;
  }
}

void
DrawGrid (int i, int erase)
{
  int y1, y2, x, d, hor_off, w;
  int MaxW, MaxH;
  char str[15], *ptr;

  if ((i < 0) || (i >= ndesks))
    return;

  MaxW = Scr.MyDisplayWidth;
  MaxH = Scr.MyDisplayHeight;

  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = desk_h;

  if ((Scr.CurrentDesk) == i)
  {
    if (uselabel)
      XFillRectangle (dpy, Desks[i].title_w, HiliteGC, 0, 0, desk_w, label_h - 1);
  }
  else
  {
    if (uselabel && erase)
      XClearArea (dpy, Desks[i].title_w, 0, 0, desk_w, label_h - 1, False);
  }

  d = i;
  ptr = Desks[i].label;
  w = XTextWidth (font, ptr, strlen (ptr));
  if (w > desk_w)
  {
    sprintf (str, "%d", d);
    ptr = str;
    w = XTextWidth (font, ptr, strlen (ptr));
  }
  if ((w <= desk_w) && (uselabel))
  {
    hor_off = (desk_w - w) / 2;
    if (i == (Scr.CurrentDesk))
      XDrawString (dpy, Desks[i].title_w, rvGC, hor_off, font->ascent + 1, ptr, strlen (ptr));
    else
      XDrawString (dpy, Desks[i].title_w, NormalGC, hor_off, font->ascent + 1, ptr, strlen (ptr));
  }
}

void
HandleExpose (XEvent * Event)
{
  PagerWindow *t;
  int i;

  for (i = 0; i < ndesks; i++)
  {
    if ((Event->xany.window == Desks[i].w) || (Event->xany.window == Desks[i].title_w))
      DrawGrid (i, 0);
  }

  t = Start;
  while (t != NULL)
  {
    if (t->PagerView == Event->xany.window)
    {
      LabelWindow (t);
    }
    else if (t->IconView == Event->xany.window)
    {
      LabelIconWindow (t);
    }

    t = t->next;
  }
}

/****************************************************************************
 *
 * Respond to a change in window geometry.
 *
 ****************************************************************************/
void
ReConfigure (void)
{
  Window root;
  unsigned border_width, depth;
  int w, h, x, y, i, j, k;


  XGetGeometry (dpy, Scr.Pager_w, &root, &x, &y, (unsigned *) &window_w, (unsigned *) &window_h, &border_width, &depth);


  desk_w = (window_w - Columns + 1) / Columns;
  desk_h = (window_h - Rows * label_h - Rows + 2) / Rows;
  w = desk_w;
  h = desk_h;

  XSetWMNormalHints (dpy, Scr.Pager_w, &sizehints);

  x = (desk_w) / (Scr.MyDisplayWidth);
  y = (desk_h) / (Scr.MyDisplayHeight);

  for (k = 0; k < Rows; k++)
  {
    for (j = 0; j < Columns; j++)
    {
      i = j * Rows + k;

      if (i < ndesks)
      {
	XMoveResizeWindow (dpy, Desks[i].title_w, (desk_w + 1) * j - 1, (desk_h + label_h + 1) * k - 1, desk_w, window_h);
	XMoveResizeWindow (dpy, Desks[i].w, -1, label_h - 1, desk_w, desk_h);
	if (i == Scr.CurrentDesk)
	  XMoveResizeWindow (dpy, Desks[i].CPagerWin, x, y, w, h);
	else
	  XMoveResizeWindow (dpy, Desks[i].CPagerWin, -1000, -100, w, h);

	XClearArea (dpy, Desks[i].w, 0, 0, desk_w, desk_h, True);
	if (uselabel)
	  XClearArea (dpy, Desks[i].title_w, 0, 0, desk_w, label_h, True);
      }
    }
  }
  /* reconfigure all the subordinate windows */
  ReConfigureAll ();
}

void
MovePage (void)
{
  int x, y, i;
  XTextProperty name;
  char str[100], *sptr;
  static int icon_desk_shown = -1000;

  Wait = 0;
  x = (desk_w) / (Scr.MyDisplayWidth);
  y = (desk_h) / (Scr.MyDisplayHeight);
  for (i = 0; i < ndesks; i++)
  {
    if (i == Scr.CurrentDesk)
    {
      XMoveWindow (dpy, Desks[i].CPagerWin, x, y);
      XLowerWindow (dpy, Desks[i].CPagerWin);
    }
    else
      XMoveWindow (dpy, Desks[i].CPagerWin, -1000, -1000);
  }

  ReConfigureIcons ();

  if (Scr.CurrentDesk != icon_desk_shown)
  {
    icon_desk_shown = Scr.CurrentDesk;

    if ((Scr.CurrentDesk >= 0) && (Scr.CurrentDesk <= desk2))
      sptr = Desks[Scr.CurrentDesk].label;
    else
    {
      sprintf (str, "Desk %d", Scr.CurrentDesk);
      sptr = &str[0];
    }
    if (XStringListToTextProperty (&sptr, 1, &name) == 0)
    {
      fprintf (stderr, "%s: cannot allocate window name", MyName);
      return;
    }
    XSetWMIconName (dpy, Scr.Pager_w, &name);
  }
}

void
ReConfigureAll (void)
{
  PagerWindow *t;

  t = Start;
  while (t != NULL)
  {
    MoveResizePagerView (t);
    t = t->next;
  }
}

void
ReConfigureIcons (void)
{
  PagerWindow *t;
  int x, y, w, h, n1, m1;

  t = Start;
  while (t != NULL)
  {
    n1 = (t->x) / Scr.MyDisplayWidth;
    m1 = (t->y) / Scr.MyDisplayHeight;
    x = (t->x) * (icon_w) / (Scr.MyDisplayWidth);
    y = (t->y) * (icon_h) / (Scr.MyDisplayHeight);
    w = (t->x + t->width + 2) * (icon_w) / (Scr.MyDisplayWidth) - 2 - x;
    h = (t->y + t->height + 2) * (icon_h) / (Scr.MyDisplayHeight) - 2 - y;

    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;

    t->icon_view_width = w;
    t->icon_view_height = h;
    if (Scr.CurrentDesk == t->desk)
      XMoveResizeWindow (dpy, t->IconView, x, y, w, h);
    else
      XMoveResizeWindow (dpy, t->IconView, -1000, -1000, w, h);
    t = t->next;
  }
}

void
SwitchToDesk (int Desk)
{
  char command[256];

  sprintf (command, "Desk 0 %d\n", Desk);

  SendInfo (fd, command, 0);
}


void
SwitchToDeskAndPage (int Desk, XEvent * Event)
{
  char command[256];

  if (Scr.CurrentDesk != (Desk))
  {
    sprintf (command, "Desk 0 %d\n", Desk);
    SendInfo (fd, command, 0);

  }
  Wait = 1;
}

void
IconSwitchPage (XEvent * Event)
{
}


void
AddNewWindow (PagerWindow * t)
{
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int i, x, y, w, h, n1, m1;

  i = t->desk;
  n1 = (t->x) / Scr.MyDisplayWidth;
  m1 = (t->y) / Scr.MyDisplayHeight;
  x = (t->x) * (desk_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (desk_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (desk_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (desk_h) / (Scr.MyDisplayHeight) - 2 - y;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  t->pager_view_width = w;
  t->pager_view_height = h;
  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
  attributes.background_pixel = t->back;
  attributes.border_pixel = fore_pix;
  attributes.event_mask = (ExposureMask);

  if ((i >= 0) && (i < ndesks))
  {
    t->PagerView = XCreateWindow (dpy, Desks[i].w, x, y, w, h, 1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);

    XMapRaised (dpy, t->PagerView);
  }
  else
    t->PagerView = None;


  x = (t->x) * (icon_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (icon_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (icon_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (icon_h) / (Scr.MyDisplayHeight) - 2 - y;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  t->icon_view_width = w;
  t->icon_view_height = h;
  if (Scr.CurrentDesk == t->desk)
  {
    t->IconView = XCreateWindow (dpy, icon_win, x, y, w, h, 1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    XGrabButton (dpy, 2, AnyModifier, t->IconView, True, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XMapRaised (dpy, t->IconView);
  }
  else
  {
    t->IconView = XCreateWindow (dpy, icon_win, -1000, -1000, w, h, 1, CopyFromParent, InputOutput, CopyFromParent, valuemask, &attributes);
    XMapRaised (dpy, t->IconView);
  }
  Hilight (t, OFF);
}



void
ChangeDeskForWindow (PagerWindow * t, long newdesk)
{
  int i, x, y, w, h, n1, m1;

  i = newdesk;

  if (t->PagerView == None)
  {
    t->desk = newdesk;
    XDestroyWindow (dpy, t->IconView);
    AddNewWindow (t);
    return;
  }

  n1 = (t->x) / Scr.MyDisplayWidth;
  m1 = (t->y) / Scr.MyDisplayHeight;
  x = (t->x) * (desk_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (desk_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (desk_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (desk_h) / (Scr.MyDisplayHeight) - 2 - y;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  t->pager_view_width = w;
  t->pager_view_height = h;
  if ((i >= 0) && (i < ndesks))
  {
    XReparentWindow (dpy, t->PagerView, Desks[i].w, x, y);
    XResizeWindow (dpy, t->PagerView, w, h);
  }
  else
  {
    XDestroyWindow (dpy, t->PagerView);
    t->PagerView = None;
  }
  t->desk = i;

  x = (t->x) * (icon_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (icon_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (icon_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (icon_h) / (Scr.MyDisplayHeight) - 2 - y;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  t->icon_view_width = w;
  t->icon_view_height = h;
  if (Scr.CurrentDesk == t->desk)
    XMoveResizeWindow (dpy, t->IconView, x, y, w, h);
  else
    XMoveResizeWindow (dpy, t->IconView, -1000, -1000, w, h);
}

void
MoveResizePagerView (PagerWindow * t)
{
  int x, y, w, h, n1, m1;

  n1 = (t->x) / Scr.MyDisplayWidth;
  m1 = (t->y) / Scr.MyDisplayHeight;
  x = (t->x) * (desk_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (desk_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (desk_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (desk_h) / (Scr.MyDisplayHeight) - 2 - y;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  t->pager_view_width = w;
  t->pager_view_height = h;
  if (t->PagerView != None)
    XMoveResizeWindow (dpy, t->PagerView, x, y, w, h);
  else if ((t->desk >= 0) && (t->desk <= desk2))
  {
    XDestroyWindow (dpy, t->IconView);
    AddNewWindow (t);
    return;
  }

  x = (t->x) * (icon_w) / (Scr.MyDisplayWidth);
  y = (t->y) * (icon_h) / (Scr.MyDisplayHeight);
  w = (t->x + t->width + 2) * (icon_w) / (Scr.MyDisplayWidth) - 2 - x;
  h = (t->y + t->height + 2) * (icon_h) / (Scr.MyDisplayHeight) - 2 - y;

  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  t->icon_view_width = w;
  t->icon_view_height = h;
  if (Scr.CurrentDesk == t->desk)
    XMoveResizeWindow (dpy, t->IconView, x, y, w, h);
  else
    XMoveResizeWindow (dpy, t->IconView, -1000, -1000, w, h);
}


void
MoveStickyWindows (void)
{
  PagerWindow *t;

  t = Start;
  while (t != NULL)
  {
    if (((t->flags & ICONIFIED) && (t->flags & StickyIcon)) || (t->flags & STICKY))
    {
      if (t->desk != Scr.CurrentDesk)
      {
	ChangeDeskForWindow (t, Scr.CurrentDesk);
      }
      else
      {
	MoveResizePagerView (t);

      }
    }
    t = t->next;
  }
}

void
Hilight (PagerWindow * t, int on)
{

  if (!t)
    return;

  if (Scr.d_depth < 2)
  {
    if (on)
    {
      if (t->PagerView != None)
	XSetWindowBackgroundPixmap (dpy, t->PagerView, Scr.gray_pixmap);
      XSetWindowBackgroundPixmap (dpy, t->IconView, Scr.gray_pixmap);
    }
    else
    {
      if (t->flags & STICKY)
      {
	if (t->PagerView != None)
	  XSetWindowBackgroundPixmap (dpy, t->PagerView, Scr.sticky_gray_pixmap);
	XSetWindowBackgroundPixmap (dpy, t->IconView, Scr.sticky_gray_pixmap);
      }
      else
      {
	if (t->PagerView != None)
	  XSetWindowBackgroundPixmap (dpy, t->PagerView, Scr.light_gray_pixmap);
	XSetWindowBackgroundPixmap (dpy, t->IconView, Scr.light_gray_pixmap);
      }
    }
  }
  else
  {
    if (on)
    {
      if (t->PagerView != None)
	XSetWindowBackground (dpy, t->PagerView, focus_pix);
      XSetWindowBackground (dpy, t->IconView, focus_pix);
    }
    else
    {
      if (t->PagerView != None)
	XSetWindowBackground (dpy, t->PagerView, t->back);
      XSetWindowBackground (dpy, t->IconView, t->back);
    }
  }
  if (t->PagerView != None)
    XClearWindow (dpy, t->PagerView);
  XClearWindow (dpy, t->IconView);
  LabelWindow (t);
  LabelIconWindow (t);
}

void
Scroll (int Desk, int x, int y)
{
  char command[256];
  int sx, sy;
  if (Wait == 0)
  {
    if (Desk != Scr.CurrentDesk)
    {
      return;
    }

    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;

    if (x > desk_w)
      x = desk_w;
    if (y > desk_h)
      y = desk_h;

    sx = (100 * (x * (Scr.MyDisplayWidth) / desk_w)) / Scr.MyDisplayWidth;
    sy = (100 * (y * (Scr.MyDisplayHeight) / desk_h)) / Scr.MyDisplayHeight;
    if (sx > 100)
      sx = 100;
    if (sx < -100)
      sx = -100;
    if (sy < -100)
      sy = -100;
    if (sy > 100)
      sy = 100;

    sprintf (command, "Scroll %d %d\n", sx, sy);
    SendInfo (fd, command, 0);
    Wait = 1;
  }
}


void
IconScroll (int x, int y)
{
  char command[256];
  int sx, sy;
  if (Wait == 0)
  {
    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;

    if (x > icon_w)
      x = icon_w;
    if (y > icon_h)
      y = icon_h;

    sx = (100 * (x * (Scr.MyDisplayWidth) / icon_w)) / Scr.MyDisplayWidth;
    sy = (100 * (y * (Scr.MyDisplayHeight) / icon_h)) / Scr.MyDisplayHeight;
    if (sx > 100)
      sx = 100;
    if (sx < -100)
      sx = -100;
    if (sy < -100)
      sy = -100;
    if (sy > 100)
      sy = 100;

    sprintf (command, "Scroll %d %d\n", sx, sy);
    SendInfo (fd, command, 0);
    Wait = 1;
  }
}


void
MoveWindow (XEvent * Event)
{
  char command[100];
  int x1, y1, finished = 0, wx, wy, x, y, xi = 0, yi = 0, wx1, wy1, x2, y2;
  Window dumwin;
  PagerWindow *t;
  int n1, m1;
  int NewDesk, KeepMoving = 0;
  int moved = 0;
  int row, column;

  t = Start;
  while ((t != NULL) && (t->PagerView != Event->xbutton.subwindow))
    t = t->next;

  if (t == NULL)
  {
    t = Start;
    while ((t != NULL) && (t->IconView != Event->xbutton.subwindow))
      t = t->next;
    if (t != NULL)
    {
      IconMoveWindow (Event, t);
      return;
    }
  }

  if (t == NULL)
    return;

  NewDesk = t->desk;
  if ((NewDesk < 0) || (NewDesk >= ndesks))
    return;

  n1 = (t->x) / Scr.MyDisplayWidth;
  m1 = (t->y) / Scr.MyDisplayHeight;
  wx = (t->x) * (desk_w) / (Scr.MyDisplayWidth);
  wy = (t->y) * (desk_h) / (Scr.MyDisplayHeight);
  wx1 = wx + (desk_w + 1) * (NewDesk / Rows);
  wy1 = wy + label_h + (desk_h + label_h + 1) * (NewDesk % Rows);

  XReparentWindow (dpy, t->PagerView, Scr.Pager_w, wx1, wy1);
  XRaiseWindow (dpy, t->PagerView);

  XTranslateCoordinates (dpy, Event->xany.window, t->PagerView, Event->xbutton.x, Event->xbutton.y, &x1, &y1, &dumwin);
  xi = x1;
  yi = y1;
  while (!finished)
  {
    XMaskEvent (dpy, ButtonReleaseMask | ButtonMotionMask | ExposureMask, Event);

    if (Event->type == MotionNotify)
    {
      XTranslateCoordinates (dpy, Event->xany.window, Scr.Pager_w, Event->xmotion.x, Event->xmotion.y, &x, &y, &dumwin);
      if (moved == 0)
      {
	xi = x;
	yi = y;
	moved = 1;
      }
      if ((x < -5) || (y < -5) || (x > window_w + 5) || (y > window_h + 5))
      {
	KeepMoving = 1;
	finished = 1;
      }
      XMoveWindow (dpy, t->PagerView, x - (x1), y - (y1));
    }
    else if (Event->type == ButtonRelease)
    {
      XTranslateCoordinates (dpy, Event->xany.window, Scr.Pager_w, Event->xbutton.x, Event->xbutton.y, &x, &y, &dumwin);
      XMoveWindow (dpy, t->PagerView, x - (x1), y - (y1));
      finished = 1;
    }
    else if (Event->type == Expose)
    {
      HandleExpose (Event);
    }
  }

  if (moved)
  {
    if ((x - xi < 3) && (y - yi < 3) && (x - xi > -3) && (y - yi > -3))
      moved = 0;
  }
  if (KeepMoving)
  {
    NewDesk = Scr.CurrentDesk;
    if (NewDesk != t->desk)
    {
      XMoveWindow (dpy, t->w, Scr.MyDisplayWidth, Scr.MyDisplayHeight);
      XSync (dpy, 0);
      sprintf (command, "WindowsDesk %d", NewDesk);
      SendInfo (fd, command, t->w);
      t->desk = NewDesk;
    }
    if ((NewDesk >= 0) && (NewDesk <= desk2))
      XReparentWindow (dpy, t->PagerView, Desks[NewDesk].w, 0, 0);
    else
    {
      XDestroyWindow (dpy, t->PagerView);
      t->PagerView = None;
    }
    XTranslateCoordinates (dpy, Scr.Pager_w, Scr.Root, x, y, &x1, &y1, &dumwin);
    if (t->flags & ICONIFIED)
    {
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      SendInfo (fd, "Move", t->icon_w);
    }
    else
    {
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      SendInfo (fd, "Move", t->w);
    }
    return;
  }
  else
  {
    column = (x / (desk_w + 1));
    row = (y / (desk_h + label_h + 1));
    NewDesk = row + (column) * Rows;
    if ((NewDesk < 0) || (NewDesk >= ndesks))
    {
      NewDesk = Scr.CurrentDesk;
      x = xi;
      y = yi;
      moved = 0;
    }
    XTranslateCoordinates (dpy, Scr.Pager_w, Desks[NewDesk].w, x - x1, y - y1, &x2, &y2, &dumwin);

    n1 = x2 * (Scr.MyDisplayWidth) / (desk_w * Scr.MyDisplayWidth);
    m1 = y2 * (Scr.MyDisplayHeight) / (desk_h * Scr.MyDisplayHeight);
    x = x2 * (Scr.MyDisplayWidth) / (desk_w);
    y = y2 * (Scr.MyDisplayHeight) / (desk_h);
    if (x > Scr.MyDisplayWidth)
      x = Scr.MyDisplayWidth - t->frame_width;
    if (y > Scr.MyDisplayHeight)
      y = Scr.MyDisplayHeight - t->frame_height;
    if (((t->flags & ICONIFIED) && (t->flags & StickyIcon)) || (t->flags & STICKY))
    {
      NewDesk = Scr.CurrentDesk;
      if (x > Scr.MyDisplayWidth - 16)
	x = Scr.MyDisplayWidth - 16;
      if (y > Scr.MyDisplayHeight - 16)
	y = Scr.MyDisplayHeight - 16;
      if (x + t->width < 16)
	x = 16 - t->width;
      if (y + t->height < 16)
	y = 16 - t->height;
    }
    if (NewDesk != t->desk)
    {
      if (((t->flags & ICONIFIED) && (t->flags & StickyIcon)) || (t->flags & STICKY))
      {
	NewDesk = Scr.CurrentDesk;
	if (t->desk != Scr.CurrentDesk)
	  ChangeDeskForWindow (t, Scr.CurrentDesk);
      }
      else
      {
	sprintf (command, "WindowsDesk %d", NewDesk);
	SendInfo (fd, command, t->w);
	t->desk = NewDesk;
      }
    }

    if ((NewDesk >= 0) && (NewDesk < ndesks))
    {
      XReparentWindow (dpy, t->PagerView, Desks[NewDesk].w, x, y);
      if (moved)
      {
	if (t->flags & ICONIFIED)
	  XMoveWindow (dpy, t->icon_w, x, y);
	else
	  XMoveWindow (dpy, t->w, x + t->border_width, y + t->title_height + t->border_width);
	XSync (dpy, 0);
      }
      else
	MoveResizePagerView (t);
      SendInfo (fd, "Raise", t->w);
    }
    if (Scr.CurrentDesk == t->desk)
    {
      XSync (dpy, 0);
      sleep_a_little (5000);
      XSync (dpy, 0);
      if (t->flags & ICONIFIED)
	XSetInputFocus (dpy, t->icon_w, RevertToParent, Event->xbutton.time);
      else if (!t->flags & SHADED)
	XSetInputFocus (dpy, t->w, RevertToParent, Event->xbutton.time);
    }
  }
}






/***********************************************************************
 *
 *  Procedure:
 *      XfwmErrorHandler - displays info on internal errors
 *
 ************************************************************************/
XErrorHandler XfwmErrorHandler (Display * dpy, XErrorEvent * event)
{
  fprintf (stderr, "%s: XError!  Bagging out!\n", MyName);
  exit (0);
}


void
LabelWindow (PagerWindow * t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;

  if (windowFont == NULL)
  {
    return;
  }
  if (t->icon_name == NULL)
  {
    return;
  }
  if (t == FocusWin)
  {
    Globalgcv.foreground = focus_fore_pix;
    Globalgcv.background = focus_pix;
    Globalgcm = GCForeground | GCBackground;
    XChangeGC (dpy, StdGC, Globalgcm, &Globalgcv);
  }
  else
  {
    Globalgcv.foreground = t->text;
    Globalgcv.background = t->back;
    Globalgcm = GCForeground | GCBackground;
    XChangeGC (dpy, StdGC, Globalgcm, &Globalgcv);

  }
  if (t->PagerView != None)
  {
    XClearWindow (dpy, t->PagerView);
    XDrawString (dpy, t->PagerView, StdGC, 2, windowFont->ascent + 2, t->icon_name, strlen (t->icon_name));
  }
}


void
LabelIconWindow (PagerWindow * t)
{
  XGCValues Globalgcv;
  unsigned long Globalgcm;

  if (windowFont == NULL)
  {
    return;
  }
  if (t->icon_name == NULL)
  {
    return;
  }

  if (t == FocusWin)
  {
    Globalgcv.foreground = focus_fore_pix;
    Globalgcv.background = focus_pix;
    Globalgcm = GCForeground | GCBackground;
    XChangeGC (dpy, StdGC, Globalgcm, &Globalgcv);
  }
  else
  {
    Globalgcv.foreground = t->text;
    Globalgcv.background = t->back;
    Globalgcm = GCForeground | GCBackground;
    XChangeGC (dpy, StdGC, Globalgcm, &Globalgcv);

  }
  XClearWindow (dpy, t->IconView);
  XDrawString (dpy, t->IconView, StdGC, 2, windowFont->ascent + 2, t->icon_name, strlen (t->icon_name));

}

void
IconMoveWindow (XEvent * Event, PagerWindow * t)
{
  int x1, y1, finished = 0, wx, wy, x = 0, y = 0, xi = 0, yi = 0;
  Window dumwin;
  int n1, m1;
  int moved = 0;
  int KeepMoving = 0;

  if (t == NULL)
    return;

  n1 = (t->x) / Scr.MyDisplayWidth;
  m1 = (t->y) / Scr.MyDisplayHeight;
  wx = (t->x) * (icon_w) / (Scr.MyDisplayWidth);
  wy = (t->y) * (icon_h) / (Scr.MyDisplayHeight);

  XRaiseWindow (dpy, t->IconView);

  XTranslateCoordinates (dpy, Event->xany.window, t->IconView, Event->xbutton.x, Event->xbutton.y, &x1, &y1, &dumwin);
  while (!finished)
  {
    XMaskEvent (dpy, ButtonReleaseMask | ButtonMotionMask | ExposureMask, Event);

    if (Event->type == MotionNotify)
    {
      x = Event->xbutton.x;
      y = Event->xbutton.y;
      if (moved == 0)
      {
	xi = x;
	yi = y;
	moved = 1;
      }

      XMoveWindow (dpy, t->IconView, x - (x1), y - (y1));
      if ((x < -5) || (y < -5) || (x > icon_w + 5) || (y > icon_h + 5))
      {
	finished = 1;
	KeepMoving = 1;
      }
    }
    else if (Event->type == ButtonRelease)
    {
      x = Event->xbutton.x;
      y = Event->xbutton.y;
      XMoveWindow (dpy, t->PagerView, x - (x1), y - (y1));
      finished = 1;
    }
    else if (Event->type == Expose)
    {
      HandleExpose (Event);
    }
  }

  if (moved)
  {
    if ((x - xi < 3) && (y - yi < 3) && (x - xi > -3) && (y - yi > -3))
      moved = 0;
  }

  if (KeepMoving)
  {
    XTranslateCoordinates (dpy, t->IconView, Scr.Root, x, y, &x1, &y1, &dumwin);
    if (t->flags & ICONIFIED)
    {
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      SendInfo (fd, "Move", t->icon_w);
    }
    else
    {
      XUngrabPointer (dpy, CurrentTime);
      XSync (dpy, 0);
      SendInfo (fd, "Move", t->w);
    }
  }
  else
  {
    x = x - x1;
    y = y - y1;
    n1 = x * (Scr.MyDisplayWidth) / (icon_w * Scr.MyDisplayWidth);
    m1 = y * (Scr.MyDisplayHeight) / (icon_h * Scr.MyDisplayHeight);
    x = x * (Scr.MyDisplayWidth) / (icon_w);
    y = y * (Scr.MyDisplayHeight) / (icon_h);

    if (((t->flags & ICONIFIED) && (t->flags & StickyIcon)) || (t->flags & STICKY))
    {
      if (x > Scr.MyDisplayWidth - 16)
	x = Scr.MyDisplayWidth - 16;
      if (y > Scr.MyDisplayHeight - 16)
	y = Scr.MyDisplayHeight - 16;
      if (x + t->width < 16)
	x = 16 - t->width;
      if (y + t->height < 16)
	y = 16 - t->height;
    }
    if (moved)
    {
      if (t->flags & ICONIFIED)
	XMoveWindow (dpy, t->icon_w, x, y);
      else
	XMoveWindow (dpy, t->w, x, y);
      XSync (dpy, 0);
    }
    else
    {
      MoveResizePagerView (t);
    }
    SendInfo (fd, "Raise", t->w);

    if (t->flags & ICONIFIED)
      XSetInputFocus (dpy, t->icon_w, RevertToParent, Event->xbutton.time);
    else
      XSetInputFocus (dpy, t->w, RevertToParent, Event->xbutton.time);
  }

}
