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
 * xfwm built-in functions
 *
 ***********************************************************************/

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

extern XEvent Event;
extern XfwmWindow *Tmp_win;
extern int menuFromFrameOrWindowOrTitlebar;
Bool desperate;

/*
 * ** be sure to keep this list properly ordered for bsearch routine!
 */
static struct functions func_config[] = {
  {"+", add_another_item, F_ADDMENU2, FUNC_NO_WINDOW},
  {"ActiveColor", SetHiColor, F_HICOLOR, FUNC_NO_WINDOW},
  {"AddModuleConfig", AddModConfig, F_ADD_MOD, FUNC_NO_WINDOW},
  {"AddToFunc", add_item_to_func, F_ADDFUNC, FUNC_NO_WINDOW},
  {"AddToMenu", add_item_to_menu, F_ADDMENU, FUNC_NO_WINDOW},
  {"AnimateWin", SetAnimate, F_ANIMATE, FUNC_NO_WINDOW},
  {"ArrangeIcons", Arrange_Icons, F_ARRANGEICONS, FUNC_NO_WINDOW},
  {"AutoRaise", SetAutoRaise, F_AUTORAISE, FUNC_NO_WINDOW},
  {"AutoRaiseDelay", SetAutoRaiseDelay, F_AUTORAISEDELAY, FUNC_NO_WINDOW},
  {"Beep", Bell, F_BEEP, FUNC_NO_WINDOW},
  {"ClickRaise", SetClickRaise, F_SETCLICKRAISE, FUNC_NO_WINDOW},
  {"ClickTime", SetClick, F_CLICK, FUNC_NO_WINDOW},
  {"Close", close_function, F_CLOSE, FUNC_NEEDS_WINDOW},
  {"ColormapFocus", SetColormapFocus, F_COLORMAP_FOCUS, FUNC_NO_WINDOW},
  {"Current", CurrentFunc, F_CURRENT, FUNC_NO_WINDOW},
  {"CursorColor", CursorColor, F_CURSOR_COLOR, FUNC_NO_WINDOW},
  {"CursorMove", movecursor, F_MOVECURSOR, FUNC_NO_WINDOW},
  {"Delete", delete_function, F_DELETE, FUNC_NEEDS_WINDOW},
  {"Desk", changeDesks_func, F_DESK, FUNC_NO_WINDOW},
  {"Destroy", destroy_function, F_DESTROY, FUNC_NEEDS_WINDOW},
  {"DestroyFunc", destroy_menu, F_DESTROY_MENU, FUNC_NO_WINDOW},
  {"DestroyMenu", destroy_menu, F_DESTROY_MENU, FUNC_NO_WINDOW},
  {"DestroyModuleConfig", DestroyModConfig, F_DESTROY_MOD, FUNC_NO_WINDOW},
  {"Echo", echo_func, F_ECHO, FUNC_NO_WINDOW},
  {"Engine", engine_func, F_ENGINE, FUNC_NO_WINDOW},
  {"Exec", exec_function, F_EXEC, FUNC_NO_WINDOW},
  {"ExecUseSHELL", exec_setup, F_EXEC_SETUP, FUNC_NO_WINDOW},
  {"FlipFocus", flip_focus_func, F_FLIP_FOCUS, FUNC_NEEDS_WINDOW},
  {"Focus", focus_func, F_FOCUS, FUNC_NEEDS_WINDOW},
  {"FocusMode", SetFocusMode, F_SETFOCUSMODE, FUNC_NO_WINDOW},
  {"ForceFocus", SetForceFocus, F_SETFORCEFOCUS, FUNC_NO_WINDOW},
  {"Function", ComplexFunction, F_FUNCTION, FUNC_NO_WINDOW},
  {"HonorWMFocusHint", SetHonorWMFocusHint, F_SETHONORWMFOCUSHINT, FUNC_NO_WINDOW},
  {"IconFont", LoadIconFont, F_ICONFONT, FUNC_NO_WINDOW},
  {"IconGrid", SetIconGrid, F_ICONGRID, FUNC_NO_WINDOW},
  {"Iconify", iconify_function, F_ICONIFY, FUNC_NEEDS_WINDOW},
  {"IconifyAll", Iconify_all, F_ICONIFYALL, FUNC_NO_WINDOW},
  {"Iconpos", SetIconPos, F_ICONPOS, FUNC_NO_WINDOW},
  {"IconSpacing", SetIconSpacing, F_ICONSPACING, FUNC_NO_WINDOW},
  {"InactiveColor", SetLoColor, F_LOCOLOR, FUNC_NO_WINDOW},
  {"Key", ParseKeyEntry, F_KEY, FUNC_NO_WINDOW},
  {"KillModule", module_zapper, F_ZAP, FUNC_NO_WINDOW},
  {"Lower", lower_function, F_LOWER, FUNC_NEEDS_WINDOW},
  {"MapFocus", SetMapFocus, F_SETMAPFOCUS, FUNC_NO_WINDOW},
  {"Margin", SetMargin, F_MARGIN, FUNC_NO_WINDOW},
  {"Maximize", Maximize, F_MAXIMIZE, FUNC_NEEDS_WINDOW},
  {"Menu", staysup_func, F_STAYSUP, FUNC_POPUP},
  {"MenuColor", SetMenuColor, F_MENUCOLOR, FUNC_NO_WINDOW},
  {"MenuFont", SetMenuFont, F_MENUFONT, FUNC_NO_WINDOW},
  {"Module", executeModule, F_MODULE, FUNC_NO_WINDOW},
  {"ModulePath", setModulePath, F_MODULE_PATH, FUNC_NO_WINDOW},
  {"Mouse", ParseMouseEntry, F_MOUSE, FUNC_NO_WINDOW},
  {"Move", move_window, F_MOVE, FUNC_NEEDS_WINDOW},
  {"Next", NextFunc, F_NEXT, FUNC_NO_WINDOW},
  {"None", NoneFunc, F_NONE, FUNC_NO_WINDOW},
  {"Nop", Nop_func, F_NOP, FUNC_NOP},
  {"OpaqueMove", SetOpaqueMove, F_OPAQUEMOVE, FUNC_NO_WINDOW},
  {"Opaqueresize", SetOpaqueResize, F_OPAQUERESIZE, FUNC_NO_WINDOW},
  {"PipeRead", PipeRead, F_READ, FUNC_NO_WINDOW},
  {"PopUp", popup_func, F_POPUP, FUNC_POPUP},
  {"Prev", PrevFunc, F_PREV, FUNC_NO_WINDOW},
  {"Quit", quit_func, F_QUIT, FUNC_NO_WINDOW},
  {"QuitScreen", quit_screen_func, F_QUIT_SCREEN, FUNC_NO_WINDOW},
  {"Raise", raise_function, F_RAISE, FUNC_NEEDS_WINDOW},
  {"RaiseLower", raiselower_func, F_RAISELOWER, FUNC_NEEDS_WINDOW},
  {"Read", ReadFile, F_READ, FUNC_NO_WINDOW},
  {"ReadCfg", ReadCfg, F_READCFG, FUNC_NO_WINDOW},
  {"Recapture", Recapture, F_RECAPTURE, FUNC_NO_WINDOW},
  {"Refresh", refresh_function, F_REFRESH, FUNC_NO_WINDOW},
  {"RefreshWindow", refresh_win_function, F_REFRESH, FUNC_NEEDS_WINDOW},
  {"Resize", resize_window, F_RESIZE, FUNC_NEEDS_WINDOW},
  {"Restart", restart_function, F_RESTART, FUNC_NO_WINDOW},
  {"Send_ConfigInfo", SendDataToModule, F_CONFIG_LIST, FUNC_NO_WINDOW},
  {"Send_WindowList", send_list_func, F_SEND_WINDOW_LIST, FUNC_NO_WINDOW},
  {"SendToModule", SendStrToModule, F_SEND_STRING, FUNC_NO_WINDOW},
  {"SessionManagement", SetSessionMgt, F_SESSIONMGT, FUNC_NO_WINDOW},
  {"set_mask", set_mask_function, F_SET_MASK, FUNC_NO_WINDOW},
  {"Shade", shade_function, F_SHADE, FUNC_NEEDS_WINDOW},
  {"ShowButtons", show_buttons, F_SHOWBUTTONS, FUNC_NO_WINDOW},
  {"ShowMeMouse", ShowMeMouse, F_SHOWMEMOUSE, FUNC_NO_WINDOW},
  {"SnapSize", SetSnapSize, F_SNAPSIZE, FUNC_NO_WINDOW},
  {"Stick", stick_function, F_STICK, FUNC_NEEDS_WINDOW},
  {"Style", ProcessNewStyle, F_STYLE, FUNC_NO_WINDOW},
  {"Switch", SwitchFunc, F_SWITCHFUNC, FUNC_NO_WINDOW},
  {"Title", Nop_func, F_TITLE, FUNC_TITLE},
  {"TitleStyle", SetTitleStyle, F_TITLESTYLE, FUNC_NO_WINDOW},
  {"UseShapedIcons", SetUseShapedIcons, F_SETUSESHAPEDICONS, FUNC_NO_WINDOW},
  {"Wait", wait_func, F_WAIT, FUNC_NO_WINDOW},
  {"WaitSession", wait_session, F_WAITSESSION, FUNC_NO_WINDOW},
  {"WarpToWindow", warp_func, F_WARP, FUNC_NEEDS_WINDOW},
  {"WindowFont", LoadWindowFont, F_WINDOWFONT, FUNC_NO_WINDOW},
  {"WindowId", WindowIdFunc, F_WINDOWID, FUNC_NO_WINDOW},
  {"WindowList", do_windowList, F_WINDOWLIST, FUNC_NO_WINDOW},
  {"WindowsDesk", changeWindowsDesk, F_CHANGE_WINDOWS_DESK, FUNC_NEEDS_WINDOW},
  {"XORValue", SetXOR, F_XOR, FUNC_NO_WINDOW},
  {"", 0, 0, 0}
};

