
/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/****************************************************************************
 *
 * Routines to handle initialization, loading, and removing of xpm's or mono-
 * icon images.
 *
 ****************************************************************************/

#include "configure.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/xpm.h>

#include "utils.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static Colormap PictureCMap;

void
InitPictureCMap (Display * dpy, Window Root)
{
  XWindowAttributes root_attr;
  XGetWindowAttributes (dpy, Root, &root_attr);
  PictureCMap = root_attr.colormap;
}

char *
findIconFile (char *icon, char *pathlist, int type)
{
  char *path;
  char *dir_end;
  int l1, l2;

  if (icon != NULL)
    l1 = strlen (icon);
  else
    l1 = 0;

  if (pathlist != NULL)
    l2 = strlen (pathlist);
  else
    l2 = 0;

  path = safemalloc (l1 + l2 + 10);
  *path = '\0';
  if (*icon == '/')
  {
    /* No search if icon begins with a slash */
    strcpy (path, icon);
    return path;
  }

  if ((pathlist == NULL) || (*pathlist == '\0'))
  {
    /* No search if pathlist is empty */
    strcpy (path, icon);
    return path;
  }

  /* Search each element of the pathlist for the icon file */
  while ((pathlist) && (*pathlist))
  {
    dir_end = strchr (pathlist, ':');
    if (dir_end != NULL)
    {
      strncpy (path, pathlist, dir_end - pathlist);
      path[dir_end - pathlist] = 0;
    }
    else
      strcpy (path, pathlist);

    strcat (path, "/");
    strcat (path, icon);
    if (access (path, type) == 0)
      return path;
    strcat (path, ".gz");
    if (access (path, type) == 0)
      return path;

    /* Point to next element of the path */
    if (dir_end == NULL)
      pathlist = NULL;
    else
      pathlist = dir_end + 1;
  }
  /* Hmm, couldn't find the file.  Return NULL */
  free (path);
  return NULL;
}
