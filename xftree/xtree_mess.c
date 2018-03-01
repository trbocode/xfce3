/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * xtree_mess.c: messages and configuration routines for xftree
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

/* probably overkill with all these includes: */
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
#include <glob.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
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
#include "../xfsamba/tubo.h"
#include "../xfdiff/xfdiff_colorsel.h"
#include "xtree_go.h"
#include "xtree_cb.h"
#include "xtree_functions.h"
#include "xtree_icons.h"
#include "fontselection.h"

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* this file is for processing certain common messages.
 * override warning to begin with */

#define XTREE_MESS_MAIN
#include "xtree_mess.h"

#define BYTES "bytes"
extern int pixmap_level;
static GdkFont *the_font; 
static char *custom_font=NULL;

GtkCTreeNode *find_root(GtkCTree * ctree){
	GtkCTreeNode *node=NULL;
	entry *en;
	node=gtk_ctree_node_nth (ctree,0);
	while (node){
		en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
		if (en->type & FT_ISROOT) return node;
		node = GTK_CTREE_ROW (node)->sibling;
	}
	return node;
}

int set_fontT(GtkWidget * ctree){
	GtkStyle  *Ostyle,*style;
	Ostyle=gtk_widget_get_style (ctree);
    	style = gtk_style_copy (Ostyle);
	if ((preferences & CUSTOM_FONT)&&(custom_font)) {
	  the_font = gdk_font_load (custom_font);
          if (the_font == NULL) {
           cfg *win;
           win = gtk_object_get_user_data (GTK_OBJECT (ctree));
           xf_dlg_error(win->top,_("Could not load specified font\n"),NULL);
           preferences &= (CUSTOM_FONT ^ 0xffffffff);
           return -1;
          }
	  style->font=the_font;
	}
	if (!(preferences & CUSTOM_FONT)){
		style->font=Ostyle->font;
	}
	gtk_widget_set_style (ctree,style);
	gtk_widget_ensure_style (ctree);
   	return (style->font->ascent + style->font->descent + 5);
	
}


void set_colors(GtkWidget * ctree){
	GtkStyle *Ostyle,*style;
	int red,green,blue;
	int Nred,Ngreen,Nblue;

	red = ctree_color.red & 0xffff;
	green = ctree_color.green & 0xffff;
	blue = ctree_color.blue & 0xffff;
	Ostyle=gtk_widget_get_style (ctree);
    	style = gtk_style_copy (Ostyle);

	if (!(preferences & CUSTOM_COLORS)){
		gtk_widget_set_style (ctree,style);
		gtk_widget_ensure_style (ctree);
		return;
	}
	
	style->base[GTK_STATE_ACTIVE].red=red;
	style->base[GTK_STATE_ACTIVE].green=green;
	style->base[GTK_STATE_ACTIVE].blue=blue;
	  
	style->base[GTK_STATE_NORMAL].red=red;
	style->base[GTK_STATE_NORMAL].green=green;
	style->base[GTK_STATE_NORMAL].blue=blue;
	  
	style->bg[GTK_STATE_NORMAL].red=red;
	style->bg[GTK_STATE_NORMAL].green=green;
	style->bg[GTK_STATE_NORMAL].blue=blue;
	  
	style->fg[GTK_STATE_SELECTED].red=red;
	style->fg[GTK_STATE_SELECTED].green=green;
	style->fg[GTK_STATE_SELECTED].blue=blue;
	  /* foregrounds */
	
	Nred=(red^0xffff)&(0xffff);
	Ngreen=(green^0xffff)&(0xffff);
	Nblue=(blue^0xffff)&(0xffff);

	
	if (red + green + blue < 3*32768) Nred=Ngreen=Nblue=65535;
	else Nred=Ngreen=Nblue=0;
	
	style->fg[GTK_STATE_NORMAL].red=Nred;
	style->fg[GTK_STATE_NORMAL].green=Ngreen;
	style->fg[GTK_STATE_NORMAL].blue=Nblue;
	  
	style->bg[GTK_STATE_SELECTED].red=Nred;
	style->bg[GTK_STATE_SELECTED].green=Ngreen;
	style->bg[GTK_STATE_SELECTED].blue=Nblue;
	
	gtk_widget_set_style (ctree,style);
	gtk_widget_ensure_style (ctree);
	return;
}

 
void
cb_select_colors (GtkWidget * widget, GtkWidget * ctree)
{
  gdouble colors[4];
  gdouble *newcolor;
  cfg *win;

  
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  colors[0] = ((gdouble) ctree_color.red) / COLOR_GDK;
  colors[1] = ((gdouble) ctree_color.green) / COLOR_GDK;
  colors[2] = ((gdouble) ctree_color.blue) / COLOR_GDK;
  
  newcolor=xfdiff_colorselect (colors);
  if (newcolor){
      ctree_color.red   = ((guint) (newcolor[0] * COLOR_GDK));
      ctree_color.green = ((guint) (newcolor[1] * COLOR_GDK));
      ctree_color.blue  = ((guint) (newcolor[2] * COLOR_GDK));
      preferences |= CUSTOM_COLORS;
  } else {
      preferences &= (CUSTOM_COLORS ^ 0xffffffff);
  }
  set_colors(ctree);
  save_defaults (NULL);  
  return;
}

