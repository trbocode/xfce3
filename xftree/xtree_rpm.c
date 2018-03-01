
/*
 * xtree_rpm.c: general rpm functions.
 *
 * Copyright (C) 
 *
 * Edscott Wilson Garcia 2002, for xfce project
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
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkenums.h>
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
#include "xtree_mess.h"
#include "xtree_pasteboard.h"
#include "xtree_go.h"
#include "xtree_cb.h"
#include "xtree_toolbar.h"
#include "xtree_functions.h"
#include "xtree_icons.h"
#include "tubo.h"

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifndef GLOB_TILDE
#define GLOB_TILDE 0
#endif

typedef struct rpm_dir{
	GtkCTreeNode *node;
	char *name;
	struct rpm_dir *next;
}rpm_dir;

static rpm_dir *headRpm=NULL;

static rpm_dir *push_rpm_dir(GtkCTreeNode *node,char *name){
	struct rpm_dir *p,*l=NULL;

		/*fprintf(stderr,"dbg:pushing %s\n",name);*/
	p=headRpm;
	while (p){l=p; p=p->next;}
	p=(rpm_dir *)malloc(sizeof(rpm_dir));
	if (!p) return NULL;
	p->name=g_strdup (name);
	p->next=NULL;
	p->node=node;
	if (l) l->next=p; else headRpm=p;
	return headRpm;	
}

static GtkCTreeNode *find_rpm_dir(char *name){
	struct rpm_dir *p;
	p=headRpm;
		/*fprintf(stderr,"dbg:looking for %s\n",name);*/
	if (!name) return NULL;
	while (p){
	       if (strcmp(p->name,name)==0) break;
		/*fprintf(stderr,"dbg:%s <-> %s\n",p->name,name);*/
       	       p=p->next;
	}
	if (p) {
		/*fprintf(stderr,"dbg:found parent at %s\n",p->name);*/
		return p->node;
	}
	return NULL;
}
static rpm_dir *clean_rpm_dir(void){
	struct rpm_dir *p,*l;
	p=headRpm;
	while (p){
	       l=p;	
       	       p=p->next;
	       if (l->name) g_free(l->name);
	       g_free(l);
	}
	return NULL;
}


static  GtkCTreeNode *parent_node(GtkCTree * ctree,char *path,GtkCTreeNode *top_level,char *rpmfile){
	GtkCTreeNode *Tnode,*s_item;
	char *c,*d,*N_path;
	entry *d_en;
	gchar *text[COLUMNS];
        icon_pix pix;  
	
	/*fprintf(stderr,"dbg: looking for %s\n",path);*/
	text[COL_DATE]=text[COL_SIZE]=text[COL_MODE]=text[COL_UID]=text[COL_GID]="";
	Tnode=find_rpm_dir(path);
	if (!Tnode) {
 	  /*fprintf(stderr,"dbg: %s not found\n",path);*/
	  if ((d_en = entry_new ())==NULL){
		fprintf(stderr,"xftree: cannot continue xtree_rpm.c\n");
                cleanup_tmpfiles();
		exit(1);
	  }
	  d_en->type =  FT_TARCHILD|FT_DIR;
	  d_en->path=(char *)malloc(strlen(path)+strlen(rpmfile)+strlen("rpm:")+3);
	  if (!d_en->path) {
		  fprintf(stderr,"xftree: error 4776\n");
		  return NULL;
	  }
	  sprintf(d_en->path,"rpm:%s:%s",rpmfile,path);
	  /* no good: d_en->path=g_strdup(path);*/
	  c=g_strdup(path);
	  
	  
	  if (strstr(c,"/")) {d=strrchr(c,'/');*d=0;}
	  
	  if (strstr(c,"/")) d=strrchr(c,'/')+1;
	  else d=NULL;
	  if (d) d_en->label=g_strdup(d);
	  else d_en->label=g_strdup(path); 
	  g_free(c);
	  text[COL_NAME] = d_en->label;
	  /*printf("dbg:label=%s,path=%s\n",d_en->label,d_en->path);*/
	  set_icon_pix(&pix,FT_RPMCHILD|FT_DIR,path,IGNORE_HIDDEN);
  
	  N_path=g_strdup(path);
	  if (strstr(N_path,"/")) c=strrchr(N_path,'/');
	  if (c) *c=0;
  	  if ((strlen(N_path))&&strstr(N_path,"/")){  
		c=strrchr(N_path,'/');
		c[1]=0;
		if (!Tnode) Tnode=parent_node(ctree,N_path,top_level,rpmfile);
	  } else Tnode=top_level;
 	  
	  /*fprintf(stderr,"dbg:inserting %s \n",path);*/
	  s_item=gtk_ctree_insert_node (ctree,Tnode, NULL, text, SPACING, 
		pix.pixmap,pix.pixmask,pix.open,pix.openmask,
		FALSE,FALSE);
	  if (s_item) {
	        headRpm=push_rpm_dir(s_item,path);
	        gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
	        /*fprintf(stderr,"subitem inserted\n");*/
		return s_item;
	  }
	  
          g_free(N_path);
	} 
	/*else fprintf(stderr,"dbg: %s present!\n",path);*/
	return Tnode;
}

