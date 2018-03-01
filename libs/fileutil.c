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



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glib.h>
#include "constant.h"
#include "sendinfo.h"
#include "my_string.h"

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static int diag[2] = { -1, -1 };

int
existfile (const char *s)
{
  struct stat buf;
  if (!stat (s, &buf))
  {
    return (buf.st_size != 0);
  }
  return (0);
}


char* getlocalizedconffilename (const char *file,int disable_user_config)
{
  char *charset_code;
  char *area_code;
  char *country_code;
  char *temp;
  char *homedir;
  char *file_default;
  int bottom_ptr = 0;

  if (!disable_user_config)
  {
    if (!(homedir = (char *) getenv ("HOME")))
    {
      fprintf (stderr, "Can't fetch $HOME. Aborting\n");
      exit (-1);
    }
  
    temp = g_strdup_printf ("%s/.xfce/%s", (char *) getenv ("HOME"), file);
    if (existfile (temp))
    {
      return (temp);
    }
    g_free (temp);
  }
  
  file_default = g_strdup_printf ("%s/%s", XFCE_CONFDIR, file);
  if ((strcmp (file_default, "") == 0) || 
      !(charset_code = getenv ("LANG")) || 
      (strcmp (charset_code, "") == 0))
  {
    return (file_default);
  }
  bottom_ptr = strlen (charset_code) - 1;

  /* Try Charset Code */
  temp = g_strdup_printf ("%s.%s", file_default, charset_code);
  if (existfile (temp))
  {
    g_free (file_default);
    return (temp);
  }
  g_free (temp);

  /* Try Area Code */
  while (charset_code[bottom_ptr] != '.')
  {
    if (bottom_ptr <= 0)
    {
      bottom_ptr = strlen (charset_code);
      break;
    }
    bottom_ptr--;
  }
  area_code = g_strndup (charset_code, bottom_ptr);
  temp = g_strdup_printf ("%s.%s", file_default, area_code);
  g_free (area_code);
  if (existfile (temp))
  {
    g_free (file_default);
    return (temp);
  }
  g_free (temp);

  /* Try Country Code */
  while (charset_code[bottom_ptr] != '_')
  {
    if (bottom_ptr <= 0)
    {
      bottom_ptr = strlen (charset_code);
      break;
    }
    bottom_ptr--;
  }
  country_code = g_strndup (charset_code, bottom_ptr);
  temp = g_strdup_printf ("%s.%s", file_default, country_code);
  g_free (country_code);
  if (existfile (temp))
  {
    g_free (file_default);
    return (temp);
  }
  g_free (temp);
  return (file_default);
}


void
my_sleep (int n)
{
  struct timeval value;

  if (n > 0)
  {
    value.tv_usec = n % 1000000;
    value.tv_sec = n / 1000000;
    (void) select (1, 0, 0, 0, &value);
  }
}

int
initdiag (void)
{
  if (diag[0] == -1)
  {
    pipe (diag);
    /*
       fcntl (diag[0], F_SETFL, O_NONBLOCK);
       fcntl (diag[1], F_SETFL, O_NONBLOCK);
     */
  }
  return (diag[0]);
}

void
exec_comm (char *comm, int wm)
{
  char *command;
  char *toexec;
  int nulldev;

  command = (char *) g_malloc ((MAXSTRLEN + 1) * sizeof (char));
  if (comm)
  {
    toexec = cleanup (comm);
    /*
       Still got something to do ? 
     */
    if (strlen (toexec) && my_strncasecmp (toexec, "None", strlen ("None")))
    {
      if (my_strncasecmp (toexec, "Module ", strlen ("Module ")))
      {
	/*
	   Do we need to start an xterm to execute the command ?
	 */
	if (!my_strncasecmp (toexec, "Term ", strlen ("Term ")))
	{
	  snprintf (command, MAXSTRLEN - 1, "exec %s -e %s", TERMINAL, toexec + strlen ("Term ") * sizeof (char));
	}
	else
	{
	  /*
	     Add 'exec ' if not present, to reduce processes nbr 
	   */
	  if (my_strncasecmp (toexec, "exec ", strlen ("exec ")))
	  {
	    snprintf (command, MAXSTRLEN - 1, "exec %s", toexec);
	  }
	  else
	  {
	    strncpy (command, toexec, MAXSTRLEN - 1);
	  }
	}
	switch (fork ())
	{
	case 0:
	  /* The following is to avoid X locking when executing 
	     terminal based application that requires user input */
	  if ((nulldev = open ("/dev/null", O_RDWR)))
	  {
	    close (0);
	    dup (nulldev);
	  }
	  if ((diag[0] != -1) && (diag[1] != -1))
	  {
	    close (1);
	    dup (diag[1]);
	    close (2);
	    dup (diag[1]);
	    close (diag[0]);
	    close (diag[1]);
	  }
          signal (SIGCHLD, SIG_DFL);
	  execl (DEFAULT_SHELL, DEFAULT_SHELL, "-c", command, NULL);
	  perror ("exec failed");
	  _exit (0);
	  break;
	case -1:
	  fprintf (stderr, "XFce : cannot execute fork()\n");
	  break;
	default:
	  break;
	}
      }
      else
      {
	/*
	   This is a module : tell xfwm to execute it 
	 */
	if (wm)
	  sendinfo (fd_internal_pipe, toexec, 0);
	else
	  fprintf (stderr, "Needs xfwm to execute modules !\n");
      }
    }
  }
  g_free (command);
}