void
cb_select_font (GtkWidget * widget, GtkWidget *ctree)
{
  char *font_selected;
  cfg *win;
      
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  font_selected = open_fontselection ("fixed");
  win->preferences ^= FONT_STATE;
  if (!font_selected) {
	  win->preferences &= (CUSTOM_FONT ^ 0xffffffff);
	  preferences = win->preferences;
  } else  {
	win->preferences |= CUSTOM_FONT;
	preferences = win->preferences;
  	if (custom_font) free(custom_font);
	custom_font=(char *)malloc(strlen(font_selected)+1);
	if (custom_font) strcpy(custom_font,font_selected);
	else preferences &= (CUSTOM_FONT ^ 0xffffffff);
  }
  create_pixmaps(set_fontT(ctree),ctree);
  save_defaults (NULL);
  regen_ctree((GtkCTree *)ctree);  
  return;
}
void cb_subsort(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= SUBSORT_BY_FILETYPE;
  preferences = win->preferences;
  gtk_clist_sort ((GtkCList *)ctree);
  save_defaults(NULL);
  return;

}

void cb_filter(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= FILTER_OPTION;
  preferences = win->preferences;
  if (preferences & FILTER_OPTION){
	if (!GTK_WIDGET_VISIBLE(win->filter->parent->parent))
		gtk_widget_show(win->filter->parent->parent);
  } else {
	if (GTK_WIDGET_VISIBLE(win->filter->parent->parent))
		gtk_widget_hide(win->filter->parent->parent);	  
  }
  cb_reload(widget, (gpointer) ctree);
  save_defaults(NULL);
  return;

}

static void filter_reload(GtkWidget * widget, GtkWidget *ctree){
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));  
  if (!(win->filterOpts)) preferences &= (FILTER_OPTION^0xffffffff);
  else preferences |= FILTER_OPTION;
  preferences = win->preferences;
  cb_reload(widget, (gpointer) ctree);
  return;
}
void cb_filter_dirs(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->filterOpts ^= FILTER_DIRS;
  filter_reload(widget,ctree);
  return;

}
void cb_filter_files(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->filterOpts ^= FILTER_FILES;
  filter_reload(widget,ctree);
  return;

}
void
cb_hide_dd (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= HIDE_DD;
  preferences = win->preferences;
  cb_reload(widget, (gpointer) ctree);
  save_defaults(NULL);
  return;
}

void
cb_hide_mode (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_MODE;
  preferences = win->preferences;
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_MODE,!(preferences & HIDE_MODE));
  if (preferences & HIDE_TITLES) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_MODE].button))){ 
	      gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_MODE].button));
	  }
  }
  save_defaults(NULL);
  return;
}

void
cb_hide_uid (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_UID;
  preferences = win->preferences;
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_UID,!(preferences & HIDE_UID));
  if (preferences & HIDE_TITLES) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_MODE].button))){ 
	      gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_MODE].button));
	  }
  }
  save_defaults(NULL);
  return;
}

void
cb_hide_gid (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_GID;
  preferences = win->preferences;
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_GID,!(preferences & HIDE_GID));
  if (preferences & HIDE_TITLES) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_GID].button))){ 
	      gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_GID].button));
	  }
  }
  save_defaults(NULL);
  return;
}


void
cb_hide_date (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_DATE;
  preferences = win->preferences;
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_DATE,!(preferences & HIDE_DATE));
  if (preferences & HIDE_TITLES) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_DATE].button))){ 
	      gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_DATE].button));
	  }
  }
  save_defaults(NULL);
  return;
}

