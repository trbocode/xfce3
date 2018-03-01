/*
 * xtree_cpy.c: these are the copy routines for xftree. The design
 *              is quite different from Rasca's originals, but 
 *              some of Rasca's ideas remain, and the input/output is
 *              enhanced (ewg).
 * 
 * copywrite 1999-2002 under GNU/GPL
 * Edscott Wilson Garcia, 
 * Olivier Fourdan, 
 * Rasca, Berlin
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
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_get.h"
#include "gtk_prop.h"
#include "gtk_dnd.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "../xfsamba/tubo.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "xtree_misc.h"
#include "xtree_functions.h"
#include "xtree_gui.h"
#include "xtree_cpy.h"
#include "xfce-common.h"
#include "xtree_mess.h"
#include "xtree_tar.h"
#include "tubo.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define X_OPT GTK_FILL|GTK_EXPAND|GTK_SHRINK

#define FORK_CHAR_LEN 255

#ifndef GLOB_TILDE
#define GLOB_TILDE 0x0
#endif

#ifndef GLOB_PERIOD
#define GLOB_PERIOD 0
#define EXTRA_ENTRIES 0
#else 
#define EXTRA_ENTRIES 2
#endif



#define CHILD_FILE_LENGTH 64
#define MAX_LINE_SIZE (sizeof(char)*1024)
static char child_file[CHILD_FILE_LENGTH];
static int  child_mode;
static int total_files=0;

static gboolean same_device=FALSE;
gboolean tar_extraction=FALSE;
static void *rw_fork_obj;
static GtkWidget *cat,*info[3],*progress;
static GtkWidget *tmp_ctree;
static gboolean I_am_child=FALSE,incomplete_target=FALSE;
static char *fork_target,*fork_source;
static int child_file_number,child_path_number;
static	char *holdfile;
static	char *targetdir;


static int ok_input(GtkWidget *parent,char *target,entry *s_en);
static gboolean force_override=FALSE;
static int internal_rw_file(char *target,char *source,off_t size);
static int rwStderr (int n, void *data);
static int rwStdout (int n, void *data);
static void rwForkOver (pid_t pid);
static void set_innerloop(gboolean state);
static int process_error(int code);
static void cb_cancel (GtkWidget * w, void *data);
static gboolean all_recursive=FALSE;

char *src_host=NULL;

/* tmo */
#define XTIME_IT
#ifdef TIME_IT
time_t tiempo;
#endif


int rsync(GtkCTree *ctree,char *src,char *tgt){
      char *cmd,*argv[6],*c;
      int status;
      pid_t pid;
      cfg *win;

      win = gtk_object_get_user_data (GTK_OBJECT (ctree));
      
      if (!sane("ssh")) {
	      xf_dlg_error(win->top,"Not found","ssh");
	      return FALSE;
      }
      
      if (win->preferences & USE_RSYNC) {
	      if (!sane("rsync")) {
		      xf_dlg_error(win->top,"Not found","rsync");
		      return FALSE;
	      }
	      c = "rsync -av --rsh=ssh";
      } else {
	      if (!sane("scp")){
		      xf_dlg_error(win->top,"Not found","scp");
		      return FALSE;
	      }
	      c = "scp -pvr";
      }
      
      cmd=(char *)malloc(strlen("echo \"%%\";")
		      +2*strlen(src_host)+2*strlen(src)+2*strlen(tgt)
		       +2*strlen(c)+2*strlen("%% %%:%% %%")+1);
      if (!cmd) return FALSE;
      sprintf(cmd,"echo \"%s %s:%s %s\";%s %s:%s %s",
		      c,src_host,src,tgt,
		      c,src_host,src,tgt);
      /*printf("dbg: %s\n",u->url);*/
      /*printf("dbg: %s\n",cmd);*/
      argv[0]=TERMINAL;
      argv[1]="-e";
      argv[2]="/bin/sh";
      argv[3]="-c";
      argv[4]=cmd;
      argv[5]=0;
      cursor_wait (GTK_WIDGET (ctree));
      pid = fork ();
      if (pid == -1) {
        g_free(cmd);
        cursor_reset (GTK_WIDGET (ctree));    
	return FALSE;
      }
      if (pid==0) { /* child process */
       	execvp (argv[0], argv); 
        /*perror (argv[0]);*/
	_exit(123);
      }
      g_free(cmd);

      do {
	while (gtk_events_pending()) gtk_main_iteration();
      } while (!waitpid(pid,&status,WNOHANG));
      cursor_reset (GTK_WIDGET (ctree));    
      update_timer (GTK_CTREE (ctree));
      return TRUE;
}



gboolean on_same_device(void){return same_device;}

/*
 * process_error sends the appropriate error text from
 * child process to parent for dialog creation.
 * code = return value from internal_rw_file()
 */
static int process_error(int code){
	char *message="runOver",*txt=NULL;
	gboolean runover=FALSE;
	switch (code){
	  case RW_ERRNO:
		  message=strerror(errno);
		  break;	
	  case RW_ERROR_MALLOC:
		  message=_("Insufficient system memory"); 
		  break;
	  case RW_ERROR_WRITING_DIR:
		  txt=targetdir; 
		  message=strerror(errno);
		  break;
	  case RW_ERROR_READING_SRC:
	  case RW_ERROR_OPENING_SRC:
	  case RW_ERROR_WRITING_SRC:
		  txt=fork_source; 
		  message=strerror(errno);
		  break;
	  case RW_ERROR_STAT_WRITE_TGT:
	  case RW_ERROR_WRITING_TGT:
	  case RW_ERROR_OPENING_TGT:
		  txt=fork_target;
		  message=strerror(errno); 
		  /*return code; */
		  break;
	  case RW_ERROR_TOO_FEW:
		  txt=fork_target;
		  message=_("Too few bytes transferred ! Device full ?"); 
		  break;
	  case RW_ERROR_TOO_MANY:
		  txt=fork_target;
		  message=_("Too many bytes transferred !?"); 
		  break;
	  case RW_ERROR_STAT_READ_SRC:
		  txt=fork_source; 
		  message=_("Can't stat() file"); 
		  break;
	  case RW_ERROR_STAT_READ_TGT:
		  txt=fork_source; 
		  message=_("Can't stat() file"); 
		  break;
	  case RW_ERROR_CLOSE_TGT:
		  txt=fork_target;
		  message=_("Error closing file"); 
		  break;		
	  case RW_ERROR_CLOSE_SRC:
		  txt=fork_source;
		  message=_("Error closing file"); 
		  break;
	  case RW_ERROR_UNLINK_SRC:
		  txt=fork_source;
		  message=_("Error deleting"); 
		  break;
	  case RW_ERROR_FIFO:
		  txt=fork_source;
		  message=_("Can't copy FIFO"); 
		  break;
	  case RW_ERROR_DEVICE:
		  txt=fork_source;
		  message=_("Can't copy device file"); 
		  break;
	  case RW_ERROR_SOCKET:
		  txt=fork_source;
		  message=_("Can't copy socket"); 
		  break;
	  default:runover=TRUE; break;
	}
	if (strstr(message,"\n")) strtok(message,"\n");
	if (I_am_child)
	{
		FILE *allF;
    		char *allfile;
		gboolean doall=FALSE;
		/* check for all flag */
    		allfile=(char *)malloc(strlen(child_file)+1+strlen(".all"));
	 	if (allfile) { 
		  sprintf(allfile,"%s.all",child_file);
		  allF=fopen(allfile,"r");
		  if (allF) {doall=TRUE;fclose(allF);}
        	  g_free(allfile);
		}
		if (!doall) {
		  FILE *hold;
		  fprintf(stdout,"child:%s %s\n",message,(txt)?txt:"*");
		  fflush(NULL);
		  if (runover) return TRUE;
		  holdfile=(char *)malloc(strlen(child_file)+1+strlen(".hold"));
	 	  if (!holdfile) return FALSE; /* will _exit() */
		  sprintf(holdfile,"%s.hold",child_file);
		  hold=fopen(holdfile,"w"); if (!hold) return FALSE;
		  fclose(hold);
		  /* active wait */
		  while ((hold=fopen(holdfile,"r"))!=NULL){
		        fprintf(stdout,"unclogging pipe\n");fflush(NULL);
		  	fclose(hold);
			usleep(500000);
		  }
		}
		return code; /* skip file with problem */
	}
	
	return code;
}



