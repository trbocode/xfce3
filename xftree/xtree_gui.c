/*
 * xtree_gui.c
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

#define __XFTREE_GUI_MAIN__

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
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
#include <gtk/gtkenums.h>
#include <X11/Xatom.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_gui.h"
#include "xtree_functions.h"
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
#include "xtree_cb.h"
#include "xtree_toolbar.h"
#include "xtree_cpy.h"
#include "xtree_icons.h"
#include "xtree_tar.h"
#include "icons/xftree_icon.xpm"


#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif



static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URI_LIST},
  {"text/plain", 0, TARGET_PLAIN},
  {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))


#define ACCEL	2
typedef struct
{
  char *label;
  void *func;
  int data;
  gint key;
  guint8 mod;
}
menu_entry;


static GtkAccelGroup *accel;

/* autotype entries will appear in file-menu when no
 * application is registered for that filetype 
 * leave a blank at end if using options (to be safe) */

autotype_t autotype[]= {
	{".tar.gz","tar -xzf ",N_("Extract files from"),N_("Extract into")},
	{".gz","gunzip",N_("Uncompress"),NULL},
	{".tar","tar -xf ",N_("Extract files from"),N_("Extract into")},
	{".tgz","tar -xzf ",N_("Extract files from"),N_("Extract into")},
	{".tar.bz2","tar --use-compress-program bunzip2 -xf",N_("Extract files from"),N_("Extract into")},
	{".bz2","bunzip2",N_("Uncompress"),NULL},
	{".Z","uncompress",N_("Uncompress"),NULL},
	{".ZIP","unzip",N_("Uncompress"),NULL},
	{".zip","unzip",N_("Uncompress"),NULL},
	{".rpm","rpm -U --percent ",N_("Install/update"),NULL},
	{".deb","dpkg -i ",N_("Install/update"),NULL},
	{NULL,NULL}
};

/* keyboard shortcuts used to be bugged because of conflicting entries.
 * these macros should make it easier to avoid conflicting entries.
 * Please place any duplicate entries as a macro. (since all entries
 * duplicate at least with help menu, all should be here).
 *
 * All should appear in help menu, if not, something is wrong
 * (delete is the only exception).
 * 
 * note: "unselect all" was eliminated since the function call was identical
 * to plain unselect. 
 * */

#define AUTOTYPE_MENU \
    {"", (gpointer) cb_autotype, 0}
    
#define AUTOTAR_MENU \
    {"", (gpointer) cb_autotar, 0,}
    
#define MAINF_ICON_VIEW \
    {N_("Open icon view"), (gpointer) cb_rox, 0, GDK_y,GDK_CONTROL_MASK}
    
#define MAINF_DIRECTORY_MENU \
    {N_("Open tree view"), (gpointer) cb_new_window, 0, GDK_w,GDK_CONTROL_MASK},\
    {N_("Open terminal view"), (gpointer) cb_term, 0, GDK_t,GDK_CONTROL_MASK}, \
    {NULL, NULL, 0}, \
    {N_("New Folder"), (gpointer) cb_new_subdir, 0, GDK_n,GDK_MOD1_MASK},\
    {N_("New file"), (gpointer) cb_new_file, 0,GDK_n,GDK_CONTROL_MASK}

#define MAINF_MENU \
    {NULL, NULL, 0}, \
    {N_("Touch ..."), (gpointer) cb_touch, 0, GDK_a,GDK_MOD1_MASK}, \
    {N_("Symlink ..."), (gpointer) cb_symlink, 0, GDK_l,GDK_MOD1_MASK}, \
    {N_("Delete ..."), (gpointer) cb_delete, 0}
    
#define INSERT_MENU \
    {N_("Paste"), (gpointer) cb_paste, 0, GDK_i,GDK_CONTROL_MASK}
    
#define DIRECTORY_MENU \
    {N_("Find ..."), (gpointer) cb_find, 0, GDK_f,GDK_CONTROL_MASK},\
    {N_("Show disk usage ..."), (gpointer) cb_du, 0, GDK_v,GDK_CONTROL_MASK}
     
#define DIR_FILE_MENU \
    {N_("Cut"), (gpointer) cb_cut, 0, GDK_k,GDK_CONTROL_MASK},\
    {N_("Copy"), (gpointer) cb_copy, 0, GDK_c,GDK_CONTROL_MASK}
    
#define MAIN_EDIT_MENU \
    {N_("List pasteboard contents"), (gpointer) cb_paste_show, 0, GDK_l,GDK_CONTROL_MASK},\
    {N_("Clear pasteboard"), (gpointer) cb_clean_pasteboard, 0, GDK_j,GDK_CONTROL_MASK},\
    {NULL, NULL, 0},\
    {N_("Select all"), (gpointer) cb_select, 0, GDK_s,GDK_CONTROL_MASK},\
    {N_("Unselect"), (gpointer) cb_unselect, 0,GDK_u,GDK_CONTROL_MASK},\
    {N_("Toggle Dotfiles"), (gpointer) on_dotfiles, 0, GDK_d,GDK_CONTROL_MASK},\
    {NULL, NULL, 0},\
    {N_("Open Trash"), (gpointer) cb_open_trash, 0, GDK_o,GDK_CONTROL_MASK},\
    {N_("Empty Trash"), (gpointer) cb_empty_trash, 0, GDK_e,GDK_CONTROL_MASK}
   
    
#define DIR_OR_FILE_MENU \
    {N_("Duplicate"), (gpointer) cb_duplicate, 0, GDK_c,GDK_MOD1_MASK}, \
    {N_("Rename ..."), (gpointer) cb_rename, 0, GDK_r,GDK_CONTROL_MASK},\
    {N_("Properties ..."), (gpointer) cb_props, 0, GDK_p,GDK_CONTROL_MASK},\
    {NULL, NULL, 0} 
     
#define COMMON_MENU_GOTO \
    {N_("Go to ..."), (gpointer) cb_go_to, 0, GDK_g,GDK_MOD1_MASK} 
    
#define TAR_MENU \
     {N_("Open with ..."), (gpointer) cb_tar_open_with, 0}
   
#define FILE_MENU \
     {N_("Open with ..."), (gpointer) cb_open_with, 0, GDK_o,GDK_MOD1_MASK}

#define REGISTER_MENU \
     {N_("Register ..."), (gpointer) cb_register, 0, GDK_r,GDK_MOD1_MASK}
     
#define SIMPLE_GOTO_MENU \
    {N_("Go to ..."), (gpointer) cb_go_to, 0, GDK_g,GDK_MOD1_MASK}
    
#define GOTO_MENU \
    {N_("Go home"), (gpointer) cb_go_home, 0,GDK_h,GDK_CONTROL_MASK},\
    {N_("Go back"), (gpointer) cb_go_back, 0,GDK_b,GDK_CONTROL_MASK},\
    {N_("Go up"), (gpointer) cb_go_up, 0,GDK_a,GDK_CONTROL_MASK},\
    {N_("Update"), (gpointer) cb_reload, 0,GDK_u,GDK_MOD1_MASK}
    
 