void
cb_hide_size (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_SIZE;
  preferences = win->preferences;
  gtk_clist_set_column_visibility ((GtkCList *)ctree,COL_SIZE,!(preferences & HIDE_SIZE));
  if (preferences & HIDE_TITLES) {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_SIZE].button))){ 
	      gtk_widget_hide(GTK_WIDGET (GTK_CLIST (ctree)->column[COL_SIZE].button));
	  }
  }
  save_defaults(NULL);
  return;
}

void
cb_hide_menu (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  win->preferences ^= HIDE_MENU;
  preferences = win->preferences;
  if (preferences & HIDE_MENU) {
	  if (GTK_WIDGET_VISIBLE(win->menu->parent)) {
		  gtk_widget_show(win->restore_menu);
		  gtk_widget_hide(win->menu->parent);
	  }
  } else {
	  if (!GTK_WIDGET_VISIBLE(win->menu->parent)){
		  gtk_widget_show(win->menu->parent);
		  gtk_widget_hide(win->restore_menu);
	  }
  }
  save_defaults(NULL);
  return;
}
void
cb_abreviate (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  

  win->preferences ^= ABREVIATE_PATHS;
  preferences = win->preferences;
  save_defaults(NULL);
  cb_reload(widget,(gpointer) ctree);
  return;
}

void
cb_hide_titles (GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  
  win->preferences ^= HIDE_TITLES;
  preferences = win->preferences;
  if (preferences & HIDE_TITLES) {
	  gtk_clist_column_titles_hide((GtkCList *)ctree);
  } else {
	  gtk_clist_column_titles_show((GtkCList *)ctree);
  }
  save_defaults(NULL);
  return;
}

char * override_txt(char *new_file,char *old_file)
{
  gboolean old_exists=FALSE;
  struct stat nst,ost;
  static char *message=NULL;
  char *ot=_("Override ?"),*otime=NULL,*ntime;
  char *with=_("with");
  off_t i;
  int osize=0,nsize;
  
  if (message) {free (message);}
  if (lstat (new_file, &nst) == ERROR){
    fprintf(stderr,"this should never happen: override_txt()\n");
    return ot;
  }
  if (lstat (old_file, &ost) != ERROR){
    old_exists=TRUE;
    osize=1;
    i=ost.st_size;
    otime=(char *)malloc( strlen(ctime(&(ost.st_mtime))) + 1 );
    strcpy(otime,ctime(&(ost.st_mtime)) );
    while (i) {i = i/10; osize++;}
  }
  nsize=1;
  i=nst.st_size;
  while (i) {i = i/10; nsize++;}
  ntime=ctime(&(nst.st_mtime));
   
  i= 1 + strlen(ot) +1+ 
	  strlen(new_file) +1+ strlen(ntime) +1+ nsize +1+ strlen("bytes") + 1;
  if (old_exists) {
    i = i + 
	  + strlen(with) +1+
	  strlen(old_file) +1+ strlen(otime) +1+ osize +1+ strlen("bytes") + 1;
  }
  message=(char *)malloc(i*sizeof(char));
  if (!message) {return ot;}
  if (old_exists){
	sprintf(message,"%s\n%s %s %lld %s\n%s\n%s %s %lld %s\n",ot,
			new_file,ntime,(long long)nst.st_size,BYTES,
			with,
			old_file,otime,(long long)ost.st_size,BYTES);
	free(otime);
  }
  else
	sprintf(message,"%s\n%s %s %lld %s\n",ot,
			new_file,ntime,(long long)nst.st_size,BYTES);
  return message;
}
	
#if 0
GtkWidget *
shortcut_menu (GtkWidget * parent, char *txt, gpointer func, gpointer data)
{
  static GtkWidget *menuitem;
  int togglevalue;

    togglevalue =(int) ((long) data) ;
    menuitem = gtk_check_menu_item_new_with_label (txt);
    GTK_CHECK_MENU_ITEM (menuitem)->active = (togglevalue & preferences)?1:0;
 /*   printf("dbg:pref=%d,toggle=%d,%s result=%d\n",preferences,togglevalue,txt,togglevalue & preferences);*/
    gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
#ifdef __GTK_2_0
    /*  */
    gtk_menu_append (GTK_MENU_SHELL (parent), menuitem);
#else
    gtk_menu_append (GTK_MENU (parent), menuitem);
#endif
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), (gpointer) data);
  gtk_widget_show (menuitem);
  return menuitem;
}
#endif 

