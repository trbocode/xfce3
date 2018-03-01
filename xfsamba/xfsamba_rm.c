/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
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

/* functions to use tubo.c for rm and rmdir */

/*******SMBrmdir******************/
extern char *randomTmpName(char *ext);

static char  *CreateTmpList(void){
  GList *s;
  smb_entry *en;
  FILE *tmpfile;
   static char *fname=NULL;
       
   if ( g_list_length (GTK_CLIST (shares)->selection)){
     if (xf_dlg_ask(smb_nav,_("Really delete remote items?"))!=DLG_RC_OK) {
         cursor_reset (GTK_WIDGET (smb_nav));
         animation (FALSE);
         print_status (_("Remove cancelled."));
         return NULL;
     }
   }
   
   if ((fname=randomTmpName(NULL))==NULL) return NULL;
   if ((tmpfile=fopen(fname,"w"))==NULL) return NULL;
   for (s = GTK_CLIST (shares)->selection; s != NULL; s=s->next){
     char *t,*ss;
     en = gtk_ctree_node_get_row_data ((GtkCTree *)shares, s->data);
     t=g_strdup(en->dirname);
     latin_1_unreadable(t); /* this a smbclient bugworkaround */
     if (en->type & S_T_DIRECTORY){
        fprintf(tmpfile,"cd /;rmdir \"%s\\\";\n",t);	     
     } else {
        ss=g_strdup(en->filename);
        latin_1_unreadable(ss); /* this a smbclient bugworkaround */
	fprintf(tmpfile,"cd \"%s\";\n",t);
	fprintf(tmpfile,"del \"%s\";\n",ss);
	g_free(ss);
     }
     g_free(t);
   }
   fclose (tmpfile);
   return fname;
}


/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBrmFork (void)
{
  char *the_netbios;
  char *the_command=NULL;
  struct stat s;
  char line[256];
  int i;
  FILE *tmpfile;
  
  the_netbios = (char *) malloc (strlen ((char *) NMBnetbios) + strlen ((char *) NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);
  
  if (stat(NMBcommand,&s)<0)  _exit(123);
  if ((the_command = (char *)malloc(s.st_size+1))==NULL)  _exit(123);
  if ((tmpfile=fopen(NMBcommand,"r"))==NULL)  _exit(123);
  strcpy(the_command,"");
  while (!feof(tmpfile) && fgets(line,255,tmpfile)){
	  char *w;
	  line[255]=0;
	  if (!strstr(line,"\n")) continue;
	  w=strtok(line,"\n");
          /*fprintf (stderr, "DBG:child->%s\n", w);fflush(NULL);*/
	  strcat(the_command,w);	  
  }
  for (i = 0; i < strlen (the_command); i++) if (the_command[i] == '/') the_command[i] = '\\';
  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", the_command, (char *) 0);
}


/* function to process stdout produced by child */
static int
SMBrmStdout (int n, void *data)
{
  char *line;
  int i;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  for (i=0;challenges[i]!=NULL;i++){
      if (strstr (line, challenges[i]))  {
        SMBResult = CHALLENGED;
        xf_dlg_warning(smb_nav,line);
      }	
  }
  print_diagnostics (line);

  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBrmForkOver (pid_t pid)
{
  GList *s;
  GList *e=NULL;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Remove failed. See diagnostics for details."));
    break;
  default:
    /* directory creation was successful: remove node from tree */
   for (s = GTK_CLIST (shares)->selection; s != NULL; s=s->next){
    char *line[3];
    if (gtk_ctree_node_get_text ((GtkCTree *)shares, s->data,COMMENT_COLUMN, line + 1)){
      eliminate2_cache (thisN->shares, line[1]);
      e=g_list_append(e,(gpointer)s->data);
    }
   }
   for (s=e; s != NULL; s=s->next){
    gtk_ctree_remove_node ((GtkCTree *) shares, s->data);
   } g_list_free(e);  
   
    print_status (_("Remove complete."));
    selected.directory = selected.file = FALSE;
    if (selected.share) free (selected.share);
    selected.share = NULL;
    if (selected.dirname) free (selected.dirname);
    selected.dirname = NULL;
    if (selected.filename) free (selected.filename);
    selected.filename = NULL;
    if (selected.comment) free (selected.comment);
    selected.comment = NULL;
    break;
  }
}

extern int mount_stderr (int n, void *data);

void
SMBrm (void)
{				/* data is a pointer to the share */

  if ((!selected.filename) && (!selected.dirname)) return;
  while (not_unique (fork_obj)) {
     while (gtk_events_pending()) gtk_main_iteration();
     usleep(50000);
  }
  stopcleanup = FALSE;

  print_status (_("Removing..."));


#if 0
/* this is taken care of*/
  if (!strncmp (selected.comment, "Disk", strlen ("Disk")))
  {
    xf_dlg_warning (smb_nav,_("Sorry, top level shares cannot be removed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    print_status (_("Remove cancelled."));
    return;
  }
#endif


  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  {
    char *tmplist;
    tmplist=CreateTmpList();
    if (!tmplist) return;
    strcpy(NMBcommand,tmplist);
  }

/* do this on fork execution:
 *   for (i = 0; i < strlen (NMBcommand); i++) if (NMBcommand[i] == '/') NMBcommand[i] = '\\';*/

  print_diagnostics ("CMD: ");
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");
  
  fork_obj = Tubo (SMBrmFork, SMBrmForkOver, TRUE, SMBrmStdout, mount_stderr);


  return;
}