char *mktgpath(entry *ten,entry *sen){
  static char *target=NULL;
  if (target) g_free(target);
  target=(char *)malloc((strlen(ten->path)+strlen(sen->label)+2)*sizeof(char));
  if (!target) target="malloc error: mktgpath()\n";
  
  if (EN_IS_DIR (ten))
  {
    if (ten->path[strlen(ten->path)-1]=='/') /* do not add a slash */
	    sprintf (target, "%s%s", ten->path, sen->label);
    else 
	    sprintf (target, "%s/%s", ten->path, sen->label);
  }
  else
  {
    sprintf (target, "%s", ten->path);
  }
  return target;
}


void set_override(gboolean state)
{force_override=state;return;}

static int nitems;

char *randomTmpName(char *ext){
    static char *fname=NULL;
    int fnamelen;
    if (fname) g_free(fname);
    if (ext==NULL) fnamelen=strlen("/tmp/xftree.XXXXXX")+1;
    else fnamelen=strlen("/tmp/xftree.XXXXXX")+strlen(ext)+2;
    fname = (char *)malloc(sizeof(char)*(fnamelen));
    if (!fname) return NULL;
    sprintf(fname,"/tmp/xftree.XXXXXX");
    close(mkstemp(fname));
    if (ext) {
	    unlink(fname);
	    strcat(fname,"."); strcat(fname,ext);
    }
    return fname;
}

/*char *randomTmpName(char *ext){
    static char *fname=NULL;
    int fnamelen;
    long long id;
    if (ext==NULL) ext="tmp";
    if (fname) g_free(fname);
    id=random()*(9999.0/RAND_MAX);
    while (id > 9999) id /= 2; 
    fnamelen=strlen("/tmp/xftree.9999.")+strlen(ext)+1;
    srandom(time(NULL));
    fname = (char *)malloc(sizeof(char)*(fnamelen));
    if (!fname) return NULL;
    sprintf(fname,"/tmp/xftree.%lld.%s",id,ext);
    return fname;
}*/

static char *SimpleTmpList(GtkWidget *parent,char *tgt,char *src){
    static char *fname=NULL;
    FILE *tmpfile;
    if ((fname=randomTmpName(NULL))==NULL) return NULL;
    if ((tmpfile=fopen(fname,"w"))==NULL) return NULL;
    fprintf(tmpfile,"%d\t%s\t%s\n",TR_COPY,src,tgt);
    fclose(tmpfile);
    return fname;
}


char  *CreateTmpList(GtkWidget *parent,GList *list,entry *t_en){
    char *target;   
    FILE *tmpfile;
    uri *u;
    static char *fname=NULL;
    entry *s_en;
    
    nitems=0;
    if ((fname=randomTmpName(NULL))==NULL) return NULL;
    if ((tmpfile=fopen(fname,"w"))==NULL) return NULL;
    same_device=TRUE;
    tar_extraction=FALSE;
    for (;list!=NULL;list=list->next){
	struct stat t_stat;
        u = list->data;
	/*fprintf(stderr,"dbg:url=%s\n",u->url);*/
	s_en = entry_new_by_path (u->url);
	/* entry new has stated path by now */
	if (!s_en) {
		/*fprintf(stderr,"dbg:s_en is NULL\n");*/
		continue;
	}
	target=mktgpath(t_en,s_en);
	
	if (stat(target,&t_stat)<0){  /* follow link stat() */
		char *route,*end;
		route=(char *)malloc(strlen(target)+1);
		if (route) {
		  strcpy(route,target);
		  end=strrchr(route,'/');
		  if (end) {
			if (end==route) end[1]=0; /* root directory */
			else end[0]=0;
		  	if (stat(route,&t_stat)<0){
				t_stat.st_dev=113;
			}
		  }
		  g_free(route);  
		}
	}

	/*fprintf(stderr,"dbg:target=%s\n",target);*/
	switch (ok_input(parent,target,s_en)){
		case DLG_RC_SKIP:
			/*fprintf(stderr,"dbg:skipping %s\n",s_en->path);*/
		      		
			  break;
		case DLG_RC_CANCEL:  /* dnd cancelled */
			/*fprintf(stderr,"dbg:cancelled %s\n",s_en->path);*/
			 entry_free(s_en); 
    			 fclose (tmpfile);
			 unlink (fname);
			 return NULL;
		default: 
			 /*if ((u->type != URI_LOCAL)||(u->type != URI_FILE)) same_device=FALSE; else */
			 if (s_en->st.st_dev != t_stat.st_dev) same_device=FALSE;
			 
			 nitems++;
			 fprintf(tmpfile,"%d\t%s\t%s\n",u->type,s_en->path,target);
			 /*fprintf(stderr,"dbg:%d\t%s\t%s\n",u->type,s_en->path,target);*/
			 fflush(NULL);
			 break;
	} 
	entry_free(s_en); 	
    }
    fclose (tmpfile);
    /*fprintf(stderr,"dbg:nitems = %d\n",nitems);*/
    if (!nitems) {
       /*fprintf(stderr,"dbg:nitems = %d\n",nitems);*/
	    unlink(fname);
	    return NULL;
    }
    /*fprintf(stderr,"dbg: same device=%d\n",same_device);*/
    return fname;
}

