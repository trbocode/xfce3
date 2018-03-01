/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
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

/* functions to use tubo.c for mkdir */

/*******SMBmkdir******************/

static char *new_dir=NULL;
/* function executed after all pipes
*  timeouts and inputs have been set up */
static void
SMBmkdirFork (void)
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



/* function to process stdout produced by child */
static int
SMBmkdirStdout (int n, void *data)
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
  print_status (line);

  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBmkdirForkOver (pid_t pid)
{
  GtkCTreeNode *node;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Directory creation failed. See diagnostics for details."));
    break;
  default:
    /* directory creation was successful: */
    {
      char *textos[SHARE_COLUMNS];
      int i;
      time_t fecha;
      fecha = time (NULL);
      print_status (_("Directory created."));
      for (i = 0; i < SHARE_COLUMNS; i++)
	textos[i] = "";
      textos[SHARE_NAME_COLUMN] = new_dir;
      textos[SHARE_SIZE_COLUMN] = "0";
      textos[SHARE_DATE_COLUMN] = ctime (&fecha);
      if (strstr(textos[SHARE_DATE_COLUMN],"\n")) strtok(textos[SHARE_DATE_COLUMN],"\n");
      textos[COMMENT_COLUMN] = (char *) malloc (1 + strlen (selected.share) + strlen (selected.dirname) + 1 + strlen (new_dir) + 1);
      if (strcmp (selected.dirname, "/") == 0)
      {
	sprintf (textos[COMMENT_COLUMN], "/%s/%s", selected.share, new_dir);
      }
      else
      {
	sprintf (textos[COMMENT_COLUMN], "/%s%s/%s", selected.share, selected.dirname, new_dir);
      }
      {
	smb_entry *data;
	data=smb_entry_new(); 
	data->i[0] = data->i[1] = 0;
	data->i[2]=1;
	data->label=g_strdup(new_dir);
	data->type |= S_T_DIRECTORY;
	data->dirname=(char *)malloc(strlen(selected.dirname+1)+strlen(new_dir)+2);
	sprintf(data->dirname,"%s/%s",selected.dirname+1,new_dir);
	data->share=g_strdup(selected.share);
        node = add_node(data,textos,(GtkCTreeNode *) selected.node);
        gtk_ctree_sort_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node);	
      }
      g_free (textos[COMMENT_COLUMN]);
    }
    break;
  }
  if (new_dir) g_free(new_dir);
}

extern int mount_stderr (int n, void *data);

void
SMBmkdir_with_name (char *new_dir)
{
  int i;
  animation (TRUE);
  cursor_wait (GTK_WIDGET (smb_nav));
  stopcleanup = FALSE;


  if (!new_dir)
  {
    print_status (_("Create directory cancelled."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;
  }

/* let's allow subdirectories, if user knows his way around */

  if (strlen (new_dir) + strlen (selected.dirname) + strlen ("mkdir") + 5 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("Create directory failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));
    return;

  }
  {
    char *t,*s;
    t=g_strdup(new_dir);
    s=g_strdup(selected.dirname);
    latin_1_unreadable(t); /* this a smbclient bugworkaround */
    latin_1_unreadable(s); /* this a smbclient bugworkaround */
    sprintf (NMBcommand, "mkdir \\\"%s\\%s\\\"", s, t);
    g_free(t);
    g_free(s);
  }
  for (i = 0; i < strlen (NMBcommand); i++)
    if (NMBcommand[i] == '/') NMBcommand[i] = '\\';
  print_diagnostics (NMBcommand);
  print_diagnostics ("\n");
  fork_obj = Tubo (SMBmkdirFork, SMBmkdirForkOver, TRUE, SMBmkdirStdout, mount_stderr);
}

void
SMBmkdir (void)
{				/* data is a pointer to the share */
  char *entry_return;
  if (!selected.dirname)  return;

  while (not_unique (fork_obj)) {
     while (gtk_events_pending()) gtk_main_iteration();
     usleep(50000);
  }

  print_status (_("Creating dir..."));
  stopcleanup = FALSE;

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  
  
  /* here, dialog to ask new dir name */
  entry_return=(char *)xf_dlg_string(smb_nav,_("New directory name:"),_("New Folder"));
  if (!entry_return){
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    return;
  }
  
  new_dir = g_strdup(entry_return);
  SMBmkdir_with_name (entry_return);

  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  return;
}
