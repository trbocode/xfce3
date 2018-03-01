/*  gxfce
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
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



#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__

#include <glib.h>

#define F_SOUNDMODULE        (1 << 0)
#define F_MOUSEMODULE        (1 << 1)
#define F_BACKDROPMODULE     (1 << 2)
#define F_PAGERMODULE        (1 << 3)
#define F_GNOMEMODULE        (1 << 4)
#define F_GNOMEMENUMODULE    (1 << 5)
#define F_KDEMENUMODULE      (1 << 6)
#define F_DEBIANMENUMODULE   (1 << 7)

typedef enum
{
  XFCE_ENGINE,
  MOFIT_ENGINE,
  TRENCH_ENGINE,
  GTK_ENGINE,
  LINEA_ENGINE
}
EngineType;

typedef struct
{
  gboolean disable_user_config;
  int panel_x;
  int panel_y;
  int wm;
  int visible_screen;
  int visible_popup;
  int select_icon_size;
  int popup_icon_size;
  gboolean colorize_root;
  gboolean gradient_root;
  gboolean detach_menu;
  gboolean clicktofocus;
  gboolean opaquemove;
  gboolean opaqueresize;
  gboolean autoraise;
  gboolean gradient_active_title;
  gboolean gradient_inactive_title;
  gboolean show_diagnostic;
  gboolean apply_xcolors;
  gboolean mapfocus;
  int iconpos;
  int tooltipsdelay;
  int snapsize;
  int startup_flags;
  char *fonts[3];
  gboolean digital_clock;
  gboolean hrs_mode;
  int panel_layer;
  EngineType xfwm_engine;
  gboolean disable_xmlconfigs;
  void* xmlconfigs;
  int switchdeskfactor;
}
config;

config current_config;

extern char *skiphead (char *);
extern char *skiptail (char *);
extern config *initconfig (config * newconf);
extern void backupconfig (char *extension);
extern void writeconfig (void);
extern void resetconfig (void);
extern void readconfig (void);
extern void update_config_screen_visible (int);

#endif