/* function to check input before
 * packing off to copy/move/link
 * */
static int ok_input(GtkWidget *parent,char *target,entry *s_en){
  struct stat t_stat;
  char *source;
  gboolean target_exists=TRUE;
  
  source=s_en->path;
  if (lstat (target, &t_stat) < 0) {
	if (errno != ENOENT) return xf_dlg_error_continue (parent,target, strerror (errno));
	else target_exists=FALSE;
  }
  /*fprintf(stderr,"dbg:at okinput %s->%s\n",s_en->path,target);*/
	/* check for valid source */
  if (strstr(s_en->path,"tar:")){
          tar_extraction=TRUE;
  }

  else if (EN_IS_DIRUP (s_en) || !io_is_valid (s_en->label)){ 
        /*fprintf(stderr,"dbg:at okinput 1 \n");*/
	  return DLG_RC_SKIP;
  }
 
  /*fprintf(stderr,"dbg:%s->%s\n",source,target);*/
  
  /*fprintf(stderr,"dbg:at okinput 2 \n");*/
  /* target and source are the same */
  if ((target_exists) && (t_stat.st_ino == s_en->st.st_ino)) {
	  /*fprintf(stderr,"dbg:nonsense imput\n");*/
	  return DLG_RC_CANCEL;  
  }

  if (!tar_extraction){
   if (S_ISFIFO(s_en->st.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy FIFO") );
   }
   if (S_ISCHR(s_en->st.st_mode)||S_ISBLK(s_en->st.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy device file") );
   }
   if (S_ISSOCK(s_en->st.st_mode)){
	return xf_dlg_error_continue (parent,source,_("Can't copy socket") );
   }
  }
 
  /*fprintf(stderr,"dbg:at okinput 3 \n");*/
  /* check for overwrite, both files and directories */
  if ((target_exists) && (!force_override)) {
	int rc;
        rc=xf_dlg_new(parent,override_txt(target,s_en->path),NULL,NULL,DLG_CANCEL|DLG_OK|DLG_SKIP|DLG_ALL,1);
        if (rc == DLG_RC_SKIP) 	 return DLG_RC_SKIP;
        if (rc == DLG_RC_CANCEL) return DLG_RC_CANCEL;
        if (rc == DLG_RC_ALL)	 {
		force_override=TRUE;
		return DLG_RC_OK;
	}
  }
   
  return DLG_RC_OK;
}

static gboolean SubChildTransfer(char *target,char *source){
	struct stat s_stat,t_stat;
	int i,rc;
	
	if (stat(target,&t_stat)<0){  /* follow link for target: stat() */
		char *route,*end;
		route=(char *)malloc(strlen(target)+1);
		if (route) {
		  strcpy(route,target);
		  if ((end=strrchr(route,'/'))!=NULL) {
		    if (end==route) end[1]=0; /* root directory */
		    else end[0]=0;
		    stat(route,&t_stat);
		  }
		  g_free(route);
		}
	}
	/* do the link business before going recursive:
	 * (link directories, not contents) */
	/* (historical, TR_LINK should fall to DirectTransfer) */
	if (child_mode & TR_LINK){ 
	  printf("xftree:Calling IndirectTransfer for TR_LINK is obsolete (but works)\n");
	  if (symlink (source, target) < 0) {
	     return process_error(RW_ERROR_WRITING_TGT);
	  } else return TRUE;
	}

	lstat(source,&s_stat); /* stat() the link itself */

/* what if we find a symlink in our path? */       	
	if (S_ISLNK(s_stat.st_mode)) {
	  char *src_lnk;
	  int len;
	  struct stat st;
	  lstat(source,&st);
	  src_lnk=(char *)malloc(st.st_size+1);
	  if (!src_lnk) return TRUE;
	  len = readlink (source, src_lnk, st.st_size);
	  if (len <= 0) {fprintf(stderr,"xftree:%s\n",strerror(errno));return (TRUE); }
      	  src_lnk[len] = 0;
	  if (symlink (src_lnk, target) == -1){
	     g_free(src_lnk);
	     return process_error(RW_ERROR_WRITING_TGT);
	  }
	  g_free(src_lnk);
      	  if (child_mode & TR_MOVE) {
		  if (unlink (source) == ERROR) {
	            return process_error(RW_ERROR_WRITING_SRC);
		  }
	  }
	  return (TRUE);
	}

	/* recursivity, if src is directory:
	 * 1- mkdir tgt/src
	 * 2- glob src
	 * 3 foreach glob in globlist recall function.
	 * */
	/* non GNU fallback: wont copy hidden files in recursivity
	 * unless specifically selected */

	if (S_ISDIR(s_stat.st_mode)) { /* directories fall in here, going recursive */
  		glob_t dirlist;
		char *globstring,*newtarget,*src;
			
		globstring = (char *)malloc(strlen(source)+3);
		if (!globstring) return FALSE; /* fatal error */
		sprintf(globstring,"%s/*",source);
		/* create target dir */
		if (mkdir(target,(s_stat.st_mode|0700))<0){
			/* dont process error if directory exists */
			if (errno!=EEXIST){
		 	   targetdir=target;
			   process_error(RW_ERROR_WRITING_DIR);/* user intervention */
			}
			/* if function returns, it means continue */
			/*return process_error(RW_ERROR_WRITING_DIR);*/
			/*fprintf(stdout,"child:%s %s\n",strerror(errno),target);*/
		}
	  	/*fprintf(stderr,"dbg:dir created: %s\n",target);*/
		/* glob source dir */
		if (glob (globstring, GLOB_ERR | GLOB_TILDE | GLOB_PERIOD, 
					NULL, &dirlist) != 0) {
		  /*fprintf (stderr, "dbg:%s: no match\n", globstring);*/
			return TRUE;
		}
  		else for (i = 0; i < dirlist.gl_pathc; i++) {
		  if (strstr(dirlist.gl_pathv[i],"/")) src=strrchr(dirlist.gl_pathv[i],'/')+1;
		  else src = dirlist.gl_pathv[i];
		  if ((strcmp(src,".")==0)||(strcmp(src,"..")==0)) continue;
		  
		  newtarget=(char *)malloc(strlen(target)+strlen(src)+3);
		  if (!newtarget) {
			  g_free(globstring);
			  globfree(&dirlist);
			  return FALSE; /* fatal error */
		  }
		  sprintf(newtarget,"%s/%s",target,src);
		  /*fprintf(stderr,"dbg:dirlist: %s\n",dirlist.gl_pathv[i]);*/
		  if (!SubChildTransfer(newtarget,dirlist.gl_pathv[i])){
			/*fprintf(stderr,"dbg:dirlisterror: %s\n",dirlist.gl_pathv[i]);*/
	  		g_free(globstring);
		 	g_free(newtarget);
			globfree(&dirlist);
			return FALSE; /* fatal error */
		  }
		  g_free(newtarget);
		}
		g_free(globstring);
		globfree(&dirlist);
		
		/* remove old directory (rmdir should fail if any interior move failed) */
		if ((child_mode & TR_MOVE) && (rmdir(source)<0)){
			process_error(RW_ERROR_WRITING_TGT); /* user intervention */
			/*return process_error(RW_ERROR_WRITING_TGT); */
			/*return FALSE;*/
		}
		return TRUE;		
	}
	
	if ((child_mode & TR_MOVE) && (s_stat.st_dev == t_stat.st_dev) ){
	   if (rename (source, target) < 0){
	    return process_error(RW_ERROR_WRITING_TGT);/* user intervention */
	   } else return TRUE;
	}
	
	if (S_ISFIFO(s_stat.st_mode)){
	  return process_error(RW_ERROR_FIFO);/* user intervention */
	}
	if (S_ISCHR(s_stat.st_mode)||S_ISBLK(s_stat.st_mode)){
	   return process_error(RW_ERROR_DEVICE);/* user intervention */
	}
	if (S_ISSOCK(s_stat.st_mode)){
	   return process_error(RW_ERROR_SOCKET);/* user intervention */
	}	
        /* we have to copy the data by reading/writing  */
 	rc=process_error(internal_rw_file(target,source,s_stat.st_size));
        /* on any error, cancel whole operation:
	 * (which means the continue and cancel buttons
	 *  in the popup are the same thing for the time being) */	
        if (!rc) return FALSE; /* user intervention */	
	 
	if (!(rc & (RW_ERROR_WRITING_TGT|RW_ERROR_OPENING_TGT))) { 
	   	
	  if ((child_mode & TR_MOVE) && (unlink(source) < 0)) {
	   return process_error(RW_ERROR_WRITING_SRC);/* user intervention */
	  }
	
	  if (chmod (target, s_stat.st_mode) < 0){
	   return process_error(RW_ERROR_STAT_WRITE_TGT);/* user intervention */
	  }
	}
	/* wow. everything went ok. */
	return TRUE;
}

void finish (int sig)
{
  if (incomplete_target) unlink(fork_target);
  unlink(child_file);
  _exit(123);
}

static void ChildTransfer(void){
	FILE *tfile;
	char *line,*source,*target;
	int type;
	
	all_recursive=FALSE;
	I_am_child=TRUE;
	signal(SIGTERM,finish);
	incomplete_target=FALSE;
	line=(char *)malloc(MAX_LINE_SIZE);
	if (!line) { /* fatal error */
		process_error(RW_ERROR_MALLOC);
		unlink(holdfile);
		_exit(123); 
	}
	tfile=fopen(child_file,"r");
	if (!tfile) {/* fatal error */
		process_error(RW_ERRNO);
		unlink(holdfile);
		_exit(123);
	}
	child_path_number=0;
	while (!feof(tfile)&&fgets(line,MAX_LINE_SIZE-1,tfile)){
		line[MAX_LINE_SIZE-1]=0;
		type=atoi(strtok(line,"\t"));
		source=strtok(NULL,"\n");
		target=strrchr(source,'\t')+1;
		*(strrchr(source,'\t'))=0;
		switch (type) {
		  case URI_LOCAL:
		  case URI_FILE:
			/*source=strtok(NULL,"\t");target=strtok(NULL,"\n");*/
			/*fprintf(stderr,"dbg:(%d)%s->%s\n",type,source,target);*/
			fprintf(stdout,"child:tgt-src:%s\t%s\n",target,source);
			fprintf(stdout,"child:item:%d\n",child_path_number++);
			if (!SubChildTransfer(target,source)) goto cut_out;
			break;
		  case URI_HTTP:
		  case URI_FTP:
			/* FIXME: check for proper error control processing */
			download(target,source);
			break;
		  case URI_TAR: 
			break;
		  default:
			/*fprintf(stdout,"child:unknown type (%d) %s\n",type,source);*/
			fprintf(stderr,"unknown type (%d) %s->%s\n",type,source,target);
			break;
		}
	}
cut_out:
	fclose(tfile);
	g_free (line);
	fflush(NULL);
	_exit(123);
}

static GtkWidget *count_label;
static gboolean count_cancelled;
static GtkWidget *countW=NULL;

static int countT;

static int SubParentCount(char *source){
	struct stat s_stat;
	int i,count;
	lstat(source,&s_stat); /* stat() the link itself */

	if (S_ISDIR(s_stat.st_mode)) { /* directories fall in here, going recursive */
  		glob_t dirlist;
		char *globstring,*src;
			
		globstring = (char *)malloc(strlen(source)+3);
		if (!globstring) return FALSE; /* fatal error */
		sprintf(globstring,"%s/*",source);
		/* glob source dir */
		{
		 char count_txt[32];
		 sprintf(count_txt,"%d",countT);
		 if (!count_cancelled) gtk_label_set_text (GTK_LABEL (count_label),count_txt );
		 while (gtk_events_pending()) gtk_main_iteration();
 			 gdk_flush();
		}
		if (glob (globstring, GLOB_ERR | GLOB_TILDE | GLOB_PERIOD, 
					NULL, &dirlist) != 0) {
			return 0;
		}
  		else {
		 count = 0;
		 countT += (dirlist.gl_pathc-3);
		 for (i = 0; i < dirlist.gl_pathc; i++) {
		  if (strstr(dirlist.gl_pathv[i],"/")) src=strrchr(dirlist.gl_pathv[i],'/')+1;
		  else src = dirlist.gl_pathv[i];
		  if ((strcmp(src,".")==0)||(strcmp(src,"..")==0)) continue;
		  if (count_cancelled) {globfree(&dirlist); return 0; }
		  count += SubParentCount(dirlist.gl_pathv[i]);
		  	  
		  /*fprintf(stdout,"dbg:%s --> %d\n",dirlist.gl_pathv[i],count);*/
		 }
		 globfree(&dirlist);
		}
		g_free(globstring);		
		return count;		
	}
       	/* not a directory entry, only one file here */
	return 1; 
}

static gint ParentCount(gpointer data){
	FILE *tfile;
	char *line,*source;
	int type;
	countT=total_files=0;
	line=(char *)malloc(MAX_LINE_SIZE);
	if (!line) goto count_done; 
	tfile=fopen(child_file,"r");
	if (!tfile) {
		g_free(line);
		goto count_done; 
	}
	while (fgets(line,MAX_LINE_SIZE-1,tfile) && !feof(tfile) && !count_cancelled){
		/*fprintf(stderr,"dbg:%s\n",line);*/
		type=atoi(strtok(line,"\t"));
		source=strtok(NULL,"\t");
		if ((type == URI_LOCAL)||(type == URI_FILE)){
			if (count_cancelled) goto count_done; 
			total_files += SubParentCount(source);
		}
		else total_files++;
		
		/*fprintf(stdout,"dbg:%s --> %d\n",source,total_files);*/
	}
	fclose(tfile);
	g_free (line);
count_done:
	gtk_main_quit();
	return FALSE;
}


static void cb_count_destroy(GtkWidget *widget,gpointer data){
	count_cancelled=TRUE; /* for user destruction */
	countW=NULL;
}
static void cb_count_cancel(GtkWidget *widget,gpointer data){
	count_cancelled=TRUE; /* for user destruction */
	gtk_widget_destroy(countW);
	countW=NULL;
}

static void count_window(GtkWidget *parent){
	GtkWidget *cancel;
  countW=gtk_dialog_new ();
  
  gtk_window_position (GTK_WINDOW (countW), GTK_WIN_POS_MOUSE);
  gtk_window_set_modal (GTK_WINDOW (countW), TRUE);
  count_label = gtk_label_new ( _("Counting files"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->vbox), count_label, TRUE, TRUE, 3);
  
  count_label = gtk_label_new ( ".........................................");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->vbox), count_label, TRUE, TRUE, 3);

  cancel=gtk_button_new_with_label(_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->action_area), cancel, FALSE, FALSE, 3);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (cb_count_cancel), NULL);
  
  gtk_widget_realize (countW);
  
  if (preferences&SMALL_DIALOGS) gdk_window_set_decorations (countW->window,GDK_DECOR_BORDER);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (countW), GTK_WINDOW (parent)); 
  gtk_signal_connect (GTK_OBJECT (countW), "destroy", GTK_SIGNAL_FUNC (cb_count_destroy), NULL);
  gtk_widget_show_all(countW);
  gdk_flush();
  /* add timeout */
  gtk_timeout_add (260, (GtkFunction) ParentCount, NULL);
  gtk_main();
  if (countW) {
	  gtk_widget_destroy(countW);
	  count_cancelled=FALSE;
  }
  return ;
}

