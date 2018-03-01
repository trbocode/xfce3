/*
 * reg.c
 *
 * Copyright (C) 1998, 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia 2001-2002 for xfce project.
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
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_gui.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_prop.h"
#include "reg.h"
#include "gtk_dnd.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "xtree_mess.h"

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static char *regfile = NULL;
extern gboolean sane (char *bin);


/*
 */
static int
compare_sfx (reg * reg1, reg * reg2)
{
  return my_strcasecmp (reg1->sfx, reg2->sfx);
}

void
reg_free_reg (reg_t * app)
{
  if (!app)
    return;
  if (app->app)
    g_free (app->app);
  if (app->arg)
    g_free (app->arg);
  if (app->sfx)
    g_free (app->sfx);
  g_free (app);
}

/*
 * on startup we build a complete list
 */
GList *
reg_build_list (char *file)
{
#define BUFSIZE 1024
  FILE *fp;
  GList *g_reg = NULL;
  char buf[BUFSIZE], *p, *ep, *buf_end;
  char suffix[256];
  char cmd[PATH_MAX + 1];
  char arg[PATH_MAX + 1];
  reg_t *prg;
  int inner;
  int line;

  regfile = g_strdup (file);
  fp = fopen (file, "r");
  if (!fp)
  {
    printf ("Warning, can't find %s\n", file);
    return (NULL);
  }
  buf_end = buf + BUFSIZE - 1;
  line = 0;
  while (fgets (buf, BUFSIZE, fp) != NULL)
  {
    line++;
    p = buf;
    while ((*p == ' ') || (*p == '\t'))
      p++;
    if (*p == '#')
      continue;
    prg = g_malloc (sizeof (reg_t));
    if (!prg)
    {
      perror ("malloc()");
      fclose (fp);
      return (g_reg);
    }
    prg->app = prg->sfx = prg->arg = NULL;
    if (*p != '(')
    {
      /* old style */
      *arg = '\0';
      sscanf (buf, "%s %[^ \n] %[^\n]", suffix, cmd, arg);
      prg->sfx = g_strdup (suffix);
      prg->app = g_strdup (cmd);
      if (*arg)
	prg->arg = g_strdup (arg);
      prg->len = strlen (suffix);
      g_reg = g_list_append (g_reg, prg);
    }
    else
    {
      /* new style:
       * (suffix)(program)(arguments)
       */
      inner = 0;
      p++;
      ep = p;
      while (*ep && (ep < buf_end))
      {
	if (*ep == '(')
	  inner++;
	else if (*ep == ')')
	{
	  if (inner)
	    inner--;
	  else
	    break;
	}
	ep++;
      }
      if (*ep != ')')
      {
	/* parse error */
	fprintf (stderr, "parse error at line %d\n", line);
	reg_free_reg (prg);
	continue;
      }
      *ep = '\0';
      prg->sfx = g_strdup (p);
      prg->len = strlen (prg->sfx);

      /* find program */
      p = ep + 1;
      while (*p && (p < buf_end))
      {
	if (*p == '(')
	  break;
	p++;
      }
      if (*p != '(')
      {
	/* parse error */
	fprintf (stderr, "parse error at line %d\n", line);
	reg_free_reg (prg);
	continue;
      }
      inner = 0;
      p++;
      ep = p;
      while (*ep && (ep < buf_end))
      {
	if (*ep == '(')
	  inner++;
	else if (*ep == ')')
	{
	  if (inner)
	    inner--;
	  else
	    break;
	}
	ep++;
      }
      if (*ep != ')')
      {
	/* parse error */
	fprintf (stderr, "parse error at line %d\n", line);
	reg_free_reg (prg);
	continue;
      }
      *ep = '\0';
      prg->app = g_strdup (p);

      /* find args */
      p = ep + 1;
      while (*p && (p < buf_end))
      {
	if (*p == '(')
	  break;
	p++;
      }
      if (*p == '(')
      {
	inner = 0;
	p++;
	ep = p;
	while (*ep && (ep < buf_end))
	{
	  if (*ep == '(')
	    inner++;
	  else if (*ep == ')')
	  {
	    if (inner)
	      inner--;
	    else
	      break;
	  }
	  ep++;
	}
	if (*ep != ')')
	{
	  /* parse error */
	  fprintf (stderr, "parse error at line %d\n", line);
	  reg_free_reg (prg);
	  continue;
	}
	*ep = '\0';
	if (p != ep)
	  prg->arg = g_strdup (p);
      }
      /* check for coherent values */
      /* only if  DISPLAY is :0.0 to avoid false removes from remotes*/
      /* if it is a directory register, does the directory exist? */
      if (strcmp(getenv("DISPLAY"),":0.0")==0) {
       if (*(prg->sfx)=='/'){
	      struct stat s;
	      if (stat(prg->sfx,&s)<0)       {
	         fprintf (stderr, "xftree:removing %s from xtree.reg\n",prg->sfx);
		 reg_free_reg (prg);
		 continue;
	      }
       }
       /* does the application exist? glob the path */
       if (!sane(prg->app)){
          fprintf (stderr, "xftree:removing %s application from xtree.reg\n",prg->app);
	  reg_free_reg (prg);
	  continue;      
       }
      }
      g_reg = g_list_append (g_reg, prg);
    }
  }
  fclose (fp);
  return (g_reg);
}

