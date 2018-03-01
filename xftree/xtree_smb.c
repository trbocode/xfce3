/* (c) 2002 Edscott Wilson Garcia GNU/GPL
 */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "constant.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif
/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "xtree_cfg.h"
#include "gtk_dlg.h"
#include "uri.h"

#include "tubo.h"
#endif

static char SMBtmpfile[32];
static void *fork_obj=NULL;
static int SMBResult;
static GtkWidget *SMBctree;
static GtkWidget *countW=NULL,*count_label,*SMBparent;

#define CHALLENGED 0x01


/* functions to use tubo.c for downloading SMB files */

/*******SMBGet******************/
void
latin_1_readable (char *the_char)
{
  unsigned char *c;
  c = (unsigned char *) the_char;
  while (c[0])
  {
    switch (c[0])
    {
    case 0x81: c[0] = 'ü'; break;
    case 0x82: c[0] = 'é'; break;
    case 0xa0: c[0] = 'á'; break;
    case 0xa1: c[0] = 'í'; break;
    case 0xa2: c[0] = 'ó'; break;
    case 0xa3: c[0] = 'ú'; break;
    case 0xa4: c[0] = 'ñ'; break;
    case 0xa5: c[0] = 'Ñ'; break;
    case 0xb5: c[0] = 'Á'; break;
    case 0x90: c[0] = 'É'; break;
    case 0xd6: c[0] = 'Í'; break;
    case 0xe0: c[0] = 'Ó'; break;
    case 0xe9: c[0] = 'Ú'; break;
    default: break;
    }
    c++;
  }

}
void
latin_1_unreadable (char *the_char)
{
  unsigned char *c;
  c = (unsigned char *) the_char;
/*print_diagnostics("DBG:"); print_diagnostics(the_char);*/

  while (c[0])
  {
    switch (c[0])
    {
    case 0xfc: c[0] = 0xa0; break;
    case 0xe9: c[0] = 0x82; break;
    case 0xe1: c[0] = 0xa1; break;
    case 0xed: c[0] = 0xa2; break;
    case 0xf3: c[0] = 0xa3; break;
    case 0xfa: c[0] = 0x81; break;
    case 0xf1: c[0] = 0xa4; break;
    case 0xd1: c[0] = 0xa5; break;
    case 0xc1: c[0] = 0xb5; break;
    case 0xc9: c[0] = 0x90; break;
    case 0xcd: c[0] = 0xd6; break;
    case 0xd3: c[0] = 0xe0; break;
    case 0xda: c[0] = 0xe9; break;
    default: break;
    }
    c++;
  }
}



static void
SMBFork (void)
{
  char *the_command,*the_user,*the_netbios;
  FILE *tmpfile;
  struct stat s;

  
  if (stat(SMBtmpfile,&s)<0)  _exit(123); /*forgetit("unable to stat temp file");*/
  if ((the_command = (char *)malloc(s.st_size+1))==NULL) _exit(123); /* forgetit("unable allocate memory for");*/
  tmpfile=fopen(SMBtmpfile,"rb");
  if (tmpfile) {
   int i;
   if (fread(the_command,1,s.st_size,tmpfile)<s.st_size) _exit(123);
   fclose(tmpfile);
   unlink(SMBtmpfile);
   the_command[s.st_size]=0;
   the_netbios=strtok(the_command,"\n");
   if (!the_netbios) _exit(123); /* error processing */
   the_user=strtok(NULL,"\n");
   if (!the_user) _exit(123); /* error processing */
   the_command=the_user+strlen(the_user)+1;
   for (i=0;i<strlen(the_netbios);i++) latin_1_unreadable(the_netbios+i); 
   fprintf(stderr,"smbclient %s -U %s\n",the_netbios,the_user);
   fprintf(stderr,"%s\n",the_command); fflush(NULL);sleep(1);
     /*read data from tmpfile */  
   execlp ("smbclient", "smbclient", the_netbios,"-U",the_user, "-c", the_command, (char *) 0);
  _exit(123); /* error processing */
  }
}

static int
SMBStdout (int n, void *data)
{
  char *line;
  if (n) return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "ERRDOS"))  {/* server has died */
    SMBResult = CHALLENGED;
  }
  if (strstr (line, "Error opening local file")) {
    SMBResult = CHALLENGED;
  }
  /*printf("dbg(child):%s\n",line);*/
  /* dont work, no kernel context switch: gtk_label_set_text ((GtkLabel *)count_label,line); */
  gdk_flush();

  return TRUE;
}


static void
SMBForkOver (pid_t pid)
{
  extern gint update_timer (GtkCTree * ctree);
   
  if (countW) {
     gtk_widget_destroy(countW);
     countW=NULL;
  }
  
  switch (SMBResult)
  {
  case CHALLENGED:
    /*printf("dbg:%s\n",_("File download failed."));*/
    xf_dlg_error(SMBparent,"smbclient",_("File download failed."));
    break;
  default:
    /* upload was successful: */
    /*printf("dbg:%s\n",_("Download done."));*/
    /*xf_dlg_error(SMBparent,"smbclient",_("Download done."));*/
    break;

  }
  fork_obj = NULL;
  cursor_reset(SMBctree);
  update_timer((GtkCTree *)SMBctree);
}

static void cb_count_destroy(GtkWidget *widget,gpointer data){
	countW=NULL;
}