gboolean IndirectTransfer(GtkWidget *ctree,int mode,char *tmpfile) {
        GtkWidget *cpy_dlg=NULL;
	cfg *win;

	tmp_ctree=ctree;
	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
        if (CHILD_FILE_LENGTH < strlen("/tmp/xftree.9999.tmp")+1){
		fprintf(stderr,"xftree:This is a serious mistake, I need %d bytes\n",
				strlen("/tmp/xftree.9999.tmp")+1);
	}		
	strncpy(child_file,tmpfile,CHILD_FILE_LENGTH);
	child_file[CHILD_FILE_LENGTH-1]=(char)0;
	child_file_number=0;
	child_mode = mode;
 	/* count total files to transfer */
	count_cancelled=FALSE;
	if (mode != TR_LINK) count_window(win->top);
	if (count_cancelled) return TRUE;
	/*fprintf(stderr,"dbg:Total files=%d\n",total_files);*/
 	if (!cpy_dlg) cpy_dlg=show_cpy(win->top,TRUE,mode);
        /*set_show_cpy_bar(0,nitems);*/
        set_show_cpy_bar(0,total_files);
        /*fprintf(stderr,"dbg:about to fork with %s\n",tmpfile);*/
	gtk_timeout_remove(win->timer);	
	fflush(NULL);

#ifdef TIME_IT
	tiempo=time(NULL); 
#endif
	
        rw_fork_obj = Tubo (ChildTransfer, rwForkOver, TRUE, rwStdout, rwStderr);
	
	/* only parent continues from here */
        win->timer = gtk_timeout_add (TIMERVAL, (GtkFunction) update_timer, ctree);	
	/*fprintf(stderr,"dbg:call to innerloop from IndirectTransfer()\n");*/
	set_innerloop(TRUE);
       if (cpy_dlg) set_show_cpy_bar(total_files,total_files);
       /*if (cpy_dlg) set_show_cpy_bar(nitems,nitems);*/
       show_cpy(win->top,FALSE,mode);
       return TRUE;
}