GtkCTreeNode *add_rpm_tree(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en){
      GtkCTreeNode *s_item=NULL;
      GtkCTreeNode *p_node;
      entry *d_en;
      FILE *pipe;
      char *ddir,*dpath;
      char *cmd,*p;
      gchar *text[COLUMNS],date[32],size[32];
      cfg *win;
      gboolean nopipe=TRUE;

      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      text[COL_DATE]=date;
      text[COL_SIZE]=size;

      /* to handle spaces in paths: (quotes are buggy in rpm) */
      {
	ddir=g_strdup(p_en->path);
	if (!strrchr(ddir,'/')) return NULL;
	dpath=strrchr(ddir,'/')+1;
	*(strrchr(ddir,'/'))=0;
	if (ddir[0]==0) return NULL;
	chdir(ddir);
      }

      cmd=(char *)malloc(strlen("rpm --dump -qlp \"\"")+strlen(dpath)+1);
      if (!cmd) return NULL;
      sprintf (cmd, "rpm --dump -qlp \"%s\"", dpath);
      /*fprintf(stderr,"dbg:%s\n",cmd);*/
      pipe = popen (cmd, "r");
      g_free(ddir);
      g_free(cmd);
      if (pipe) {
              icon_pix pix;  
	      char *d;
	      char line[256];
	      while (!feof(pipe) && fgets (line, 255, pipe)){
		      nopipe=FALSE;
      		      if ((d_en = entry_new ())==NULL) {
			      pclose (pipe);
			      return NULL;
		      }
		      d_en->type =  FT_RPMCHILD;
		      /*fprintf(stderr,"dbg:%s",line);*/
		      p=strtok(line," ");
		      {
			char *tag="  ";
			unsigned long long tama;
			tama =  atol(strtok(NULL," "));
			if (tama >= (long long)1024*1024*1024*10){
				tama /= (long long)1024*1024*1024; tag=" GB";
			}
		  	else if (tama >= 1024*1024*10) {tama /= 1024*1024; tag=" MB";}
		  	else if (tama >= 1024*10) {tama /= 1024; tag=" KB";}
		  	sprintf (text[COL_SIZE], " %llu%s", tama,tag);
		      }
		      {
			time_t tt;
			struct tm *tm_v;
			tt=atol(strtok(NULL," "));
			tm_v=localtime(&tt);
			sprintf (text[COL_DATE], "%02d-%02d-%02d  %02d:%02d", 
					tm_v->tm_year+1900,tm_v->tm_mon,tm_v->tm_mday,
					tm_v->tm_hour,tm_v->tm_min);
		        /*strcpy(text[COL_DATE],ctime(&tt));*/
		      }
		      strtok(NULL," ");/*md5sum*/
		      {
			int modo;
			char *s;
			s=strtok(NULL," ");
			sscanf(s,"%o",&modo);				
		        text[COL_MODE]=mode_txt(modo);
			d_en->st.st_mode=modo;
		      }
		      text[COL_UID]=strtok(NULL," ");
		      text[COL_GID]=strtok(NULL," ");
		      /*sscanf(text[COL_MODE],"%o",&(d_en->st.st_mode));*/
		      
		      if (p[strlen(p)-1]=='/')  d_en->type |=  FT_DIR;
		      else d_en->type |=  FT_FILE;
		      d_en->path =(char *)malloc(strlen("rpm:")+strlen(p_en->path)+1+strlen(p)+1);
		      if (!d_en->path) {
			      pclose (pipe);
			      return NULL;
		      }
		      sprintf(d_en->path,"rpm:%s:%s",p_en->path,p);
		      d=strrchr(d_en->path,':')+1;
		      if ((!(d_en->type &  FT_DIR))&&strstr(d,"/")){
			      d=strrchr(d_en->path,'/')+1;
		      } 				   
		      d_en->label=g_strdup (d);
		      		           		      
		      if (win->preferences&ABREVIATE_PATHS) 
			      text[COL_NAME] = (d_en->type &  FT_DIR)?
				      abreviate(d_en->label):abreviateP(d_en->label);
		      else  text[COL_NAME] = d_en->label;
			      

		      {
			      char *P_path,*d=NULL;
			      P_path=g_strdup (p);
			      if (strstr(P_path,"/")) d=strrchr(P_path,'/');
			      if (d) d[1]=0;
			      p_node=parent_node(ctree,P_path,parent,p_en->path);
			      if (!p_node) p_node=parent; /* error fallback */
			      g_free(P_path);
			      
		      }
		      if (!(d_en->type &  FT_DIR)) {
	      	        set_icon_pix(&pix,d_en->type,d_en->label,d_en->flags);
		        s_item=gtk_ctree_insert_node (ctree,p_node, NULL, text, SPACING, 
		  		pix.pixmap,pix.pixmask,pix.open,pix.openmask,
				(d_en->type &  FT_DIR)?FALSE:TRUE,FALSE);
		        if (s_item) {
		          if (d_en->type &  FT_DIR) headRpm=push_rpm_dir(s_item,p);
		          gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
			  /*fprintf(stderr,"subitem inserted\n");*/
		        }
		      }
	      }
	      pclose (pipe);
      }
      if (nopipe) {
         icon_pix pix;  
	 set_icon_pix(&pix,FT_PD," ",IGNORE_HIDDEN);
	 if ((d_en = entry_new ())==NULL) return NULL;
	 d_en->type =  FT_RPM|FT_RPMCHILD;
	 d_en->path=d_en->label=NULL;
	 text[COL_DATE]=text[COL_SIZE]=text[COL_MODE]=text[COL_UID]=text[COL_GID]="";
	 text[COL_NAME] = _("rpm: execution error");
	 /* use open pixmaps for error situation */
	 s_item=gtk_ctree_insert_node (ctree,parent, NULL, text, SPACING, 
	  		pix.open,pix.openmask,NULL,NULL,TRUE,FALSE);
	 if (s_item) {
		 gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
	 }
      }
      /*fprintf(stderr,"dbg:done inserting rpm stuff\n");*/
      headRpm=clean_rpm_dir();
      gtk_ctree_sort_node (ctree, parent);
      return s_item;
}