void save_defaults (GtkWidget *parent)
{
  FILE *defaults;
  char *homedir;
  int len;
  struct stat h_stat;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFTREE_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
failed:
    if (parent) xf_dlg_error(parent,strerror(errno),_("Default xftreerc file cannot be created\n"));
    else my_show_message (_("Default xftreerc file cannot be created\n"));
    return;
  }
  /* if .xfce directory isnot there, create it. */
  snprintf (homedir, len, "%s/.xfce", (char *) getenv ("HOME"));
  if (stat(homedir,&h_stat) < 0){
	if (errno!=ENOENT) goto failed;
	if (mkdir(homedir,0770) < 0) goto failed;
  }

  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"), XFTREE_CONFIG_FILE);
  defaults = fopen (homedir, "w");
  free (homedir);

  if (!defaults)
  {
    if (parent) xf_dlg_error(parent,strerror(errno),_("Default xftreerc file cannot be created\n"));
    else my_show_message (_("Default xftreerc file cannot be created\n"));
    return;
  }
  fprintf (defaults, "# file created by xftree, if removed xftree returns to defaults.\n");
  fprintf (defaults, "# do a bitwise or for preferences with %u (0x%x) to enable smaller dialogs.\n",(unsigned int)SMALL_DIALOGS,SMALL_DIALOGS);
  
  fprintf (defaults, "preferences : %d\n", preferences);
  fprintf (defaults, "pixmap_level : %d\n", pixmap_level);
  fprintf (defaults, "smallTB : %d\n",stateTB[0] );
  fprintf (defaults, "largeTB : %d\n",stateTB[1] );
  fprintf (defaults, "custom_font :%s\n",(custom_font)?custom_font:"fixed");
  if (custom_home_dir) fprintf (defaults, "custom_home :%s\n",custom_home_dir);
  if (preferences & SAVE_GEOMETRY) {
	  /*printf("dbg:x=%d,y=%d\n",geometryX,geometryY);*/
	  fprintf (defaults, "geometry : %d,%d\n",geometryX,geometryY);
  }
  fprintf (defaults, "ctree_color : %d,%d,%d\n",ctree_color.red,ctree_color.green,ctree_color.blue);
  
  fclose (defaults);
  return;
}

void read_defaults(void){
  FILE *defaults;
  char *homedir,*word;
  int len;

  /* default custom colors: */
  ctree_color.red = ctree_color.green = ctree_color.blue = 10000;
  /* default masks */
  /*stateTB[0] = 0x1fff;*/
  stateTB[0] = 0xffffffff;
  stateTB[1] = 0x801e1;
  
  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFTREE_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
    my_show_message (_("Default xftreerc file cannot be read\n"));
    return;
  }
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"),XFTREE_CONFIG_FILE );
  defaults = fopen (homedir, "r");
  free (homedir);

  if (!defaults) return;
  
  if (custom_home_dir) free(custom_home_dir);
  custom_home_dir=NULL;

  homedir = (char *)malloc(256);
  while (!feof(defaults)){
	fgets(homedir,255,defaults);
	if (feof(defaults))break;
	if (strstr(homedir,"preferences :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		preferences=atoi(word);
	}
	if (strstr(homedir,"pixmap_level :")){
		int lpl;
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		lpl=atoi(word);
  		/* the default pixmap_level */
  		if (pixmap_level > 3) 
		pixmap_level=lpl;
	}
	if (strstr(homedir,"smallTB :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		stateTB[0]=atoi(word);
	}
	if (strstr(homedir,"largeTB :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		stateTB[1]=atoi(word);
	}
	if (strstr(homedir,"geometry :")){
		strtok(homedir,":");
		word=strtok(NULL,",");if (!word) break;
		geometryX=atoi(word);
		word=strtok(NULL,"\n");if (!word) break;
		geometryY=atoi(word);
	}
	if (strstr(homedir,"custom_font :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");if (!word) break;
		if (custom_font) free(custom_font);
		custom_font=(char *)malloc(strlen(word)+1);
		if (custom_font) strcpy(custom_font,word);
	}
	if (strstr(homedir,"custom_home :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");if (!word) break;
		custom_home_dir=(char *)malloc(strlen(word)+1);
		if (custom_home_dir) strcpy(custom_home_dir,word);
	}
if (strstr(homedir,"ctree_color :")){
		strtok(homedir,":");
		word=strtok(NULL,",");if (!word) break;
		ctree_color.red=atoi(word);
		word=strtok(NULL,",");if (!word) break;
		ctree_color.green=atoi(word);
		word=strtok(NULL,"\n");if (!word) break;
		ctree_color.blue=atoi(word);
	}
  }
  free(homedir);
  fclose(defaults);  
}

void cb_status_follows_expand(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  entry *en;
  GtkCTreeNode *root;
  root = find_root((GtkCTree *)ctree);

  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  
  win->preferences ^= STATUS_FOLLOWS_EXPAND;
  preferences = win->preferences;
  save_defaults (NULL);
  if (preferences & STATUS_FOLLOWS_EXPAND){
  	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), win->status_node);
  } else {
  	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
  }
  set_title_ctree(ctree,en->path);
}