/* function for non forked move on same device, or symlink too */
gboolean DirectTransfer(GtkWidget *ctree,int mode,char *tmpfile) {
	FILE *tfile;
	char *line,*source,*target;
	int type,i;
	cfg *win;
	struct stat st;
    	/*fprintf(stderr,"dbg: at DirectTransfer\n");*/
        
	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
	line=(char *)malloc(MAX_LINE_SIZE);
	if (!line) {
		xf_dlg_error(win->top,strerror(errno),NULL);
		return FALSE;
	}
	tfile=fopen(tmpfile,"r");
	if (!tfile) {
		xf_dlg_error(win->top,strerror(errno),tmpfile);
		return FALSE;
	}
	i=0;
	while (!feof(tfile)&&fgets(line,MAX_LINE_SIZE-1,tfile)){
		line[MAX_LINE_SIZE-1]=0;
		type=atoi(strtok(line,"\t"));
		source=strtok(NULL,"\n");
		target=strrchr(source,'\t')+1;
		*(strrchr(source,'\t'))=0;
		lstat(source,&st);
		if (S_ISLNK(st.st_mode)) {
	  	  char *src_lnk;
 	  	  int len;
	 	  struct stat st;
		  lstat(source,&st);
		  src_lnk=(char *)malloc(st.st_size+1);
		  if (!src_lnk) continue;
		  len = readlink (source, src_lnk, st.st_size);
		  if (len <= 0) {
			  if (xf_dlg_error_continue(win->top,strerror(errno),source)==DLG_RC_CANCEL)
			  {
		            g_free(src_lnk);
			    continue;
			  }
		  }
	      	  src_lnk[len] = 0;
		  if (symlink (src_lnk, target) == -1){
		     if (xf_dlg_error_continue(win->top,strerror(errno),target)==DLG_RC_CANCEL){
		        g_free(src_lnk);
 		        continue;
		     }
		  }
		  g_free(src_lnk);
	      	  if (mode & TR_MOVE) {
		   if (unlink (source) == ERROR) {
	             if (xf_dlg_error_continue(win->top,strerror(errno),source)==DLG_RC_CANCEL)
 		        continue;
		   }
		  }
 		  continue;
		}
		
		if (tar_extraction){
			char *o_src;
			o_src=g_strdup(source);
			/* strtoks down the line... */
			if (strncmp(source,"tar:",strlen("tar:"))!=0) continue;
			tar_extract((GtkCTree *)ctree,target,source);
			if (mode==TR_MOVE) tar_delete((GtkCTree *)ctree,o_src);
			g_free (o_src);
		} else {/* moveit */
			if (mode & TR_MOVE) {
			  if (rename (source, target) < 0){
			    if (xf_dlg_error_continue(win->top,strerror(errno),target)==DLG_RC_CANCEL)
	          	    return FALSE;
			  }
			} else if (mode & TR_LINK) {
	                    if (symlink (source, target) < 0) {
				if ((xf_dlg_error_continue(win->top,strerror(errno),target)==DLG_RC_CANCEL))
				    return process_error(RW_ERROR_WRITING_TGT);
			    }
			    else return TRUE;
			}
		}
	}
	fclose(tfile);
	g_free (line);
	return TRUE; 
}