/*
 */
char *
reg_app_by_suffix (GList * g_reg, char *sfx)
{
  reg *prg;
  while (g_reg)
  {
    prg = g_reg->data;
    if (my_strcasecmp (prg->sfx, sfx) == 0)
    {
      return (prg->app);
    }
    g_reg = g_reg->next;
  }
  return (NULL);
}

/*
 */
reg_t *
reg_prog_by_suffix (GList * g_reg, char *sfx)
{
  reg_t *prg;
  while (g_reg)
  {
    prg = g_reg->data;
    if (my_strcasecmp (prg->sfx, sfx) == 0)
    {
      return (prg);
    }
    g_reg = g_reg->next;
  }
  return (NULL);
}

/*
 */
char *
reg_app_by_file (GList * g_reg, char *file)
{
  reg *prg;
  int len;
  static char *app_arg=NULL;

  if (app_arg) g_free(app_arg);

  if (!file) return NULL;
  len = strlen (file);

  while (g_reg)
  {
    prg = g_reg->data;
    if ((!prg) || (prg->len > len))
    {
      g_reg = g_reg->next;
      continue;
    }
    if (my_strcasecmp (file + (len - prg->len), prg->sfx) == 0){
      int len;
      len=(prg->arg)?strlen(prg->arg):0;
      len+=strlen(prg->app);
      len+=2;
      app_arg=(char *) malloc(len);
      if (app_arg) {
        if (prg->arg) sprintf(app_arg,"%s %s",prg->app,prg->arg); 
        else  sprintf(app_arg,"%s",prg->app);
      }
      return (app_arg);
    }
    g_reg = g_reg->next;
  }
  return (NULL);
}

/*
 */
reg_t *
reg_prog_by_file (GList * g_reg, char *file)
{
  reg_t *prg;
  int len;

  if (!file)
    return NULL;
  len = strlen (file);

  while (g_reg)
  {
    prg = g_reg->data;
    if ((!prg) || (prg->len > len))
    {
      g_reg = g_reg->next;
      continue;
    }
    if (my_strcasecmp (file + (len - prg->len), prg->sfx) == 0)
    {
      return (prg);
    }
    g_reg = g_reg->next;
  }
  return (NULL);
}

/*
 */
GList *
reg_add_suffix (GList * g_reg, char *sfx, char *program, char *args)
{
  reg *prg = NULL;
  GList *g_tmp = g_reg;
  int found = 0;
  /* is suffix already registered?
   * check it out ..
   */
  while (g_tmp)
  {
    prg = g_tmp->data;
    if (!my_strcasecmp (prg->sfx, sfx))
    {
      found = 1;
      break;
    }
    g_tmp = g_tmp->next;
  }
  if (found)
  {
    g_free (prg->app);
    g_free (prg->arg);
    prg->app = g_strdup (program);
    prg->arg = NULL;
    if (args)
    {
      prg->arg = g_strdup (args);
    }
  }
  else
  {
    prg = g_malloc (sizeof (reg));
    prg->app = g_strdup (program);
    prg->sfx = g_strdup (sfx);
    prg->len = strlen (sfx);
    if (args)
    {
      prg->arg = g_strdup (args);
    }
    else
    {
      prg->arg = NULL;
    }
    g_reg = g_list_append (g_reg, prg);
  }
  if (g_reg) g_reg = g_list_sort (g_reg, (GCompareFunc) compare_sfx);
  return g_reg;
}

/*
 */
void
reg_destroy_list (GList * list)
{
  GList *g_tmp = list;
  reg *prg;
  while (g_tmp)
  {
    prg = (reg *) g_tmp->data;
    g_free (prg->sfx);
    g_free (prg->app);
    if (prg->arg) g_free (prg->arg);
    g_free (prg);
    g_tmp = g_tmp->next;
  }
  g_list_free (list);
  list=NULL;
}


/*
 * save registry to named file
 */
int
reg_save (GList * g_reg)
{
  FILE *fp;
  reg *prg;

  if (!regfile)
    return (0);
  fp = fopen (regfile, "w");
  if (!fp)
  {
    perror (regfile);
    return (FALSE);
  }
  fprintf (fp, "# (suffix) (program) (args)\n");
  while (g_reg)
  {
    prg = g_reg->data;
    if (prg)
    {
      if (prg->arg)
      {
	fprintf (fp, "(%s) (%s) (%s)\n", prg->sfx, prg->app, prg->arg);
      }
      else
      {
	fprintf (fp, "(%s) (%s) ()\n", prg->sfx, prg->app);
      }
    }
    g_reg = g_reg->next;
  }
  fclose (fp);
  return (TRUE);
}

