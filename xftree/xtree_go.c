
/*
 * xtree_go.c: go routines for xftree
 *
 * Copyright (C) 1999 Rasca, Berlin
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
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pwd.h>
#include <grp.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_functions.h"
#include "xtree_gui.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_prop.h"
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
#include "xtree_pasteboard.h"
#include "xtree_go.h"
#include "xtree_icons.h"

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern GdkPixmap  *gPIX_dir_close,  *gPIX_dir_open;
extern GdkBitmap *gPIM_dir_close, *gPIM_dir_open;



static GList *
free_list (GList * list)
{
  g_list_free (list);
  return NULL;
}

golist *pushgo(char *path,golist *thisgo){
	char *p;
	golist *lastgo,*gogo;
	gogo=thisgo;	
	lastgo=gogo;
	gogo=(golist *)malloc(sizeof(golist));
	p=g_strdup(path);
	if ((!gogo)||(!p)){
		gogo=lastgo;
		if (p) g_free(p);
		return gogo;
	}
	gogo->previous=lastgo;
	if (strcmp(p,"/..")==0) strcpy(p,"/");
	if ((strlen(p)>3)&&(p[strlen(p)-1]=='.')&&(p[strlen(p)-2]=='.')&&(p[strlen(p)-3]=='/')){
		p[strlen(p)-3]=0;
		if (strrchr(p,'/')) *(strrchr(p,'/'))=0;	
		if ((strlen(p)==0) || (!strrchr(p,'/')))  strcpy(p,"/");		  
	}
	gogo->path=g_strdup(p);
	if (!(gogo->path)){
		free(gogo);
		g_free(p);
		gogo=lastgo;
		return gogo;
	}
	g_free(p);
	/*fprintf(stderr,"dbg: path pushed=%s\n",path);*/
	return gogo;
}

static void pushpath(GtkCTree * ctree,char *path){
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->gogo=pushgo( path,win->gogo);
  gtk_object_set_user_data (GTK_OBJECT (ctree),win);
  return;
}

golist *popgo(golist *thisgo){
	golist *gogo;
	gogo=thisgo;
	if (!gogo) return gogo;
	thisgo=gogo->previous;
	if (gogo->path) {
		/*fprintf(stderr,"dbg: path popped=%s\n",gogo->path);*/
		free(gogo->path);
	}
	free(gogo);
	gogo=thisgo;
	return gogo;
}

static void internal_go_to (GtkCTree * ctree, GtkCTreeNode * root, char *path, int flags)
{
  int i;
  char *label[COLUMNS];
  entry *en;
  cfg *win;
  struct passwd *pw;
  struct group *gr;
  
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
	/*fprintf(stderr,"dbg: go_to path=%s\n",path);*/
  if (strstr(path,"/..")) {
     if (strlen(path)<=3) return; /* no higher than root */
	*(strrchr(path,'/'))=0;
	if (strstr(path,"/")) *(strrchr(path,'/'))=0;
	if (path[0]==0) strcpy(path,"/");
  }    		
  en = entry_new_by_path (path);
  if (!en)
  {
    xf_dlg_error(win->top,_("Cannot find path"),path);
    /*fprintf (stderr,"dbg:Can't find row data at go_to()\n");*/
    return;
  }

  if (!io_is_valid (en->label)) return;
  en->flags = flags;

  /*fprintf(stderr,"dbg: 2go_to path=%s\n",path);*/
  pw=getpwuid(en->st.st_uid); 
  gr=getgrgid (en->st.st_gid); 
   
  for (i = 0; i < COLUMNS; i++)
  {
    if (i == COL_NAME)
      label[i] = (win->preferences&ABREVIATE_PATHS)? abreviate(en->path):en->path;   
    else if (i==COL_MODE) label[i] = mode_txt(en->st.st_mode); 
    else if (i==COL_UID) label[i] = (pw)? pw->pw_name : _("unknown"); 
    else if (i==COL_GID) label[i] = (gr)? gr->gr_name : _("unknown"); 
    else label[i] = "";
  }
  ctree_freeze (ctree);
  
  /* fprintf(stderr,"dbg: 3go_to path=%s\n",path); */
  gtk_ctree_remove_node (ctree, root);
  {
    icon_pix pix;
    set_icon_pix(&pix,en->type,en->label,en->flags);   
    root = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, NULL, label, 8, 
		  pix.pixmap,pix.pixmask,
		  pix.open,pix.openmask,
		  FALSE, TRUE);  
    en->type |= FT_ISROOT;
  }    
  gtk_ctree_node_set_row_data_full (ctree, root, en, node_destroy);
  add_subtree (ctree, root, en->path, 2, en->flags);
  reset_icon(ctree, root);
  gtk_ctree_select (GTK_CTREE (ctree), root);
  
  ctree_thaw (ctree);
  set_title_ctree (GTK_WIDGET (ctree), en->path);
