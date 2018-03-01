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


#include <stdlib.h>
#include <sys/stat.h>
#include "my_intl.h"
#include "selects.h"
#include "fileutil.h"
#include "xpmext.h"
#include "xfce.h"
#include "my_string.h"
#include "action.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/*
   Load pixmap files 
 */
#include "file1.h"
#include "file2.h"
#include "oldmail.h"
#include "nomail.h"
#include "mail.h"
#include "edit.h"
#include "terminal.h"
#include "man.h"
#include "paint.h"
#include "print.h"
#include "multimedia.h"
#include "games.h"
#include "schedule.h"
#include "sound.h"
#include "network.h"
#include "defaulticon.h"

#define MAIL_ICON_NUMBER 2
#define MAIL_SPOOL_DIR "/var/spool/mail"

static char *icon_name[] = {
  N_("Files related"),
  N_("Terminals/Connections"),
  N_("Mail tools/browsers"),
  N_("Print manager"),
  N_("Configuration"),
  N_("Misc. tools/Utilities"),
  N_("Manual/Help browser"),
  N_("Writing/Text tools"),
  N_("Schedule/Appointments"),
  N_("Multimedia"),
  N_("Games"),
  N_("Network"),
  N_("Sound"),
  N_("External...")
};

static char **icon_data[] = {
  file1,
  term,
  nomail,
  print,
  paint,
  file2,
  man,
  edit,
  schedule,
  multimedia,
  games,
  network,
  sound,
  defaulticon
};

char pathbuf[MAXSTRLEN] = "";

void
alloc_selects (void)
{
  int i;

  for (i = 0; i < NBSELECTS + 1; i++)
  {
    selects[i].command = (char *) g_malloc (5 * sizeof (char));
    strcpy (selects[i].command, "None");
    selects[i].ext_icon = (char *) g_malloc (5 * sizeof (char));
    strcpy (selects[i].ext_icon, "None");
    selects[i].icon_nbr = 99;
  }
}

void
free_selects (void)
{
  int i;

  for (i = 0; i < NBSELECTS + 1; i++)
  {
    g_free (selects[i].command);
    g_free (selects[i].ext_icon);
  }

}

int
load_icon_str (char *str)
{
  char *a;
  int i, error = 0;
  if ((a = strtok (str, ",")))
  {
    selects[0].icon_nbr = atoi (a);
    for (i = 1; i < NBSELECTS; i++)
      if ((a = strtok (NULL, ",")))
	selects[i].icon_nbr = (((atoi (a) < NB_PANEL_ICONS) || (atoi (a) == 99)) ? atoi (a) : i);
      else
      {
	error = i + 1;
	break;
      }
  }
  else
    error = 1;
  if (error)
    for (i = (error - 1); i < NBSELECTS; i++)
      selects[i].icon_nbr = 99;
  return (error);
}

char *
save_icon_str (void)
{
  static char str[(NBSELECTS + 2) * 3 + 1];
  char *temp;
  int i;

  temp = (char *) g_malloc (4);
  sprintf (str, "%i", selects[0].icon_nbr);
  for (i = 1; i < NBSELECTS; i++)
  {
    sprintf (temp, ",%i", selects[i].icon_nbr);
    strcat (str, temp);
  }
  g_free (temp);
  return (str);
}

void
set_exticon_str (int i, char *s)
{
  if (i < NBSELECTS)
  {
    g_free (selects[i].ext_icon);
    if ((s) && (strlen (s)))
    {
      selects[i].ext_icon = (char *) g_malloc ((strlen (s) + 1) * sizeof (char));
      strcpy (selects[i].ext_icon, s);
    }
    else
    {
      selects[i].ext_icon = (char *) g_malloc (5 * sizeof (char));
      strcpy (selects[i].ext_icon, "None");
    }
  }
}

char *
get_exticon_str (int i)
{
  return selects[i].ext_icon;
}

void
setup_icon (void)
{
  int i;

  for (i = 0; i < NBSELECTS; i++)
    if ((selects[i].icon_nbr >= 0) && (selects[i].icon_nbr < NB_PANEL_ICONS))
      MySetPixmapData (select_buttons.select_pixmap[i], select_buttons.select_button[i], icon_data[selects[i].icon_nbr]);
    else if ((selects[i].icon_nbr == 99) && (selects[i].ext_icon) && (existfile (selects[i].ext_icon)))
      MySetPixmapFile (select_buttons.select_pixmap[i], select_buttons.select_button[i], selects[i].ext_icon);
    else
      MySetPixmapData (select_buttons.select_pixmap[i], select_buttons.select_button[i], defaulticon);

}

