/*   *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <glob.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>

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
#include "fileselect.h"

/* local xfsamba includes : */
#include "tubo.h"
#include "xfsamba.h"
#include "../xftree/ft_types.h"
#include "../xftree/xtree_icons.h"

#endif

static int cual_chingao;
static GList *items = NULL;
static void
SMBprintTitles (void)
{
  char message[256];
  gtk_clist_clear ((GtkCList *) shares);
  gtk_clist_clear ((GtkCList *) servers);
  gtk_clist_clear ((GtkCList *) workgroups);

  {
    nmb_history *currentH;
    if (items != NULL)
    {
      g_list_free (items);
      items = NULL;
    }
    currentH = thisH;
    while (currentH)
    {
      if (currentH->record)
      {
	if (currentH->record->server)
	  items = g_list_append (items, currentH->record->server);
	if (currentH->record->serverIP)
	  gtk_label_set_text ((GtkLabel *) locationIP, currentH->record->serverIP);
	else
	  gtk_label_set_text ((GtkLabel *) locationIP, "-");
      }
      currentH = currentH->previous;
    }
    if (items)
      gtk_combo_set_popdown_strings (GTK_COMBO (location), items);
    if ((thisH) && (thisH->record) && (thisH->record->serverIP))
      gtk_label_set_text ((GtkLabel *) locationIP, thisH->record->serverIP);
    else
      gtk_label_set_text ((GtkLabel *) locationIP, "");
  }
  if (thisN->server)
  {
    sprintf (message, "%s : %s", thisN->server, _("Shares"));
    gtk_label_set_text ((GtkLabel *) sharesL, message);
    sprintf (message, "%s : %s", thisN->server, _("Links to other servers"));
    gtk_label_set_text ((GtkLabel *) serversL, message);
    sprintf (message, "%s : %s", thisN->server, _("Links to other workgroups"));
    gtk_label_set_text ((GtkLabel *) workgroupsL, message);
  }
}

static void
SMBprint (nmb_list * currentN)
{
  icon_pix pix;  
  nmb_cache *cache;
  GtkCTreeNode *node;
  smb_entry *data;
  cache = currentN->shares;
  while (cache)
  {
    data=smb_entry_new();
    if ((cache->textos[COMMENT_COLUMN]) 
		    && (strncmp (cache->textos[COMMENT_COLUMN], 
				    "Printer", strlen ("Printer")) == 0))
    {
      data->type |= S_T_PRINTER;
    } else if ((cache->textos[COMMENT_COLUMN]) 
		    && (strncmp (cache->textos[COMMENT_COLUMN], 
				    "IPC", strlen ("IPC")) == 0))
    {
      data->type |= S_T_IPC;
    } else {
      data->dirname=g_strdup("/");
      data->share=g_strdup(cache->textos[SHARE_NAME_COLUMN]);
      data->type |= S_T_SHARE;
    }
    if (cache->textos[SHARE_NAME_COLUMN])
    {
        node = add_node(data,cache->textos,NULL);	
    }

    cache = cache->next;
  }
  cache = currentN->servers;
  while (cache)
  {
    gint row;
    row = gtk_clist_append ((GtkCList *) servers, cache->textos);
    if (cache->visited) set_icon_pix(&pix,FT_COMP2|FT_SMB,"",0);
    else set_icon_pix(&pix,FT_COMP1|FT_SMB,"",0);
    
    gtk_clist_set_pixmap ((GtkCList *) servers, row, 0, 
		    pix.pixmap,pix.pixmask);
    cache = cache->next;
  }
  cache = currentN->workgroups;
  while (cache)
  {
    gint row;
    row = gtk_clist_append ((GtkCList *) workgroups, cache->textos);
    if (cache->visited) set_icon_pix(&pix,FT_WG2|FT_SMB,"",0);
    else set_icon_pix(&pix,FT_WG1|FT_SMB,"",0);
    gtk_clist_set_pixmap ((GtkCList *) workgroups, row, 0, 
		    pix.pixmap,pix.pixmask);
    cache = cache->next;
  }
  if (SMBResult == SUCCESS){
    GtkCList *clist;
    clist=(GtkCList *)shares;
    print_status (_("Query done."));
    gtk_clist_set_sort_column (clist, SHARE_NAME_COLUMN);
    clist->sort_type = GTK_SORT_ASCENDING;
    gtk_clist_sort (clist);    
  }
  if (SMBResult == FAILED)
    print_status (_("Query failed. Machine may be down."));

}

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
SMBForkOver (pid_t pid)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  SMBprint (thisN);
  thisN->loaded = 1;
  if (SMBResult == CHALLENGED)
  {
    print_status (_("Query password has been requested."));
    gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (1)), GTK_WINDOW (smb_nav));
  }
  fork_obj = NULL;
  nonstop = FALSE;
}