#define TOOLS_MENU \
    {N_("Run program ..."), (gpointer) cb_exec, WINCFG,GDK_x,GDK_CONTROL_MASK},\
    {N_("Find ..."), (gpointer) cb_find, 0, GDK_f,GDK_CONTROL_MASK},\
    {N_("Differences ..."), (gpointer) cb_diff, 0, GDK_i,GDK_MOD1_MASK},\
    {N_("Patch viewer ..."), (gpointer) cb_patch, 0, GDK_p,GDK_MOD1_MASK},\
    {N_("Browse SMB network ..."), (gpointer) cb_samba, 0, GDK_w,GDK_MOD1_MASK},\
    {N_("Show disk usage ..."), (gpointer) cb_du, 0, GDK_v,GDK_CONTROL_MASK}
    
#define NONE_MENU \
    {N_("Close window"), (gpointer) cb_destroy, 0, GDK_z,GDK_CONTROL_MASK}, \
    {N_("Quit ..."), (gpointer) cb_quit, 0, GDK_q,GDK_CONTROL_MASK}
    
/* quit only on main menu now, so that default geometry is saved correctly with cb_quit.*/
    
#define VIEW_MENU \
    {N_("Subsort by file type"),cb_subsort, SUBSORT_BY_FILETYPE, GDK_t, GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Enable filter"),cb_filter,FILTER_OPTION, GDK_f,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Abbreviate paths"),cb_abreviate,ABREVIATE_PATHS, GDK_a,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {NULL, NULL, 0}, \
    {N_("Status box"),cb_show_status, SHOW_STATUS, GDK_m,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Status titles"),cb_status_follows_expand,STATUS_FOLLOWS_EXPAND, GDK_y,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Short titles"),cb_short_titles,SHORT_TITLES, GDK_w,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {NULL, NULL, 0}, \
    {N_("Hide toolbar"),toggle_toolbar, HIDE_TOOLBAR, GDK_t,GDK_MOD1_MASK}, \
    {N_("Hide titles"), cb_hide_titles, HIDE_TITLES, GDK_h,GDK_MOD1_MASK}, \
    {N_("Hide sizes"), cb_hide_size, HIDE_SIZE, GDK_s,GDK_MOD1_MASK}, \
    {N_("Hide dates"),cb_hide_date, HIDE_DATE, GDK_d,GDK_MOD1_MASK}, \
    {N_("Hide owner"),cb_hide_uid, HIDE_UID, GDK_o,GDK_MOD1_MASK}, \
    {N_("Hide group"),cb_hide_gid, HIDE_GID, GDK_y,GDK_MOD1_MASK}, \
    {N_("Hide mode"),cb_hide_mode, HIDE_MODE, GDK_e,GDK_MOD1_MASK}, \
    {N_("Hide ../ "),cb_hide_dd, HIDE_DD, GDK_f,GDK_MOD1_MASK }
 
#define HELP_MENU \
    {N_("Sort by file name"), NULL, SORT_NAME, GDK_n, GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by file size"), NULL, SORT_SIZE, GDK_s,GDK_CONTROL_MASK | GDK_MOD1_MASK}, \
    {N_("Sort by file date"), NULL, SORT_DATE, GDK_d,GDK_CONTROL_MASK | GDK_MOD1_MASK}


    
char *mode_txt(mode_t mode){
	static char modeT[11];
	modeT[10]=0;
	if (S_ISDIR(mode)) modeT[0]='d'; else modeT[0]='-';
	if (mode&0400) modeT[1]='r'; else modeT[1]='-';
	if (mode&0200) modeT[2]='w'; else modeT[2]='-';
	if ((mode&04000)&&(mode&0100)) modeT[3]='s'; 
	else if (mode&04000) modeT[3]='S';
	else if (mode&0100) modeT[3]='x';
	else modeT[3]='-';
	if (mode&040)  modeT[4]='r'; else modeT[4]='-';
	if (mode&020)  modeT[5]='w'; else modeT[5]='-';
	if ((mode&02000)&&(mode&010)) modeT[6]='s'; 
	else if (mode&02000) modeT[6]='S';
	else if (mode&010) modeT[6]='x';
	else modeT[6]='-';
	if (mode&04)   modeT[7]='r'; else modeT[7]='-';
	if (mode&02)   modeT[8]='w'; else modeT[8]='-';
	if ((mode&01000)&&(mode&01)) modeT[9]='t'; 
	else if (mode&01000) modeT[9]='T';
	else if (mode&01) modeT[9]='x';
	else modeT[9]='-';
	return modeT;
}

static gint startit(GtkWidget * ctree,entry *en,int mod_mask,GtkCTreeNode *node){
  cfg *win;
  reg_t *prg;
  char *argv[24];
  char *a=NULL;
  int j=0;
    entry *up;
   /* disable openwith on FT_TARCHILD */
    win = gtk_object_get_user_data (GTK_OBJECT (ctree));
    if (en->type & FT_RPMCHILD) return TRUE;
    if (en->type & FT_TARCHILD) {
	    cb_tar_open_with(NULL,GTK_CTREE (ctree));
	    return TRUE;
    }

    if (en->type & (FT_TAR|FT_RPM)){
      if (!GTK_CTREE_ROW (node)->expanded) {
	      on_expand(GTK_CTREE (ctree),node,NULL);
	      gtk_ctree_expand (GTK_CTREE (ctree), node); 
      } else {
	      on_collapse(GTK_CTREE (ctree),node,NULL);
	      gtk_ctree_collapse (GTK_CTREE (ctree), node);
      }
      return (TRUE);
    }
    if (en->type & FT_DIR_UP)
    {
      cb_go_up(NULL,GTK_CTREE (ctree)); 	    
      return (TRUE);
    }
    if (!(en->type & FT_FILE)) return (FALSE);
    up = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), GTK_CTREE_ROW (node)->parent);
    cursor_wait (GTK_WIDGET (ctree));
    chdir (up->path);
    if (en->type & FT_EXE) { /*io_can_exec (en->path)) */
      /* here let's do a check if its a script file 
       * and if it is, then runit in a terminal window.*/
      char *cmd;
      FILE *pipe;
      gboolean doterminal=FALSE;
      cmd=(char *)malloc(strlen("file \"xx\"")+strlen(en->path)+1);
      if (cmd) {	
         sprintf (cmd, "file \"%s\"",en->path);
         pipe = popen (cmd, "r");
       	 if (pipe)
  	 {
  	  char *p,line[LINE_MAX+1];
  	  fgets (line, LINE_MAX, pipe);
  	  line[LINE_MAX] = 0;
   	  pclose (pipe);
  	  if ((p = strstr (line, ": ")) != NULL){
   	   p += 2;
	   if (strstr(p,"script")) doterminal=TRUE;
   	  }
 	 }
	 g_free(cmd);
      }
	    
      if (doterminal || (mod_mask & GDK_MOD1_MASK)){
	argv[j++]=TERMINAL;
	argv[j++]="-e";
	argv[j++]=en->path;
	argv[j]=0;
      } else {
	argv[j++]=en->path;
	argv[j]=0;
      }
      io_system (argv,win->top); /* open directly */ 
    }
    else
    {
      /* call open with dialog */
      prg = reg_prog_by_file (win->reg, en->path);
      if (prg)
      {
	argv[j++]=prg->app;
	if (prg->arg){
	  if (strstr(prg->arg," ")){
		a=g_strdup(prg->arg);
		argv[j++]=strtok(a," ");
		do {
		      argv[j]=strtok(NULL," ");
		      if (!argv[j]) break;
		      j++;
		      if (j>=24) { argv[24]=0; break; }

	        } while (1);
	  } else  argv[j++]=prg->arg;
	  argv[j++]=en->path;
	  argv[j]=0;
	} else {
	  argv[j++]=en->path;
	  argv[j]=0;
	}
	io_system (argv,win->top); /* open direct */
	if (a) g_free(a);
      }
      else
      {
	xf_dlg_open_with (ctree,win->xap, DEF_APP, en->path);
      }
    }
    cursor_reset (GTK_WIDGET (ctree));
    return (TRUE);
}
    