void
default_icon_str (void)
{
  char *s;

  s = (char *) g_malloc ((NBSELECTS + 2) * 3 + 1);
  strcpy (s, DEFAULT_ICON_SEQ);
  load_icon_str (s);
  setup_icon ();
  g_free (s);
}

int
get_icon_nbr (int no_cmd)
{
  int i;

  i = selects[no_cmd].icon_nbr;
  return ((i < NB_PANEL_ICONS) ? i : 99);
}

void
set_icon_nbr (int no_cmd, int icon_nbr)
{
  if (icon_nbr < NB_PANEL_ICONS)
  {
    selects[no_cmd].icon_nbr = icon_nbr;
    MySetPixmapData (select_buttons.select_pixmap[no_cmd], select_buttons.select_button[no_cmd], icon_data[icon_nbr]);
  }
  else
  {
    selects[no_cmd].icon_nbr = 99;
    if ((selects[no_cmd].ext_icon) && (existfile (selects[no_cmd].ext_icon)))
      MySetPixmapFile (select_buttons.select_pixmap[no_cmd], select_buttons.select_button[no_cmd], selects[no_cmd].ext_icon);
    else
      MySetPixmapData (select_buttons.select_pixmap[no_cmd], select_buttons.select_button[no_cmd], defaulticon);
  }
}

void
init_choice_str (void)
{
  int i;

  for (i = 0; i < (NB_PANEL_ICONS + 1); i++)
    action_addto_choice (_(icon_name[i]));
}

void
set_choice_value (int no_cmd)
{
  if (selects[no_cmd].icon_nbr < NB_PANEL_ICONS)
    action_set_choice_selected (selects[no_cmd].icon_nbr);
  else
    action_set_choice_selected (NB_PANEL_ICONS);
}

void
set_command (int no_sel, char *s)
{
  g_free (selects[no_sel].command);
  if (!strlen (s) || !my_strncasecmp (s, "None", strlen ("None")))
  {
    selects[no_sel].command = (char *) g_malloc (5 * sizeof (char));
    strcpy (selects[no_sel].command, "None");
  }
  else
  {
    selects[no_sel].command = (char *) g_malloc ((strlen (s) + 1) * sizeof (char));
    strcpy (selects[no_sel].command, s);
  }
  gtk_tooltips_set_tip (select_buttons.select_tooltips[no_sel], select_buttons.select_button[no_sel], selects[no_sel].command, "ContextHelp/buttons/?");
}

char *
get_command (int no_sel)
{
  return selects[no_sel].command;
}

int
got_mail ()
{
  struct stat s;
  char *p;

  if (!pathbuf[0])
  {
    p = getenv ("MAIL");
    if (p)
    {
      strcpy (pathbuf, p);
    }
    else
    {
      p = getenv ("LOGNAME");
      if (p)
      {
	sprintf (pathbuf, MAIL_SPOOL_DIR "/%s", p);
      }				/* perhaps it would make sense to do something else here? */
    }
  }

  if (stat (pathbuf, &s) < 0)
    return 0;

  if (s.st_size)
  {
    if (s.st_mtime >= s.st_atime)
      return 2;
    else
      return 1;
  }

  return 0;
}

gint check_mail (gpointer data)
{
  static int had_mail = 0;
  int have_mail, i;

  have_mail = got_mail ();

  if (have_mail != had_mail)
  {
    switch (have_mail)
    {
    case 0:
      icon_data[MAIL_ICON_NUMBER] = nomail;
      break;
    case 1:
      icon_data[MAIL_ICON_NUMBER] = oldmail;
      break;
    case 2:
      icon_data[MAIL_ICON_NUMBER] = mail;
      break;
    default:
      icon_data[MAIL_ICON_NUMBER] = nomail;
    }
    for (i = 0; i < NB_PANEL_ICONS; i++)
    {
      if (get_icon_nbr (i) == MAIL_ICON_NUMBER)
      {
	set_icon_nbr (i, MAIL_ICON_NUMBER);
      }
    }
  }

  had_mail = have_mail;
  return TRUE;
}