/* now using a dynamic buffer instead of static. */
/* function called by rw_file() 
 *
 * recursive function for directories
 * */
#define BUFFER_SIZE (4096)
static int internal_rw_file(char *target,char *source,off_t size){
	int i,j=0,source_file,target_file;
	char *buffer;
	off_t total_size=0;
	gboolean too_few=FALSE,too_many=FALSE;
	int b_size=BUFFER_SIZE;
        struct stat statbuf;

	
	fork_target=target;
	fork_source=source;
	/* open source */
	source_file=open(source,O_RDONLY);
	if (source_file < 0) return (RW_ERROR_OPENING_SRC);
	/* open target */
	target_file=open(target,O_WRONLY|O_TRUNC|O_CREAT);
	if (target_file < 0) {
		close (source_file);
		return (RW_ERROR_OPENING_TGT);
	}

		/* assign optimum block size */
	if (fstat(source_file,&statbuf)==0) b_size=ST_BLKSIZE(statbuf);
	/* allocate buffer */
	buffer=(char *)malloc(BUFFER_SIZE);
	if (!buffer) {
		close (source_file);
		close (target_file);
		unlink(target); /* erase empty file created */
		return (RW_ERROR_MALLOC);
	}

	incomplete_target=TRUE;
	/* read/write loop */
	/* 
	 * interruption occurs with signal SIGTERM
	 * sent from parent process via the dialog
	 * in which case parent proceeds with cpyForkCleanup()
	 * Closing the source file is not necesary 
	 * because the child process will terminate, closing it then. 
	 * */
	child_file_number++;
	fprintf(stdout,"child:bytes: %d / %d \n",child_file_number-1,total_files);fflush(NULL);
	fprintf(stdout,"child:item: %d \n",child_file_number-1);fflush(NULL);
	while ((i = read (source_file,buffer,BUFFER_SIZE)) > 0){
		if ((j=write(target_file,buffer,i)) < 0) break;
		if (i > j){ too_few=TRUE; break;}
		if (i < j){ too_many=TRUE; break;}
		total_size += j;
		
		/* This is not really any good information, and it just
		 * slows xftree down too much.
		fprintf(stdout,"child:bytes:path %d, file %d -> %lld bytes\n",
				child_path_number,child_file_number,(long long)total_size);fflush(NULL);
		usleep(50);	*/
	}
	g_free(buffer);
	if (close(source_file)<0){
		close(target_file);
		return (RW_ERROR_CLOSE_SRC);
	}
	if (close(target_file)<0){
		return (RW_ERROR_CLOSE_TGT);
	}
	incomplete_target=FALSE;

	if ((i<0) || (j<0) || (too_few) || (too_many))
	{
		if (unlink(target))return (RW_ERROR_UNLINK_SRC); 
		if (too_few) return (RW_ERROR_TOO_FEW);
		if (too_many) return (RW_ERROR_TOO_MANY);
		if (i<0) return (RW_ERROR_READING_SRC);
		if (j<0) return (RW_ERROR_WRITING_TGT);
	}
	
	if (total_size < size){
		return (RW_ERROR_TOO_FEW);
	}
	/* everything went OK */
	return (RW_OK);
}

/* FUNCTION start/stop inner gtk_main() */
static void set_innerloop(gboolean state){
	static gboolean innerloop=FALSE;
	if (state==innerloop) return;
	innerloop=state;
	/*fprintf(stderr,"dbg:innerloop %s\n",(state)?"started":"terminated");fflush(NULL);*/
	if (state) gtk_main();
	else gtk_main_quit();
}

/* function to process stderr produced by child */
static int rwStderr (int n, void *data){
  char *line;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  fprintf(stderr,"child stderr:%s\n",line);
  return TRUE;
}


/* function to process stdout produced by child */
static int rwStdout (int n, void *data){
  char *line,*texto;
  int rc;
  
  if (n) return TRUE; /* this would mean binary data */
  line = (char *) data;
  /*fprintf(stderr,"dbg(rwStdout):%s\n",line);fflush(NULL);*/

  /* only process child specific lines: */  
  if (strncmp(line,"child:",strlen("child:"))!=0){
         /*fprintf(stderr,"dbg:discarding line\n");fflush(NULL);*/
	 return TRUE;
  }
/* clear to process next file */
  if (strncmp(line,"child:runOver",strlen("child:runOver"))==0) {
  	/*set_innerloop(FALSE);*/
	return TRUE;
  }
/* transferred bytes progress report */
  if (strncmp(line,"child:bytes:",strlen("child:bytes:"))==0) {
  	strtok(line,":");
  	strtok(NULL,":");
  	texto=strtok(NULL,"\n");
	if (texto && info[2]) gtk_label_set_text (GTK_LABEL (info[2]),texto );
	return TRUE;
  }
 /* src-tgt update */
  if (strncmp(line,"child:tgt-src:",strlen("child:tgt-src:"))==0) {
	char *src,*tgt;
  	strtok(line,":");
  	strtok(NULL,":");
	tgt=strtok(NULL,"\t");
	src=strtok(NULL,"\n");
        if (tgt && src) set_show_cpy(tgt,src);
	return TRUE;
  }
 /* nitems update */
  if (strncmp(line,"child:item:",strlen("child:item:"))==0) {
  	strtok(line,":");
  	strtok(NULL,":");
        set_show_cpy_bar(atoi(strtok(NULL,"\n")),total_files);
        /*set_show_cpy_bar(atoi(strtok(NULL,"\n")),0);*/
	return TRUE;
  }
  
 /* anything else is a process_error() output: */
  strtok(line,":");
  texto=strtok(NULL,"\n");
  /*fprintf(stderr,"dbg(rwStdout error):%s\n",texto);fflush(NULL);*/
  /*set_dnd_status sets static variable in xtree_dnd to break loop;*/
/*  rc=xf_dlg_error_continue (cat,texto,NULL);*/
  rc=xf_dlg_new(cat,texto,NULL,NULL,DLG_CANCEL|DLG_CONTINUE|DLG_ALL,1);
  /*rc=xf_dlg_error_continue_all (cat,texto,NULL);*/
  if (rc==DLG_RC_ALL){
    FILE *allF;
    char *allfile;
    allfile=(char *)malloc(strlen(child_file)+1+strlen(".all"));
    if (allfile) { 
	sprintf(allfile,"%s.all",child_file);
	allF=fopen(allfile,"w"); if (allF) fclose(allF);	    
        g_free(allfile);
    }
  }
  
  holdfile=(char *)malloc(strlen(child_file)+1+strlen(".hold"));
  if (!holdfile) { /* this should not happen */
	  cb_cancel(NULL,NULL);
	  return TRUE; 
  }
  sprintf(holdfile,"%s.hold",child_file);  
  if (rc==DLG_RC_CANCEL) cb_cancel(NULL,NULL); /* terminates child */
  usleep(500000); /* give child time to close */
  unlink(holdfile); g_free(holdfile);
  /*set_innerloop(FALSE);*/
  return TRUE;
}

