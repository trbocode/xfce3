/* Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifndef HAVE_SCANDIR

#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int
scandir (const char *dir, struct dirent ***namelist, int (*select) (), int (*cmp) ())
{
  DIR *dp = opendir (dir);
  struct dirent **v = NULL;
  size_t vsize = 0, i;
  struct dirent *d;
  int save;

  if (dp == NULL)
    return -1;

  save = errno;
  errno = 0;

  i = 0;
  while ((d = readdir (dp)) != NULL)
    if (select == NULL || (*select) (d))
    {
      if (i == vsize)
      {
	struct dirent **new;
	if (vsize == 0)
	  vsize = 10;
	else
	  vsize *= 2;
	new = (struct dirent **) realloc (v, vsize * sizeof (*v));
	if (new == NULL)
	{
	lose:
	  errno = ENOMEM;
	  break;
	}
	v = new;
      }

      v[i] = (struct dirent *) malloc (sizeof (**v));
      if (v[i] == NULL)
	goto lose;

      *v[i++] = *d;
    }

  if (errno != 0)
  {
    save = errno;
    (void) closedir (dp);
    while (i > 0)
      free (v[--i]);
    free (v);
    errno = save;
    return -1;
  }

  (void) closedir (dp);
  errno = save;

  /* Sort the list if we have a comparison function to sort with.  */
  if (cmp != NULL)
    qsort (v, i, sizeof (*v), cmp);
  *namelist = v;
  return i;
}

int
alphasort (struct dirent *a, struct dirent *b)
{
  return strcmp (((struct dirent *) a)->d_name, ((struct dirent *) b)->d_name);
}

#else
 /* keep compilers happy about empty files */
void
dummy_scandir (void)
{
}
#endif /* !HAVE_SCANDIR */
