/* (c) 2001-2002 Edscott Wilson Garcia GNU/GPL
 */

/* functions to use tubo.c for listing contents of SMB shares */

/******* SMBlist (was SMDclient) */
#ifndef INCLUDED_BY_XFSAMBA_C

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "../xftree/ft_types.h"
#include "../xftree/xtree_icons.h"

#endif


static GtkCTreeNode *LastNode;
void
smb_entry_free (smb_entry *data)
{
  if (data) {
   if (data->label) g_free(data->label);
   if (data->share) g_free(data->share);
   if (data->dirname) g_free(data->dirname);
   if (data->filename) g_free(data->filename);
   g_free (data);
  }
}

/*
 */
smb_entry *smb_entry_new (void)
{
  smb_entry *en = g_malloc (sizeof (smb_entry));
  if (!en) return (NULL);
  en->share = en->dirname = en->label = en->filename = NULL;
  en->i[0] = en->i[1] = en->i[2] = en->type = 0;
  return (en);
}


GtkCTreeNode *add_node(smb_entry *en,char **textos,GtkCTreeNode *nodo){
  GtkCTreeNode *node;
  icon_pix pix;  
  gboolean isleaf;
  if (en->type & S_T_DIRECTORY) { 
     if (textos[SHARE_SIZE_COLUMN]) 
	en->i[0] = atoi (textos[SHARE_SIZE_COLUMN]);
     else en->i[0] = 0;
     set_icon_pix(&pix,FT_DIR,en->label,0);
     isleaf=FALSE;
  } else if (en->type & S_T_PRINTER){
      isleaf=TRUE;
      en->i[0] =  0;
      set_icon_pix(&pix,FT_SMB|FT_PRINT,en->label,0);
  } else if (en->type & S_T_SHARE) {
     isleaf=FALSE;
      en->i[0] =  0;
      set_icon_pix(&pix,FT_DIR|FT_EXE,en->label,0);
  } else if (en->type & S_T_IPC) {
      isleaf=TRUE;
      en->i[0] =  0;
      set_icon_pix(&pix,FT_FILE|FT_HIDDEN,en->label,0);
  } else {
	isleaf=TRUE;
        if (textos[SHARE_SIZE_COLUMN]) 
	   en->i[0] = atoi (textos[SHARE_SIZE_COLUMN]);
        else en->i[0] = 0;
        set_icon_pix(&pix,FT_FILE,textos[SHARE_NAME_COLUMN],0);
	  /*FIXME: FT_READONLY nor FT_HIDDEN are not used at xtree_icons.c */
	if ((en->type & S_T_READONLY)&&(en->type & S_T_HIDDEN)){
          set_icon_pix(&pix,FT_FILE|FT_READONLY|FT_HIDDEN,textos[SHARE_NAME_COLUMN],0);
	} else if (en->type & S_T_READONLY){
          set_icon_pix(&pix,FT_FILE|FT_READONLY,textos[SHARE_NAME_COLUMN],0);
	} else if (en->type & S_T_HIDDEN) {
          set_icon_pix(&pix,FT_FILE|FT_HIDDEN,textos[SHARE_NAME_COLUMN],0);
	}
  }
  node = gtk_ctree_insert_node ((GtkCTree *) shares, 
		  nodo, NULL, textos, 
		  SHARE_COLUMNS, 
		  pix.pixmap,pix.pixmask, pix.open,pix.openmask,
		  isleaf, FALSE); 
  en->i[1] = 0; /* to have date sorting work, must parse date into a time_t number */

  /* subsorting order */
  if (en->type & (S_T_DIRECTORY)) en->i[2] = 3;
  else if (en->type & S_T_SHARE) en->i[2] = 2;
  else if (en->type & S_T_PRINTER) en->i[2] = 1;
  else if (en->type & S_T_IPC) en->i[2] = 0;
  else  en->i[2] = 4;
  en->label = g_strdup(textos[SHARE_NAME_COLUMN]);
    /*printf("dbg:%s-->%d\n",textos[SHARE_NAME_COLUMN],data[2]);*/
  gtk_ctree_node_set_row_data_full ((GtkCTree *) shares, node, en, node_destroy);
    /* FIXME: insert a dummy entry here for dirs. Be sure to remove it on the first expansion,
     *        as determined by reload. */
  return node;
 
}