/* function to process stdout produced by child */
static int
SMBparseLookup (int n, void *data)
{
  char *line;
  int i;
  gchar *textos[SHARE_COLUMNS];
  static char *position[2];

  /* data is a static memory location */
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  if (strstr (line, "Connection") && strstr (line, "failed"))
  {
    cual_chingao = LOCATION_SHARES;
    SMBResult = FAILED;
    position[0] = line;
    position[1] = NULL;
    print_diagnostics (line);
  }
  /* pre samba 2.2 */
  if (strstr (line, "Access") && strstr (line, "denied"))
  {
    cual_chingao = LOCATION_SHARES;
    SMBResult = CHALLENGED;
    position[0] = line;
    position[1] = NULL;
    print_diagnostics (line);
  }
  /* post samba 2.2 */
  for (i=0;challenges[i]!=NULL;i++){
      if (strstr (line, challenges[i]))  {
        cual_chingao = LOCATION_SHARES;
        SMBResult = CHALLENGED;
        position[0] = line;
        position[1] = NULL;
        print_diagnostics (line);
      }	
  }
  if (strstr (line, "--------"))
  {
    char *buf;
    position[0] = strstr (line, "---");
    buf = strtok (position[0], " ");

    if (buf)
    {
      buf = strtok (NULL, "\n");
      if (buf)
	position[1] = strstr (buf, "---");
    }
    return TRUE;
  }
  if (strlen (line) < 3)
    return TRUE;
  if (strstr (line, "Sharename") && strstr (line, "Comment"))
  {
    cual_chingao = LOCATION_SHARES;
    position[0] = position[1] = NULL;
    return TRUE;
  }
  if (strstr (line, "Server") && strstr (line, "Comment"))
  {
    cual_chingao = LOCATION_SERVERS;
    position[0] = position[1] = NULL;
    return TRUE;
  }
  if (strstr (line, "Workgroup") && strstr (line, "Master"))
  {
    cual_chingao = LOCATION_WORKGROUPS;
    position[0] = position[1] = NULL;
    return TRUE;
  }
  if (!position[0])
    return TRUE;
  if (strstr (line, "\n"))
    strtok (line, "\n");	/* chop */
  latin_1_readable (line);

  {
    int i;
    for (i = 0; i < SHARE_COLUMNS; i++)
      textos[i] = "";
  }
  textos[SHARE_NAME_COLUMN] = position[0];
  if (!position[1])
  {
    textos[COMMENT_COLUMN] = "*";
    if (cual_chingao == LOCATION_WORKGROUPS)
      textos[WG_MASTER_COLUMN] = "*";
    if (cual_chingao == LOCATION_SERVERS)
      textos[SERVER_COMMENT_COLUMN] = "*";
  }
  else
  {
    *(position[1] - 1) = 0;
    textos[COMMENT_COLUMN] = position[1];
    if (cual_chingao == LOCATION_WORKGROUPS)
      textos[WG_MASTER_COLUMN] = position[1];
    if (cual_chingao == LOCATION_SERVERS)
      textos[SERVER_COMMENT_COLUMN] = position[1];
  }


  switch (cual_chingao)
  {
  case LOCATION_SHARES:
    thisN->shares = push_nmb_cache (thisN->shares, textos);
    break;
  case LOCATION_SERVERS:
    thisN->servers = push_nmb_cache (thisN->servers, textos);
    break;
  case LOCATION_WORKGROUPS:
    thisN->workgroups = push_nmb_cache (thisN->workgroups, textos);
    break;
  default:
    return TRUE;
  }

  return TRUE;
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
SMBFork (void)
{
  if (strlen (NMBpassword))
    execlp ("smbclient", "smbclient", "-N", "-L", NMBnetbios, "-U", NMBpassword, (char *) 0);
  else
    execlp ("smbclient", "smbclient", "-N", "-L", NMBnetbios, (char *) 0);
}

void
SMBLookup (unsigned char *servidor, int reload)
{
  char message[256];
  /*static gboolean first=TRUE;*/
  if (fork_obj)
  {
    print_diagnostics ("DBG:fork object not NULL!\n");
    return;
  }
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  SMBCleanLevel2 ();
  cual_chingao = 0;
  SMBResult = SUCCESS;
  thisN = headN;
  if (servidor)
  {
    while (thisN)		/* push into first level cache (if not there) */
    {
      if (strcmp ((char *) (thisN->server), (char *) servidor) == 0)
      {
	break;
      }
      thisN = thisN->next;
    }
    if (!thisN)
      thisN = push_nmbName (servidor);
  }

  if (!thisN->netbios)
  {				
    /* this will enter if nmblookup was stopped */
    clean_nmb ();
    NMBmastersLookup (NULL);
    return;
  }

  sprintf (message, "%s %s (%s)", _("Querying"), thisN->server, thisN->netbios);
  print_status (message);
  sprintf (NMBnetbios, "%s", thisN->netbios);

  if (!thisN->password)
  {
    thisN->password = (char *) malloc (strlen (default_user) + 1);
    strcpy (thisN->password, default_user);
  }
  strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
  NMBpassword[XFSAMBA_MAX_STRING] = 0;
/*  if (thisN->password)
    {
      sprintf (NMBpassword, "%s", thisN->password);
    }
  else NMBpassword[0]=0;*/

  if ((!thisH) || ((thisH) && (thisN != thisH->record)))
  {
    smoke_history (thisH);
    thisH = push_nmb_history (thisN);
  }
  SMBprintTitles ();

  {
    if ((reload) || (!thisN->loaded))
    {
#ifdef DBG_XFSAMBA
      print_diagnostics ("DBG:Reloading server shares.\n");
#endif
      thisN->shares = clean_cache (thisN->shares);
      thisN->servers = clean_cache (thisN->servers);
      thisN->workgroups = clean_cache (thisN->workgroups);
      /*if (first) {
	      Tubo (SMBFork, SMBForkOver, FALSE, SMBparseLookup, parse_stderr);
	      first=FALSE;
      } else
	      fork_obj =*/ Tubo (SMBFork, SMBForkOver, FALSE, SMBparseLookup, parse_stderr);
    }
    else
      SMBForkOver (1);		/* load from cache  instead */
  }


  return;
}


void
SMBrefresh (unsigned char *servidor, int reload)
{
  SMBResult = SUCCESS;
  if (!headN)
  {
    stopcleanup = TRUE;
    if (servidor)
      headN = push_nmbName (servidor);
    else
    {
      NMBmastersLookup (NULL);
      return;
    }
  }

  switch (reload)
  {
  case REFRESH:
    if (!servidor)
    {
      SMBLookup (headN->server, FALSE);
    }
    else
    {
      SMBprintTitles ();
      SMBForkOver (1);
    }
    break;
  case FORCERELOAD:
    SMBLookup (servidor, TRUE);
    break;
  case RELOAD:
  default:
    SMBLookup (servidor, FALSE);
    break;
  }
}


