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

#ifndef __POPUP_H__
#define __POPUP_H__

#include "constant.h"

typedef struct
{
  char *label;
  char *pixfile;
  char *command;
  GtkTooltips *item_tooltip;
  GtkWidget *item_button;
  GtkWidget *item_pixmap_frame;
  GtkWidget *item_pixmap;
  GtkWidget *item_label;
}
ITEM_buttons;

typedef struct
{
  gboolean detach;
  gint entries;
  GtkWidget *popup_toplevel;
  GtkWidget *popup_addicon_button;
  GtkWidget *popup_addicon_icon_frame;
  GtkWidget *popup_addicon_separator;
  ITEM_buttons popup_buttons[NBMAXITEMS];
  GtkWidget *animatewin;
  GtkWidget *popup_detach;
  gint lastpx;
  gint lastpy;
  gint uposx;
  gint uposy;
}
POPUP_menu;

POPUP_menu popup_menus[NBPOPUPS];

GtkWidget *create_popup_item (GtkWidget * toplevel, gint nbr_menu, gint nbr_item);

GtkWidget *create_popup_menu (gint nbr);

void create_all_popup_menus (GtkWidget * main);

void show_popup_menu (gint nbr, gint x, gint y, gboolean);

void hide_popup_menu (gint nbr, gboolean);

void close_popup_menu (gint nbr);

void update_popup_entries (gint nbr);

void set_entry (gint menu, gint entry, char *label, char *pixfile, char *command);

void update_popup_size (void);

void set_like_entry (gint menu_s, gint entry_s, gint menu_d, gint entry_d);

void add_popup_entry (gint nbr, char *label, char *pixfile, char *command, int position);

void move_popup_entry (gint menu, gint entry_s, gint entry_d);

void remove_popup_entry (gint nbr_menu, gint entry);

char *get_popup_entry_label (gint menu, gint entry);

char *get_popup_entry_command (gint menu, gint entry);

char *get_popup_entry_icon (gint menu, gint entry);

gint get_popup_menu_entries (gint menu);

/* Added by Jason Litowitz */
void hide_current_popup_menu ();

#endif
