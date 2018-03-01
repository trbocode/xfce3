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
#include <string.h>
#include <ctype.h>
#include "sendinfo.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

int fd_internal_pipe[2];

void
sendinfo (int *fd, char *message, unsigned long window)
{
  static int w;
  static unsigned long win;

  win = window;
  if (message != NULL)
  {
    write (fd[0], &win, sizeof (unsigned long));
    w = strlen (message);
    write (fd[0], &w, sizeof (int));
    write (fd[0], message, w);
    w = 1;
    write (fd[0], &w, sizeof (int));
  }
}

int
readpacket (int fd, unsigned long *header, unsigned long **body)
{
  int count, total, count2, body_length;
  char *cbody;

  if ((count = read (fd, header, HEADER_SIZE * sizeof (unsigned long))) > 0)
  {
    if (header[0] == START_FLAG)
    {
      body_length = header[2] - HEADER_SIZE;
      *body = (unsigned long *) malloc (body_length * sizeof (unsigned long) + 1);
      cbody = (char *) (*body);
      total = 0;
      while (total < body_length * sizeof (unsigned long))
      {
	if ((count2 = read (fd, &cbody[total], body_length * sizeof (unsigned long) - total)) > 0)
	{
	  total += count2;
	}
      }
    }
    else
      count = 0;
  }
  return (count);
}