/*
 * set drag set for ctree
 *  */
gboolean was_double_click=FALSE;
static gint
on_button_release (GtkWidget * ctree, GdkEventButton * event, void *menu){
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (was_double_click) return 1;
  if ( (event->button == 1)){
     if (!win->source_set_sem) {
        win->source_set_sem=TRUE;
        gtk_drag_source_set (ctree, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, 
		  target_table, NUM_TARGETS, 
		  GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
     }
  }
  return 1;
}

/* 
 * start the marked program on double click 
 * */
static gint
on_double_click (GtkWidget * ctree, GdkEventButton * event, void *menu)
{
  GtkCTreeNode *node;
  entry *en;
  cfg *win;
  gint row, col;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
    /*fprintf(stderr,"dbg:  click detected\n"); */

  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
  {
    /*fprintf(stderr,"dbg: double click detected\n"); */
    was_double_click=TRUE;	  
    /* check if the double click was over a directory */
    row = -1;
    gtk_clist_get_selection_info (GTK_CLIST (ctree), event->x, event->y, &row, &col);
    if (row > -1)
    {
      node = gtk_ctree_node_nth (GTK_CTREE (ctree), row);
      en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
      if (EN_IS_DIR (en) && ((event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK)) || (win->preferences & DOUBLE_CLICK_GOTO)))
      {
        /* disable goto on FT_TARCHILD */
        if (en->type & (FT_RPMCHILD|FT_TARCHILD)) return TRUE;
        /* Alt or Ctrl button is pressed, it's the same as _go_to().. */
	go_to (GTK_CTREE (ctree), find_root((GtkCTree *)ctree), en->path, en->flags);
	return (TRUE);
      }
    }
    if (!count_selection (GTK_CTREE (ctree), &node))
    {
      return (TRUE);
    }
    if (!node)
    {
      return (TRUE);
    }
    en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
    return startit(ctree,en,event->state, node);
  } else was_double_click=FALSE;
  return (FALSE);
}

static gint
on_click_column (GtkCList * clist, gint column, gpointer data)
{
  int num;
  GtkCTreeNode *node;
  GList *selection = NULL;

  if (column != clist->sort_column)
    gtk_clist_set_sort_column (clist, column);
  else
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else
      clist->sort_type = GTK_SORT_ASCENDING;
  }
  num = count_selection (GTK_CTREE (clist), &node);
  if (num)
  {
    for (selection = g_list_copy (GTK_CLIST (clist)->selection); selection; selection = selection->next)
    {
      node = selection->data;
      if (!GTK_CTREE_ROW (node)->children || (!GTK_CTREE_ROW (node)->expanded))
      {
	/* select parent node */
	node = GTK_CTREE_ROW (node)->parent;
      }
      gtk_ctree_sort_node (GTK_CTREE (clist), node);
    }
  }
  else
  {
    gtk_clist_sort (clist);
  }
  g_list_free (selection);
  return TRUE;
}

/*
 * popup context menu
 */
static gint
on_button_press (GtkWidget * widget, GdkEventButton * event, void *data)
{
  GtkCTree *ctree = GTK_CTREE (widget);
  GtkWidget **menu = (GtkWidget **) data;
  GtkWidget *whatmenu;
  GtkCTreeNode *node;
  entry *en;
  cfg *win;
  reg_t *prg;

  int num, row, column = MN_NONE;
  
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (event->button == 3)
  {
    num = selection_type (ctree, &node);
    if (!num) {
      row = -1;
      gtk_clist_get_selection_info (GTK_CLIST (widget), event->x, event->y, &row, &column);
      if (row > -1) {
	gtk_clist_select_row (GTK_CLIST (ctree), row, 0);
	if (GTK_CLIST (ctree)->selection) num = selection_type (ctree, &node);
      }
    }
    gtk_widget_hide(win->autotype_D);
    gtk_widget_hide(win->autotype_C);
    if (node==find_root((GtkCTree *)ctree))
	    gtk_widget_hide(win->autotar_C);
    else gtk_widget_show(win->autotar_C);
    en = gtk_ctree_node_get_row_data (ctree, node); 
    if (!(en->type & FT_TARCHILD)&&((num==MN_FILE)||(num==MN_DIR))){
      int i;
      static char *text=NULL;
      GtkLabel *label;
      prg = reg_prog_by_file (win->reg, en->path);
      if (prg) {/* look in registered programs first */
        char cmd[(PATH_MAX + 3) * 2];
	GtkWidget *at;
	if (num==MN_DIR) at=win->autotype_D;
	else at=win->autotype_C;
        label=(GtkLabel *)(((GtkBin *)(at))->child);
	if (prg->arg) sprintf (cmd, "%s %s %s", prg->app, prg->arg, en->path);
	else sprintf (cmd, "%s %s", prg->app, en->path);
	gtk_label_set_text(label,cmd);
	gtk_widget_show(at);
      } else if (num==MN_FILE) {/* use default autotypes */	      
       char *loc;
       /*loc=strrchr(en->path,'.'); if (loc) */
       for (i=0;1;i++){
	       if (autotype[i].extension==NULL) break;
	       loc=strstr(en->path,autotype[i].extension);
	       if ((loc)&&(strcmp(loc,autotype[i].extension)==0)){
		       GtkLabel *label;
	               if (text) free(text);
		       label=(GtkLabel *)(((GtkBin *)(win->autotype_C))->child);
	               text=(char *)malloc(strlen(_(autotype[i].label))+strlen(en->label)+2);
		       if (!text) break;
	               sprintf(text,"%s %s",_(autotype[i].label),en->label);
		       gtk_label_set_text(label,text);
		       gtk_widget_show(win->autotype_C);
		       break;
	       }
       }
      } else if (num==MN_DIR) {/* use static directory entry default */
		GtkLabel *label;
	        if (text) free(text);
		label=(GtkLabel *)(((GtkBin *)(win->autotype_D))->child);
	        text=(char *)malloc(strlen(_("Open icon view"))+1);
		if (text){
		  sprintf(text,"%s",_("Open icon view"));
  		  gtk_label_set_text(label,text);
  		  gtk_widget_show(win->autotype_D);
		}
      }
    } 
    if (!(en->type & FT_TARCHILD)&&(num==MN_DIR)){ 
        GtkLabel *label;
        static char *text=NULL;
	label=(GtkLabel *)(((GtkBin *)(win->autotar_C))->child);
	if (text) free(text);	    
	text=(char *)malloc(strlen("%% %%.tgz")+strlen(_("Create"))+strlen(en->label)+2);
        if (text){
           sprintf(text,"%s %s.tgz",_("Create"),en->label);
	   gtk_label_set_text(label,text);
	}
    }
    whatmenu=menu[num];
    if (en->type & FT_TARCHILD)whatmenu=menu[MN_TARCHILD];
    if ((en->type & FT_TARCHILD)&&(en->type & FT_DIR))whatmenu=menu[MN_NONE];
    gtk_menu_popup (GTK_MENU (whatmenu), NULL, NULL, NULL, NULL, 3, event->time);
    return TRUE;
  }
  return FALSE;
}