void cb_show_status(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  entry *en;
  GtkCTreeNode *root;

  root = find_root((GtkCTree *)ctree);
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  
  win->preferences ^= SHOW_STATUS;
  preferences = win->preferences;
  if (preferences & SHOW_STATUS) {
	  if (!GTK_WIDGET_VISIBLE(GTK_WIDGET (win->status->parent))){ 
	      gtk_widget_show(GTK_WIDGET (win->status->parent));
	  } 
  } else {
	  if (GTK_WIDGET_VISIBLE(GTK_WIDGET (win->status->parent))){ 
	      gtk_widget_hide(GTK_WIDGET (win->status->parent));
	  } 
  }
  save_defaults (NULL);

  set_title_ctree(ctree,en->path);
}



void cb_short_titles(GtkWidget * widget, GtkWidget *ctree)
{
  cfg *win;
  entry *en;
  GtkCTreeNode *root;
  root = find_root((GtkCTree *)ctree);
  win = gtk_object_get_user_data (GTK_OBJECT (ctree)); 
  win->preferences ^= SHORT_TITLES;
  preferences = win->preferences;
  save_defaults (NULL);
  
  if (preferences & STATUS_FOLLOWS_EXPAND){
  	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), win->status_node);
  } else {
  	en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), root);
  }
  
  set_title_ctree(ctree,en->path);
}
#if 0
void
cb_toggle_preferences (GtkWidget * widget, gpointer data)
{
  int toggler;
  toggler = (long)(data);
  preferences ^= toggler;
  save_defaults (NULL);
}


void
cb_sizeKB (GtkWidget * widget, gpointer ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= SIZE_IN_KB;
  preferences = win->preferences;
  save_defaults (NULL);
  regen_ctree((GtkCTree *)ctree);  
}
#endif
void
cb_save_geo (GtkWidget * widget, gpointer ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= SAVE_GEOMETRY;
  preferences = win->preferences;
  save_defaults (NULL);
}

void
cb_use_rsync (GtkWidget * widget, gpointer ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= USE_RSYNC;
  preferences = win->preferences;
  save_defaults (NULL);
}

void
cb_doubleC_goto (GtkWidget * widget, gpointer ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= DOUBLE_CLICK_GOTO;
  preferences = win->preferences;
  save_defaults (NULL);
}
void
cb_drag_copy (GtkWidget * widget, gpointer ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= DRAG_DOES_COPY;
  preferences = win->preferences;
  save_defaults (NULL);
}
			  
void cb_custom_SCK(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top, _("Xftree handles keyboard shortcuts dynamically.\n" 
"This means that you can open a menu,highlight an entry\nand press a keyboard key to create the shortcut.")
	    );
}

void cb_help_iv(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top, _("The Xftree icon view is the ROX-filer. At the moment this has to be installed\n" 
"separately from XFCE. A ROX-filer specific for XFCE will soon be available.")
	    );
}

void cb_rsync(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top, _("Xftree allows you to drag and drop to/from\n"
			  "xftree windows running on different hosts.\n"
			  "You may use either scp or rsync for the copying.\n"
			  "User authentication is done by ssh.\n"
			  "See \"man ssh\" if you desire host based\n"
			  "instead of password based authentication.\n\n"
			  "Xftree will also accept drops from other\n"
			  "filemanagers running on different hosts.\n"
			  )
	    );
}

void cb_registered(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top, _("Xftree can register applications by filename or filetype\nby selecting \"Register\" from one of the menus or by selecting\n\"Register application\" on mouse double-click.\n" 
"If you wish to edit the applications that are already registered,\n please look at ~/.xfce/xtree.reg.")
	    );
}
	  
