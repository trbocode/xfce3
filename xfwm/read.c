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
#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *xfwm_file = NULL;

char *
build_path (char *value)
{
  static char filename[MAXSTRLEN];
  char *path;

  path = (char *) getenv ("XFCE_DATA");
  if (path)
    strcpy (filename, path);
  else
    strcpy (filename, XFCE_DIR);

  if ((value) && strlen (value))
    strcat (filename, value);

  return filename;
}

struct moduleInfoList
{
  char *data;
  struct moduleInfoList *next;
};

struct moduleInfoList *modlistroot = NULL;

int numfilesread = 0;

static int syscfg_read = 0;

static const char *read_system_rc_cmd = "Read xfwmrc";

void AddToModList (char *tline);

extern void StartupStuff (void);

/*
 * ** func to do actual read/piperead work
 */
static void
ReadSubFunc (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module, int piperead)
{
  char *filename = NULL, *Home = NULL, *home_file, *ofilename = NULL;
  char *rest, *tline, line[1000];
  int HomeLen;
  FILE *fd = NULL;
  int thisfileno;

  thisfileno = numfilesread;
  numfilesread++;

  rest = GetNextToken (action, &ofilename);
  if (ofilename == NULL)
  {
    xfwm_msg (ERR, piperead ? "PipeRead" : "Read", "missing parameter");
    return;
  }

  filename = ofilename;

  if (piperead)
    fd = popen (filename, "r");
  else
    fd = fopen (filename, "r");

  if (!piperead)
  {
    if ((fd == NULL) && (ofilename[0] != '/'))
    {
      /* find the home directory to look in */
      Home = getenv ("HOME");
      if (Home == NULL)
	Home = "./";
      HomeLen = strlen (Home);
      home_file = safemalloc (HomeLen + strlen (ofilename) + strlen ("/.xfce/") + 1);
      strcpy (home_file, Home);
      strcat (home_file, "/.xfce/");
      strcat (home_file, ofilename);
      filename = home_file;
      fd = fopen (filename, "r");
    }
    if ((fd == NULL) && (ofilename[0] != '/'))
    {
      if ((filename != NULL) && (filename != ofilename))
	free (filename);
      /* find the home directory to look in */
      Home = XFCE_CONFDIR;
      HomeLen = strlen (Home);
      home_file = safemalloc (HomeLen + strlen (ofilename) + 3);
      strcpy (home_file, Home);
      strcat (home_file, "/");
      strcat (home_file, ofilename);
      filename = home_file;
      fd = fopen (filename, "r");
    }
    if ((fd == NULL) && (ofilename[0] == '/'))
      fd = fopen (ofilename, "r");
  }

  if (fd == NULL)
  {
    if ((ofilename != filename) && (filename != NULL))
    {
      free (filename);
      filename = NULL;
    }
    if (ofilename != NULL)
    {
      free (ofilename);
      ofilename = NULL;
    }
    return;
  }
  if ((ofilename != NULL) && (filename != ofilename))
    free (ofilename);
  fcntl (fileno (fd), F_SETFD, 1);
  if (xfwm_file != NULL)
    free (xfwm_file);
  xfwm_file = filename;

  tline = fgets (line, (sizeof line) - 1, fd);
  while (tline != (char *) 0)
  {
    int l;
    while (tline && (l = strlen (line)) < sizeof (line) && line[l - 1] == '\n' && line[l - 2] == '\\')
    {
      tline = fgets (line + l - 2, sizeof (line) - l, fd);
    }
    tline = line;
    while (isspace (*tline))
      tline++;
    /* should these next checks be moved into ExecuteFunction? */
    if ((strlen (&tline[0]) > 1) && (tline[0] != '#') && (tline[0] != '*'))
      ExecuteFunction (tline, tmp_win, eventp, context, *Module);
    if (tline[0] == '*')
      AddToModList (tline);
    tline = fgets (line, (sizeof line) - 1, fd);
  }

  if (piperead)
    pclose (fd);
  else
    fclose (fd);
}

void
ReadFile (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  ReadSubFunc (eventp, junk, tmp_win, context, action, Module, 0);
}

void
PipeRead (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  ReadSubFunc (eventp, junk, tmp_win, context, action, Module, 1);
}

void
ReadCfg (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  int this_read = numfilesread;

  ReadSubFunc (eventp, junk, tmp_win, context, action, Module, 0);

  if (!syscfg_read && this_read == 0)
  {
    xfwm_msg (INFO, "Read", "trying to read system rc file");
    ExecuteFunction ((char *) read_system_rc_cmd, NULL, &Event, C_ROOT, -1);
  }
  syscfg_read = 1;
}

void
AddToModList (char *tline)
{
  struct moduleInfoList *t, *prev, *this;

  /* Find end of list */
  t = modlistroot;
  prev = NULL;

  while (t != NULL)
  {
    prev = t;
    t = t->next;
  }

  this = (struct moduleInfoList *) safemalloc (sizeof (struct moduleInfoList));
  this->data = (char *) safemalloc (strlen (tline) + 1);
  this->next = NULL;
  strcpy (this->data, tline);
  if (prev == NULL)
  {
    modlistroot = this;
  }
  else
    prev->next = this;
}

/* interface function for AddToModList */
void
AddModConfig (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  AddToModList (action);
}

/**************************************************************/
/* delete from module configuration                           */
/**************************************************************/
void
DestroyModConfig (XEvent * eventp, Window junk, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  struct moduleInfoList *this, *that, *prev;
  char *info;			/* info to be deleted - may contain wildcards */
  char *mi;

  action = GetNextToken (action, &info);
  if (info == NULL)
  {
    return;
  }

  this = modlistroot;
  prev = NULL;

  while (this != NULL)
  {
    GetNextToken (this->data, &mi);
    that = this->next;
    if (matchWildcards (info, mi + 1))
    {
      free (this->data);
      free (this);
      if (prev)
      {
	prev->next = that;
      }
      else
      {
	modlistroot = that;
      }
    }
    else
    {
      prev = this;
    }
    this = that;
    free (mi);
  }
  free (info);
}

void
SendDataToModule (XEvent * eventp, Window w, XfwmWindow * tmp_win, unsigned long context, char *action, int *Module)
{
  struct moduleInfoList *t;
  char msg2[32];

  sprintf (msg2, "ClickTime %d\n", Scr.ClickTime);
  SendName (*Module, XFCE_M_CONFIG_INFO, 0, 0, 0, msg2);

  t = modlistroot;
  while (t != NULL)
  {
    SendName (*Module, XFCE_M_CONFIG_INFO, 0, 0, 0, t->data);
    t = t->next;
  }
  SendPacket (*Module, XFCE_M_END_CONFIG_INFO, 0, 0, 0, 0, 0, 0, 0, 0);
}
