/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

/* functions to use tubo.c for uploading SMB files */

/*******SMBPut******************/
/* function to process stdout produced by child */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <glob.h>
#include <time.h>
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
#include "fileselect.h"

#include "tubo.h"
#include "xfsamba.h"
#endif

static char *fileUp,*fileS;

/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBPutFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG:smbclient %s -c \"%s\"\n", the_netbios, NMBcommand);
  fflush (NULL);
  sleep (1);
#endif

  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", NMBcommand, (char *) 0);
}


static int
SMBPutStdout (int n, void *data)
{
  char *line;
  int i;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  for (i=0;challenges[i]!=NULL;i++){
      if (strstr (line, challenges[i]))  {
        SMBResult = CHALLENGED;
      }	
  }
  print_diagnostics (line);

  return TRUE;
}

static void putin(GtkCTree *ctree,GtkCTreeNode *nodo,char *path,char *label) {
      char line[256];
      GtkCTreeNode *node;
      time_t fecha;
      char sizeo[64];
      struct stat st;
      char *textos[SHARE_COLUMNS];
      smb_entry *data;
      off_t i, sizei = 0;
      for (i = 0; i < SHARE_COLUMNS; i++)textos[i] = "";
      textos[SHARE_NAME_COLUMN] = label;

      if (!path) textos[SHARE_SIZE_COLUMN] = "0";
      else if (lstat (path, &st) == 0) {
	sprintf (sizeo, "%lld", (long long)st.st_size);
	textos[SHARE_SIZE_COLUMN] = sizeo;
	sizei = st.st_size;
      } else {
	textos[SHARE_SIZE_COLUMN] = "0";
      }
      fecha=time(NULL);
      textos[SHARE_DATE_COLUMN] = ctime (&fecha);
      if (strstr(textos[SHARE_DATE_COLUMN],"\n"))
	      textos[SHARE_DATE_COLUMN] = strtok (textos[SHARE_DATE_COLUMN],"\n");  

      if (path) textos[COMMENT_COLUMN] = _("Uploaded file.");
      else {
	      sprintf(line,"/%s%s/%s",selected.share,selected.dirname,label);
	      textos[COMMENT_COLUMN] =line;
      }
      if (!path){
	data=smb_entry_new();
	data->type |= S_T_DIRECTORY;
	data->share=g_strdup(selected.share);
	data->dirname=(char *)malloc(strlen(selected.dirname)+strlen(label)+2);
	sprintf(data->dirname,"%s/%s",selected.dirname,label);
      } else {
	data=smb_entry_new();
	data->type |= S_T_FILE;
	data->share=g_strdup(selected.share);
	data->dirname=g_strdup(selected.dirname);
	data->filename=g_strdup(textos[SHARE_NAME_COLUMN]);
      }
      node = add_node(data,textos,nodo);
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */

static void
SMBPutForkOver (pid_t pid)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("File upload failed. See diagnostics for reason."));
    break;
  default:
    /* upload was successful: */
    {
      print_status (_("Upload done."));
      putin((GtkCTree *)shares,(GtkCTreeNode *)selected.node,fileS,fileUp);
    }
    break;

  }
  fork_obj = NULL;
}