#if 0
  icon_name = strrchr (en->path, '/');
  if ((icon_name) && (!(*(++icon_name)))) icon_name = NULL;
  gdk_window_set_icon_name (gtk_widget_get_toplevel (GTK_WIDGET (ctree))->window, (icon_name ? icon_name : "/"));
#endif

}

void regen_ctree(GtkCTree *ctree){
  GtkCTreeNode * root;
  entry *en;
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  root = find_root((GtkCTree *)ctree);
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
/*  gtk_clist_set_column_title((GtkCList *)ctree,COL_SIZE,
	(win->preferences & SIZE_IN_KB)?_("Size (Kb)"):_("Size (bytes)"));*/	  
  internal_go_to (ctree, root,en->path, en->flags);
  return;
}

void go_to (GtkCTree * ctree, GtkCTreeNode * root, char *path, int flags){
  cfg *win;
  pushpath(ctree,path);
  internal_go_to (ctree,root,path,flags);
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (win->source_set_sem) {
	  gtk_drag_source_unset ((GtkWidget *)ctree);
	  win->source_set_sem=FALSE;
  }
}

void cb_reload(GtkWidget * item, GtkCTree * ctree){
  cfg *win;
  regen_ctree(ctree);
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (win->source_set_sem) {
	  gtk_drag_source_unset ((GtkWidget *)ctree);
	  win->source_set_sem=FALSE;
  }
}


    /* A big bug was found and fixed at cb_go_to: Freeing memory twice
     * and appending glist elements to an already freed glist. */
void
cb_go_to (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *node, *root;
  entry *en;
  static GList *list=NULL;
  int count;
  cfg *win;
  golist *thisgo;
  char *entry_return;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (list != NULL) list = free_list (list);
  
  root = find_root((GtkCTree *)ctree);
  /* count selection returns root-node when count==0 ;-) */
  /* therefore, en != NULL */
  count = count_selection (ctree, &node); 
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  /* if double click is selected for goto, then doing the 
   * following line is totally unnecesary and hinders the speed 
   * of xftree, so it is disactivated */
  if (!(win->preferences & DOUBLE_CLICK_GOTO) && (count == 1) && (en->type & FT_DIR)) {
  	go_to (ctree, root, en->path, en->flags);
	return;
  }
   /* make combo box */
  if (win->gogo) for (thisgo=win->gogo->previous; thisgo!=NULL; thisgo=thisgo->previous){
	golist *testgo;
	for (testgo=thisgo->previous;testgo!=NULL;testgo=testgo->previous) {
		/* if ahead in list, dont put it in now */
		if (strcmp(testgo->path,thisgo->path)==0) goto skipit;
	}
	list = g_list_prepend (list,thisgo->path);
skipit:;
  } 
  /*entry_return = (char *)xf_dlg_combo (win->top,_("Go to"), "/", list);*/
  entry_return = (char *)xf_dlg_comboOK (win->top,_("Go to"), "/", list);
  if (!entry_return) return;
  go_to (ctree, root,entry_return , en->flags);
}

void cb_go_back (GtkWidget * item, GtkCTree * ctree){
  GtkCTreeNode *root;
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (!(win->gogo)) {
	  fprintf(stderr,"dbg:This shouldn't happen. cb_go_back()\n");
	  return; 
  }
  if ((win->gogo) && (win->gogo->previous)) {
    win->gogo=popgo (win->gogo); 
    root = find_root((GtkCTree *)ctree);
    internal_go_to (ctree, root, win->gogo->path, IGNORE_HIDDEN);
    gtk_object_set_user_data (GTK_OBJECT (ctree),win);
    if (win->source_set_sem) {
	  gtk_drag_source_unset ((GtkWidget *)ctree);
	  win->source_set_sem=FALSE;
    }   
  }
}


/*
 */
void
cb_go_home (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *root;
  root = find_root((GtkCTree *)ctree);
  go_to (ctree, root, 
	  (custom_home_dir)?custom_home_dir:getenv ("HOME"), 
	  IGNORE_HIDDEN);
}


/*
 * change root and go one directory up
 */
void
cb_go_up (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  static char *path=NULL;
  char *p;
  GtkCTreeNode *root;

  root = find_root((GtkCTree *)ctree);
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
  if (!en) return;
  if (path) free(path);
  if (!(path=(char *)malloc(strlen(en->path)+1) ) ) return;
  strcpy (path, en->path);
	/*fprintf(stderr,"dbg: go_up=%s\n",path);*/
  p = strrchr (path, '/');
  if (p == path)
  {
    if (!*(p + 1)) return;
    *(p + 1) = '\0';
  }
  else *p = '\0';
  go_to (ctree, root, path, en->flags);
}


