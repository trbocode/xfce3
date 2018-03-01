
/*
 * xtree_tar.c: general tar functions.
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
#include "xtree_cpy.h"
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

typedef struct tar_dir{
	GtkCTreeNode *node;
	char *name;
	struct tar_dir *next;
}tar_dir;

static tar_dir *headTar=NULL;

static tar_dir *push_tar_dir(GtkCTreeNode *node,char *name){
	struct tar_dir *p,*l=NULL;

		/*fprintf(stderr,"dbg:pushing %s\n",name);*/
	p=headTar;
	while (p){l=p; p=p->next;}
	p=(tar_dir *)malloc(sizeof(tar_dir));
	if (!p) return NULL;
	p->name=g_strdup (name);
	p->next=NULL;
	p->node=node;
	if (l) l->next=p; else headTar=p;
	return headTar;	
}

static int
compare_node_path (gconstpointer ptr1, gconstpointer ptr2)
{
  entry *en1 = (entry *) ptr1, *en2 = (entry *) ptr2;

  return strcmp (en1->path, en2->path);
}

static GtkCTreeNode *find_tar_dir(char *name){
	struct tar_dir *p;
	p=headTar;
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

static tar_dir *clean_tar_dir(void){
	struct tar_dir *p,*l;
	p=headTar;
	while (p){
	       l=p;	
       	       p=p->next;
	       g_free(l->name);
	       g_free(l);
	}
	return NULL;
}

/* dummy entry to get expander without expanding */
GtkCTreeNode *add_tar_dummy(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en){
   GtkCTreeNode *item;
   icon_pix pix;
   entry *en;
   gchar *text[COLUMNS];
   text[COL_NAME]=text[COL_DATE]=text[COL_SIZE]="";
   text[COL_MODE]=text[COL_UID]=text[COL_GID]="";
   if ((en = entry_new ())==NULL) return NULL;
   en->type =  FT_TARCHILD;
   en->label=g_strdup(".");
   en->path=g_strdup("tar:.");
   memcpy((void *)(&(en->st)),(void *)(&(p_en->st)),sizeof(struct stat));
   set_icon_pix(&pix,en->type,en->label,en->flags);   
   item=gtk_ctree_insert_node (ctree,parent, NULL, text, SPACING, 
		  pix.pixmap,pix.pixmask,
		  pix.open,pix.openmask,
		  TRUE,FALSE);
   gtk_ctree_node_set_row_data_full (ctree,item,en,node_destroy);
   return (item);   
}
#if 10
/* preliminary bug fix for non gnu generated tar file expansion */
static  GtkCTreeNode *parent_node(GtkCTree * ctree,char *path,GtkCTreeNode *top_level,int type,char *tarfile){
	GtkCTreeNode *Tnode,*s_item;
	char *c,*d,*N_path;
	entry *d_en;
	gchar *text[COLUMNS];
        icon_pix pix;  
	
	/*fprintf(stderr,"dbg: looking for %s\n",path);*/
	if (!strchr(path,'/') && !strchr(path,'\\')) return NULL;
	text[COL_DATE]=text[COL_SIZE]=text[COL_MODE]=text[COL_UID]=text[COL_GID]="";
	Tnode=find_tar_dir(path);
	if (!Tnode) {
 	  /*fprintf(stderr,"dbg: %s not found\n",path);*/
	  if ((d_en = entry_new ())==NULL){
		fprintf(stderr,"xftree: cannot continue xtree_tar.c\n");
                cleanup_tmpfiles();
		exit(1);
	  }
	  d_en->type =  FT_TARCHILD|FT_DIR;
	  d_en->path=(char *)malloc(strlen(path)+strlen(tarfile)+strlen("tar:")+3);
	  if (!d_en->path) {
		  fprintf(stderr,"xftree:error 3342\n");
		  return NULL;
	  }
	  sprintf(d_en->path,"tar:%s:%s",tarfile,path);
	  /* buggy: d_en->path=g_strdup(path);*/
	  c=g_strdup(path);
	  
	  
	  if (strstr(c,"/")) {d=strrchr(c,'/');*d=0;}
	  else if (strstr(c,"\\")) {d=strrchr(c,'\\');*d=0;}
	  
	  if (strstr(c,"/")) d=strrchr(c,'/')+1;
	  else if (strstr(c,"\\")) d=strrchr(c,'\\')+1;
	  else d=NULL;
	  if (d) d_en->label=g_strdup(d);
	  else d_en->label=g_strdup(path); 
	  g_free(c);
	  text[COL_NAME] = d_en->label;
	  set_icon_pix(&pix,FT_TARCHILD|FT_DIR|type,path,IGNORE_HIDDEN);
  
	  N_path=g_strdup(path);
	  if (strstr(N_path,"/")) c=strrchr(N_path,'/');else c=strrchr(N_path,'\\');
	  if (c) *c=0;
  	  if ((strlen(N_path))&&(strstr(N_path,"/")||strstr(N_path,"\\"))){  
		if (strstr(N_path,"/")) c=strrchr(N_path,'/');
		else c=strrchr(N_path,'\\');
		c[1]=0;

		if (!Tnode) Tnode=parent_node(ctree,N_path,top_level,type,tarfile);
	  } else Tnode=top_level;
 	  
	  /*fprintf(stderr,"dbg:inserting %s \n",path);*/
	  s_item=gtk_ctree_insert_node (ctree,Tnode, NULL, text, SPACING, 
		pix.pixmap,pix.pixmask,pix.open,pix.openmask,
		FALSE,FALSE);
	  if (s_item) {
	        headTar=push_tar_dir(s_item,path);
	        gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
	        /*fprintf(stderr,"subitem inserted\n");*/
		return s_item;
	  }
	  
          g_free(N_path);
	} 
	/*else fprintf(stderr,"dbg: %s present!\n",path);*/
	return Tnode;
}
#endif
GtkCTreeNode *add_tar_tree(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en){
      GtkCTreeNode *s_item=NULL,*p_node;
      entry *d_en;
      FILE *pipe;
      char *cmd;
      gchar *text[COLUMNS],date[32],size[32],mode[12],uid[32],gid[32];
      cfg *win;
      gboolean nopipe=TRUE;

      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      text[COL_DATE]=date;
      text[COL_SIZE]=size;
      text[COL_MODE]=mode;
      text[COL_UID]=uid;
      text[COL_GID]=gid;

      cmd=(char *)malloc(strlen("tar -vtzZf --use-compress-program bunzip2 ")+strlen(p_en->path)+1);
      if (!cmd) return NULL;
      if (p_en->type & FT_GZ) sprintf (cmd, "tar -vtzf \"%s\"", p_en->path);
      else if (p_en->type & FT_COMPRESS ) sprintf (cmd, "tar -vtZf \"%s\"", p_en->path);
      else if (p_en->type & FT_BZ2 ) sprintf (cmd, "tar --use-compress-program bunzip2 -vtf \"%s\"", p_en->path);
      else sprintf (cmd, "tar -vtf \"%s\"", p_en->path);
      /*printf("dbg:%s\n",cmd);*/
      pipe = popen (cmd, "r");
      g_free(cmd);
      if (pipe) {
              icon_pix pix;  
	      char *p,*d,*u,*t;
	      char line[256];
	      while (!feof(pipe) && fgets (line, 255, pipe)){
		      char *l;
      		      if ((d_en = entry_new ())==NULL) {
			      pclose (pipe);
			      return NULL;
		      }
		      nopipe=FALSE;
		      d_en->type =  FT_TARCHILD;
		      /*fprintf(stderr,"dbg:%s",line);*/
		      /* mode */
		      strcpy(text[COL_MODE],strtok(line," "));
		      d_en->st.st_mode=0;
		      if (strlen(line)>8) {
		       if (line[0]=='r')d_en->st.st_mode |= 0400; 
		       if (line[1]=='w')d_en->st.st_mode |= 0200; 
		       switch (*(line+2)){
			      case 'x':  d_en->st.st_mode |= 0100;break;
			      case 's':  d_en->st.st_mode |= 04100;break;
			      case 'S':  d_en->st.st_mode |= 04000;break;
		       }
		       if (line[3]=='r')d_en->st.st_mode |= 040; 
		       if (line[4]=='w')d_en->st.st_mode |= 020; 
		       switch (*(line+5)){
			      case 'x':  d_en->st.st_mode |= 010;break;
			      case 's':  d_en->st.st_mode |= 02010;break;
			      case 'S':  d_en->st.st_mode |= 02000;break;
		       }
		       if (line[6]=='r')d_en->st.st_mode |= 04; 
		       if (line[7]=='w')d_en->st.st_mode |= 02; 
		       switch (*(line+8)){
			      case 'x':  d_en->st.st_mode |= 01;break;
			      case 't':  d_en->st.st_mode |= 01001;break;
			      case 'T':  d_en->st.st_mode |= 01000;break;
		       }
		      }
		      /* uid/gid */
		      u=strtok(NULL," ");
		      /* size */
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
		      /* date */
		      t=strtok(NULL,"\n");
		      /*printf("dbg:tp=%s\n",t);*/
		      /* name */
		      /* symlink */
		      if ((l=strstr(t," ->"))!=NULL){
			     d_en->type|=FT_LINK;
			     l=strstr(t," ->");
			     l[0]=0;l++; 
			     /*printf("dbg:symlink %s\n",l);*/
		      }
		       
		      p=strrchr(t,' '); 
		      if (l) t=l;
		      if (!p) p="?";
		      else {p[0]=0;p++;}
		      while (*p ==' ' || *p=='\t') p++;
		
		      /*printf("dbg:t=%s\n",t);printf("dbg:p=%s\n",p);*/
		      strcpy(text[COL_DATE],t);
		      
		      if ((p[strlen(p)-1]=='/')||(p[strlen(p)-1]=='\\'))  d_en->type |=  FT_DIR;
		      else d_en->type |=  FT_FILE;
		      d_en->path =(char *)malloc(strlen("tar:")+strlen(p_en->path)+1+strlen(p)+1);
		      if (!d_en->path) {
			      pclose (pipe);
			      return NULL;
		      }
		      /*sprintf(d_en->path,"tar:%s:%s",p_en->path,p); //double colon format */
		      sprintf(d_en->path,"tar:%s/%s",p_en->path,p);
		      /* use either / or \ */
		      /*d=strrchr(d_en->path,':')+1;*/
		      d=p;
		      if ((!(d_en->type &  FT_DIR))&&(strstr(d,"/")||strstr(d,"\\"))){
			      if (strstr(d,"/")) d=strrchr(d_en->path,'/')+1;
			      else if (strstr(d,"\\")) d=strrchr(d_en->path,'\\')+1;
		      } 				   
		      d_en->label=g_strdup (d);
		      
		      if (win->preferences&ABREVIATE_PATHS) 
			      text[COL_NAME] = (d_en->type &  FT_DIR)?
				      abreviate(d_en->label):abreviateP(d_en->label);
		      else  text[COL_NAME] = d_en->label;
			      
		      strcpy(text[COL_GID],u);
		      strcpy(text[COL_UID],strtok(u,"/"));
		      if (strstr(text[COL_GID],"/"))text[COL_GID]=strrchr(text[COL_GID],'/')+1;	      

		      {
			      char *P_path,*d=NULL;
			      P_path=g_strdup (p);
			      if (strstr(P_path,"/")) d=strrchr(P_path,'/');
			      else if (strstr(P_path,"\\")) d=strrchr(P_path,'\\');
			      if (d) d[1]=0;
			      /* non gnu tar generated view fix, preliminary */
			      p_node=parent_node(ctree,P_path,parent,
					      p_en->type&(FT_COMPRESS|FT_BZ2|FT_GZ),p_en->path);
			      if (!p_node) p_node=parent; /* error fallback */
			      g_free(P_path);
			      
		      }
		      if (!(d_en->type &  FT_DIR)) {
			/* check if already in */	
			s_item=gtk_ctree_find_by_row_data_custom (ctree, 
				find_root((GtkCTree *)ctree), 
			        d_en, compare_node_path);
			if (!s_item)	{      
	      	          set_icon_pix(&pix,d_en->type,d_en->label,d_en->flags);
		          s_item=gtk_ctree_insert_node (ctree,p_node, NULL, text, SPACING, 
		  		pix.pixmap,pix.pixmask,pix.open,pix.openmask,
				(d_en->type &  FT_DIR)?FALSE:TRUE,FALSE);
		          if (s_item) {
		            if (d_en->type &  FT_DIR) headTar=push_tar_dir(s_item,p);
		            gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
			    /*fprintf(stderr,"subitem inserted\n");*/
		          }
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
	 text[COL_NAME] = _("tar: execution error");
	 /* use open pixmaps for error situation */
	 s_item=gtk_ctree_insert_node (ctree,parent, NULL, text, SPACING, 
	  		pix.open,pix.openmask,NULL,NULL,TRUE,FALSE);
	 if (s_item) {
		 gtk_ctree_node_set_row_data_full (ctree, s_item, d_en, node_destroy);
	 }
      }
      /*fprintf(stderr,"dbg:done inserting tar stuff\n");*/
      headTar=clean_tar_dir();
      gtk_ctree_sort_node (ctree, parent);
      return s_item;
}


#define TAR_CMD_LEN 1024
static char tar_cmd[TAR_CMD_LEN];
static char tar_tgt[TAR_CMD_LEN];
static gboolean tar_cmd_error;
static GtkWidget *tar_parent;
static GtkCTree *tar_ctree;
static void *tar_fork_obj=NULL;
static GtkCTreeNode *tar_node;

static void tubo_cmd(void){
	char *args[10];
	int i;
	int status;
	args[0]=strtok(tar_cmd," ");
	if (args[0]) for (i=1;i<10;i++){
		args[i]=strtok(NULL," ");
		if (!args[i]) break;
	}
	i=fork();
	if (!i){
	   execvp(args[0],args);
	   _exit(123);
	}
	wait(&status);
	_exit(123);
}

int tar_output;
static void tubo_cmdE(void){
	  char *args[10];
	  int i;
	  if (tar_output) {close(tar_output);tar_output=0;}
	  /*fprintf(stdout,"forked:%s\n",tar_cmd);*/
	  args[0]=strtok(tar_cmd," ");
	  if (args[0]) for (i=1;i<10;i++){
		args[i]=strtok(NULL," ");
		if (!args[i]) break;
  	  }
	  /*fprintf(stdout,"args:\n");*/
	  for (i=0;i<10;i++){
		  if (!args[i]) break;
		  fprintf(stdout,"%s ",args[i]);
	  }
	  /*fprintf(stdout,"\n");*/
	  if (args[0]) execvp(args[0],args);
	  _exit(123);
}

/* function to process stderr produced by child */
static int rwStderr (int n, void *data){
  char *line;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  /*fprintf(stderr,"dbg (child):%s\n",line);*/
  tar_cmd_error=TRUE;
  xf_dlg_error(tar_parent,line,NULL);
  return TRUE;
}
/* function to process stdout produced by child */
static int rwStdout (int n, void *data){
  char *line;
  if (!tar_output) return TRUE;
  if (n) {/* this would mean binary data */
	  write(tar_output,data, n);
	  /*write(1,data, n);*/
  } else{
	  line = (char *) data;
	  write(tar_output,data, strlen(line));
	  /*write(1,data, strlen(line));*/
  }
  return TRUE;
}

/* function called when child is dead */
static void rwForkOver (pid_t pid)
{
  if (tar_cmd_error) {
    /*fprintf(stderr,"dbg fork is over with error\n");*/
  } else {
    /*fprintf(stderr,"dbg fork is over OK\n");*/
    if (tar_node) {
	gtk_ctree_unselect(tar_ctree,(GtkCTreeNode *)tar_node);
        gtk_ctree_remove_node (tar_ctree,(GtkCTreeNode *)tar_node);
        while (gtk_events_pending()) gtk_main_iteration();
    }
  }
  if (tar_output) {
	  close(tar_output);
	  tar_output=0;
  }
  tar_fork_obj=NULL;
  update_timer (tar_ctree);
}
/* function called when child is dead */
static void rwForkOverE (pid_t pid)
{
  if (tar_output) {
	  close(tar_output);
	  tar_output=0;
  }
  tar_fork_obj=NULL;
  update_timer (tar_ctree);
}


static int inner_tar_delete(GtkCTree *ctree,char *path){
	char *tar_file,*tar_entry;
	cfg *win;
	entry check;
	int type=0;

	/*fprintf(stderr,"dbg:del path=%s\n",path);*/
	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
	check.path=g_strdup(path);
	tar_node=gtk_ctree_find_by_row_data_custom (ctree, 
			find_root((GtkCTree *)ctree), 
			&check, compare_node_path);

	if (strncmp(check.path,"tar:",strlen("tar:"))!=0) {errno=EFAULT;return -1;}
	if (strchr(strchr(check.path,':')+1,':')) {
	 /* double colon format, not standard */
	strtok(check.path,":");
	tar_file=strtok(NULL,":"); if (!tar_file){errno=EFAULT;return -1;}
	tar_entry=strtok(NULL,"\n"); if (!tar_entry){errno=EFAULT;return -1;}
	} else {
	  char *p,*g;
	  struct stat s;
	  p=g_strdup(strchr(check.path,':')+1);
	  while (stat(p,&s)<0){
		  g=strrchr(p,'/');
		  if (!g){
			  fprintf(stderr,"xftree: error 989-2\n");/* should not happen */
			  errno=EFAULT;
			  return -1;
		  }
		  *g=0;
	  }
	  tar_file=strchr(check.path,':')+1;
	  tar_file[strlen(p)]=0;
	  tar_entry=tar_file+strlen(p)+1;
	  g_free(p);
	}

	
	if (strchr(strchr(check.path,':')+1,':')) {
	 /* double colon format, not standard */
	 tar_entry=strrchr(check.path,':');
	 if (!tar_entry) return FALSE;
	 *tar_entry='+';	
	 tar_file=strrchr(check.path,':');
	 if (!tar_file) return FALSE;
	 tar_file++;
	 *tar_entry=0;
	 tar_entry++;
	} else {
	  char *p,*g;
	  struct stat s;
	  p=g_strdup(strchr(check.path,':')+1);
	  while (stat(p,&s)<0){
		  g=strrchr(p,'/');
		  if (!g){
			  fprintf(stderr,"xftree: error 989-1\n");/* should not happen */
			  return FALSE;
		  }
		  *g=0;
	  }
	  tar_file=strchr(check.path,':')+1;
	  tar_file[strlen(p)]=0;
	  tar_entry=tar_file+strlen(p)+1;
	  g_free(p);
	}
	/* avoid directories with spaces */
	{
	  char *ddir;
	  ddir=tar_file;
	  tar_file=strrchr(tar_file,'/')+1;
	  *(strrchr(ddir,'/'))=0;
	  chdir(ddir);
	}
	
	
/*	strtok(check.path,":");
	tar_file=strtok(NULL,":"); if (!tar_file){errno=EFAULT;return -1;}
	tar_entry=strtok(NULL,"\n"); if (!tar_entry){errno=EFAULT;return -1;}*/
	if (strlen("tar -zZ --use-compress-program bunzip2 --delete  -f ")+strlen(tar_file)+1+strlen(tar_entry)+1 >TAR_CMD_LEN){
		errno=EIO;return -1;
	}
	
	{
	  char *w;
	  w=strrchr(tar_file,'.');
      	  if ( (strcmp(w,".tgz")==0)||(strcmp(w,".gz")==0) ) type = FT_GZ;
	  else if (strcmp(w,".Z")==0)  type = FT_COMPRESS;
	  else if (strcmp(w,".bz2")==0) type = FT_BZ2; 
	}
	
	if (type & FT_GZ) sprintf (tar_cmd, "tar -zf %s --delete %s",tar_file,tar_entry);
	else if (type & FT_COMPRESS ) sprintf (tar_cmd, "tar -Zf %s --delete %s",tar_file,tar_entry);
	else if (type & FT_BZ2 ) sprintf (tar_cmd, "tar --use-compress-program bunzip2 -f %s --delete %s",tar_file,tar_entry);
        else  sprintf (tar_cmd, "tar -f %s --delete %s",tar_file,tar_entry);
        /*printf("dbg:%s\n",tar_cmd);*/

	tar_cmd_error=FALSE;
	tar_parent=win->top;
	tar_ctree=ctree;
	tar_fork_obj=Tubo (tubo_cmd, rwForkOver, 0, rwStderr, rwStderr);
	usleep(50000);
	g_free(check.path);
        return 0;
	
}

int tar_delete(GtkCTree *ctree,char *path){
	while (1){
		if (!tar_fork_obj){
			return inner_tar_delete(ctree,path);
			break;
		}
                while (gtk_events_pending()) gtk_main_iteration();
		usleep(50000);
	}
	return 0;
}

int tar_extract(GtkCTree *ctree,char *tgt,char *o_src){
	char *tar_file,*tar_entry,*loc;
	static char *src=NULL;
	entry *en;
	cfg *win;
	entry check;
	mode_t mode=S_IRUSR|S_IWUSR|S_IRGRP;

	if (src) g_free(src);
	src=g_strdup(o_src);
	if (!src) return FALSE;
	
	tar_ctree=ctree;
	
	while (tar_fork_obj){
                while (gtk_events_pending()) gtk_main_iteration();
		usleep(50000);
	}
	if (src[strlen(src)-1]=='\n') src[strlen(src)-1]=0;
	if (src[strlen(src)-1]=='\r') src[strlen(src)-1]=0;
	/*fprintf(stderr,"tgt=%s\n",tgt);
	fprintf(stderr,"src=%s\n",src);*/

	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
/* FIXME: mode setting dont always work because the search might be done in the wrong xftree window. 
 *        this is not the case with a "delete" command */
	check.path=g_strdup(src);
	tar_node=gtk_ctree_find_by_row_data_custom (ctree, 
			find_root((GtkCTree *)ctree), 
			&check, compare_node_path);
        en = gtk_ctree_node_get_row_data (GTK_CTREE (tar_ctree), tar_node);
	if (en && (en->st.st_mode)) mode= en->st.st_mode;
	
	/*if (!en) fprintf(stderr,"dbg:entry not found\n");
	else fprintf(stderr,"dbg:mode=0%o\n",en->st.st_mode);*/

	if (strchr(strchr(src,':')+1,':')) {
	 /* double colon format, not standard */
	 tar_entry=strrchr(src,':');
	 if (!tar_entry) return FALSE;
	 *tar_entry='+';	
	 tar_file=strrchr(src,':');
	 if (!tar_file) return FALSE;
	 tar_file++;
	 *tar_entry=0;
	 tar_entry++;
	} else {
	  char *p,*g;
	  struct stat s;
	  p=g_strdup(strchr(src,':')+1);
	  while (stat(p,&s)<0){
		  g=strrchr(p,'/');
		  if (!g){
			  fprintf(stderr,"xftree: error 989-1\n");/* should not happen */
			  return FALSE;
		  }
		  *g=0;
	  }
	  tar_file=strchr(src,':')+1;
	  tar_file[strlen(p)]=0;
	  tar_entry=tar_file+strlen(p)+1;
	  g_free(p);
	}
	/* avoid directories with spaces */
	{
	  char *ddir;
	  ddir=tar_file;
	  tar_file=strrchr(tar_file,'/')+1;
	  *(strrchr(ddir,'/'))=0;
	  chdir(ddir);
	}
	
	if (strlen("tar --use-compress-program bunzip2 -OZ -f %% -x %%")+strlen(tar_file)+1+strlen(tar_entry)+1 >TAR_CMD_LEN){
		errno=EIO;return 0;
	}
        loc=strrchr(tar_file,'.');
	if (!loc) { 
		return FALSE;
	}
	
	if ( (strcmp(loc,".tgz")==0)||(strcmp(loc,".gz")==0) )
		sprintf(tar_cmd,"tar -Oz -f %s -x %s ",tar_file,tar_entry);
	else if (strcmp(loc,".Z")==0)	
		sprintf(tar_cmd,"tar -OZ -f %s -x %s ",tar_file,tar_entry);
	else if (strcmp(loc,".bz2")==0)	
		sprintf(tar_cmd,"tar -O --use-compress-program bunzip2 -f %s -x %s ",tar_file,tar_entry);
	else
		sprintf(tar_cmd,"tar -O -f %s -x %s ",tar_file,tar_entry);
	/*fprintf(stderr,"%s\n",tar_cmd);*/
	/*fprintf(stderr,"tgt=%s\n",tgt);*/
	
        if ((tar_output=creat(tgt,mode))<0){
		  fprintf(stderr,"open:%s(%s)\n",strerror(errno),tgt);
		  return FALSE;
	}	  
	strcpy(tar_tgt,tgt);	  
	tar_fork_obj=Tubo (tubo_cmdE, rwForkOverE, 0, rwStdout, rwStderr);
	usleep(50000);
        return TRUE;
}

static char *tar_open_with(GtkCTree *ctree,char *src,char *loc){
   char *tgt;
   tgt=randomTmpName(loc);
   if (!tar_extract(ctree,tgt,src)) return NULL;
   return tgt;  
}

void cb_tar_open_with (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  cfg *win;
  GtkCTreeNode *node;
  char *prg,*tgt;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (!count_selection (ctree, &node))
  {
    xf_dlg_warning (win->top,_("No files marked !"));
    return;
  }
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  prg = reg_app_by_file (win->reg, en->path);
  
  tgt=tar_open_with(ctree,en->path,en->label);
  if (!tgt) fprintf(stderr,"Could not extract %s\n",en->path);
  else xf_dlg_open_with ((GtkWidget *)ctree,win->xap, prg ? prg : DEF_APP, tgt);
}





