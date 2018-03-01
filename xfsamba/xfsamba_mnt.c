/*  gui modules for xfsamba
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "xpmext.h"

/* local xfsamba includes : */
#undef XFSAMBA_MAIN
#include "xfsamba.h"
#include "xfsamba_dnd.h"
#include "tubo.h"

/*********************************************************/
/**** mount stuff ***********/
typedef struct smbmnt_t{
	pid_t pid;
	char *mnt_point;
	gboolean mounted;
	struct smbmnt_t *next;
}smbmnt_t;
static smbmnt_t *headM=NULL;

gboolean check_mnt(char *mount_point){
  FILE *mtab;
  mtab = fopen ("/etc/mtab", "r");
  if (mtab){
      char line[256];
      while (fgets(line,255,mtab)){
	line[255]=0;
	/*printf("dbg:%s",line);*/
	if (strstr(line,"smbfs") && strstr(line,mount_point)){
		fclose(mtab);
		return TRUE;
	}  
      }
      fclose (mtab);
  }
  return FALSE; 	
}

void
clean_smbmnt (void)
{
  smbmnt_t *last;
  while (headM) {
    if (headM->mnt_point) {
	    strcpy(NMBcommand,headM->mnt_point);
	    /*printf("dbg:unmounting %s\n",headM->mnt_point);*/
	    if (!fork()){
		  execlp("smbumount","smbumount",NMBcommand,(char *)0);
		  _exit(123); 
	    }
	    usleep(50000);
	    rmdir(headM->mnt_point);
	    g_free (headM->mnt_point);
    }
    last = headM;
    headM = headM->next;
    g_free (last);
  }
  return;
}

static smbmnt_t *
push_smbmnt (char *mnt_point,pid_t pid)
{
  smbmnt_t *currentM;
  currentM = headM;
  /*printf("dbg: pushing %s, pid=%d\n",mnt_point,pid);*/
  if (!currentM) {
    headM = currentM = (smbmnt_t *) malloc (sizeof (smbmnt_t));
  }
  else  {
    while (currentM->next) currentM = currentM->next;
    currentM->next = (smbmnt_t *) malloc (sizeof (smbmnt_t));
    currentM = currentM->next;
  }
  if (mnt_point)
  {
    currentM->mnt_point = (char *) malloc (strlen (mnt_point) + 1);
    strcpy (currentM->mnt_point, mnt_point);
  } else currentM->mnt_point=NULL;
  currentM->pid=pid;
  currentM->next = NULL;
  /*printf("dbg:  %s, pid=%d is pushed\n",mnt_point,pid);*/
  return headM;
}


static char *
find_smbmnt (pid_t pid,char *mnt_point){
  smbmnt_t *currentM;
  currentM = headM;
  /*printf("dbg: finding pid=%d\n",pid);*/
  while (currentM) {
     /*printf("dbg: %d ?= %d (%s)\n",pid,currentM->pid,currentM->mnt_point);*/
    if (pid && (pid==currentM->pid)) {
            /*if (currentM->mnt_point) printf("dbg: found %s\n",currentM->mnt_point);*/
	    return currentM->mnt_point;
    }
    if (mnt_point && (strcmp(mnt_point,currentM->mnt_point)==0)){
	    return currentM->mnt_point;
    }
    currentM = currentM->next;
  }
  return NULL;
}

static smbmnt_t *
pop_smbmnt (pid_t pid){
  smbmnt_t *currentM, *lastM = NULL;
  /*printf("dbg: poping pid=%d\n",pid);*/
  if (!pid) return headM;
  currentM = headM;
  while (currentM) {
    if (pid==currentM->pid) {
	    if (lastM) lastM->next=currentM->next;
	    else headM=currentM->next;
	    if (currentM->mnt_point) g_free(currentM->mnt_point);
	    /*printf("dbg:smbmnt popped\n");*/
	    return headM;
    }
    lastM=currentM;
    currentM = currentM->next;
  }
  return headM;
}


static void *mnt_fork=NULL;
int mount_stderr (int n, void *data)
{

  char *line;
  if (n) return TRUE;
  line = (char *) data;
  print_diagnostics (line);
  xf_dlg_warning(smb_nav,line);
  return TRUE;
}