/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a xfwm built in function
 *
 *  Inputs:
 *	func	- the function to execute
 *	action	- the menu action to execute 
 *	w	- the window to execute this function on
 *	tmp_win	- the xfwm window structure
 *	event	- the event that caused the function
 *	context - the context in which the button was pressed
 *      val1,val2 - the distances to move in a scroll operation 
 *
 ***********************************************************************/

void
ExecuteFunction (char *Action, XfwmWindow * tmp_win, XEvent * eventp, unsigned long context, int Module)
{
  Window w;
  int matched, j;
  char *function;
  char *action, *taction;
  char *arguments[10];

  for (j = 0; j < 10; j++)
    arguments[j] = NULL;

  if (tmp_win == NULL)
    w = Scr.Root;
  else
    w = tmp_win->w;

  if ((tmp_win) && (eventp))
    w = eventp->xany.window;
  if ((tmp_win) && (eventp->xbutton.subwindow != None) && (eventp->xany.window != tmp_win->w))
    w = eventp->xbutton.subwindow;

  taction = expand (Action, arguments, tmp_win);
  action = GetNextToken (taction, &function);
  j = 0;
  matched = False;

  while ((!matched) && (strlen (func_config[j].keyword) > 0))
  {
    if (strcasecmp (function, func_config[j].keyword) == 0)
    {
      matched = True;
      /* found key word */
      func_config[j].action (eventp, w, tmp_win, context, action, &Module);
    }
    else
      j++;
  }

  if (!matched)
  {
    desperate = 1;
    ComplexFunction (eventp, w, tmp_win, context, taction, &Module);
    if (desperate)
      executeModule (eventp, w, tmp_win, context, taction, &Module);
    desperate = 0;
  }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions or modules. */
  if (Module == -1)
    WaitForButtonsUp ();

  free (function);
  if (taction != NULL)
    free (taction);
  return;
}

int
find_func_type (char *action)
{
  int j, len = 0;
  char *endtok = action;
  Bool matched;
  while (*endtok && !isspace (*endtok))
    ++endtok;
  len = endtok - action;
  j = 0;
  matched = False;
  while ((!matched) && (strlen (func_config[j].keyword) > 0))
  {
    int mlen = strlen (func_config[j].keyword);
    if ((mlen == len) && (mystrncasecmp (action, func_config[j].keyword, mlen) == 0))
    {
      matched = True;
      /* found key word */
      return (int) func_config[j].code;
    }
    else
      j++;
  }
  /* No clue what the function is. Just return "BEEP" */
  return F_BEEP;
}