static void download_window(GtkWidget *parent,char *host){
 	
  countW=gtk_dialog_new ();
  
  gtk_window_position (GTK_WINDOW (countW), GTK_WIN_POS_MOUSE);
  gtk_window_set_modal (GTK_WINDOW (countW), TRUE);
  count_label = gtk_label_new ( _("Downloading files from "));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->vbox), count_label, TRUE, TRUE, 3);
  count_label = gtk_label_new (host);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->vbox), count_label, TRUE, TRUE, 3);
  count_label = gtk_label_new (".............................................................");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (countW)->action_area), count_label, TRUE, TRUE, 3);
  gtk_widget_realize (countW);
  if (preferences&SMALL_DIALOGS) gdk_window_set_decorations (countW->window,GDK_DECOR_BORDER);
  if (parent) gtk_window_set_transient_for (GTK_WINDOW (countW), GTK_WINDOW (parent)); 
  gtk_signal_connect (GTK_OBJECT (countW), "destroy", GTK_SIGNAL_FUNC (cb_count_destroy), NULL);
  gtk_widget_show_all(countW);
  gdk_flush();
  return ;
}


void
SMBGetFile (GtkCTree *ctree, char *target,GList *list)
{
  char *dndS,*host=NULL,*user,*orig_share=NULL,*share,*file,*filename=NULL;
  uri *u;
  cfg *win;
  static char *fname;
  FILE *tmpfile=NULL;
  extern char *randomTmpName(char *);
    char *w;
    int i;
    gboolean first=TRUE,isdir;

  SMBctree=(GtkWidget *)ctree;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  SMBparent=win->top;
  if (!sane("smbclient")){
	  xf_dlg_error(win->top,_("SMB downloads disabled.\nCommand not found"),"smbclient");
	  return;
  }

  if ((fname=randomTmpName(NULL))==NULL) return;
    if ((tmpfile=fopen(fname,"w"))==NULL) {
	xf_dlg_error(win->top,_("Cannot open"),(fname)?fname:"/tmp/?");
	return;
  }
  
 for (;list!=NULL;list=list->next){
  u = list->data;
  if (strchr(u->url,'\n')) u->url=strtok(u->url,"\n");
  if (strchr(u->url,'\r')) u->url=strtok(u->url,"\r");
  dndS=u->url;
  /*fprintf(stderr,"dbg: processing %s\n",dndS);*/
/*
 * 1- parse file into NMBcommand
 * 2- open modal dialog with animation
 * 3- download via tubo. 
 * 4- on forkover, close modal animation dialog
 * 5- process errors
 * */
	
/* * 1- parse file into NMBcommand */
/* format smb://user@host:share/file */

  /*printf("dbg:dnd=%s\n",dndS);*/

  if (strncmp("smb://",dndS,strlen("smb://")!=0)) {
	incorrect_DND:
	xf_dlg_error(win->top,_("DnD error"),_("Incorrect DnD specification"));
	return;
  }

  user=dndS+strlen("smb://");
  user=strtok(user,"@");  if (!user) goto incorrect_DND;
  host=strtok(NULL,":");  if (!host) goto incorrect_DND;
  share=strtok(NULL,"/"); if (!share) goto incorrect_DND;
  file=share+strlen(share)+1;
 
  w=strrchr(file,'/');
  if (w) {
	  if (w[1]==0) {
	    isdir=TRUE;
	    w[0]=0;
            w=strrchr(file,'/');
	    if (!w) w=file;
	    else w++; 
	  } else {
	    isdir=FALSE;
	    w++;
	  }
	  if (!strlen(w)) continue;
	  else filename=g_strdup(w);	  
  }
  else {
	  isdir=FALSE;
	  filename=g_strdup(file);
  }

  for (i=0;i<strlen(file);i++) if (file[i]=='/') file[i]='\\'; 
  /*for (i=0;i<strlen(file);i++) latin_1_unreadable(file+i); 
  for (i=0;i<strlen(filename);i++) latin_1_readable(filename+i); */
/* 2.5- get drop target */  
  
/* 3- download via tubo */ 
  if (first){
    first=FALSE;
    orig_share=g_strdup(share);
    fprintf(tmpfile,"//%s/%s\n",host,share);
    fprintf(tmpfile,"%s\n",user);
  }
  /* only process files from first drop */
  if (orig_share && strcmp(share,orig_share)!=0) {
	  xf_dlg_warning(win->top,_("Only drops from a single share are allowed"));
	  g_free(orig_share);
	  return;
  }
    if (isdir){
      fprintf(tmpfile,
	"lcd \"%s\";cd \"/%s\";cd ..;prompt;recurse; mget %s;recurse;prompt;cd /;",target,file,filename);
    }
    else 
	    fprintf(tmpfile,"lcd \"%s\";get \"%s\" \"%s\";",target,file,filename);
 
 } /* end for list elements */
  fclose(tmpfile);
  strcpy(SMBtmpfile,fname); 
  if (filename) g_free(filename);
  filename=NULL;
  /* wait until OK to proceed */
  while (fork_obj) {
     while (gtk_events_pending()) gtk_main_iteration();
     usleep(50000);
  }
  cursor_wait (GTK_WIDGET (SMBctree));
  download_window(win->top,host);
  SMBResult=0;
  fork_obj = Tubo (SMBFork, SMBForkOver, TRUE, SMBStdout, SMBStdout);
  if (orig_share) g_free(orig_share);

  return;
}
