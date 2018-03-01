/* (c) 2001 Edscott Wilson Garcia GNU/GPL
 */

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

static int tar_options;
static char *tarO = NULL;
static GtkWidget *dialog;

static void
SMBtarFork (void)
{
  int i = 0;
  char *argument[15], *the_netbios;
  the_netbios = (char *) malloc (strlen (NMBnetbios) + strlen (selected.share) + 1 + 3);
  sprintf (the_netbios, "//%s/%s", NMBnetbios, selected.share);


  argument[i++] = "smbclient";
  argument[i++] = the_netbios;
  argument[i++] = "-U";
  argument[i++] = NMBpassword;
  argument[i++] = "-D";
  argument[i++] = selected.dirname;

  argument[i++] = "-T";
  if (tar_options & 0xf00)
  {
    argument[i++] = "x";
  }
  else
  {
    switch (tar_options & 0xff)
    {
    case 0x11:
      argument[i++] = "cga";
      break;
    case 0x01:
      argument[i++] = "cg";
      break;
    case 0x10:
      argument[i++] = "ca";
      break;
    default:
      argument[i++] = "c";
      break;
    }
  }
  argument[i++] = tarO;
#ifdef DBG_XFSAMBA
  {
    int j;
    for (j = 0; j < i; j++)
      fprintf (stderr, "%s ", argument[j]);
    fprintf (stderr, "\n");
    fflush (NULL);
  }
#endif
  argument[i++] = (char *) 0;


  execvp (argument[0], argument);
}



/* function to process stdout produced by child */
static int
SMBtarStdout (int n, void *data)
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

static void
SMBtarForkOver (pid_t pid)
{
  cursor_reset (GTK_WIDGET (smb_nav));
  animation (FALSE);
  fork_obj = NULL;
  switch (SMBResult)
  {
  case CHALLENGED:
    print_status (_("Tar failed. See diagnostics for details."));
    break;
  default:
    if (tar_options & 0xf00)
    {
      xf_dlg_warning (smb_nav,_("Tar extraction has completed"));
      SMBrefresh (thisN->netbios, FORCERELOAD);
    }
    else
    {
      print_status (_("Tar command complete."));
    }
    break;
  }
}

static void
proceed_tar (GtkWidget * widget, gpointer data)
{
  char *fileS;
  int i, ok;
  ok = (int) ((long) data);
  if (ok)
  {
    strncpy (NMBnetbios, thisN->netbios, XFSAMBA_MAX_STRING);
    NMBnetbios[XFSAMBA_MAX_STRING] = 0;

    strncpy (NMBpassword, thisN->password, XFSAMBA_MAX_STRING);
    NMBpassword[XFSAMBA_MAX_STRING] = 0;

    if (tar_options & 0xf00)
    {
      xf_dlg_warning (smb_nav,_("Please select input tarfile next."));
    }
    else
    {
      xf_dlg_warning (smb_nav,_("Please select the output tarfile next."));
    }

    if (tarO)
      free (tarO);

    tarO = (char *) malloc (strlen (thisN->netbios) + strlen (selected.share) + strlen (selected.dirname) + 3 + strlen (".tar"));

    if (tar_options & 0xf00)
    {
      strcpy (tarO, "*.tar");
    }
    else
    {
      if (strcmp (selected.dirname, "/"))
      {
	sprintf (tarO, "//%s/%s%s.tar", thisN->netbios, selected.share, selected.dirname);
      }
      else
      {
	sprintf (tarO, "//%s/%s.tar", thisN->netbios, selected.share);
      }
      for (i = 0; i < strlen (tarO); i++)
      {
	if (tarO[i] == '/')
	{
	  tarO[i] = '\\';
	}
      }
    }
  error_3361:
    fileS = open_fileselect (tarO);
    if (!fileS)
    {
      print_status (_("Tar cancelled."));
      animation (FALSE);
      cursor_reset (GTK_WIDGET (smb_nav));
      gtk_widget_destroy (dialog);
      return;
    }
    if (tar_options & 0xf00)
    {
      FILE *test;
      test = fopen (fileS, "rb");
      if (!test)
      {
	xf_dlg_warning (smb_nav,_("Cannot open tar file"));
	goto error_3361;
      }
      fclose (test);
    }
    else
    {
      FILE *test;
      test = fopen (fileS, "wb");
      if (!test)
      {
	xf_dlg_warning (smb_nav,_("Cannot create tar file"));
	goto error_3361;
      }
      fclose (test);
    }


    free (tarO);
    tarO = (char *) malloc (strlen (fileS) + 1);
    sprintf (tarO, "%s", fileS);


    fork_obj = Tubo (SMBtarFork, SMBtarForkOver, TRUE, SMBtarStdout, parse_stderr);
  }
  else
  {
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    print_status (_("Tar cancelled."));
  }
  gtk_widget_destroy (dialog);
}