static void
unmountForkOver (pid_t pid)
{
  char *mount_path;
  /* do a check to verify if unmounted and warn otherwise. */

  mount_path=find_smbmnt(pid,NULL);
  if (mount_path) {
	  if (!check_mnt(mount_path)) rmdir(mount_path); 
	  else {
	   xf_dlg_error(smb_nav,_("Could not unmount"),mount_path);
	   return;
	  }
  }
  /* mount point can be eliminated from records now */
  headM = pop_smbmnt(pid);		
  return;
}
static void
unmountFork (void){
   execlp ("smbumount", "smbumount",  NMBcommand, (char *) 0);
   fprintf(stderr,_("Cannot execute smbumount\n"));
}

static void
mount_xftree_ForkOver (pid_t pid)
{
  void *fo;
  char *mount_path;
  /*printf("dbg:  fork2 is over\n");*/
  mount_path=find_smbmnt(pid,NULL);
  if (mount_path) {
   strcpy(NMBcommand,mount_path);
   headM = pop_smbmnt(pid);	
   fo=Tubo(unmountFork,unmountForkOver,TRUE,parse_stderr,mount_stderr);
   headM=push_smbmnt (NMBcommand,TuboPID(fo));
  }   
  return;
}
static void
mount_xftree_Fork (void){
  execlp ("xftree", "xftree",    NMBcommand,(char *) 0);
  fprintf(stderr,_("Cannot execute xftree\n"));
}
static void
mountForkOver (pid_t pid)
{
  void *fo;
  char *mount_path;
  /*printf("dbg:  fork1 is over\n");*/
  mount_path=find_smbmnt(pid,NULL);
  /*printf("dbg: mount_path=%s \n",mount_path);*/
  if (mount_path) {
   strcpy(NMBcommand,mount_path);
  /* check /etc/mtab for mount success */
   if (!check_mnt(mount_path)){
	   mnt_fork=NULL;
           headM = pop_smbmnt(pid);  
	   xf_dlg_error(smb_nav,_("Could not mount"),mount_path);
	   return;
   }
   headM = pop_smbmnt(pid);  
   fo=Tubo(mount_xftree_Fork,mount_xftree_ForkOver,TRUE,parse_stderr,parse_stderr);
   headM=push_smbmnt (NMBcommand,TuboPID(fo));
  }   
  mnt_fork=NULL; /* free resource for next mnt */ 
  
  return;
}

static void
mountFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
  execlp ("smbmount", "smbmount", the_netbios, NMBcommand, "-o", NMBpassword, (char *) 0);
}

void
cb_mount (GtkWidget * item, GtkWidget * ctree){
  GList *s;
  smb_entry *en;
  char *mount_point,*argv[5];

	/* check if a share is selected */
  if ( g_list_length (GTK_CLIST (shares)->selection)==0){
	xf_dlg_error(smb_nav,_("Error"),_("No share selected"));
	return;
  }
  s = GTK_CLIST (shares)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)shares, s->data);
  if (!(en->type & S_T_SHARE)){
	xf_dlg_error(smb_nav,_("Error"),_("No share selected"));
	return;
  }
	
	/* sane xftree. if no xftree, no mount */
	argv[0]="xftree";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	argv[0]="smbmount";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	argv[0]="smbmnt";
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,_("Could not find"),argv[0]);
		return;
	}
	/* query for mountpoint. default is to create a dir at /tmp/whatever */

	/* entry_return should not be freed! */
	mount_point=(char *)xf_dlg_string(smb_nav,_("Mount point /tmp/xfsamba/"),en->share);
	if (!mount_point) return;
	/* do the smb mount, if error return */
	
  while (mnt_fork){
  	while (gtk_events_pending()) gtk_main_iteration();
	usleep(500);
  }
  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, en->share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  sprintf (NMBpassword,"username=%s", thisN->password);
  sprintf(NMBcommand,"/tmp/xfsamba/%s",mount_point);
  
	if ((mkdir("/tmp/xfsamba", 0xFFFF)<0)&&(errno != EEXIST)){
		xf_dlg_error(smb_nav,strerror(errno),"/tmp/xfsamba");
		return;
	}
	if ((mkdir(NMBcommand, 0xFFFF)<0)&&(errno != EEXIST)){
		xf_dlg_error(smb_nav,strerror(errno),NMBcommand);
		return;
	}
  if (find_smbmnt(0,NMBcommand)){
    /* smbunmount has trouble unmounting multiple mounts (kernel 2.4 stuff) */
    xf_dlg_error(smb_nav,_("Already mounted"),NMBcommand);
  } else {
    mnt_fork=Tubo(mountFork, mountForkOver, TRUE, parse_stderr,mount_stderr);
    headM=push_smbmnt (NMBcommand,TuboPID(mnt_fork));	
  }
  return;
}