void cb_dnd_help(GtkWidget * item, GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  xf_dlg_info (win->top, _("Xftree handles drag and drop the following way:\n"
"SHIFT drag+drop --> move\n"
"CTRL drag+drop --> copy\n"
"SHIFT-CTRL drag+drop --> link\n"
"The default drag+drop action is move,\n" 
"but can be changed in the preferences menu.\n"
"To drag multiple files, use the middle button.\n"
"Also:\n"
"Multiple selection by button 1 click-and-drag\n"
"is available on the first click after a GOTO or REGEN.\n")
	);
}

void cb_default_SCK(GtkWidget * item,  GtkWidget * ctree)
{
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));	
  
  xf_dlg_info (win->top, _("Keyboard shortcuts are a fast way to access menu functions."));
	 
}

static GtkWidget *cat=NULL,*text;

void clear_cat (GtkWidget * widget, gpointer data)
{
  guint lg;
  if (text != NULL) {
	  lg = gtk_text_get_length (GTK_TEXT (text));
	  gtk_text_backward_delete (GTK_TEXT (text), lg);
  }
}

static void
on_ok (GtkWidget * widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (cat));
  gdk_window_withdraw ((GTK_WIDGET (cat))->window);
}

	  
void
show_cat (char *message)
{
  GtkWidget *bbox, *scrolled, *button;
  
  if ((!message) || (!strlen (message)))
    return;
  if (cat != NULL)
  {
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
    if (!GTK_WIDGET_VISIBLE (cat)) gtk_widget_show (cat);
    gdk_window_raise (cat->window);
    
    return;
  }

  cat = gtk_dialog_new ();
  gtk_container_border_width (GTK_CONTAINER (cat), 5);
  gtk_widget_set_usize(cat,350,200);

  gtk_window_position (GTK_WINDOW (cat), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (cat),_("Xftree results"));
  gtk_widget_realize (cat);
  if (preferences & SMALL_DIALOGS) gdk_window_set_decorations (cat->window,GDK_DECOR_BORDER);


  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->vbox), scrolled, TRUE, TRUE, 0);
  gtk_widget_show (scrolled);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  gtk_container_add (GTK_CONTAINER (scrolled), text);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->action_area), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);
  button = gtk_button_new_with_label (_("Clear"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (clear_cat), (gpointer) GTK_WIDGET (cat));

  button = gtk_button_new_with_label (_("Dismiss"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) GTK_WIDGET (cat));
  gtk_signal_connect (GTK_OBJECT (cat), "delete_event", GTK_SIGNAL_FUNC (on_ok), (gpointer) GTK_WIDGET (cat));

  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
  gtk_widget_show (text);
/*  set_icon (cat, _("Show disk usage ..."), put_some_icon_here_xpm); FIXME*/
  gtk_widget_show (cat);

  return;

}