static void
toggle_tar_options (GtkWidget * widget, gpointer data)
{
  int choice;
  choice = (int) ((long) data);
  switch (choice)
  {
  case 1:
    tar_options |= 0x01;
    break;
  case 0:
    tar_options &= 0xff0;
    break;
  case 3:
    tar_options |= 0x10;
    break;
  case 2:
    tar_options &= 0xf0f;
    break;
  case 5:
    tar_options |= 0x100;
    break;
  case 4:
    tar_options &= 0x0ff;
    break;
  }
}

static GtkWidget *
SMBtarDialog (void)
{
  GtkWidget *button, *hbox, *vbox, *label, *notebook, *scrolled;
  char *pathname;
  tar_options = 0x0;
  pathname = (char *) malloc (2 + strlen (thisN->server) + 1 + strlen (selected.share) + strlen (selected.dirname) + 1 + ((!selected.file) ? 0 : strlen (selected.filename)) + 1);

  sprintf (pathname, "//%s/%s", thisN->server, selected.share);
  if (strcmp (selected.dirname, "/") != 0)
  {
    strcat (pathname, selected.dirname);
  }
  if (selected.file)
  {
    strcat (pathname, "/");
    strcat (pathname, selected.filename);
  }

  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_widget_realize (dialog);

  notebook = gtk_notebook_new ();
  {
    gtk_container_border_width (GTK_CONTAINER (notebook), 3);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook, EXPAND, FILL, 0);
    gtk_widget_show (notebook);

    /* tar page */
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    {
      gtk_container_border_width (GTK_CONTAINER (scrolled), 3);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (notebook), scrolled);
      gtk_widget_show (scrolled);

      vbox = gtk_vbox_new (FALSE, 0);
      {
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), vbox);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled)));
	gtk_widget_show (vbox);
	label = gtk_label_new (pathname);
	gtk_box_pack_start (GTK_BOX (vbox), label, NOEXPAND, NOFILL, 0);
	gtk_widget_show (label);
	hbox = gtk_hbox_new (FALSE, 0);
	{
	  int i;
	  GSList *group = NULL;
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (hbox);
	  for (i = 4; i < 6; i++)
	  {
	    switch (i)
	    {
	    case 4:
	      button = gtk_radio_button_new_with_label (group, _("create"));
	      break;
	    default:
	      button = gtk_radio_button_new_with_label (group, _("extract"));
	      if (tar_options & 0x01)
	      {
	      }
	      break;
	    }
	    group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	    gtk_box_pack_start (GTK_BOX (hbox), button, NOEXPAND, NOFILL, 0);
	    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_tar_options), (gpointer) ((long) i));

	    gtk_widget_show (button);
	  }
	}
      }
    }
    label = gtk_label_new ("Tar");
    gtk_notebook_set_tab_label ((GtkNotebook *) notebook, scrolled, label);

    /* options page */
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    {
      int i;

      gtk_container_border_width (GTK_CONTAINER (scrolled), 3);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (notebook), scrolled);
      gtk_widget_show (scrolled);

      vbox = gtk_vbox_new (FALSE, 0);
      {
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), vbox);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled)));
	gtk_widget_show (vbox);
	hbox = gtk_hbox_new (FALSE, 0);
	{
	  GSList *group = NULL;
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (hbox);
	  for (i = 0; i < 2; i++)
	  {
	    switch (i)
	    {
	    case 0:
	      button = gtk_radio_button_new_with_label (group, _("full"));
	      break;
	    default:
	      button = gtk_radio_button_new_with_label (group, _("incremental"));
	      if (tar_options & 0x01)
	      {
	      }
	      break;
	    }
	    group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	    gtk_box_pack_start (GTK_BOX (hbox), button, NOEXPAND, NOFILL, 0);
	    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_tar_options), (gpointer) ((long) i));

	    gtk_widget_show (button);
	  }
	}
	hbox = gtk_hbox_new (FALSE, 0);
	{
	  GSList *group = NULL;
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (hbox);
	  for (i = 2; i < 4; i++)
	  {
	    switch (i)
	    {
	    case 2:
	      button = gtk_radio_button_new_with_label (group, _("do not reset archive bit"));
	      break;
	    default:
	      button = gtk_radio_button_new_with_label (group, _("reset archive bit"));
	      break;
	    }
	    group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	    gtk_box_pack_start (GTK_BOX (hbox), button, NOEXPAND, NOFILL, 0);
	    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_tar_options), (gpointer) ((long) i));
	    gtk_widget_show (button);
	  }
	}

      }
    }
    label = gtk_label_new (_("Options"));
    gtk_notebook_set_tab_label ((GtkNotebook *) notebook, scrolled, label);


  }				/* end notebook */



  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_tar), (gpointer) ((long) 1));

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (proceed_tar), (gpointer) ((long) 0));
  gtk_widget_show (dialog);

  free (pathname);

  return dialog;
}

void
SMBtar (void)
{
  cursor_wait (GTK_WIDGET (smb_nav));
  animation (TRUE);
  print_status (_("Tar..."));
  stopcleanup = FALSE;
  gtk_window_set_transient_for (GTK_WINDOW (SMBtarDialog ()), GTK_WINDOW (smb_nav));
}
