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


#ifndef __THEMES_H__
#define __THEMES_H__

#include <X11/Xlib.h>

#include "xfwm.h"
#include "screen.h"

#define GetButtonState(window) (onoroff ? Active : Inactive)

/* macro to change window background color/pixmap */
#define ChangeWindowColor(window,valuemask) {				\
        if(NewColor)							\
        {								\
          XChangeWindowAttributes(dpy,window,valuemask, &attributes);	\
          XClearWindow(dpy,window);					\
        }								\
      }

#define NewFontAndColor(newfont,color,backcolor) {\
   Globalgcv.font = newfont;\
   Globalgcv.foreground = color;\
   Globalgcv.background = backcolor;\
   Globalgcm = (newfont ? GCFont : 0) | GCForeground | GCBackground; \
   XChangeGC(dpy,Scr.ScratchGC3,Globalgcm,&Globalgcv); \
}


void ClearArea (Window, XRectangle *);
void RelieveRoundedRectangle (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawLinePattern (Window, XRectangle *, GC, GC, struct vector_coords *, int, int);
void RelieveRectangle (Window, XRectangle *, int, int, int, int, GC, GC);
void RelieveRectangleGtk (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawUnderline (Window, XRectangle *, GC, int, int, char *, int);
void DrawSeparator (Window, XRectangle *, GC, GC, int, int, int, int, int);
void DrawIconPixmap(XfwmWindow *, XRectangle *, GC, int, int);
void RedoIconName (XfwmWindow *, XRectangle *);
void DrawIconWindow (XfwmWindow *, XRectangle *);
void PaintEntry (MenuRoot *, MenuItem *);

void SetInnerBorder (XfwmWindow *, Bool);
void DrawButton (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress (void);
void SetTitleBar (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveIconTitle (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap (Window, XRectangle *, int, int, GC, GC);
void RelieveHalfRectangle (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);

void SetInnerBorder_xfce (XfwmWindow *, Bool);
void DrawButton_xfce (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress_xfce (void);
void SetTitleBar_xfce (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow_xfce (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_xfce (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_xfce (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu_xfce (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu_xfce (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern_xfce (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_xfce (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap_xfce (Window, XRectangle *, int, int, GC, GC);

void SetInnerBorder_mofit (XfwmWindow *, Bool);
void DrawButton_mofit (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress_mofit (void);
void SetTitleBar_mofit (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow_mofit (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_mofit (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_mofit (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu_mofit (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu_mofit (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern_mofit (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_mofit (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap_mofit (Window, XRectangle *, int, int, GC, GC);

void SetInnerBorder_trench (XfwmWindow *, Bool);
void DrawButton_trench (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress_trench (void);
void SetTitleBar_trench (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow_trench (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_trench (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_trench (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu_trench (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu_trench (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern_trench (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_trench (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap_trench (Window, XRectangle *, int, int, GC, GC);

void SetInnerBorder_gtk (XfwmWindow *, Bool);
void DrawButton_gtk (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress_gtk (void);
void SetTitleBar_gtk (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow_gtk (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_gtk (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_gtk (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu_gtk (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu_gtk (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern_gtk (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_gtk (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap_gtk (Window, XRectangle *, int, int, GC, GC);

void SetInnerBorder_linea (XfwmWindow *, Bool);
void DrawButton_linea (XfwmWindow *, Window, XRectangle *, int, int, ButtonFace *, GC, GC, Bool, int);
Bool RedrawTitleOnButtonPress_linea (void);
void SetTitleBar_linea (XfwmWindow *, XRectangle *, Bool);
void RelieveWindow_linea (XfwmWindow *, Window, XRectangle *, int, int, int, int, GC, GC, int);
void RelieveHalfRectangle_linea (Window, XRectangle *, int, int, int, int, GC, GC, int);
void DrawSelectedEntry_linea (Window, XRectangle *, int, int, int, int, GC *);
void DrawTopMenu_linea (Window, XRectangle *, int, GC, GC);
void DrawBottomMenu_linea (Window, XRectangle *, int, int, int, int, GC, GC);
void DrawTrianglePattern_linea (Window, XRectangle *, GC, GC, GC, int, int, int, int, short);
void RelieveIconTitle_linea (Window, XRectangle *, int, int, GC, GC);
void RelieveIconPixmap_linea (Window, XRectangle *, int, int, GC, GC);

#endif