/*
 * handle keyboard short cuts
 */
static gint
on_key_press (GtkWidget * ctree, GdkEventKey * event, void *menu)
{
  int num,i;
  entry *en;
  GtkCTreeNode *node;
  /*GdkEventButton evbtn;*/
  GList *selection;
  
  selection = GTK_CLIST (ctree)->selection;
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))==0)return (TRUE);
  node=selection->data;
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree),node);

  switch (event->keyval)
  {
  case GDK_Delete:
    cb_delete (NULL, GTK_CTREE (ctree));
    return (TRUE);
    break;
  case GDK_Return:
    if (!en->path) return (TRUE); 
    if ((en->type & FT_DIR) && !(en->type & FT_DIR_UP)){
	  if (event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK))
	  {
	    /* Alt or Ctrl button is pressed, it's the same as _go_to().. */
	    go_to (GTK_CTREE (ctree), find_root((GtkCTree *)ctree), en->path, en->flags);
	    return (TRUE);
	  }
	  if (!GTK_CTREE_ROW (node)->expanded) gtk_ctree_expand (GTK_CTREE (ctree), node);
	  else gtk_ctree_collapse (GTK_CTREE (ctree), node);
	  return (TRUE);
    }
    return startit(ctree,en,event->state, node);
  default: /*FIXME: */
    if ((event->keyval >= GDK_A) && (event->keyval <= GDK_z) && (event->state <= GDK_SHIFT_MASK))
    {
      num = g_list_length (GTK_CLIST (ctree)->row_list);
      /*(GTK_CLIST (ctree)->row_list);*/
      for (i = 0; i < num; i++)
      {
	node = gtk_ctree_node_nth (GTK_CTREE (ctree), i);
	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
	if (en->label && (*en->label == (char) event->keyval) && gtk_ctree_node_is_visible (GTK_CTREE (ctree), node))
	{
	  GTK_CLIST (ctree)->focus_row = i;
	  gtk_ctree_unselect_recursive (GTK_CTREE (ctree), NULL);
	  gtk_clist_moveto (GTK_CLIST (ctree), i, COL_NAME, 0, 0);
	  gtk_clist_select_row (GTK_CLIST (ctree), i, COL_NAME);
	  break;
	}
      }
      return (TRUE);
    }
    break;
  }
  return (FALSE);
}


/*
 * my own sort function
 * honor if an entry is a directory or a file
 */
static gint
my_compare (GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
  GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
  entry *en1, *en2;
  char *loc1=NULL,*loc2=NULL;
  int type1, type2;
  gboolean omit_subsort=FALSE;
    cfg *win;
    win = gtk_object_get_user_data (GTK_OBJECT (clist));

  en1 = row1->row.data;
  en2 = row2->row.data;
  /* FT_DIR_UP should always be on top (or bottom) */
 
  type1 = en1->type & (FT_DIR | FT_FILE);
  type2 = en2->type & (FT_DIR | FT_FILE);
  if (type1 != type2) {
    /* I want to have the directories at the top  */
    return (type1 < type2 ? -1 : 1);
  }

  if ((en1->type & FT_DIR)||(en2->type & FT_DIR)) omit_subsort=TRUE;
  
/* subsort by filetype */
  if (win->preferences&SUBSORT_BY_FILETYPE) {
	  loc1=strrchr(en1->label,'.');
	  loc2=strrchr(en2->label,'.');
  }
 
  if (clist->sort_column != COL_NAME)
  {
    /* use default compare function which we have saved before  */
    /* please don't compare numbers as strings! */
    GtkCListCompareFunc compare;
    compare = (GtkCListCompareFunc) win->compare;
    if ((win->preferences&SUBSORT_BY_FILETYPE)&&(!omit_subsort)) {
	  if ((!loc1)&&(!loc2)) { /* subsorted */
	     if (clist->sort_column == COL_DATE){
	       return en1->st.st_mtime - en2->st.st_mtime;
	     } else if (clist->sort_column == COL_SIZE){
	       return en1->st.st_size - en2->st.st_size;
	     } else if (clist->sort_column == COL_MODE){	
	       return en1->st.st_mode - en2->st.st_mode;
	     }	  
	     else return compare (clist, ptr1, ptr2);
	  }
          /* do subsorting */
	  if ((!loc1)&&(loc2)) return strcmp (".",loc2);
	  if ((loc1)&&(!loc2)) return strcmp (loc1,".");
	  if (strcmp(loc1,loc2)) return strcmp(loc1,loc2);
    }
    /* no subsorting */
    if (clist->sort_column == COL_DATE){
       return en1->st.st_mtime - en2->st.st_mtime;
    } else if (clist->sort_column == COL_SIZE){
       return en1->st.st_size - en2->st.st_size;
    } else if (clist->sort_column == COL_MODE){	
       return en1->st.st_mode - en2->st.st_mode;
    }	  
    else return compare (clist, ptr1, ptr2);
  }
  
  /* special case, sorting by column name */
  if ((win->preferences&SUBSORT_BY_FILETYPE)&&(!omit_subsort)) {
    if ((!loc1)&&(!loc2)) return strcmp (en1->label, en2->label);
    if ((!loc1)&&(loc2)) return strcmp (".",loc2);
    if ((loc1)&&(!loc2)) return strcmp (loc1,".");
    if (strcmp(loc1,loc2)) return strcmp(loc1,loc2);
  }
  return strcmp (en1->label, en2->label);
}


/* FIXME: this routine should be a loop to reduce excessive lines of code */
 
