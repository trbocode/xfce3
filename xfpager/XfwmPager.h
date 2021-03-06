#ifndef __XFWMPAGER_H__
#define __XFWMPAGER_H__

typedef struct ScreenInfo
{
  unsigned long screen;
  int d_depth;
  int MyDisplayWidth;
  int MyDisplayHeight;

  char *XfwmRoot;
  Window Root;

  Window Pager_w;

  Font PagerFont;

  GC NormalGC;

  char *Hilite;


  unsigned VScale;
  int CurrentDesk;
  Pixmap sticky_gray_pixmap;
  Pixmap light_gray_pixmap;
  Pixmap gray_pixmap;

}
ScreenInfo;

typedef struct pager_window
{
  char *t;
  Window w;
  Window frame;
  int x;
  int y;
  int width;
  int height;
  int desk;
  int frame_x;
  int frame_y;
  int frame_width;
  int frame_height;
  int title_height;
  int border_width;
  int icon_x;
  int icon_y;
  int icon_width;
  int icon_height;
  int shade_x;
  int shade_y;
  int shade_width;
  int shade_height;
  Pixel text;
  Pixel back;
  unsigned long flags;
  Window icon_w;
  Window icon_pixmap_w;
  char *icon_name;
  int pager_view_width;
  int pager_view_height;
  int icon_view_width;
  int icon_view_height;

  Window PagerView;
  Window IconView;

  struct pager_window *next;
}
PagerWindow;


typedef struct desk_info
{
  Window w;
  Window title_w;
  Window CPagerWin;
  int x;
  int y;
  char *Dcolor;
  char *label;
}
DeskInfo;

#define ON 1
#define OFF 0

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
char *GetNextToken (char *indata, char **token);
void Loop (int *fd);
void SendInfo (int *fd, char *message, unsigned long window);
char *safemalloc (int length);
void DeadPipe (int nonsense);
void process_message (unsigned long type, unsigned long *body);
void ParseOptions (void);

void list_add (unsigned long *body);
void list_configure (unsigned long *body);
void list_destroy (unsigned long *body);
void list_focus (unsigned long *body);
void list_toggle (unsigned long *body);
void list_new_desk (unsigned long *body);
void list_raise (unsigned long *body);
void list_lower (unsigned long *body);
void list_unknown (unsigned long *body);
void list_iconify (unsigned long *body);
void list_deiconify (unsigned long *body);
void list_shade (unsigned long *body);
void list_unshade (unsigned long *body);
void list_window_name (unsigned long *body);
void list_icon_name (unsigned long *body);
void list_class (unsigned long *body);
void list_res_name (unsigned long *body);
void list_mini_icon (unsigned long *body);
void list_restack (void);
int My_XNextEvent (Display * dpy, XEvent * event);

/* Stuff in x_pager.c */
void initialize_pager (void);
Pixel GetColor (char *name);
void nocolor (char *a, char *b);
void ReConfigure (void);
void ReConfigureAll (void);
void MovePage (void);
void DrawGrid (int i, int erase);
void DrawIconGrid (int erase);
void SwitchToDesk (int Desk);
void SwitchToDeskAndPage (int Desk, XEvent * Event);
void AddNewWindow (PagerWindow * prev);
void MoveResizePagerView (PagerWindow * t);
void ChangeDeskForWindow (PagerWindow * t, long newdesk);
void MoveStickyWindow (void);
void Hilight (PagerWindow *, int);
void Scroll (int Desk, int x, int y);
void MoveWindow (XEvent * Event);
void LabelWindow (PagerWindow * t);
void LabelIconWindow (PagerWindow * t);
void ReConfigureIcons (void);
void IconSwitchPage (XEvent * Event);
void IconScroll (int x, int y);
void IconMoveWindow (XEvent * Event, PagerWindow * t);
void MoveStickyWindows (void);

#ifdef BROKEN_SUN_HEADERS
#include "sun_headers.h"
#endif

#endif
