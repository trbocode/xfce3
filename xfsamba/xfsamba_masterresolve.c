/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

/* functions to use tubo.c for resolving master
*  browser IP address into a netbios name  */

#ifndef INCLUDED_BY_XFSAMBA_C
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
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

static GtkWidget *master;
static int NMBfirst = 1;
static GtkWidget *dialog;
static GList *items = NULL;


/* function to be run by parent after child has exited
*  and all data in pipe has been read : */

static void
destroy_dialog_master (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
  gtk_widget_destroy (dialog);
}

static void
ok_dialog_master (GtkWidget * widget, gpointer data)
{
  FILE *archie;
  char *home, line[MAXSTRLEN];
  char *s;
  s = gtk_entry_get_text (GTK_ENTRY (master));
  thisN->netbios = (char *) malloc (strlen (s) + 1);
  strcpy (thisN->netbios, s);
  latin_1_unreadable (thisN->netbios);
  gtk_main_quit ();
  gtk_widget_destroy (dialog);

  home = getenv ("HOME");
  snprintf (line, MAXSTRLEN, "%s/.xfce/lmhosts.xfsamba", home);
  archie = fopen (line, "a");
  if (!archie)
    return;
  fprintf (archie, "%s:%s\n", thisN->serverIP, thisN->netbios);
  fclose (archie);

  return;
}


static void
entry_keypress_master (GtkWidget * entry, GdkEventKey * event, gpointer data)
{
  if (event->keyval == GDK_Return)
  {
    ok_dialog_master (NULL, NULL);
  }
  return;

}

static GtkWidget *
master_dialog (char *ip)
{
  GtkWidget *button, *hbox, *label;

  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_widget_realize (dialog);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);
  label = gtk_label_new (_("Please provide netbios name for master browser at "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  label = gtk_label_new (ip);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  master = gtk_entry_new ();

  gtk_box_pack_start (GTK_BOX (hbox), master, EXPAND, NOFILL, 0);
  gtk_signal_connect (GTK_OBJECT (master), "key-press-event", GTK_SIGNAL_FUNC (entry_keypress_master), NULL);
  gtk_widget_show (master);


  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ok_dialog_master), (gpointer) dialog);
  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (destroy_dialog_master), (gpointer) dialog);
  gtk_widget_show (dialog);
  gtk_widget_grab_focus (master);
  return dialog;
}

static void
ask4master (void)
{
  FILE *archie;
  char *home, line[MAXSTRLEN];
  home = getenv ("HOME");
  snprintf (line, MAXSTRLEN, "%s/.xfce/lmhosts.xfsamba", home);
  archie = fopen (line, "r");
  if (archie)
  {
    while (!feof (archie) && fgets (line, 255, archie))
    {
      char *ip, *netbios;
      if (feof (archie))
	break;
      line[255] = 0;
      if (strstr (line, "#"))
	continue;
      if (!strstr (line, ":"))
	continue;
      ip = strtok (line, ":");
      netbios = strtok (NULL, "\n");
      if (strstr (thisN->serverIP, ip))
      {
	thisN->netbios = (char *) malloc (strlen (netbios) + 1);
	strcpy (thisN->netbios, netbios);
	latin_1_unreadable (thisN->netbios);
	fclose (archie);
	/* here we could go to the popup with the name, to confirm, 
	   * instead of proceeding blind */
	return;
      }
    }
    fclose (archie);
  }

  print_status (_("Master browser netbios name must be provided."));
  gtk_window_set_transient_for (GTK_WINDOW (master_dialog (thisN->serverIP)), GTK_WINDOW (smb_nav));
  gtk_main ();
  return;
}


static void
NMBmastersResolveOver (pid_t pid)
{
  gboolean NMBmastersResolve (nmb_list * currentN);
  char *message;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  if (!thisN)
    return;
again_master:
  if (thisN->netbios)
  {
    thisN->server = (char *) malloc (strlen (thisN->netbios) + 1);
    strcpy (thisN->server, thisN->netbios);
    latin_1_readable (thisN->server);
    if (items != NULL)
    {
      g_list_free (items);
      items = NULL;
    }
    items = g_list_append (items, thisN->server);

    gtk_combo_set_popdown_strings (GTK_COMBO (location), items);
    smoke_nmb (thisN);
    reverse_smoke_nmb (thisN);
    headN = thisN;
    message = (char *) malloc (strlen (_("Resolved")) + strlen (thisN->netbios) + 5);
    sprintf (message, "%s : %s", _("Resolved"), thisN->netbios);
    print_status (message);
    free (message);
    SMBLookup (NULL, TRUE);
    return;
  }
  /* master browser not resolved  */
  if (thisN->serverIP)
  {
    message = (char *) malloc (strlen (_("No status response")) + strlen (thisN->serverIP) + 5);
    sprintf (message, "%s : %s", _("No status response"), thisN->serverIP);
    print_status (message);
    free (message);
  }
  else
  {
    print_status (_("No status response"));
  }

  if (thisN->next)
    thisN = thisN->next;
  else				/* alert no master browser resolved on network */
  {
    ask4master ();

    if (thisN->netbios)
      goto again_master;
    {
      print_status (_("Master browser not resolved. Use location box to browse."));
      xf_dlg_warning (smb_nav,_("No master browser resolved.\nPlease type a computer name at location and hit RETURN"));
      clean_nmb ();
    }
    return;
  }
  NMBmastersResolve (thisN);
}

/* function to process stdout produced by child */
static int
NMBparseMastersResolve (int n, void *data)
{
  char *line;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  if (!NMBfirst)
    return TRUE;
  if (!strstr (line, "<00>"))
    return TRUE;
  NMBfirst = 0;
  strtok (line, " ");
  thisN->netbios = (char *) malloc (strlen (line) + 1);
  /* chupo faros check */
  strcpy (thisN->netbios, line + 1);
  return TRUE;
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
NMBmastersResolveFork (void)
{
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG: nmblookup -A %s\n", NMBserverIP);
  fflush (NULL);
#endif
  execlp ("nmblookup", "nmblookup", "-A", NMBserverIP, (char *) 0);
}

gboolean
NMBmastersResolve (nmb_list * currentN)
{
  char *message;
  if (!currentN)
    return FALSE;
  if (fork_obj)
    return FALSE;
  stopcleanup = TRUE;
  NMBfirst = 1;
  thisN = currentN;
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  message = (char *) malloc (strlen (currentN->serverIP) + strlen (_("Resolving")) + 2);
  sprintf (message, "%s %s", _("Resolving"), currentN->serverIP);
  print_status (message);
  free (message);

  strncpy (NMBserverIP, currentN->serverIP, XFSAMBA_MAX_STRING);
  NMBserverIP[XFSAMBA_MAX_STRING] = 0;
  {
    FILE *archie;
    char *home, line[MAXSTRLEN];
    home = getenv ("HOME");
    snprintf (line, MAXSTRLEN, "%s/.xfce/lmhosts.xfsamba", home);
    archie = fopen (line, "r");
    if (archie)
      fclose (archie);
    else
    {
      archie = fopen (line, "w");
      if (archie)
      {
	fprintf (archie, "#File created by xfsamba. Do not edit.\n#serverIP:win95_netbios\n");
	fclose (archie);
      }
    }
  }

  /* fork_obj = */
  Tubo (NMBmastersResolveFork, NMBmastersResolveOver, FALSE, NMBparseMastersResolve, parse_stderr);
  return FALSE;
}
