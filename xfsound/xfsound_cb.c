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

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "my_intl.h"
#include "xfsound.h"
#include "xfsound_cb.h"
#include "constant.h"
#include "fileutil.h"
#include "fileselect.h"
#include "my_string.h"
#include "xfce-common.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

void
doplay_cb (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (play_sound_checkbutton)))
    sndcfg.playsnd = 1;
  else
    sndcfg.playsnd = 0;
}

void
play_com_cb (GtkWidget * widget, gpointer data)
{
  strcpy (sndcfg.playcmd, cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (command_entry))));
}

void
soundfile_cb (GtkWidget * widget, gpointer data)
{
  update_all_data ();
}

void
browsefile_cb (GtkWidget * widget, gpointer data)
{
  char *fselect;
  int i;

  i = list_get_choice_selected ();
  fselect = get_all_data (i);
  if (strlen (fselect) && existfile (fselect))
    fselect = open_fileselect (fselect);
  else
    fselect = open_fileselect (build_path (XFCE_SOUNDS));
  if ((fselect) && strlen (fselect))
  {
    gtk_entry_set_text (GTK_ENTRY (soundfile_entry), fselect);
    strcpy (sndcfg.datafiles[i], fselect);
  }
}

void
internal_cb (GtkWidget * widget, gpointer data)
{
  gtk_entry_set_text (GTK_ENTRY (command_entry), INTERNAL_PLAYER);
  strcpy (sndcfg.playcmd, INTERNAL_PLAYER);
}

void
defaultcmd_cb (GtkWidget * widget, gpointer data)
{
  gtk_entry_set_text (GTK_ENTRY (command_entry), DEFAULT_PLAYER);
  strcpy (sndcfg.playcmd, DEFAULT_PLAYER);
}

void
ok_cb (GtkWidget * widget, gpointer data)
{
  int i;

  i = list_get_choice_selected ();
  get_all_data (i);
  savecfg (&sndcfg);
  gtk_main_quit ();
  exit (0);
}

void
apply_cb (GtkWidget * widget, gpointer data)
{
  int i;

  i = list_get_choice_selected ();
  get_all_data (i);
  savecfg (&sndcfg);
}

void
cancel_cb (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
  exit (0);
}

void
testfile_cb (GtkWidget * widget, gpointer data)
{
  int i;

  i = list_get_choice_selected ();
  get_all_data (i);
  audio_play (i, 0);
}

gboolean delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  cancel_cb (widget, data);
  return (TRUE);
}
