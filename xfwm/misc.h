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

#ifndef _MISC_
#define _MISC_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "menus.h"
#include "utils.h"
#include "screen.h"
#include "xfwm.h"

/************************************************************************
 * ReapChildren - wait() for all dead child processes
 ************************************************************************/
#include <sys/wait.h>
#if HAVE_WAITPID
#define ReapChildren() while (waitpid (-1, NULL, WNOHANG) > 0);
#elif HAVE_WAIT3
#define ReapChildren() while (wait3 (NULL, WNOHANG, NULL) > 0);
#else
#error One of waitpid or wait3 is needed.
#endif

#define my_max(a,b) ((a > b) ? a : b)
#define my_min(a,b) ((a > b) ? b : a)
#define my_abs(a)   ((a > 0) ? a : -a)

#define AcceptInput(t)  (t && \
                        ((t->flags & DoesWmTakeFocus) || \
			 !(Scr.Options & HonorWMFocusHint) || \
			 !(t->wmhints) || \
                        ((t->wmhints) && !(t->wmhints->flags & InputHint)) || \
                        ((t->wmhints) && (t->wmhints->flags & InputHint) && (t->wmhints->input))))

#define ICON_HEIGHT (Scr.IconFont.height + 6)

#define UP 1
#define DOWN 0

extern XGCValues Globalgcv;
extern unsigned long Globalgcm;
extern Time lastTimestamp;
extern XEvent Event;

extern char NoName[];
extern char NoClass[];
extern char NoResource[];

void MoveOutline (XfwmWindow *, int, int, int, int);
void DisplayPosition (XfwmWindow *, int, int, int, int);
void DoResize (int, int, XfwmWindow *);
void SetupFrame (XfwmWindow *, int, int, int, int, Bool, Bool);
void CreateGCs (void);
void InstallWindowColormaps (XfwmWindow *);
void InstallRootColormap (void);
void UninstallRootColormap (void);
void FetchWmProtocols (XfwmWindow *);
void FetchWmColormapWindows (XfwmWindow * tmp);
void PaintMenu (MenuRoot *);
void InitEventHandlerJumpTable (void);
void DispatchEvent (void);
void HandleEvents (void);
void HandleExpose (void);
void HandleFocusIn (void);
void HandleFocusOut (void);
void HandleDestroyNotify (void);
void HandleMapRequest (void);
void HandleMapNotify (void);
void HandleUnmapNotify (void);
void HandleMotionNotify (void);
void HandleButtonPress (void);
void HandleButtonRelease (void);
void HandleEnterNotify (void);
void HandleLeaveNotify (void);
void HandleConfigureRequest (void);
void HandleClientMessage (void);
void HandlePropertyNotify (void);
void HandleKeyPress (void);
void HandleVisibilityNotify (void);
void HandleColormapNotify (void);
void RestoreWithdrawnLocation (XfwmWindow *, Bool);
void Destroy (XfwmWindow *);
XfwmWindow *AddWindow (Window w);
int MappedNotOverride (Window w);
void GrabButtons (XfwmWindow *);
void GrabKeys (XfwmWindow *);
void GetWindowSizeHints (XfwmWindow *);
Bool GetWindowState (XfwmWindow *, unsigned long *);
void SetWindowState (XfwmWindow *);
Bool moveLoop (XfwmWindow *, int, int, int, int, int *, int *, Bool, Bool);

void Keyboard_shortcuts (XEvent *, int);
void CreateIconWindow (XfwmWindow * tmp_win, int def_x, int def_y);
#define NO_HILITE     0x0000
#define TOP_HILITE    0x0001
#define RIGHT_HILITE  0x0002
#define BOTTOM_HILITE 0x0004
#define LEFT_HILITE   0x0008
#define FULL_HILITE   0x000F
#define HH_HILITE     0x0010