static GtkWidget *
create_menu (GtkWidget * top, GtkWidget * ctree, cfg * win,GtkWidget *hlpmenu)
{
  int i;
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  menubar = gtk_menu_bar_new ();
  gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
  gtk_widget_show (menubar);

#define LAST_MENU_ENTRY (sizeof(mlist)/sizeof(menu_entry))
  /* Create "File" menu */
  menuitem = gtk_menu_item_new_with_label (_("File"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  {
    menu_entry mlist[] = {
      MAINF_ICON_VIEW,
      MAINF_DIRECTORY_MENU,
      REGISTER_MENU,
      MAINF_MENU,
      DIR_OR_FILE_MENU,
      NONE_MENU
    };
    
    gtk_menu_set_accel_group (GTK_MENU (menu), accel);
    for (i = 0; i < LAST_MENU_ENTRY; i++){
     if (mlist[i].label) menuitem = gtk_menu_item_new_with_label (_(mlist[i].label));
     else menuitem = gtk_menu_item_new ();

     gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
     if (mlist[i].func) {
       gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (mlist[i].func), (gpointer) ctree);
       gtk_widget_add_accelerator (menuitem, "activate", accel,
		    mlist[i].key, mlist[i].mod,GTK_ACCEL_VISIBLE );
     }
    }
  }
  
  /* Create "Edit" menu */
  menuitem = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  {
    menu_entry mlist[] = {
      INSERT_MENU,
      DIR_FILE_MENU,
      MAIN_EDIT_MENU
     };
    
    gtk_menu_set_accel_group (GTK_MENU (menu), accel);
    for (i = 0; i < LAST_MENU_ENTRY; i++){
     if (mlist[i].label) menuitem = gtk_menu_item_new_with_label (_(mlist[i].label));
     else menuitem = gtk_menu_item_new ();

     gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
     if (mlist[i].func) {
     	gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (mlist[i].func), (gpointer) ctree);
     	gtk_widget_add_accelerator (menuitem, "activate", accel,
		    mlist[i].key, mlist[i].mod,GTK_ACCEL_VISIBLE );
     }
    }
  }

  
  /* Create "Goto" menu */
  menuitem = gtk_menu_item_new_with_label (_("Go to"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  {
    menu_entry mlist[] = {
	    SIMPLE_GOTO_MENU,
	    GOTO_MENU
    };
    
    gtk_menu_set_accel_group (GTK_MENU (menu), accel);
    for (i = 0; i < LAST_MENU_ENTRY; i++){
     if (mlist[i].label) menuitem = gtk_menu_item_new_with_label (_(mlist[i].label));
     else menuitem = gtk_menu_item_new ();

     gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
     gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (mlist[i].func), (gpointer) ctree);
     gtk_widget_add_accelerator (menuitem, "activate", accel,
		    mlist[i].key, mlist[i].mod,GTK_ACCEL_VISIBLE );
    }
  }
   
  /* Create "Tools" menu */
  menuitem = gtk_menu_item_new_with_label (_("Tools"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  {
    menu_entry mlist[] = {
	    TOOLS_MENU
    };
    
    gtk_menu_set_accel_group (GTK_MENU (menu), accel);
    for (i = 0; i < LAST_MENU_ENTRY; i++){
     if (mlist[i].label) menuitem = gtk_menu_item_new_with_label (_(mlist[i].label));
     else menuitem = gtk_menu_item_new ();

     gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
     gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (mlist[i].func), (gpointer) ctree);
     gtk_widget_add_accelerator (menuitem, "activate", accel,
		    mlist[i].key, mlist[i].mod,GTK_ACCEL_VISIBLE );
    }
  }
  
  /* Create "View" menu */
  menuitem = gtk_menu_item_new_with_label (_("View"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  {
    menu_entry mlist[] = {
      VIEW_MENU
    };
    
    gtk_menu_set_accel_group (GTK_MENU (menu), accel);
    for (i = 0; i < LAST_MENU_ENTRY; i++){
     if (mlist[i].label){
	menuitem = gtk_check_menu_item_new_with_label (_(mlist[i].label));
  	GTK_CHECK_MENU_ITEM (menuitem)->active = (mlist[i].data & preferences)?1:0;
  	gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
     }
     else menuitem = gtk_menu_item_new ();
     gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
     if (mlist[i].func) 
     {
	     gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (mlist[i].func), (gpointer) ctree);
	     gtk_widget_add_accelerator (menuitem, "activate", accel,
		    mlist[i].key, mlist[i].mod,GTK_ACCEL_VISIBLE );
     }
    }
  }

  /* Create "Preferences" menu */
  menuitem = gtk_menu_item_new_with_label (_("Preferences"));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_menu_item_new_with_label (_("Hide main menu"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		  GTK_SIGNAL_FUNC (cb_hide_menu),(gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu),menuitem);
  gtk_widget_add_accelerator (menuitem, "activate", accel,
		   GDK_m,GDK_MOD1_MASK,GTK_ACCEL_VISIBLE );
  gtk_widget_show (menuitem);
  
  menuitem = gtk_check_menu_item_new_with_label (_("Save geometry on exit"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (SAVE_GEOMETRY & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (cb_save_geo), (gpointer) ctree);

  menuitem = gtk_check_menu_item_new_with_label (_("Double click does GOTO"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (DOUBLE_CLICK_GOTO & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (cb_doubleC_goto), (gpointer) ctree);


  menuitem = gtk_check_menu_item_new_with_label (_("Drag does copy"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (DRAG_DOES_COPY & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (cb_drag_copy), (gpointer) ctree);
  
  menuitem = gtk_check_menu_item_new_with_label (_("Use rsync instead of scp"));
  GTK_CHECK_MENU_ITEM (menuitem)->active = (USE_RSYNC & preferences)?1:0;
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
  gtk_menu_append (GTK_MENU (menu), menuitem);  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", 
		    GTK_SIGNAL_FUNC (cb_use_rsync), (gpointer) ctree);

#if 0
  shortcut_menu (menu, _("Save geometry on exit"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(SAVE_GEOMETRY)) );
  shortcut_menu (menu, _("Double click does GOTO"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(DOUBLE_CLICK_GOTO)) );
  shortcut_menu (menu, _("Drag does copy"), (gpointer) cb_toggle_preferences, 
		  (gpointer)((long)(DRAG_DOES_COPY)) );
#endif

  menuitem = gtk_menu_item_new_with_label (_("Set background color"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_select_colors), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Set font"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_select_font), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);


  menuitem = gtk_menu_item_new_with_label (_("Configure toolbar"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_config_toolbar), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Redefine home directory"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_custom_home), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Create "Help" menu */
  menuitem = gtk_menu_item_new_with_label (_("Help"));
  gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));
  gtk_menu_bar_append (GTK_MENU_BAR (menubar), menuitem);
  gtk_widget_show (menuitem);
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
  
  menuitem = gtk_menu_item_new_with_label (_("Icon view"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_help_iv), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Drag and drop"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_dnd_help), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Rsync and scp"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_rsync), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);



  menuitem = gtk_menu_item_new_with_label (_("Unlisted shortcut keys"));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), hlpmenu);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Custom shortcut keys"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_custom_SCK), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  menuitem = gtk_menu_item_new_with_label (_("Registered applications"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_registered), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);


  menuitem = gtk_menu_item_new_with_label (_("About ..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (cb_about), ctree);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);


  return menubar;
}

static gint on_filter (GtkWidget * widget, GdkEventKey * event, GtkWidget *ctree)
{
  if (event->keyval == GDK_Return)
  {
    cb_reload(widget, (gpointer) ctree);
    return (TRUE);
  }
  return (FALSE);
}


/*
 * create a new toplevel window
 */
cfg *
new_top (char *path, char *xap, char *trash, GList * reg, int width, int height, int flags)
{
  GtkWidget *vbox,*widget;
  GtkWidget *handlebox[4];
  GtkWidget *menutop;
  GtkWidget *scrolled;
  GtkWidget *toolbar;
  GtkWidget *ctree;
  GtkWidget **menu;
  GtkWidget *menu_item;
  GtkCTreeNode *root;
  gchar *label[COLUMNS];
  gchar *titles[COLUMNS];
  entry *en;
  cfg *win;
  GtkAccelGroup *inner_accel;
  int i;
  struct passwd *pw;
  struct group *gr;
  GdkAtom atomo;

	  
  menu_entry dir_mlist[] = {
    AUTOTYPE_MENU,
    MAINF_DIRECTORY_MENU,
    FILE_MENU,
    REGISTER_MENU,
    AUTOTAR_MENU,
    DIRECTORY_MENU,
    MAINF_MENU,
    INSERT_MENU,
    DIR_FILE_MENU,
    DIR_OR_FILE_MENU,
    SIMPLE_GOTO_MENU,
    NONE_MENU
  };
#define LAST_DIR_MENU_ENTRY (sizeof(dir_mlist)/sizeof(menu_entry))


  menu_entry tarchild_mlist[] = {
     TAR_MENU,
     DIR_FILE_MENU,
     MAINF_MENU,
     NONE_MENU
  };
#define LAST_TARCHILD_MENU_ENTRY (sizeof(tarchild_mlist)/sizeof(menu_entry))

  

  menu_entry file_mlist[] = {
     AUTOTYPE_MENU,
     FILE_MENU,
     REGISTER_MENU,
     DIR_FILE_MENU,
     MAINF_MENU,
     DIR_OR_FILE_MENU,
     NONE_MENU
  };
#define LAST_FILE_MENU_ENTRY (sizeof(file_mlist)/sizeof(menu_entry))

  menu_entry mixed_mlist[] = {
     DIR_FILE_MENU,
     MAINF_MENU,
     NONE_MENU
   };
#define LAST_MIXED_MENU_ENTRY (sizeof(mixed_mlist)/sizeof(menu_entry))

  menu_entry none_mlist[] = {
     NONE_MENU
   };
#define LAST_NONE_MENU_ENTRY (sizeof(none_mlist)/sizeof(menu_entry))
  menu_entry help_mlist[] = {
     HELP_MENU
  };
#define LAST_HELP_MENU_ENTRY (sizeof(help_mlist)/sizeof(menu_entry))

  read_defaults();

  /* valid path? */
  if (!io_is_directory (path)) {
    fprintf(stderr,"xftree warning: %s (%s). Defaulting to /\n",path, strerror (errno));
    path="/";
  }
  if (SAVE_GEOMETRY & preferences)
  {
	  width=geometryX;
	  height=geometryY;
  }
  
  /* Set up X error Handler */
  XSetErrorHandler ((XErrorHandler) ErrorHandler);

  win = g_malloc (sizeof (cfg));
  win->source_set_sem=FALSE;
  win->dnd_row = -1;
  win->dnd_data = NULL;
  win->gogo =NULL;
  win->preferences = preferences;
  win->filterOpts=FILTER_DIRS|FILTER_FILES;
  menu = g_malloc (sizeof (GtkWidget) * MENUS);
  titles[COL_NAME] = _("Name");
  /*titles[COL_SIZE] = (preferences & SIZE_IN_KB)?_("Size (Kb)"):_("Size (bytes)");*/
  titles[COL_SIZE] = _("Size (bytes)");
  titles[COL_DATE] = _("Last changed");
  titles[COL_MODE] = _("Mode");
  titles[COL_UID] = _("Owner");
  titles[COL_GID] = _("Group");
 
  win->top = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy ((GtkWindow *)win->top,FALSE,TRUE,FALSE);
  gtk_widget_realize (win->top);

  win->gogo = pushgo(path,win->gogo);
                                              
  top_register (win->top);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (win->top), vbox);
  gtk_widget_show (vbox);
  
  atomo=gdk_atom_intern("WM_COMMAND",FALSE);
  gdk_property_change (gtk_widget_get_parent_window(vbox),
		  atomo,
                  XA_STRING, /* GdkAtom type,*/
                  8, /* bit per data element: gint format*/
 	      	  GDK_PROP_MODE_REPLACE,
                  (guchar *)"xftree",  
		  strlen("xftree")+1);  
  
#if 0
  atomo=gdk_atom_intern("WM_CLIENT_MACHINE",FALSE);
  gdk_property_change (gtk_widget_get_parent_window(vbox),
		  atomo,
                  XA_STRING, /* GdkAtom type,*/
                  8, /* bit per data element: gint format*/
 	      	  GDK_PROP_MODE_REPLACE,
                  (guchar *)our_host_name(),  
		  strlen(our_host_name())+1);  
#endif 
  atomo=gdk_atom_intern("WM_CLIENT_LEADER",TRUE);
  if (atomo != GDK_NONE) {
	  gdk_property_delete (gtk_widget_get_parent_window(vbox),atomo);
  }
  
  for (i=0;i<4;i++) {
	  handlebox[i] = gtk_handle_box_new ();
	  gtk_container_border_width (GTK_CONTAINER (handlebox[i]), 2);
	  gtk_box_pack_start (GTK_BOX (vbox), handlebox[i], FALSE, FALSE, 0);
	  gtk_widget_show (handlebox[i]);
  }

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  widget = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  
#ifdef STATUS_LABEL
  win->status = gtk_label_new("");
  gtk_box_pack_start (GTK_BOX (widget), win->status, FALSE, FALSE, 0);
#else
  win->status = gtk_entry_new();
  gtk_entry_set_editable((GtkEntry *)win->status,FALSE);
  gtk_entry_set_max_length ((GtkEntry *)win->status,256);
  gtk_box_pack_start (GTK_BOX (widget), win->status, TRUE, TRUE, 0);
#endif  
  gtk_widget_show (win->status); 
  
  ctree = gtk_ctree_new_with_titles (COLUMNS, 0, titles);
  gtk_clist_set_column_justification((GtkCList *)ctree,COL_SIZE,GTK_JUSTIFY_RIGHT);




  {
	GtkWidget *box;
  	box=gtk_hbox_new (FALSE, 0);
  	gtk_container_add (GTK_CONTAINER (handlebox[3]),box );
  	gtk_widget_show (box); 	
	widget=gtk_label_new(_("Filter:"));
  	gtk_box_pack_start (GTK_BOX (box),widget, FALSE, FALSE, 0);
  	gtk_widget_show (widget); 
	win->filter=gtk_entry_new();	
	gtk_entry_set_text ((GtkEntry *)win->filter,"*");

  	gtk_box_pack_start (GTK_BOX (box),win->filter, TRUE,TRUE, 0);
  	gtk_widget_show (win->filter); 
        gtk_signal_connect (GTK_OBJECT (win->filter), "key_press_event",
		       GTK_SIGNAL_FUNC (on_filter), (gpointer) ctree);
	
	widget=gtk_check_button_new_with_label (_("dirs"));
  	gtk_box_pack_start (GTK_BOX (box),widget, FALSE,FALSE, 0);
	gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
	gtk_signal_connect (GTK_OBJECT (widget), "toggled", 
		    GTK_SIGNAL_FUNC (cb_filter_dirs), (gpointer) ctree);
  	gtk_widget_show (widget); 
	widget=gtk_check_button_new_with_label (_("files"));
	gtk_box_pack_start (GTK_BOX (box),widget, FALSE,FALSE, 0);
	gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
	gtk_signal_connect (GTK_OBJECT (widget), "toggled", 
		    GTK_SIGNAL_FUNC (cb_filter_files), (gpointer) ctree);
	gtk_widget_show (widget); 
 }

  
  gtk_clist_set_auto_sort (GTK_CLIST (ctree), FALSE);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree-expand", GTK_SIGNAL_FUNC (tree_unselect),ctree);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree-collapse", GTK_SIGNAL_FUNC (tree_unselect),ctree);
  
  gtk_signal_connect (GTK_OBJECT (win->top), "destroy", GTK_SIGNAL_FUNC (on_destroy), (gpointer) ctree);
  gtk_signal_connect (GTK_OBJECT (win->top), "delete_event", GTK_SIGNAL_FUNC (on_delete), (gpointer) ctree);

  accel = gtk_accel_group_new ();
  gtk_accel_group_attach (accel, GTK_OBJECT (win->top));

  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_NAME].button), "clicked", accel, GDK_n, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_DATE].button), "clicked", accel, GDK_d, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (GTK_WIDGET (GTK_CLIST (ctree)->column[COL_SIZE].button), "clicked", accel, GDK_s, GDK_CONTROL_MASK | GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  win->compare = (gpointer) GTK_CLIST (ctree)->compare;
  win->trash = g_strdup (trash);
  win->xap = g_strdup (xap);
  win->reg = reg;
  win->width = width;
  win->height = height;
  gtk_object_set_user_data (GTK_OBJECT (ctree), win);
  if (preferences & CUSTOM_COLORS) set_colors(ctree); 
  if (preferences & CUSTOM_FONT) create_pixmaps(set_fontT(ctree),ctree);
  else create_pixmaps(-1,ctree);
  gtk_clist_set_compare_func (GTK_CLIST (ctree), my_compare);
  gtk_clist_set_shadow_type (GTK_CLIST (ctree), GTK_SHADOW_IN);

  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_NAME, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_SIZE, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_DATE, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_UID, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_GID, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), COL_MODE, TRUE);
  /*gtk_clist_set_column_resizeable (GTK_CLIST (ctree), 1, TRUE);*/
  /*gtk_clist_set_column_resizeable (GTK_CLIST (ctree), 2, TRUE);*/
  gtk_clist_set_column_width (GTK_CLIST (ctree), 2, 115);

  gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_EXTENDED);
  gtk_ctree_set_line_style (GTK_CTREE (ctree), GTK_CTREE_LINES_NONE);
  gtk_clist_set_reorderable (GTK_CLIST (ctree), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled), ctree);

  
