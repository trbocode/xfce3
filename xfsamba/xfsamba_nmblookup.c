/* (c) 2001 Edscott Wilson Garcia GNU/GPL
*/

/* functions to use tubo.c resolve netbios name into IP */

/* nmb resolve: **/

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

#endif

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
NMBLookupForkOver (pid_t pid)
{
  char *message;
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;

  message = (char *) malloc (strlen (_("Resolved")) + strlen ((thisN->serverIP) ? thisN->serverIP : "?") + 5);
  sprintf (message, "%s : %s", _("Resolved"), (thisN->serverIP) ? thisN->serverIP : "?");
  print_status (message);
  free (message);
}

/* function to process stdout produced by child */
static int
NMBLookupParse (int n, void *data)
{
  char *line, *buffer;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);
  if (!strncmp (line, "querying", strlen ("querying")))
    return TRUE;
  if (!strstr (line, "<00>"))
    return TRUE;
  buffer = strtok (line, " ");
  if (thisN->serverIP)
    free (thisN->serverIP);
  thisN->serverIP = (char *) malloc (strlen (buffer) + 1);
  strcpy (thisN->serverIP, buffer);
  if (thisN->serverIP)
    gtk_label_set_text ((GtkLabel *) locationIP, thisN->serverIP);
  return TRUE;
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
NMBLookupFork (void)
{
  /* no problem with spaces in netbios name */
  fprintf (stderr, "DBG:nmblookup %s\n", NMBnetbios);
  execlp ("nmblookup", "nmblookup", NMBnetbios, (char *) 0);
}

void
NMBLookup (GtkWidget * widget, gpointer data)
{
  char *message;

  if (fork_obj)
    return;

  if ((!thisN) || (!thisN->server) || (!thisN->netbios))
    return;

  print_status ("entering nmblookup");
  stopcleanup = FALSE;


  message = (char *) malloc (strlen (_("Resolving for")) + strlen (thisN->server) + strlen (thisN->netbios) + 7);
  sprintf (message, "%s %s (%s)\n", _("Resolving for"), thisN->server, thisN->netbios);
  print_status (message);
  free (message);

  strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
  NMBnetbios[XFSAMBA_MAX_STRING] = 0;

  fork_obj = Tubo (NMBLookupFork, NMBLookupForkOver, FALSE, NMBLookupParse, parse_stderr);
  return;
}