void sleep_a_little (int);
void Maximize (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void Shade (XfwmWindow * tmp_win);
void Unshade (XfwmWindow * tmp_win);
void shade_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void MyXGrabButton (Display *, unsigned int, unsigned int, Window, Bool, unsigned int, int, int, Window, Cursor);
void MyXUngrabButton (Display *, unsigned int, unsigned int, Window);
void MyXGrabKey (Display *, int, unsigned int, Window, Bool, int, int);
void MyXUngrabKey (Display *, int, unsigned int, Window);
Bool GrabEm (int);
void UngrabEm (void);
MenuRoot *NewMenuRoot (char *name, int function_or_popup);
void AddToMenu (MenuRoot *, char *, char *);
void MakeMenu (MenuRoot *);
void CaptureAllWindows (void);
void SetTimer (int);
void ExecuteFunction (char *Action, XfwmWindow * tmp_win, XEvent * eventp, unsigned long context, int Module);
void do_windowList (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void RaiseThisWindow (int);
int GetContext (XfwmWindow *, XEvent *, Window * dummy);
void ConstrainSize (XfwmWindow *, int *, int *);
void SetShape (XfwmWindow *, int);
void AutoPlace (XfwmWindow *, Bool);
Bool CheckIconPlace (XfwmWindow *);
void executeModule (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetFocus (Window, XfwmWindow *, Bool FocusByMouse, Bool force);
void CheckAndSetFocus (void);
void initModules (void);
void HandleModuleInput (Window w, int channel);
void match_string (struct config *, char *, char *, FILE *);
void no_popup (char *ptr);
void KillModule (int channel, int place);
void ClosePipes (void);
char *findIconFile (char *icon, char *pathlist, int mode);
int find_func_type (char *action);
void GetBitmapFile (XfwmWindow * tmp_win);
void SetDefaultXPMIcon (XfwmWindow * tmp_win);
void GetXPMFile (XfwmWindow * tmp_win);
void GetIconWindow (XfwmWindow * tmp_win);
void GetIconBitmap (XfwmWindow * tmp_win);
void Broadcast (unsigned long event_type, unsigned long num_datum, unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4, unsigned long data5, unsigned long data6, unsigned long data7);
void BroadcastConfig (unsigned long event_type, XfwmWindow * t);
void SendPacket (int channel, unsigned long event_type, unsigned long num_datum, unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4, unsigned long data5, unsigned long data6, unsigned long data7);
void SendConfig (int Module, unsigned long event_type, XfwmWindow * t);
void BroadcastName (unsigned long event_type, unsigned long data1, unsigned long data2, unsigned long data3, char *name);
void SendName (int channel, unsigned long event_type, unsigned long data1, unsigned long data2, unsigned long data3, char *name);
void SendStrToModule (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void DeadPipe (int nonsense);
void GetMwmHints (XfwmWindow * t);
void GetExtHints (XfwmWindow * t);
void GetKDEHints (XfwmWindow *);
void SelectDecor (XfwmWindow *, unsigned long, int);
Bool PopUpMenu (MenuRoot *, int, int);
void ComplexFunction (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
int DeferExecution (XEvent *, Window *, XfwmWindow **, unsigned long *, int, int);
void my_sleep (int);
void Animate (int, int, int, int, int, int, int, int);
void ShowMeMouse (XEvent *, Window, XfwmWindow *, unsigned long, char *, int *);
void SetBorder (XfwmWindow *, XRectangle *, Bool, Bool, Bool, Window);
void RedrawLeftButtons (XfwmWindow *, XRectangle *, Bool, Bool, Window);
void RedrawRightButtons (XfwmWindow *, XRectangle *, Bool, Bool, Window);
void move_window (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void resize_window (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void CreateIconWindow (XfwmWindow *, int, int);
void SetMapStateProp (XfwmWindow *, int);
void SetStickyProp (XfwmWindow *, int, int, int);
void SetClientProp (XfwmWindow *);
void Iconify (XfwmWindow *, int, int, Bool);
void Iconify_all (XEvent *, Window, XfwmWindow *, unsigned long, char *, int *);
void DeIconify (XfwmWindow *);
void PopDownMenu (void);
void WaitForButtonsUp (void);
void FocusOn (XfwmWindow * t, Bool DeIconifyFlag);
void WarpOn (XfwmWindow * t, int warp_x, int x_unit, int warp_y, int y_unit);
void GetGravityOffsets (XfwmWindow * tmp_win);
Bool PlaceWindow (XfwmWindow * tmp_win, unsigned long flags, int Desk);
void GetWMName (XfwmWindow *);
void GetWMIconName (XfwmWindow *);
void free_window_names (XfwmWindow * tmp, Bool nukename, Bool nukeicon);
void RevertFocus (XfwmWindow *, Bool);
int do_menu (MenuRoot * menu, int style);
int check_allowed_function (MenuItem * mi);
void ReInstallActiveColormap (void);
unsigned int vmod (int m);
void ParsePopupEntry (char *, FILE *, char **, int *);
void ParseMouseEntry (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *tline, int *Module);
void ParseKeyEntry (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *tline, int *Module);
void AddToStyleList (char *name, char *icon_name, unsigned long off_flags, unsigned long on_flags, int desk, int bw, int layer, char *forecolor, char *backcolor, unsigned long off_buttons, unsigned long on_buttons);
void FreeStyleList (void);
unsigned long LookInStyleList (char *, XClassHint *, char **value, int *Desk, int *bw, int *layer, char **forecolor, char **backcolor, unsigned long *buttons);

void ParseStyle (char *text, FILE *, char **, int *);
void assign_string (char *text, FILE * fd, char **arg, int *);
void SetFlag (char *text, FILE * fd, char **arg, int *);
void SetCursor (char *text, FILE * fd, char **arg, int *);
void SetInts (char *text, FILE * fd, char **arg, int *);
void SetBox (char *text, FILE * fd, char **arg, int *);
void set_func (char *, FILE *, char **, int *);
void copy_config (FILE ** config_fd);
void CursorColor (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetColormapFocus (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void DrawPattern (Window, GC, GC, int, int, int);
Pixel GetShadow (Pixel);
Pixel GetHilite (Pixel);
int brightness (Pixel);
void MapIt (XfwmWindow * t);
void UnmapIt (XfwmWindow * t);
void do_save (void);
void sendclient_event (XfwmWindow *, int, int, int, int);
#ifdef REQUIRES_STASHEVENT
Bool StashEventTime (XEvent * ev);
#endif
int flush_expose (Window w);
void fast_process_expose (void);
int discard_events(long);
int My_XNextEvent (Display * dpy, XEvent * event);
void FlushQueue (int Module);
void AddFuncKey (char *, int, int, int, char *, int, int, MenuRoot *, char, char);
char *GetNextPtr (char *ptr);

Bool InteractiveMove (Window * w, XfwmWindow * tmp_win, int *FinalX, int *FinalY, XEvent * eventp);

MenuRoot *FindPopup (char *action);

void Bell (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void movecursor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void iconify_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void raise_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void lower_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void destroy_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void delete_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void close_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void restart_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void exec_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void exec_setup (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void refresh_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void refresh_win_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void stick_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);

void changeDesks_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void changeDesks (int, int, Bool, Bool, Bool);
void changeWindowsDesk (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void sendWindowsDesk (int desk, XfwmWindow * t);

int GetTwoArguments (char *action, int *val1, int *val2, int *val1_unit, int *val2_unit, int x, int y);
int GetOneArgument (char *action, long *val1, int *val1_unit, int x, int y);
void wait_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void flip_focus_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void focus_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void warp_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SendDataToModule (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void send_list_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void popup_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void staysup_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void quit_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void quit_screen_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void echo_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void raiselower_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void Nop_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void set_mask_function (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetFocusMode (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetAnimate (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetOpaqueMove (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetOpaqueResize (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetAutoRaise (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetAutoRaiseDelay (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetIconPos (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void Arrange_Icons (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetIconGrid (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetIconSpacing (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void engine_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void show_buttons (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void DestroyMenu (MenuRoot * mr);
void GetColors (void);
XColor GetXColor (char *);
Pixel GetColor (char *);
Pixel *AllocLinearGradient (char *s_from, char *s_to, int npixels);
Pixel *AllocNonlinearGradient (char *s_colors[], int clen[], int nsegs, int npixels);
void bad_binding (int num);
void nocolor (char *note, char *name);
void MakeMenus (void);
void GetMenuXPMFile (char *name, MenuItem * it);
void GetMenuBitmapFile (char *name, MenuItem * it);
void add_item_to_menu (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void destroy_menu (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void add_another_item (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void add_item_to_func (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void setModulePath (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void ProcessNewStyle (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetHiColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetLoColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetMenuColor (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void LoadIconFont (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void LoadWindowFont (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetMenuFont (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetTitleStyle (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetXOR (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetClick (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void PrevFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void NextFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SwitchFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void NoneFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void CurrentFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void WindowIdFunc (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void ReadFile (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void PipeRead (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void ReadCfg (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void module_zapper (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
char *expand (char *input, char *arguments[], XfwmWindow * tmp_win);
void Recapture (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void DestroyModConfig (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void AddModConfig (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SnapXY (int *x, int *y, XfwmWindow * skip);
void SnapXYWH (int *x, int *y, int w, int h, XfwmWindow * skip);
void SnapMove (int *FinalX, int *FinalY, int Width, int Height, XfwmWindow * skip);
void SetSnapSize (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void wait_session (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetSessionMgt (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetClickRaise (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetForceFocus (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetMapFocus (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetMargin (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetHonorWMFocusHint (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);
void SetUseShapedIcons (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module);

int check_existfile (char *filename);
/*
 * ** message levels for xfwm_msg:
 */
#define DBG  -1
#define INFO 0
#define WARN 1
#define ERR  2
void xfwm_msg (int type, char *id, char *msg, ...);

#ifdef BROKEN_SUN_HEADERS
#include "sun_headers.h"
#endif
#ifdef NEEDS_ALPHA_HEADER
#include "alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */
#endif /* _MISC_ */