/* function called when child is dead */
static void rwForkOver (pid_t pid)
{
  char *allfile;
  allfile=(char *)malloc(strlen(child_file)+1+strlen(".all"));
  if (allfile) { 
    sprintf(allfile,"%s.all",child_file);
    unlink(allfile);
    g_free(allfile);
  }  
/*  cursor_reset (GTK_WIDGET (smb_nav));*/
  rw_fork_obj = NULL;
  /*fprintf(stderr,"dbg: call to innerloop from forkover()\n");fflush(NULL);*/
  set_innerloop(FALSE);
#ifdef TIME_IT
  fprintf(stderr,"seconds to copy=%lu\n",time(NULL)-tiempo);
#endif
}


/* function to set source and target strings
 * in the dialog box.
 * */
void set_show_cpy(char *target,char *source){
	char line[256];
	if (!cat) return; /* abort if show_cpy not called yet */
	gtk_entry_set_text (GTK_ENTRY (info[0]), source);
	gtk_entry_set_text (GTK_ENTRY (info[1]), target);
	sprintf (line, _("%d / %d"), 0,total_files);
	if (info[2])gtk_label_set_text (GTK_LABEL (info[2]),line );
}

/* function to set the state of the progress bar*/
void set_show_cpy_bar(int item,int nitems){
	float p;
	static int n;
	if (nitems) n=nitems;
	if (n < item) n=item;
	if (!cat) return; /* abort if show_cpy not called yet */
	/* this optionally sets the progress bar */
	if (n) p = ((float)item)/((float)n); else p=1;
	if ((p>=0)&&(p<=1))
		gtk_progress_set_percentage((GtkProgress *)progress,p);	
}

/* function to clean up the mess after child process
 * has been sent a SIGTERM. (Might a unlink target be 
 * good here?)
 * */
static void cpyForkCleanup(void)
{
	/* break inner gtk_main() loop */
	/*fprintf(stderr,"dbg:call to innerloop from forkcleanup\n");*/
	
  	set_innerloop(FALSE);
	/* unlink(fork_target); */
}

/* this cancel will interrupt the copy in progress
 * and the copy loop. (status is what breaks the loop)
 * */
static void
cb_cancel (GtkWidget * w, void *data)
{
	/* kill the current copy process */
	rw_fork_obj = TuboCancel (rw_fork_obj, cpyForkCleanup);
	gtk_widget_destroy(cat);
	cat=NULL;
}

/* This creates the dialog progress box for copy/move
 * operations. 
 * FIXME: it is still bugged in the respect that the
 * source and target paths don't always fit into the
 * respective entry boxes
 * */
GtkWidget *show_cpy(GtkWidget *parent,gboolean show,int mode)
{
  static char *title=NULL,*tit="";
  GtkWidget *cancel, *label[2], *box, *table;
  
  if (!show) {
     if (cat) gtk_widget_destroy(cat);
     cat=NULL;
     return cat;     
  }

  if (cat) {
	  fprintf(stderr,"This never happens-- show_cat(): cat != NULL");
	  return cat;
  }
  
 
  cat = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (cat), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (cat), "destroy", GTK_SIGNAL_FUNC (cb_cancel), NULL);
 
  box = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->vbox), box, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);
 
  label[0] = gtk_label_new (_("Source: "));
  gtk_table_attach (GTK_TABLE (table), label[0], 0, 1, 0, 1, X_OPT, 0, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label[0]), GTK_JUSTIFY_RIGHT);

  info[0] = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), info[0], 1, 2, 0, 1, X_OPT, 0, 0, 0);

  label[1] = gtk_label_new (_("Target: "));
  gtk_table_attach (GTK_TABLE (table), label[1], 0, 1, 1, 2, X_OPT, 0, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label[1]), GTK_JUSTIFY_RIGHT);

  info[1] = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), info[1], 1, 2, 1, 2, X_OPT, 0, 0, 0);

  info[2] = gtk_label_new (_("beginning copy operation"));
  gtk_box_pack_start (GTK_BOX (box), info[2], TRUE, TRUE, 0);

  progress = gtk_progress_bar_new();
  gtk_progress_bar_set_bar_style  ((GtkProgressBar *)progress,GTK_PROGRESS_CONTINUOUS);  
  gtk_progress_bar_set_orientation((GtkProgressBar *)progress,GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_box_pack_start (GTK_BOX (box), progress, TRUE, TRUE, 0);

  cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->action_area), cancel, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (cb_cancel),NULL);

  gtk_widget_realize(cat);  
  gtk_widget_show_all (cat);

  if (mode == TR_COPY)      tit = _("XFTree: Copy");	  
  else if (mode == TR_MOVE) tit = _("XFTree: Move");
  else if (mode == TR_LINK) tit = _("XFTree: Link");
  else printf("This never happens either.\n");
  title = (char *)malloc(sizeof(char)*(strlen (tit)+1));	  
  if (title) {
	sprintf (title, tit);
	gtk_window_set_title (GTK_WINDOW (cat), title);
        g_free(title);
  }

  
  /*gdk_flush();*/
  gtk_window_set_modal (GTK_WINDOW (cat), TRUE);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (cat), GTK_WINDOW (parent));
  else fprintf(stderr,"This should not happen show_cat(): parent is null!!\n");

  return cat;

}