void
SMBPutFile (void)
{
  glob_t dirlist;
  char *dataO;
  int i;

  if (!selected.directory)
  {
    return;
  }
  if (not_unique (fork_obj))
  {
    return;
  }

  stopcleanup = FALSE;
  print_status (_("Uploading file..."));


  fileS = open_fileselect ("");
  if (!fileS)
  {
    print_status (_("File upload cancelled."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }


  if (glob (fileS, GLOB_ERR, NULL, &dirlist) != 0)
  {
    globfree (&dirlist);
    xf_dlg_warning (smb_nav,_("Specified file does not exist"));
    print_status (_("Upload failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }
  globfree (&dirlist);

  fileUp = fileS;
  while (strstr (fileUp, "/"))
    fileUp = strstr (fileUp, "/") + 1;

  if (strlen (fileUp) + strlen (selected.dirname) + strlen (fileS) + strlen ("put") + 5 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("Upload failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;

  }

  dataO = (char *) malloc (strlen (selected.dirname) + 1);

  strcpy (dataO, selected.dirname);
  for (i = 0; i < strlen (dataO); i++)
  {
    if (dataO[i] == '/')
    {
      dataO[i] = '\\';
    }
  }


  {
   char *t,*s;
   t=g_strdup(dataO);
   s=g_strdup(fileUp);
   latin_1_unreadable(t); /* this a smbclient bugworkaround */
   latin_1_unreadable(s); /* this a smbclient bugworkaround */
   sprintf (NMBcommand, "put \"%s\" \\\"%s\\%s\\\"", fileS, t, s);
   g_free(t);
   g_free(s);
  }
  free (dataO);
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");


  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  fork_obj = Tubo (SMBPutFork, SMBPutForkOver, TRUE, SMBPutStdout, parse_stderr);
  return;
}


static char *upload_tmpfile;

GtkCTreeNode *DropNode=NULL;

static void
SMBDropForkOver (pid_t pid)
{
  while (gtk_events_pending()) gtk_main_iteration();
  gdk_flush();
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Upload produced errors. See diagnostics for reason."));
    break;
  default:
    /* upload was successful: */
    if (upload_tmpfile) {
	FILE *tmpF;
	tmpF=fopen(upload_tmpfile,"r");
	if (tmpF){
	  char line[256];
	  while (!feof(tmpF) && fgets(line,255,tmpF)){
	    char *w;
	    line[255]=0;
	    if (strstr(line,"mput")) continue;
	    if (strstr(line,"put")){
	      w=strtok(line,"\""); if (!w) break;
	      w=strtok(NULL,"\""); if (!w) break;
	      if (!strchr(w,'/')) break;
	      if (!DropNode) break;
              /*fprintf (stderr, "DBG:parent->%s\n", w);*/
	      putin((GtkCTree *)shares,DropNode,w,strrchr(w,'/')+1);	    
	    }
	    else if (strstr(line,"mkdir")){
	      w=strtok(line,"\""); if (!w) break;
	      w=strtok(NULL,"\""); if (!w) break;
	      putin((GtkCTree *)shares,DropNode,NULL,w);	    
	    }
	  }
	  fclose (tmpF);
	}
    }
    print_status (_("Upload done."));
    break;

  }
  if (upload_tmpfile) unlink(upload_tmpfile);
  fork_obj = NULL;
}

static void forgetit(char *why,char *who){
          fprintf (stderr, "xfsamba: %s %s\n",(why)?why:" ",(who)?who:" ");
	  fflush(NULL);
	  usleep(50000);
          _exit(123);
}

/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBDropFork (void)
{
  FILE *tmpfile;
  char *the_netbios;
  char *the_command=NULL;
  char line[256];
  struct stat s;
  if ((the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3))==NULL)
	  forgetit("insufficient memory",NULL);
  if (stat(NMBcommand,&s)<0)  forgetit("unable to stat temp file",NMBcommand);
  if ((the_command = (char *)malloc(s.st_size+1))==NULL) forgetit("unable allocate memory for",NMBcommand);
  if ((tmpfile=fopen(NMBcommand,"r"))==NULL) forgetit("unable to open",NMBcommand);
  strcpy(the_command,"");
  while (!feof(tmpfile) && fgets(line,255,tmpfile)){
	  char *w;
	  line[255]=0;
	  if (!strstr(line,"\n")) continue;
	  w=strtok(line,"\n");
          /*fprintf (stderr, "DBG:child->%s\n", w);fflush(NULL);*/
	  strcat(the_command,w);	  
  }
  /*fprintf (stderr, "DBG:child, the_command-> %s\n", the_command);fflush(NULL);*/
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);

  
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG:smbclient %s -c %s\n", the_netbios, the_command);
  fflush (NULL);
  sleep (1);
#endif

  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", the_command, (char *) 0);
}



void
SMBDropFile (char *tmpfile)
{

  if (!selected.directory) goto abortdrop;
  SMBabortdrop=FALSE;
  while (not_unique (fork_obj)) {
     while (gtk_events_pending()) gtk_main_iteration();
     usleep(50000);
  }

  /* when password challenged, do the drop again. */
  if (SMBabortdrop) {
abortdrop:
	gdk_flush();
        cursor_reset (GTK_WIDGET (smb_nav));
        animation (FALSE);
        return;
  }	
  /*printf("dbg: going on. SMBResult=%d (challenged=%d)\n",SMBResult,CHALLENGED);*/
  
  upload_tmpfile=tmpfile;
  stopcleanup = FALSE;
  print_status (_("Uploading files..."));
  strncpy (NMBcommand, tmpfile, XFSAMBA_MAX_STRING);
  NMBcommand[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;
  while (fork_obj) usleep(50000);
  fork_obj = Tubo (SMBDropFork, SMBDropForkOver, TRUE, SMBPutStdout, parse_stderr);
  return;
}
