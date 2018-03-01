/*  xfsound
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

#ifndef __XFSOUND_H__
#define __XFSOUND_H__

#include <gtk/gtk.h>

#define KNOWN_MESSAGES          13
#define BUILTIN_STARTUP		KNOWN_MESSAGES
#define BUILTIN_SHUTDOWN	KNOWN_MESSAGES+1
#define BUILTIN_UNKNOWN		KNOWN_MESSAGES+2
#define KNOWN_BUILTIN		3

#define INTERNAL_PLAYER "internal"
#define DEFAULT_PLAYER  "xfplay 2>/dev/null"

GtkWidget *xfsound;
GtkWidget *play_sound_checkbutton;
GtkWidget *list_event_optionmenu;
GtkWidget *list_event_optionmenu_menu;
GtkWidget *soundfile_entry;
GtkWidget *command_entry;

typedef struct
{
  int playsnd;
  char *playcmd;
  char *datafiles[KNOWN_MESSAGES + KNOWN_BUILTIN];
}
XFSound;

typedef struct
{
  long message_id;
  char *label;
}
T_messages;

char *homedir, *tempstr;
XFSound sndcfg;
int fd_width;
int fd[2];

void allocXFSound (XFSound * s);
void freeXFSound (XFSound * s);
void loadcfg (XFSound * s);
int savecfg (XFSound * s);
void audio_play (short code, short backgnd);
char *get_all_data (int i);
void update_all_data (void);
GtkWidget *create_xfsound (void);
GtkWidget *list_addto_choice (char *name);
int list_get_choice_selected (void);
void list_set_choice_selected (int index);
void init_choice_str (void);

#endif
