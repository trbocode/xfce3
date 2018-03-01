/*
 * uri.c
 *
 * Copyright (C) 1998 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 * 
 * Edscott Wilson Garcia 2001, for xfce project
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
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <glib.h>
#include "uri.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/*
 */
int
uri_remove_file_prefix (char *path, int len)
{
  char *p;
  int striped = 0;
  if ((p = strstr (path, "file:///")) != NULL)
  {
    if ((p + 7) <= (path + len))
    {
      memmove (path, path + 7, len - 7);
      path[len - 7] = '\0';
      striped = 7;
    }
  }
  else if ((p = strstr (path, "file://")) != NULL)
  {
    if ((p + 6) <= (path + len))
    {
      memmove (path, path + 6, len - 6);
      path[len - 6] = '\0';
      striped = 6;
    }
  }
  else if ((p = strstr (path, "file:/")) != NULL)
  {
    if ((p + 5) <= (path + len))
    {
      memmove (path, path + 5, len - 5);
      path[len - 5] = '\0';
      striped = 5;
    }
  }
  return (striped);
}

/*
 */
int
uri_remove_file_prefix_from_list (GList * list)
{
  int rc = 0;
  uri *u;

  while (list)
  {
    u = (uri *) list->data;
    if (u->type == URI_FILE)
    {
      rc = uri_remove_file_prefix (u->url, u->len);
      u->len -= rc;
      u->type = URI_LOCAL;
    }
    list = list->next;
  }
  return (rc);
}

/*
 * remove ".." and trailing "/" and "/." from a path
 */
char *
uri_clear_path (const char *org_path)
{
  static char *path;
  char *p, *ld;
  int len;

  if (!org_path) return (NULL);
  if (path) free(path);
  path=(char *)malloc(strlen(org_path)+1);
  if (!path) return (NULL);
  strcpy (path, org_path);
  /* remove ".." */
    /*fprintf (stderr,"dbg:uri:%s\n",path);*/
  
  p = path + 1;
  ld = path;
  while (*p)
  {
    if (*p == '/')
    {
      if (*(p + 1) != '\0')
      {
	if (!((*(p + 1) == '.') && (*(p + 2) == '.')))
	{
	  ld = p;
	}
      }
      else
      {
	break;
      }
    }
    else
    {
      if ((*(p - 1) == '/') && (*p == '.') && (*(p + 1) == '.'))
      {
	len = strlen (p + 2);
	if (!len)
	{
	  *(ld + 1) = '\0';
	}
	else
	{
	  memmove (ld, p + 2, len + 1);
	}
	ld = p = path;
      }
    }
    p++;
  }
  /* remove trailing '/' and '/.'
   */
  while (1)
  {
    len = strlen (path);
    if (len > 1)
    {
      if (path[len - 1] == '/')
	path[len - 1] = '\0';
      else if ((path[len - 2] == '/') && (path[len - 1] == '.'))
      {
	if (len == 2)
	  path[len - 1] = '\0';
	else
	  path[len - 2] = '\0';
      }
      else
	break;
    }
    else
    {
      break;
    }
  }
  return (path);
}

/*
 */
int
uri_type (char *s)
{
  if (*s == '/') return (URI_LOCAL);
  if (strncmp (s, "file:/", 6) == 0)  return (URI_FILE);
  if (strncmp (s, "http:/", 6) == 0)  return (URI_HTTP);
  if (strncmp (s, "ftp:/", 5) == 0)   return (URI_FTP);
  if (strncmp (s, "tar:/", 5) == 0)   return (URI_TAR);
  if (strncmp (s, "smb:/", 5) == 0)   return (URI_SMB);
  return (URI_LOCAL);
}

/*
 */
int
uri_parse_list (const char *text, GList ** list)
{
  int num, org_len, len, tlen, i, no;
  const char *p, *end;
  uri *u;

  if (!text)
    return 0;
  
 
  /*fprintf(stderr,"uri:%s\n",text);*/

  *list = NULL;
  org_len = strlen (text);
  p = text;
  num = 0;
  while ((p = strchr (p, '\n')) != NULL)
  {
    p++;
    num++;
  }
  if ((!num) || (text[org_len - 1] != '\n'))
    num++;
  p = text;
  no = num;
  for (i = 0; i < num; i++)
  {
    tlen = 2;			/* terminator length */
    end = strchr (p, '\r');
    if (!end)
    {
      end = strchr (p, '\n');
      tlen = 1;
    }
    if (end)
      end--;
    else
      end = text + org_len - 1;
    len = end - p + 1;
    if ((len > 0) && (*p != '#'))
    {
      u = g_malloc (sizeof (uri));
      if (!u)
	return (0);
      u->url = g_malloc (len + 1);
      strncpy (u->url, p, len);
      u->url[len] = '\0';
      /*fprintf(stderr,"uri:%s\n",u->url);*/
      u->len = len;
      u->type = uri_type (u->url);
      if (u->len > URI_MAX)
      {
	u->len = URI_MAX;
	u->url[URI_MAX] = '\0';
      }
      *list = g_list_append (*list, u);
    }
    else
    {
      no--;
    }
    p = p + len + tlen;

  }
  return (no);
}

/*
 */
GList *
uri_free_list (GList * list)
{
  GList *t = list;
  while (t)
  {
    g_free (((uri *) t->data)->url);
    g_free ((uri *) (t->data));
    t = t->next;
  }
  g_list_free (list);
  return NULL;
}

/*
 */
char *
uri_to_quoted_list (GList * list)
{
  int nitems = 0;
  int len = 0;
  char *p, *string, quote;
  GList *t = list;
  uri *u;

  /* count items and sum the lenght */
  while (t)
  {
    len += ((uri *) t->data)->len;
    nitems++;
    t = t->next;
  }
  string = p = g_malloc (len + (nitems * 3) + 1);
  p[len + (nitems * 3)] = '\0';
  while (list)
  {
    u = (uri *) list->data;
    list = list->next;
    if (!u)
      continue;
    if (strchr (u->url, '\''))
      quote = '"';
    else
      quote = '\'';
    *p++ = quote;
    memcpy (p, u->url, u->len);
    p += u->len;
    *p++ = quote;
    *p++ = ' ';
  }
  return (string);
}

/*
 */
char *
uri_basename (const char *path)
{
  char *p;
  if (!path)
    return (NULL);
  p = strrchr (path, '/');
  if (p)
  {
    p++;
    if (*p != '\0')
      return (p);
  }
  return (NULL);
}