void cb_duplicate (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  struct stat s;
  GtkCTreeNode *node;
  char *nfile,*tmpfile;
  int num,selected;
  cfg *win;

  cursor_wait (GTK_WIDGET (ctree));
  gtk_clist_freeze (GTK_CLIST (ctree));
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  selected = count_selection (ctree, &node);
  if (!selected) {
	  xf_dlg_error(win->top,"No file or directory selected for duplication!",NULL);
	  goto duplicate_return;
  } else if (selected > 1) {
	  xf_dlg_error(win->top,"Only one file or directory can be selected for duplication!",NULL);
	  goto duplicate_return;
  }
  
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  if (en->type & FT_TARCHILD) {
	xf_dlg_error(win->top,_("This function is not available for the contents of tar files"),NULL);
	goto duplicate_return;
  }

  if (!io_is_valid (en->label)) goto duplicate_return;
  cursor_wait (GTK_WIDGET (ctree));
  num = 0;
  if ((nfile=(char *) malloc(strlen(en->path)+32))==NULL) goto duplicate_return;
  /* get highest duplicated version number*/
  {
     glob_t dirlist;
     int i,j;
     char *wd;
     sprintf (nfile, "%s-*", en->path);
     if (glob (nfile, GLOB_ERR, NULL, &dirlist) == 0) {
	     
	for (i = 0; i < dirlist.gl_pathc; i++) {
	     wd = strrchr(dirlist.gl_pathv[i],'-')+1;
	     for (j=0;j<strlen(wd);j++) if ((wd[j] > '9')||(wd[j] < '0')) goto next;
	     if (num <= atoi(wd)) num = atoi(wd) + 1; 
	     /*printf("%d:%s\n",i,dirlist.gl_pathv[i]);*/
	}
next:;
     }
     globfree(&dirlist);
     
  }
     
  sprintf (nfile, "%s-%d", en->path, num++);
  while (stat (nfile, &s) != -1) {
    sprintf (nfile, "%s-%d", en->path, num++);
  }

  
  tmpfile = SimpleTmpList(win->top,nfile,en->path);
  IndirectTransfer((GtkWidget *)ctree,TR_COPY,tmpfile);
  
  
   /* immediate refresh */
  update_timer (ctree);
  cursor_reset (GTK_WIDGET (ctree));
  g_free(nfile);
duplicate_return:
  gtk_clist_thaw (GTK_CLIST (ctree));
  cursor_reset (GTK_WIDGET (ctree));   return;
}

void cb_symlink (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  GtkCTreeNode *node;
  char *nfile,*ofile,*p,*entry_return;
  int num,selected;
  struct stat st;
  cfg *win;
  char *argv[6];

  cursor_wait (GTK_WIDGET (ctree));
  gtk_clist_freeze (GTK_CLIST (ctree));
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  selected = count_selection (ctree, &node);
  if (!selected) {
	  xf_dlg_error(win->top,_("No file or directory selected!"),NULL);
	  goto symlink_return;
  } else if (selected > 1) {
	  xf_dlg_error(win->top,_("Only one file or directory can be selected!"),NULL);
	  goto symlink_return;
  }
  
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree), node);
  if (en->type & (FT_TARCHILD|FT_RPMCHILD)) {
	xf_dlg_error(win->top,_("This function is not available for the contents of tar/rpm files"),NULL);
	goto symlink_return;
  }

  if (!io_is_valid (en->label)) goto symlink_return;
  num = 0;
  if ((nfile=(char *) malloc(strlen(en->path)+12))==NULL) goto symlink_return;

  sprintf (nfile, "%s-lnk", en->label);
  
  entry_return = (char *)xf_dlg_string (win->top,_("Symlink name : "),nfile);
  
  if (!entry_return || !strlen(entry_return) || !io_is_valid (entry_return)){
	goto symlink_return;
  }

  if ((p = strchr (entry_return, '/')) != NULL) {
      p[1] = '\0';
      xf_dlg_error (win->top,_("Character not allowed in filename"), p);
      goto symlink_return;
  }
  ofile = (char *)malloc(strlen(en->path)+1);
  if (!ofile){
	  goto symlink_return;
  }	  
  strcpy(ofile,en->path);
  g_free(nfile);
  nfile = (char *)malloc(strlen(en->path)+strlen(entry_return)+1);
  if (!nfile) {
	  g_free(ofile);
	  goto symlink_return;
  }
  strcpy (nfile,ofile);
  p=strrchr(nfile,'/');
  p[1]=0;
  strcat(nfile,entry_return);

  /*fprintf(stderr,"dbg: rename %s->%s\n",ofile,nfile);*/

  if (lstat (nfile, &st) != ERROR)  {
      xf_dlg_error (win->top,_("File exists !"), nfile);
      g_free(ofile); g_free(nfile);
      goto symlink_return;
  }

  /*fprintf(stderr,"dbg:ln -s %s %s\n",ofile,nfile);*/
  argv[0]="ln";
  argv[1]="-s";
  argv[2]=ofile;
  argv[3]=nfile;
  argv[4]=0;
  
  io_system(argv,win->top);
  usleep(50000);
    
   /* immediate refresh */
  update_timer (ctree);
  cursor_reset (GTK_WIDGET (ctree));
  g_free(nfile);
  g_free(ofile);
symlink_return:
  gtk_clist_thaw (GTK_CLIST (ctree));
  cursor_reset (GTK_WIDGET (ctree));   return;
}

void cb_touch (GtkWidget * item, GtkCTree * ctree)
{
  entry *en;
  GtkCTreeNode *node;
  int num;
  cfg *win;
  char *argv[4];
  GList *selection;

  cursor_wait (GTK_WIDGET (ctree));
  gtk_clist_freeze (GTK_CLIST (ctree));
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  num = count_selection (ctree, &node);
  if (!num) {
	  xf_dlg_error(win->top,_("No file or directory selected!"),NULL);
	  goto touch_return;
  }    
  
  selection = GTK_CLIST (ctree)->selection;
  node = selection->data;
  en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree),node);

  if (!io_is_valid (en->label)) goto touch_return;

  argv[0]="touch";
  argv[2]=0;
  for (;selection != NULL;selection=selection->next) {
     node = selection->data;
     en = gtk_ctree_node_get_row_data (GTK_CTREE (ctree),node);
     if (en->type & (FT_TARCHILD|FT_RPMCHILD)) continue;
     argv[1]=en->path;
     io_system(argv,win->top);
     usleep(50000);
  }
   
   /* immediate refresh */
  update_timer (ctree);
  cursor_reset (GTK_WIDGET (ctree));
touch_return:
  gtk_clist_thaw (GTK_CLIST (ctree));
  cursor_reset (GTK_WIDGET (ctree));   
  return;
}