/* FIXME: these menus should be created in a loop, to reduce excessive lines of code */
  
 /**** help menu, non operation, for displaying default kyeboard shortcuts */
  menu[MN_HLP] = gtk_menu_new ();
  inner_accel = gtk_accel_group_new ();
  gtk_accel_group_attach (inner_accel, GTK_OBJECT (menu[MN_HLP]));


  for (i = 0; i < LAST_HELP_MENU_ENTRY; i++)
  {
    if (help_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(help_mlist[i].label));
    else 
      menu_item = gtk_menu_item_new ();
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (cb_default_SCK),ctree);
    if (help_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", inner_accel, help_mlist[i].key, help_mlist[i].mod,GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_HLP]), menu_item);
    gtk_widget_show (menu_item);
  }


 /***** directory list ...***/ 
 
  menu[MN_DIR] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_DIR]), accel);


  for (i = 0; i < LAST_DIR_MENU_ENTRY; i++)
  {
    if (dir_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(dir_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (!i) win->autotype_D=menu_item;	    
    if (dir_mlist[i].func)
    {
      if (dir_mlist[i].func==cb_autotar) win->autotar_C=menu_item;
      if (dir_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), win);
      else if (dir_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (dir_mlist[i].func), (gpointer) ctree);
    }
    if (dir_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, dir_mlist[i].key, dir_mlist[i].mod,GTK_ACCEL_VISIBLE );
    }
    gtk_menu_append (GTK_MENU (menu[MN_DIR]), menu_item);
    gtk_widget_show (menu_item);
  }
  
  win->restore_menu = gtk_menu_item_new_with_label (_("Show main menu"));
  gtk_signal_connect (GTK_OBJECT (win->restore_menu), "activate", 
		  GTK_SIGNAL_FUNC (cb_hide_menu),(gpointer) ctree);
  gtk_menu_append (GTK_MENU (menu[MN_DIR]),win->restore_menu);
  gtk_widget_add_accelerator (win->restore_menu, "activate", accel,
		   GDK_m,GDK_MOD1_MASK,GTK_ACCEL_VISIBLE );
  if (preferences & HIDE_MENU) 
	  gtk_widget_show (win->restore_menu);
  
  
 
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_DIR]), ctree, (GtkMenuDetachFunc) menu_detach);
/**** tarchild list ...  */  
  menu[MN_TARCHILD] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_TARCHILD]), accel);
  for (i = 0; i < LAST_TARCHILD_MENU_ENTRY; i++)
  {
    if (tarchild_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(tarchild_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (tarchild_mlist[i].func)
    {
      if (tarchild_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (tarchild_mlist[i].func), win);
      else if (file_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (tarchild_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (tarchild_mlist[i].func), (gpointer) ctree);
    }
    if (tarchild_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, tarchild_mlist[i].key, tarchild_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_TARCHILD]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_TARCHILD]), ctree, (GtkMenuDetachFunc) menu_detach);


/**** file list ...  */  
  menu[MN_FILE] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_FILE]), accel);
  for (i = 0; i < LAST_FILE_MENU_ENTRY; i++)
  {
    if (file_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(file_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (file_mlist[i].func)
    {
      if (!i) win->autotype_C=menu_item;	    
      if (file_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), win);
      else if (file_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (file_mlist[i].func), (gpointer) ctree);
    }
    if (file_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, file_mlist[i].key, file_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_FILE]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_FILE]), ctree, (GtkMenuDetachFunc) menu_detach);

  /*** mixed list... */
  menu[MN_MIXED] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_MIXED]), accel);
  for (i = 0; i < LAST_MIXED_MENU_ENTRY; i++)
  {
    if (mixed_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(mixed_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (mixed_mlist[i].func)
    {
      if (mixed_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), win);
      else if (mixed_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (mixed_mlist[i].func), (gpointer) ctree);
    }
    if (mixed_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, mixed_mlist[i].key, mixed_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_MIXED]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_MIXED]), ctree, (GtkMenuDetachFunc) menu_detach);

  menu[MN_NONE] = gtk_menu_new ();
  gtk_menu_set_accel_group (GTK_MENU (menu[MN_NONE]), accel);

  for (i = 0; i < LAST_NONE_MENU_ENTRY; i++)
  {
    if (none_mlist[i].label)
      menu_item = gtk_menu_item_new_with_label (_(none_mlist[i].label));
    else
      menu_item = gtk_menu_item_new ();
    if (none_mlist[i].func)
    {
      if (none_mlist[i].data == WINCFG)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), win);
      else if (none_mlist[i].data == TOPWIN)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), GTK_WIDGET (win->top));
      else
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (none_mlist[i].func), (gpointer) ctree);
    }
    if (none_mlist[i].key)
    {
      gtk_widget_add_accelerator (menu_item, "activate", accel, none_mlist[i].key, none_mlist[i].mod, GTK_ACCEL_VISIBLE);
    }
    gtk_menu_append (GTK_MENU (menu[MN_NONE]), menu_item);
    gtk_widget_show (menu_item);
  }
  gtk_menu_attach_to_widget (GTK_MENU (menu[MN_NONE]), ctree, (GtkMenuDetachFunc) menu_detach);