/*
 * return a list of all registered applications with arguments
 */
GList *
reg_app_list (GList * g_reg)
{
  GList *g_apps = NULL;
  GList *g_tmp;
  char *app_arg;

  while (g_reg)
  {
    char *app,*arg;
    int len;
    app=((reg *) g_reg->data)->app;
    arg=((reg *) g_reg->data)->arg;
    len=(arg)?strlen(arg):0;
    len+=strlen(app);
    len+=2;
    app_arg=(char *) malloc(len);
    if (app_arg) {
      if (arg) sprintf(app_arg,"%s %s",app,arg); 
      else  sprintf(app_arg,"%s",app);
      g_apps = g_list_append (g_apps, app_arg);
    }
    g_reg = g_reg->next;
  }
  if (g_apps) g_apps = g_list_sort (g_apps, (GCompareFunc) my_strcasecmp);
  /* remove dupes */
  g_tmp = g_apps;
  while (g_tmp)
  {
    if (g_tmp->next)
    {
      if (my_strcasecmp ((char *) g_tmp->data, (char *) g_tmp->next->data) == 0)
      {
	char *rem;
	rem=g_tmp->data;
	g_tmp = g_apps = g_list_remove (g_apps, rem);
	g_free(rem);
	continue;
      }
    }
    if (!g_tmp) break;
    else g_tmp = g_tmp->next;
  }
  return (g_apps);
}

GList *
reg_app_list_free (GList * g_apps)
{
  char *rem;
  GList *g_tmp;
  g_tmp=g_apps;
  while (g_tmp)
  {
    rem=g_tmp->data;
    if (rem) {
	    g_tmp = g_apps = g_list_remove (g_apps, rem);
	    g_free(rem);
    }
    if (!g_tmp) break;
    else g_tmp = g_tmp->next;
  }
  return (g_tmp);
}

/*
 */
int
reg_execute (reg_t * app, char *file)
{
  return 0;
}

/*
 * ask user if he want to register a named suffix
 */
void
cb_register (GtkWidget * item, GtkWidget * ctree)
{
  GtkCTreeNode *node;
  char label[PATH_MAX + 1];
  char path[PATH_MAX + 1];
  char *entry_return;
  char *sfx, *arg;
  entry *en;
  cfg *win;
  GList *apps=NULL;
  reg_t *prog;

  if (!GTK_CLIST (ctree)->selection)
    return;
  cursor_wait (GTK_WIDGET (ctree));
  gtk_clist_freeze (GTK_CLIST (ctree));
  node = GTK_CLIST (ctree)->selection->data;
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (en->type & FT_TARCHILD) {
	xf_dlg_error(win->top,_("This function is not available for the contents of tar files"),NULL);
  	gtk_clist_thaw (GTK_CLIST (ctree));
  	cursor_reset (GTK_WIDGET (ctree)); 
	return;
  }

  if ((en->type & FT_DIR)&& !(en->type &(FT_RPM|FT_TAR))) {
    sfx=en->path;
    sprintf (label, _("Register program for directory \"%s\""), en->path);
    /* FIXME: on xftree startup, remove directory regitrations that are not found */
  }
  else {
   sfx = strrchr (en->label, '.');
   if (!sfx)
   {
    if (xf_dlg_continue (win->top,_("Can't find suffix in filename, using complete filename"), en->label) != DLG_RC_OK)
    {
      gtk_clist_thaw (GTK_CLIST (ctree));
      cursor_reset (GTK_WIDGET (ctree));
      return;
    }
    sfx = en->label;
    sprintf (label, _("Register program for file \"%s\""), sfx);
   }
   else
   {
    sprintf (label, _("Register program for suffix \"%s\""), sfx);
   }
  }
  
  prog = reg_prog_by_suffix (win->reg, sfx);
  if (prog)
  {
    if (prog->arg)
    {
      snprintf (path, PATH_MAX, "%s %s", prog->app, prog->arg);
    }
    else
    {
      strcpy (path, prog->app);
    }
  }
  else
    strcpy (path, DEF_APP);
  apps = reg_app_list (win->reg);
  entry_return = (char *) xf_dlg_comboOK (win->top,label, path, apps);
  if (entry_return)
  {
    if (strlen(entry_return))
    {
      if ((arg = strchr (entry_return, ' ')) != NULL)
      {
	*arg++ = '\0';
	if (!*arg)
	  arg = NULL;
      }
      if (!sane(entry_return)){
          xf_dlg_error (win->top,_("Can't find in PATH"),entry_return);
      }
      else {
	 win->reg = reg_add_suffix (win->reg, sfx, entry_return, arg);
         reg_save (win->reg);
      }
    }
  }
  gtk_clist_thaw (GTK_CLIST (ctree));
  cursor_reset (GTK_WIDGET (ctree)); 
  g_list_free (apps);
}


