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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <gtk/gtk.h>
#include "constant.h"
#include "xfclock_config.h"
#include "my_string.h"
#include "xfce-common.h"
#include "fileutil.h"
#include "my_intl.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *rcfile = "xfclockrc";
int nl = 0;
int buffersize = 127;

void
syntax_error (char *s)
{
  fprintf (stderr, _("XFclock : Syntax error in configuration file\n(%s)\n"), s);
  my_alert (_("Syntax error in configuration file\nAborting"));
  exit (0);
}

void
data_error (char *s)
{
  fprintf (stderr, _("XFclock : Data mismatch error in config file\n(%s)\n"), s);
  my_alert (_("Data mismatch error in configuration file\nAborting"));
  exit (0);
}

char *
nextline (FILE * f, char *lineread)
{
  char *p;
  do
  {
    nl++;
    if (!fgets (lineread, MAXSTRLEN + 1, f))
    {
      return (NULL);
    }
    if (strlen (lineread))
    {
      lineread[strlen (lineread) - 1] = '\0';
    }
    p = skiphead (lineread);
  }
  while (!strlen (p) && !feof (f));
  if (strlen (p))
    skiptail (p);
  return ((!feof (f)) ? p : NULL);
}

config *
initconfig (config * conf)
{
  if (!conf)
    conf = (config *) malloc (sizeof (config));
  conf->x = -1;
  conf->y = -1;
  conf->w = -1;
  conf->h = -1;
  conf->calendar = TRUE;
  conf->menubar = TRUE;
  conf->digital = FALSE;
  conf->seconds = FALSE;
  conf->ampm = FALSE;		/* Medium size */
  conf->military = FALSE;
  conf->back_red = -1;
  conf->back_green = -1;
  conf->back_blue = -1;
  conf->fore_red = -1;
  conf->fore_green = -1;
  conf->fore_blue = -1;
  conf->font = (char *) malloc (sizeof (char) * MAXSTRLEN);
  strcpy (conf->font, "*");
  conf->holy_days = 0x01;
  conf->calendar_opt = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_WEEK_NUMBERS;

  return conf;
}

void
writeconfig (GtkWidget * widget, config * conf)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile = NULL;
  int x, y;
  int w, h;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  configfile = fopen (homedir, "w");

  if (!configfile)
    my_alert (_("Cannot create file"));
  else
  {
    fprintf (configfile, "[Coords]\n");
    gdk_window_get_root_origin (((GtkWidget *) widget)->window, &x, &y);
    gdk_window_get_size (((GtkWidget *) widget)->window, &w, &h);
    fprintf (configfile, "\t%i\n", x);
    fprintf (configfile, "\t%i\n", y);
    fprintf (configfile, "\t%i\n", w);
    fprintf (configfile, "\t%i\n", h);
    fprintf (configfile, "[Colors]\n");
    fprintf (configfile, "\t%i %i %i\n", conf->fore_red, conf->fore_green, conf->fore_blue);
    fprintf (configfile, "\t%i %i %i\n", conf->back_red, conf->back_green, conf->back_blue);
    fprintf (configfile, "\t%s\n", conf->font);
    fprintf (configfile, "[Options]\n");
    fprintf (configfile, conf->calendar ? "\tDisplayCalendar\n" : "\tHideCalendar\n");
    fprintf (configfile, conf->menubar ? "\tDisplayMenuBar\n" : "\tHideMenuBar\n");
    fprintf (configfile, conf->digital ? "\tDigitalClock\n" : "\tAnalogClock\n");
    fprintf (configfile, conf->seconds ? "\tDisplaySeconds\n" : "\tHideSeconds\n");
    fprintf (configfile, conf->ampm ? "\tDisplayAMPM\n" : "\tHideAMPM\n");
    fprintf (configfile, conf->military ? "\tDisplayMilitary\n" : "\tHideMilitary\n");
    fprintf (configfile, "\tHoly_days : 0x%02x\n", conf->holy_days);
    fprintf (configfile, "\tCalendar opts : 0x%02x\n", conf->calendar_opt);
    fflush (configfile);
    fclose (configfile);
  }
}


void
readconfig (config * conf)
{
  char homedir[MAXSTRLEN + 1];
  char lineread[MAXSTRLEN + 1];
  char *p;
  FILE *configfile = NULL;

  initconfig (conf);

  nl = 0;
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);

  if (existfile (homedir))
  {
    configfile = fopen (homedir, "r");
  }
  else
  {
    snprintf (homedir, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, rcfile);
    if (existfile (homedir))
    {
      configfile = fopen (homedir, "r");
    }
  }
  if (configfile)
  {
    p = nextline (configfile, lineread);
    if (my_strncasecmp (p, "[Coords]", strlen ("[Coords]")))
      syntax_error (p);
    p = nextline (configfile, lineread);
    conf->x = atoi (p);
    p = nextline (configfile, lineread);
    conf->y = atoi (p);
    p = nextline (configfile, lineread);
    conf->w = atoi (p);
    p = nextline (configfile, lineread);
    conf->h = atoi (p);
    p = nextline (configfile, lineread);
    if (my_strncasecmp (p, "[Colors]", strlen ("[Colors]")))
      syntax_error (p);
    p = nextline (configfile, lineread);
    sscanf (p, "%i %i %i", &(conf->fore_red), &(conf->fore_green), &(conf->fore_blue));
    p = nextline (configfile, lineread);
    sscanf (p, "%i %i %i", &(conf->back_red), &(conf->back_green), &(conf->back_blue));
    p = nextline (configfile, lineread);
    strcpy (conf->font, p);
    p = nextline (configfile, lineread);
    if (my_strncasecmp (p, "[Options]", strlen ("[Options]")))
      syntax_error (p);
    p = nextline (configfile, lineread);
    conf->calendar = (my_strncasecmp (p, "DisplayCalendar", strlen ("DisplayCalendar")) == 0);
    p = nextline (configfile, lineread);
    conf->menubar = (my_strncasecmp (p, "DisplayMenuBar", strlen ("DisplayMenuBar")) == 0);
    p = nextline (configfile, lineread);
    conf->digital = (my_strncasecmp (p, "DigitalClock", strlen ("DigitalClock")) == 0);
    p = nextline (configfile, lineread);
    conf->seconds = (my_strncasecmp (p, "DisplaySeconds", strlen ("DisplaySeconds")) == 0);
    p = nextline (configfile, lineread);
    conf->ampm = (my_strncasecmp (p, "DisplayAMPM", strlen ("DisplayAMPM")) == 0);
    p = nextline (configfile, lineread);
    conf->military = (my_strncasecmp (p, "DisplayMilitary", strlen ("DisplayMilitary")) == 0);
    p = nextline (configfile, lineread);
    if (p == NULL)
      conf->holy_days = 0x01;
    else
    {
      char *word;
      word = strtok (p, ":");
      if (word == NULL)
	conf->holy_days = 0x01;
      else
      {
	word += (strlen (word) + 1);
	sscanf (word, "%x", &(conf->holy_days));
	conf->holy_days &= 0x07;
      }
    }
    p = nextline (configfile, lineread);
    if (p == NULL)
      conf->calendar_opt = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_WEEK_NUMBERS;
    else
    {
      char *word;
      word = strtok (p, ":");
      if (word == NULL)
	conf->calendar_opt = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_WEEK_NUMBERS;
      else
      {
	word += (strlen (word) + 1);
	sscanf (word, "%x", &(conf->calendar_opt));
	conf->calendar_opt &= GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_WEEK_NUMBERS | GTK_CALENDAR_WEEK_START_MONDAY;
      }
    }

    fclose (configfile);
  }
}
