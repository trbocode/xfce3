/* (c) 2001 Edscott Wilson Garcia GNU/GPL
*  */

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

/* functions to use tubo.c for master browsere lookup */

/* nmb lookup: **/

/* function to be run by parent after child has exited
*  and all data in pipe has been read : */


static void
NMBmastersForkOver (pid_t pid)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  /* print_diagnostics("DBG:Master servers on subnet:\n"); */

  if (headN)
  {
    nmb_list *currentN;
    currentN = headN;
    while (currentN)
    {
      print_diagnostics ("Master browser at ");
      if (currentN->serverIP)
	print_diagnostics (currentN->serverIP);
      else
	print_diagnostics ("?");
      print_diagnostics ("\n");
      currentN = currentN->next;
    }

    if (headN->serverIP)
    {
      /*sleep(10); */
      NMBmastersResolve (headN);
    }
  }
  else
  {
    print_diagnostics ("Your subnet might be SMB free, or have a win95 master browser.\n");
    print_status (_("No master browser found. See diagnostics."));
    xf_dlg_warning (smb_nav,_("No master browser found.\nPlease type a computer name at location and hit RETURN"));
  }
}

/* function to process stdout produced by child */
static int
NMBmastersParseLookup (int n, void *data)
{
  char *line, *buffer;
  nmb_list *currentN;
  if (n)
    return TRUE;		/* this would mean binary data */
  line = (char *) data;
  print_diagnostics (line);	/* verbose diagnostics */
  if (!strncmp (line, "querying", strlen ("querying")))
    return TRUE;
  if (strstr (line, "name_query") && strstr (line, "failed") && strstr (line, "__MSBROWSE__"))
    return TRUE;
  if (strstr (line, "__MSBROWSE__") == NULL)
    return TRUE;

  buffer = strtok (line, " ");
  currentN = push_nmb (buffer);
  if (currentN->serverIP)
    gtk_label_set_text ((GtkLabel *) locationIP, currentN->serverIP);
  return TRUE;
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
NMBmastersFork (void)
{
#ifdef DBG_XFSAMBA
  fprintf (stderr, "DBG:%s  %s  %s\n", "nmblookup", "-M", "-");
  fflush (NULL);
#endif
  execlp ("nmblookup", "nmblookup", "-M", "-", (char *) 0);
}


gboolean
NMBmastersLookup (gpointer data)
{
  animation (FALSE);
  cursor_reset (GTK_WIDGET (smb_nav));
  stopcleanup = TRUE;


  /* animation may already be set in main... */


  if (fork_obj)
    return FALSE;
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  print_status (_("Looking for master browsers..."));
  /*fork_obj = */
  Tubo (NMBmastersFork, NMBmastersForkOver, FALSE, NMBmastersParseLookup, parse_stderr);
  return FALSE;
}