/* function to process stdout produced by child */
static int
SMBListStdout (int n, void *data)
{
  char *line;
  char *textos[SHARE_COLUMNS];
  char directorio[XFSAMBA_MAX_STRING];
  int i, filenamelen;
  char *pw;
  GtkCTreeNode *node;
  smb_entry *en;
  

  if (n) return TRUE;  /* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  for (i=0;challenges[i]!=NULL;i++){
      if (strstr (line, challenges[i]))  {
        SMBResult = CHALLENGED;
        print_diagnostics ("DBG:");
        print_diagnostics (line);
        /* here we must pop it from the cache!!!!! */    
      }	
  }
  if (strlen (line) < 2)	return TRUE;
  if (strstr (line, "  .   "))	return TRUE;
  if (strstr (line, "  ..   "))	return TRUE;
  if (strncmp (line, "  ", 2))	return TRUE;
  
  /* ok. Now we have a line to process
   * This is a line to watch, if smbclient changes the format
   * for output in file client.c, this must be taken into account */
  /* client.c: "  %-30s%7.7s %8.0f  %s",filename,attr,size,asctime */
  /* asctime=25 */
  /* if (strlen(line) > 25+2+8+1+7) */

  en=smb_entry_new();

  pw = line + (strlen (line) - 1 - 25 - 2 - 8);
  while (pw[0] != ' ') {
    if (pw == line) break;
    pw--;
  }

  filenamelen = strlen (line) - strlen (pw) - 7;

  while (pw[0] == ' ') {
    if (pw[0] == 0) break;
    pw++;
  }

  /*filenamelen = strlen (line) - 25 - 2 - 8 - 1 - 7; */
  for (i = 0; i < SHARE_COLUMNS; i++) textos[i] = "";
  textos[SHARE_NAME_COLUMN] = line + 2;
  for (i = filenamelen + 1; i < filenamelen + 8; i++)  {
    if (line[i] == 'D') en->type |= 0x08;
    if (line[i] == 'H') en->type |= 0x04;
    if (line[i] == 'R') en->type |= 0x02;
    line[i] = 0;
  }

  if (strstr (pw, "\n"))  strtok (pw, "\n"); /* chop */

  if (strstr (pw, " ")) {
    textos[SHARE_SIZE_COLUMN] = strtok (pw, " ");
    textos[SHARE_DATE_COLUMN] = pw + strlen (pw) + 1;
  }

/* This bugworkaround no longer necesary. Still necesary for
 * smblookup though */
  /*latin_1_readable (line);*/
  en->share=g_strdup(NMBshare);
  {
    char *word;
    word = textos[SHARE_NAME_COLUMN];
    while (word[strlen (word) - 1] == ' ')
      word[strlen (word) - 1] = 0;
  }

  if (en->type & S_T_DIRECTORY)
  {
    if (strcmp (selected.dirname, "/") == 0) {
      sprintf (directorio, "/%s/%s", NMBshare, line + 2);
      en->dirname=(char *)malloc(strlen(line + 2)+2);
      sprintf(en->dirname,"/%s",line + 2);
    } else {
      sprintf (directorio, "/%s%s/%s", NMBshare, selected.dirname, line + 2);
      en->dirname=(char *)malloc(strlen(line + 2)+strlen(selected.dirname)+2);
      sprintf(en->dirname,"%s/%s",selected.dirname,line + 2);
    }
    textos[COMMENT_COLUMN] = directorio;
  } else {
    en->dirname=g_strdup(selected.dirname);
    en->filename=g_strdup(textos[SHARE_NAME_COLUMN]);
    en->type = S_T_FILE;
  }
  node = add_node(en,textos,(GtkCTreeNode *) selected.node);
  if (selected.node) gtk_ctree_sort_node ((GtkCTree *) shares, (GtkCTreeNode *) selected.node);
  return TRUE;
}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBListForkOver (pid_t pid)
{
  /* no jalo para arreglar directorios: 
     gtk_ctree_sort_node ((GtkCTree *)shares,(GtkCTreeNode *)selected.node);
   */
  print_status (_("Retrieve done."));
  fork_obj = 0;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Query password has been requested."));
    gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (1)), GTK_WINDOW (smb_nav));
    break;
  default:
    break;

  }
  gtk_clist_thaw (GTK_CLIST (shares));
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);

}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
SMBListFork (void)
{
  char *the_netbios;
  the_netbios = (char *) malloc (strlen (NMBnetbios) + strlen (NMBshare) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, NMBshare);

  fprintf (stderr, "CMD: SMBclient fork: ");
  fprintf (stderr, "smbclient %s   %s   %s   %s   %s\n", the_netbios, "-U", "*******", "-c", NMBcommand);
  fflush (NULL);
  execlp ("smbclient", "smbclient", the_netbios, "-U", NMBpassword, "-c", NMBcommand, (char *) 0);
}


void
SMBList (void)
{
  stopcleanup = FALSE;
  if (not_unique (fork_obj))
  {
    return;
  }
  /* LastNode not used in this file! anywhere else? */
  LastNode = (GtkCTreeNode *) selected.node;

  if (strlen (selected.dirname) + strlen ("ls") + 4 > XFSAMBA_MAX_STRING)
  {
    print_diagnostics ("DBG: Max string exceeded!");
    print_status (_("List failed."));
    animation (FALSE);
    cursor_reset (GTK_WIDGET (smb_nav));

    return;

  }
  {
   char *t;
   t=g_strdup(selected.dirname);
   latin_1_unreadable(t); /* this a smbclient bugworkaround */
   sprintf (NMBcommand, "ls \\\"%s\\\"*", t);
   g_free(t);

  }
  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBshare, selected.share, XFSAMBA_MAX_STRING);
  NMBshare[XFSAMBA_MAX_STRING] = 0;

  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;

  print_status (_("Retreiving..."));

  gtk_clist_freeze (GTK_CLIST (shares));
  fork_obj = Tubo (SMBListFork, SMBListForkOver, TRUE, SMBListStdout, parse_stderr);
  return;
}