/* first pixmap appearance */
  
  
  en = entry_new_by_path_and_label (path, (preferences&ABREVIATE_PATHS)?abreviate(path):path);
  if (!en)
  {
    cleanup_tmpfiles();
    exit (1);
  }
  en->flags = flags;
  en->type |= FT_ISROOT;

  if (preferences & ABREVIATE_PATHS) {
	  label[COL_NAME] = abreviate(path);
  } else label[COL_NAME] = path; 
  
  label[COL_SIZE] = "";
  label[COL_DATE] = "";
  
  label[COL_MODE] = mode_txt(en->st.st_mode);
  pw=getpwuid(en->st.st_uid); 
  label[COL_UID] = (pw)? pw->pw_name : _("unknown");
  gr=getgrgid (en->st.st_gid); 
  label[COL_GID] = (gr)? gr->gr_name : _("unknown");
  root = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, NULL, label, 8, 
		  NULL, NULL,NULL, NULL,
		  FALSE, TRUE);  
  gtk_ctree_node_set_row_data_full (GTK_CTREE (ctree), root, en, node_destroy);
  add_subtree (GTK_CTREE (ctree), root, path, 2, flags);
  reset_icon(GTK_CTREE (ctree), root); 
  gtk_ctree_select (GTK_CTREE (ctree), root);

  gtk_signal_connect (GTK_OBJECT (ctree), "tree_expand", GTK_SIGNAL_FUNC (on_expand), NULL);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_collapse", GTK_SIGNAL_FUNC (on_collapse), NULL);
  gtk_signal_connect (GTK_OBJECT (ctree), "click_column", GTK_SIGNAL_FUNC (on_click_column), en);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_press_event", GTK_SIGNAL_FUNC (on_double_click), root);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_release_event", GTK_SIGNAL_FUNC (on_button_release), root);
  gtk_signal_connect (GTK_OBJECT (ctree), "button_press_event", GTK_SIGNAL_FUNC (on_button_press), menu);
  gtk_signal_connect (GTK_OBJECT (ctree), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), menu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data), win);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_get", GTK_SIGNAL_FUNC (on_drag_data_get), win);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_motion", GTK_SIGNAL_FUNC (on_drag_motion), NULL);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_end", GTK_SIGNAL_FUNC (on_drag_end), win);

                              


  if (win->width > 0 && win->height > 0)
  {
    gtk_window_set_default_size (GTK_WINDOW (win->top), width, height);
  }

  win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);
  
  /*
   * DO NOT put gtk_drag_source_set here. It is very buggy if you do.
   * Instead set at first mouse click and unset on expand+contract
   * (inherent GtkCTree bug workaround)
   * gtk_drag_source_set (ctree, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, 
		  target_table, NUM_TARGETS, 
		  GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);*/
  
  gtk_drag_dest_set (ctree, GTK_DEST_DEFAULT_DROP|GTK_DEST_DEFAULT_HIGHLIGHT, 
		  target_table, NUM_TARGETS, 
		  GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
		  
	  
  menutop = create_menu (win->top, ctree, win, menu[MN_HLP]);
  gtk_container_add (GTK_CONTAINER (handlebox[0]), menutop);
  gtk_widget_show (menutop);
  win->menu = menutop;

  toolbar = create_toolbar (win->top, ctree, win,FALSE);
  gtk_container_add (GTK_CONTAINER (handlebox[1]), toolbar);
  gtk_widget_show (toolbar);
  win->toolbar=toolbar;
  
  {
    GtkStyle *Ostyle,*style;
    Ostyle=gtk_widget_get_style (toolbar);
    style = gtk_style_copy (Ostyle);
    gtk_widget_set_style (win->status,style);
    gtk_widget_ensure_style (win->status);
   
  }
  
  toolbar = create_toolbar (win->top, ctree, win,TRUE);
  gtk_container_add (GTK_CONTAINER (handlebox[2]), toolbar);
  gtk_widget_show (toolbar);
  win->toolbarO=toolbar;

  gtk_widget_show_all (win->top);

  /* hide what must be hidden */
  if (!(preferences & SHOW_STATUS)) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (win->status->parent))){ 
	      gtk_widget_hide(GTK_WIDGET (win->status->parent));
	  } 
  }
  if (preferences&HIDE_MENU) gtk_widget_show(win->restore_menu);
  if (!(preferences & FILTER_OPTION)) {
	if (GTK_WIDGET_VISIBLE(win->filter->parent->parent))
		  gtk_widget_hide(win->filter->parent->parent);		  
  }
  if (preferences & HIDE_TOOLBAR) {
	  if (GTK_WIDGET_VISIBLE(win->toolbar->parent))
		  gtk_widget_hide(win->toolbar->parent);
	  if (GTK_WIDGET_VISIBLE(win->toolbarO->parent))
		  gtk_widget_hide(win->toolbarO->parent);
  }  
  else if (preferences & LARGE_TOOLBAR) gtk_widget_hide((win->toolbar)->parent);
  else gtk_widget_hide((win->toolbarO)->parent);

  if (preferences & HIDE_MENU) {
	  if (GTK_WIDGET_VISIBLE(win->menu->parent))
		  gtk_widget_hide(win->menu->parent);
  }
  if (preferences & HIDE_TITLES) {
      gtk_clist_column_titles_hide((GtkCList *)ctree);
  }
  