xf_dirent *xf_opendir(char *path,GtkWidget *ctree){
      cfg *win;
      xf_dirent *diren;
      
      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      diren=(xf_dirent *)malloc(sizeof(xf_dirent));
      if (!diren) return NULL;
      diren->dir = NULL;
      diren->globstring= NULL;
      diren->glob_count=0;
     if ((!(win->preferences & FILTER_OPTION))||
        (!(win->filterOpts & FILTER_DIRS) && !(win->filterOpts & FILTER_FILES))){ /* no filtering */
       /* no filtering */
          diren->dir = opendir (path);
     } else {
       char *name=NULL;
    
       name=gtk_entry_get_text (GTK_ENTRY (win->filter));
       if ((name)&&(strlen(name)==0)) name="*";
       
       diren->globstring=(char *)malloc(strlen(name)+strlen(path)+2);
       diren->globstar=(char *)malloc(strlen("*")+strlen(path)+2);
       if (!diren->globstring) {
	       free(diren);
	       diren=NULL;
	       return diren;
       }
       diren->globstar=(char *)malloc(strlen("/*")+strlen(path)+1);
       if (!diren->globstring) {
	       free(diren->globstring);
	       free(diren);
	       diren=NULL;
	       return diren;
       }
             
       strcpy(diren->globstring,path);
       strcpy(diren->globstar,path);
       if (path[strlen(path)-1] != '/') {
	       strcat(diren->globstring,"/");
	       strcat(diren->globstar,"/");
       }
       strcat(diren->globstring,name);
       strcat(diren->globstar,"*");
       /*fprintf(stderr,"dbg:globbing %s\n",diren->globstring);*/
       /* if GLOB_PERIOD not in C compiler, then a fallback is
	* taken into consideration in io.h */
#ifndef GLOB_PERIOD
#endif
       /* if glob constants not defined (as in a very outdated C compiler),
	*  then both directories and files will be filtered */
#ifndef GLOB_ONLYDIR
#define GLOB_ONLYDIR 0
       win->filterOpts = (FILTER_DIRS|FILTER_FILES);
#endif
#ifndef GLOB_MARK
#define GLOB_MARK 0
       win->filterOpts = (FILTER_DIRS|FILTER_FILES);
#endif   
#ifndef GLOB_APPEND
#define GLOB_APPEND 0
       win->filterOpts = (FILTER_DIRS|FILTER_FILES);
#endif  
#ifndef GLOB_NOSPACE
#define GLOB_NOSPACE 34554
#endif         
       if (win->filterOpts == (FILTER_DIRS|FILTER_FILES)) {
          /*fprintf(stderr,"dbg:dirandfiles....(%d)**********\n",win->filterOpts);*/
	       /* both files and dirs are filtered */
	       if (glob (diren->globstring, GLOB_ERR | GLOB_PERIOD, NULL, &(diren->dirlist))
			       ==GLOB_NOSPACE){
		       fprintf(stderr,"xftree:insufficient resources for filtering %s\n",diren->globstring);
		       free(diren->globstring);
      	       	       globfree(&(diren->dirlist));
                       free (diren);
                       diren=NULL;
	       }
       } else if (win->filterOpts == FILTER_FILES){
          /*fprintf(stderr,"dbg:only files....**********\n");*/
	       /* only files are filtered. duplicate directories zapped at xf_readdir() */
	       if ((glob (diren->globstar, GLOB_ERR | GLOB_PERIOD | GLOB_ONLYDIR, 
		      NULL, &(diren->dirlist))
			       ==GLOB_NOSPACE)
		|| (glob (diren->globstring,
		      GLOB_ERR | GLOB_PERIOD | GLOB_APPEND | GLOB_MARK, 
		      NULL, &(diren->dirlist))
			       ==GLOB_NOSPACE)){
		       fprintf(stderr,"xftree:insuficient resources for filtering %s\n",diren->globstring);
		       free(diren->globstring);
      	       	       globfree(&(diren->dirlist));
                       free (diren);
                       diren=NULL;
	       }
	 			
       } 
       else { 
	       /* only directories are filtered */
          /*fprintf(stderr,"dbg:dirs....**********\n");*/
 	       if ((glob (diren->globstring, GLOB_ERR | GLOB_PERIOD | GLOB_ONLYDIR , 
		      NULL, &(diren->dirlist))
			       ==GLOB_NOSPACE)
		|| (glob (diren->globstar,
		      GLOB_ERR | GLOB_PERIOD | GLOB_APPEND |  GLOB_MARK, 
		      NULL, &(diren->dirlist))
			       ==GLOB_NOSPACE)){
		       fprintf(stderr,"xftree:insuficient resources for filtering %s\n",diren->globstring);
		       free(diren->globstring);
      	       	       globfree(&(diren->dirlist));
                       free (diren);
                       diren=NULL;
	       }
      }
     } /* end if filtered by glob */
     return diren;	      
}
xf_dirent *xf_closedir(xf_dirent *diren){
       /*fprintf(stderr,"dbg:at xf_closedir (%d)\n",(int)diren->dir);*/
      if (!diren) return NULL;
      if (diren->globstring) {
	      free(diren->globstring);
      	      globfree(&(diren->dirlist));
      }
      if (diren->dir) closedir (diren->dir);
      free (diren);
      diren=NULL;
      
      return diren;
}
	     
char *xf_readdir(xf_dirent *diren,GtkWidget *ctree){
      cfg *win;
      if (!diren) return NULL;
      
      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      if (!(win->preferences & FILTER_OPTION)|| 
        (!(win->filterOpts & FILTER_DIRS) && !(win->filterOpts & FILTER_FILES))){ /* no filtering */
        struct dirent *de;
        if (!diren->dir) return NULL;
	de = readdir (diren->dir);
	if (!de) return NULL; else return (de->d_name);
      } else {
        char *name;
	if (!diren) return NULL;
skip_it:
       /*fprintf(stderr,"dbg:at xf_readdir (%d)\n",(int)diren->dirlist.gl_pathc);*/
	
	if (diren->glob_count >= diren->dirlist.gl_pathc) return NULL;
        /*fprintf(stderr,"dbg:%s\n",diren->dirlist.gl_pathv[diren->glob_count]);*/
	if ((strlen(diren->dirlist.gl_pathv[diren->glob_count])>1)
		&&
	    (diren->dirlist.gl_pathv[diren->glob_count][strlen(diren->dirlist.gl_pathv[diren->glob_count])-1] == '/')){ 
		struct stat s;
		/* eliminate directories that are GLOB_MARKED */
		/* but dont eliminate symlinks that are GLOB_MARKED */
		diren->dirlist.gl_pathv[diren->glob_count][strlen(diren->dirlist.gl_pathv[diren->glob_count])-1]=0;
		if (lstat(diren->dirlist.gl_pathv[diren->glob_count],&s)<0){
		  diren->glob_count++;
	  	  goto skip_it;			
		} 
	       	if (!S_ISLNK (s.st_mode)){
		  diren->glob_count++;
		  goto skip_it;
		}
	}
	
	name=strrchr(diren->dirlist.gl_pathv[diren->glob_count++],'/');
	
	if (name == NULL) return NULL;
	/*printf("name=%s\n",name);*/
	if (strcmp(name,"/..")==0) goto skip_it;
	return name+1;
      }

}

