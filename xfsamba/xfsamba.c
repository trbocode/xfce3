/*   xfsamba.c */

/*  an smb navigator for xfce desktop: requires the samba suite programs:
 *  smbclient and nmblookup (included in most linux distributions).
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 *  xfce modules used by xfsamba are copyright by Olivier Fourdan
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <glob.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
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
#include "xpmext.h"

/* local xfsamba includes : */
#define XFSAMBA_MAIN
#include "tubo.h"
#include "xfsamba.h"

/*#define DBG_XFSAMBA*/

#include "icons/warning.xpm"
#include "icons/xfsamba.xpm"

char *challenges[]={
	  "Error opening local file",
	  "ERRDOS",
	  "ERRSRV",
	  "ERRbadpw", /* before samba 2.2.3 */
	  "NT_STATUS_WRONG_PASSWORD", 
	  "NT_STATUS_ACCESS_DENIED",
	  "NT_STATUS_ILL_FORMED_PASSWORD",
	  "NT_STATUS_PASSWORD_RESTRICTION",
	  "NT_STATUS_NO_SUCH_USER",
	  "NT_STATUS_LOGON_FAILURE",
	  "NT_STATUS_ACCOUNT_RESTRICTION",
	  "NT_STATUS_INVALID_LOGON_HOURS",
	  "NT_STATUS_INVALID_WORKSTATION",
	  "NT_STATUS_PASSWORD_EXPIRED",
	  "NT_STATUS_ACCOUNT_DISABLED",
	  NULL
};


void cleanup_tmpfiles(void){
   glob_t dirlist;
   int i;
   /* unmount any shares mounted by xfsamba */
   clean_smbmnt();
   rmdir("/tmp/xfsamba");
   if (glob ("/tmp/xfsamba*", GLOB_ERR ,NULL, &dirlist) != 0) {
		  /*fprintf (stderr, "dbg:%s: no match\n", globstring);*/
	return;
   } else for (i = 0; i < dirlist.gl_pathc; i++) {
	   unlink(dirlist.gl_pathv[i]);
   }
   save_defaults();
}

void
xfsamba_abort (int why)
{
  gtk_main_quit ();
}

void
print_diagnostics (char *message)
{
  if (!message)
    return;
  gtk_text_insert (GTK_TEXT (diagnostics), NULL, NULL, NULL, message, strlen (message));
}

void
print_status (char *mess)
{
  static char message[256];
  int i;
  strncpy (message, mess, 255);
  message[255] = 0;
  if (strstr (message, "\n")) strtok (message, "\n");	/* chop chop */
  for (i=0;i<strlen(message);i++) if (message[i]=='\t') message[i]=' ';
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:");
  print_diagnostics (message);
  print_diagnostics ("\n");
#endif
  gtk_label_set_text ((GtkLabel *) statusline, message);
}

/* parse stderr sent by child for diagnostics: */
int
parse_stderr (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  return TRUE;
}

gboolean
not_unique (void *object)
{
  if (object)
  {
    print_diagnostics ("DBG:Fork object not null!\n");
    return 1;
  }
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:comment=");
  print_diagnostics (selected.comment);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:share=");
  print_diagnostics (selected.share);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:dirname=");
  print_diagnostics (selected.dirname);
  print_diagnostics ("\n");
  if (selected.file)
  {
    print_diagnostics ("DBG:filename=");
    print_diagnostics (selected.filename);
    print_diagnostics ("\n");
  }
#endif
  SMBResult = SUCCESS;
  return 0;
}


/* smb lookup particulars:**/

void
SMBCleanLevel2 (void)
{
  nmb_cache *cache;
  if (!thisN)
    return;
  if (!thisN->shares)
    return;
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:Cleaning second level cache\n");
#endif

  cache = thisN->shares;
  while (cache)
  {
    cache->visited = 0;
    if (!cache->textos[SHARE_NAME_COLUMN])
      smoke_nmb_cache (cache);
    cache = cache->next;
  }

}

#if 0
void
SMBreload (void)
{
  char *line[3];
  /* is variable LastNode used anywhere?? */
  /* function should be called only if node is in 
     *  second level cache. This will requiere some
     *  tweaking when multiple selections are enabled */
  if (!NMBselectedNode)
    return;
  if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) NMBselectedNode, COMMENT_COLUMN, line))
    return;
  if (line[0][0] != '/')
    return;			/* return !directory */
  while (line[0][strlen (line[0]) - 1] == ' ')
    line[0][strlen (line[0]) - 1] = 0;

  sprintf (the_share, "%s", line[0] + 1);

  /* delete tree entries */
  /* this is wrong, it should only remove children */
  gtk_ctree_remove_node ((GtkCTree *) shares, NMBselectedNode);
  /* */
  SMBTreeNode = NMBselectedNode;
  SMBList ((gpointer) the_share);

}
#endif

static void
on_ok_abort (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
    cleanup_tmpfiles();
  exit (1);
}

GtkWidget *
abort_dialog (char *message)
{
  GtkWidget *label, *button, *dialog, *pixmapwid;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;


  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_realize (dialog);

  pixmap = MyCreateGdkPixmapFromData (warning, dialog, &mask, FALSE);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), pixmapwid, FALSE, FALSE, 0);
  gtk_widget_show (pixmapwid);

  label = gtk_label_new (_("Samba failure! Xfsamba could not find file:"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("(please install Samba or else correct the PATH environment variable)"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok_abort), (gpointer) dialog);
  gtk_widget_show (button);

  gtk_widget_show (dialog);


  return dialog;
}

GtkWidget *io_parent;
static void
finishit (int sig)
{
#if 0
  if (sig == SIGUSR1) {
          /*while (gtk_events_pending()) gtk_main_iteration();*/
	  /* must do it this way to avoid threads fighting for gtk_main loop */
          gtk_timeout_add (260, (GtkFunction) open_warning, NULL);
	  return;
  } else 
#endif
  {
    fprintf(stderr,"xfsamba: signal %d received. Cleaning up before exiting\n",sig);
    cleanup_tmpfiles();
    on_signal(sig);
    exit(1);
  }
}



int
main (int argc, char *argv[])
{
  headN = thisN = NULL;
  thisH = NULL;
  fork_obj = NULL;
  selected.parent_node = selected.node = NULL;
  selected.comment = selected.share = selected.dirname = selected.filename = NULL;
  stopcleanup = TRUE;
  default_user = (char *) malloc (strlen ("Guest%") + 1);
  strcpy (default_user, "Guest%");

  read_defaults();

  xfce_init (&argc, &argv);

  /* for temporary file cleanup */
  signal (SIGHUP, finishit);
  signal (SIGINT, finishit);
  signal (SIGQUIT, finishit);
  signal (SIGABRT, finishit);
  signal (SIGBUS, finishit);
  signal (SIGSEGV, finishit);
  signal (SIGTERM, finishit);
  signal (SIGFPE, finishit);

  signal (SIGKILL, finishit);
  signal (SIGUSR1, finishit);
  signal (SIGUSR2, finishit);

  create_smb_window ();
  set_icon (smb_nav, "Xfsamba", xfsamba_xpm);
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  if (!sane ("nmblookup")) {
	  xf_dlg_error(smb_nav,"File not found","nmblookup");
	  exit(1);
  }
  if (!sane ("smbclient")) {
	  xf_dlg_error(smb_nav,"File not found","smbclient");
	  exit(1);
  }
  gtk_timeout_add (500, (GtkFunction) NMBmastersLookup, NULL);
  gtk_main ();
  cleanup_tmpfiles();
  return (0);
}