/*  win->iconname = (char *)malloc((strlen("XFTree: ")+strlen(path)+1)*sizeof(char));
  if (win->iconname) {
   if (strncmp(path,getenv ("HOME"),strlen(getenv ("HOME")))==0){	  
     sprintf(win->iconname,"~%s",path+strlen(getenv ("HOME")));
   } else sprintf (win->iconname, "%s", (path)?path:"/");
   gdk_window_set_icon_name (gtk_widget_get_toplevel (GTK_WIDGET (ctree))->window, 
		  win->iconname);
  } */
  win->iconname = NULL;
  set_title_ctree (ctree, en->path);
  set_icon (win->top,win->iconname, xftree_icon_xpm);
  

 /* icon_name = strrchr (path, '/');
  if ((icon_name) && (!(*(++icon_name)))) icon_name = NULL;
  set_icon (win->top, (icon_name ? icon_name : "/"), xftree_icon_xpm);*/

  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_DATE,!(preferences & HIDE_DATE));
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_SIZE,!(preferences & HIDE_SIZE));
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_MODE,!(preferences & HIDE_MODE));
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_UID,!(preferences & HIDE_UID));
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_GID,!(preferences & HIDE_GID));
  
      
  return (win);
}

/* create a new toplevel tree widget */

void
gui_main (char *path, char *xap_path, char *trash, char *reg_file, wgeo_t * geo, int flags)
{
  GList *reg;
  cfg *new_win;
  
  /*fprintf(stderr,"dbg:sizeof(off_t)=%d\n",sizeof(off_t));*/
  init_pixmaps();  

  reg = reg_build_list (reg_file);
  /* clean up stale entries should be cleaned with reg_build */
  /* now save clean register */
  reg_save(reg);
  if (SAVE_GEOMETRY & preferences){
	  geo->width=geometryX;
	  geo->height=geometryY;
  }
  new_win = new_top (path,xap_path, trash, reg, geo->width, geo->height, flags);
  if (geo->x > -1 && geo->y > -1)
  {
    gint x,y;
    gtk_widget_set_uposition (new_win->top, geo->x, geo->y);
    gdk_window_get_root_origin ((new_win->top)->window, &x, &y);
    gtk_widget_set_uposition (new_win->top,geo->x+(geo->x-x), geo->y+(geo->y-y) );
    gdk_flush();
    /*fprintf(stderr,"root: x=%d,y=%d\n",x,y);*/
  }

  gtk_main ();
  save_defaults(NULL);
  cleanup_tmpfiles();
  exit (0);
}