char *abreviate(char *path)
{
    static char *shortpath=NULL,*w;
    if (shortpath) free(shortpath);
    shortpath=(char *)malloc(strlen(path)+4);
    if (!shortpath) return path;
    w=strrchr(path,'/');
    if (!w)  return path;
    if (w==path) return path;
    if (strlen(w)<2) return path;
    shortpath=(char *)malloc(strlen(path)+4);
    /*sprintf(shortpath,"...%s",w);*/
    sprintf(shortpath,"%s",w+1);
    return shortpath;
}

char *abreviateP(char *path)
{
    int i;
    static char *label=NULL;
    if (strlen(path)<=16) return path;
    
    if (label) free(label);
    label=(char *)malloc(strlen(path)+1);
    if (!label) return path;
    strcpy (label,path);
    label[8]='~';
    for (i=9;i<=16;i++) label[i]=label[strlen(label)-(16-i)];
    return label;
}

void cb_custom_home(GtkWidget *widget,gpointer ctree){
  cfg *win;
  char *entry_return;	
  if (custom_home_dir) free(custom_home_dir); 
  custom_home_dir=NULL;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  entry_return = (char *)xf_dlg_string (win->top,_("Home directory : "),getenv ("HOME"));
  if (!entry_return || !strlen(entry_return)){
	return;
  }
  custom_home_dir=(char *)malloc(strlen(entry_return)+1);
  if (!custom_home_dir) return;
  strcpy(custom_home_dir,entry_return);
  save_defaults(win->top);
  return;
}

void cleanup_tmpfiles(void){
   glob_t dirlist;
   int i;
   if (glob ("/tmp/xftree*", GLOB_ERR ,NULL, &dirlist) != 0) {
		  /*fprintf (stderr, "dbg:%s: no match\n", globstring);*/
	return;
   } else for (i = 0; i < dirlist.gl_pathc; i++) {
	   unlink(dirlist.gl_pathv[i]);
   }
}

gboolean
sane (char *bin)
{
  char *spath[52], *path, globstring[256],*b,*c;
  glob_t dirlist;
  int i;

   /*printf("dbg:getenv=%s\n",getenv("PATH")); */
  if (getenv ("PATH"))
  {
    path = (char *) malloc (strlen (getenv ("PATH")) + 2);
    strcpy (path, getenv ("PATH"));
    strcat (path, ":");
  }
  else
  {
    path = (char *) malloc (4);
    strcpy (path, "./:");
  }

  /*globstring = (char *) malloc (strlen (path) + strlen (bin) + 1);*/


  spath[0] = strtok (path, ":");
  for (i=1;i<52;i++) {
	spath[i] = strtok (NULL, ":");
 	if (spath[i] == NULL) break;
  }

  for (i=0;i<52 && spath[i]!=NULL;i++) {
        /* printf("spath=%s\n",spath[i]);*/
    b=g_strdup(bin);
    if (strchr(b,' ')) c=strtok(b," "); else c=b; 
    sprintf (globstring, "%s/%s", spath[i], c);
        /*printf("dbg: globstring=%s\n",globstring);*/
    g_free(b);
/*	 printf("checking for %s...\n",globstring);*/
    if (glob (globstring, GLOB_ERR, NULL, &dirlist) == 0)
    {
             /*printf("found at %s\n",globstring); */
      /*free (globstring);*/
      globfree (&dirlist);
      free (path);
      return TRUE;
    }
    globfree (&dirlist);
  }
  /*free (globstring);*/
  free(path);
  return FALSE;
}


