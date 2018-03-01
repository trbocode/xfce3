/*
 * gtk_get.c
 *
 * Copyright (C) 1998 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia Copyright 2001-2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H   
#  include <config.h>   
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <gtk/gtk.h>
#include "my_intl.h"
#include "uri.h"
#include "io.h"
#include "gtk_get.h"
#include "gtk_dlg.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifndef URLFETCH
#define URLFETCH "curl"
#endif

int
download (char *tgt,char *src)
{
#define BUFLEN 8192
  char target[PATH_MAX + 1];
  char cmd[URI_MAX + (PATH_MAX * 2) + 1];
  char buf[BUFLEN];
  int num, len, buflen, bps;
  FILE *fp, *pipe;
  time_t start_time, elapsed;

  fprintf(stderr,"dbg: download %s->%s\n",src,tgt);
  if (io_is_directory (tgt))
  {
    sprintf (target, "%s/%s",tgt, uri_basename (src));
  }
  else
  {
    sprintf (target, "%s", tgt);
  }

  sprintf (cmd, "%s %s", URLFETCH, src);
  fp = fopen (target, "wb");
  if (!fp) {
    fprintf(stdout,"child:%s %s\n",strerror(errno),target);
    return 0;
  }
  pipe = popen (cmd, "r");
  if (!pipe) {
    fprintf(stdout,"child:%s %s\n",strerror(errno),cmd);
    fclose (fp);
    return 0;
  }

   /* read the data from the pipe */
  len = 0;
  start_time = time (NULL);
  buflen = 16;
  while ((num = fread (buf, 1, buflen, pipe)) > 0)
  {
    fwrite (buf, 1, num, fp);
    len += num;
    elapsed = time (NULL) - start_time;
    bps = len / (elapsed > 0 ? elapsed : 1);
    buflen = BUFLEN < bps ? BUFLEN : bps;
    buflen = buflen < 32 ? 32 : buflen;
    fprintf (stdout, "child:bytes:%d", len);
    fprintf (stdout, _(" bytes received"));
    fprintf (stdout,"(%d bps)",  bps);
  }
  fclose (fp);
  fclose (pipe);
  return (TRUE);
}
