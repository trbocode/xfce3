/*  xfsound
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_GDK_IMLIB
#include <fcntl.h>
#include <gdk_imlib.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "my_intl.h"
#include "xfsound.h"
#include "xfsound_cb.h"
#include "xfdsp.h"
#include "utils.h"
#include "module.h"
#include "constant.h"
#include "fileselect.h"
#include "my_string.h"
#include "xfce-common.h"
#include "xfcolor.h"
#include "fileutil.h"
#include "sendinfo.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfsound_icon.xpm"

#define RCFILE                  "xfsoundrc"

T_messages messages[KNOWN_MESSAGES + KNOWN_BUILTIN] = {
  {XFCE_M_NEW_DESK, N_("New Desk")},
  {XFCE_M_ADD_WINDOW, N_("Add Window")},
  {XFCE_M_RAISE_WINDOW, N_("Raise Window")},
  {XFCE_M_LOWER_WINDOW, N_("Lower Window")},
  {XFCE_M_CONFIGURE_WINDOW, N_("Configure Window")},
  {XFCE_M_FOCUS_CHANGE, N_("Focus Change")},
  {XFCE_M_DESTROY_WINDOW, N_("Destroy Window")},
  {XFCE_M_ICONIFY, N_("Iconify")},
  {XFCE_M_DEICONIFY, N_("De-iconify")},
  {XFCE_M_SHADE, N_("Shade")},
  {XFCE_M_UNSHADE, N_("Un-Shade")},
  {XFCE_M_MAXIMIZE, N_("Maximize")},
  {XFCE_M_DEMAXIMIZE, N_("De-maximize")},
  {0, N_("Startup")},
  {0, N_("Shutdown")},
  {0, N_("Unknown")}
};

static gint private_menu_selected = -1;
static gint prev = -1;

void
reap (int sig)
{
  signal (SIGCHLD, SIG_DFL);
#if HAVE_WAITPID
  while (waitpid (-1, NULL, WNOHANG) > 0);
#elif HAVE_WAIT3
  while (wait3 (NULL, WNOHANG, NULL) > 0);
#else
# error One of waitpid or wait3 is needed.
#endif
  signal (SIGCHLD, reap);
}

void
display_error (char *s)
{
  fprintf (stderr, "%s\n", s);
  my_alert (s);
}

void
allocXFSound (XFSound * s)
{
  int i;
  s->playcmd = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
  for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
    s->datafiles[i] = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
}

void
freeXFSound (XFSound * s)
{
  int i;
  free (s->playcmd);
  for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
    free (s->datafiles[i]);
}

void
readstr (int i, char *str, FILE * f)
{
  if (str)
  {
    if ((f) && (fgets (str, i - 1, f)) && (strlen (str)))
    {
      str[strlen (str) - 1] = '\0';
    }
    else
    {
      strcpy (str, "\0");
    }
  }
}

void
loadcfg (XFSound * s)
{
  static time_t last_ctime = (time_t) 0;
  struct stat buf;
  int i;
  FILE *f = NULL;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);
  if (existfile (homedir))
  {
    if (stat (homedir, &buf))
    {
      if (buf.st_ctime > last_ctime)
      {
        last_ctime = buf.st_ctime;
        return;
      }
    }
    f = fopen (homedir, "r");
  }
  else
  {
    snprintf (homedir, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, RCFILE);
    if (existfile (homedir))
    {
      if (stat (homedir, &buf))
      {
        if (buf.st_ctime > last_ctime)
        {
          last_ctime = buf.st_ctime;
          return;
        }
      }
      f = fopen (homedir, "r");
    }
  }

  if (f)
  {
    readstr (15, tempstr, f);
    s->playsnd = (my_strncasecmp (tempstr, "Play", strlen ("Play")) == 0);
    readstr (MAXSTRLEN, s->playcmd, f);
    for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
      readstr (MAXSTRLEN, s->datafiles[i], f);
    fclose (f);
  }
  else
  {
    s->playsnd = 0;
    strcpy (s->playcmd, "\0");
    for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
      strcpy (s->datafiles[i], "\0");
  }
}

int
savecfg (XFSound * s)
{
  int i;
  FILE *f;

  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), RCFILE);

  if ((f = fopen (homedir, "w")))
  {
    fprintf (f, "%s\n", ((s->playsnd) ? "Play" : "NoPlay"));
    fprintf (f, "%s\n", s->playcmd);
    for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
      fprintf (f, "%s\n", s->datafiles[i]);
    fclose (f);
  }
  return ((f != NULL));
}

int
value_table (long id)
{
  int i;
  for (i = 0; i < KNOWN_MESSAGES; i++)
    if (messages[i].message_id == id)
      return i;
  return -1;
}

void
audio_play (short code, short backgnd)
{
  if ((sndcfg.playsnd) && strlen (sndcfg.playcmd) && strlen (sndcfg.datafiles[code]))
  {
    pid_t pidchild;

    /* 
       This is required if the process should not be spawned "in background"
       because, in this case,  we want to catch the SIGCHLD signal with waitpid.
     */
    signal (SIGCHLD, SIG_DFL);
    switch (pidchild = fork ())
    {
      case 0:
      {
	if (my_strncasecmp (sndcfg.playcmd, INTERNAL_PLAYER, strlen (INTERNAL_PLAYER)) == 0)
	{
	  i_play (sndcfg.datafiles[code]);
          _exit (0);
	}
	else
	{
	  int l1, l2, l3;
          int nulldev;
          char *command;

	  l1 = strlen (sndcfg.playcmd);
	  l2 = strlen (sndcfg.datafiles[code]);
	  l3 = l1 + l2 + 8;
	  command = (char *) malloc (l3 * sizeof (char));

	  strcpy (command, sndcfg.playcmd);
	  strcat (command, " ");
	  strcat (command, sndcfg.datafiles[code]);
	  if (my_strncasecmp (sndcfg.playcmd, "exec ", strlen ("exec ")))
	  {
	    snprintf (command, l3, "exec %s %s", sndcfg.playcmd, sndcfg.datafiles[code]);
	  }
	  else
	  {
	    snprintf (command, l3, "%s %s", sndcfg.playcmd, sndcfg.datafiles[code]);
	  }
	  /* The following is to avoid X locking when executing 
	     terminal based application that requires user input */
	  if ((nulldev = open ("/dev/null", O_RDWR)))
	  {
	    close (0);
	    dup (nulldev);
	  }
	  execl (DEFAULT_SHELL, DEFAULT_SHELL, "-c", command, NULL);
	  perror ("exec failed");
	  _exit (0);
	  break;
      }
      case -1:
	fprintf (stderr, "xfsound : cannot execute fork()\n");
	break;
      default:
	if (!backgnd)
	{
	  int status;
#if HAVE_WAITPID
          waitpid (pidchild, &status, 0);
#elif HAVE_WAIT3
          wait4 (pidchild, &status, 0, NULL);
#else
# error One of waitpid or wait3 is needed.
#endif
	}
	break;
      }
    }
    /* 
      Once it's over, we can put back reap in place so we will remove the zombies
     */
    signal (SIGCHLD, reap);
  }
}

void
done (int n)
{
  /* Cancel all signals handling to prevent the sound to be played twice ! */
  signal (SIGPIPE, SIG_IGN);
  signal (SIGINT, SIG_IGN);
  signal (SIGKILL, SIG_IGN);
  signal (SIGTERM, SIG_IGN);
  signal (SIGQUIT, SIG_IGN);
  signal (SIGSTOP, SIG_IGN);
  signal (SIGHUP, SIG_IGN);

  audio_play (BUILTIN_SHUTDOWN, 0);
  exit (n);
}

void
DeadPipe (int nonsense)
{
  done (nonsense);
}

void
Loop (int *fd)
{
  unsigned long header[HEADER_SIZE];
  unsigned long *body = NULL;
  unsigned long count;
  long code;

  while (1)
  {
    if ((count = ReadXfwmPacket (fd[1], header, &body)) <= 0)
    {
      free (body);
      done (0);
    }
    free (body);
    code = value_table (header[1]);

    loadcfg (&sndcfg);

    if (code >= 0 && code < MAX_MESSAGES)
      audio_play (code, 1);
    else if (code >= MAX_MESSAGES)
      audio_play (BUILTIN_UNKNOWN, 1);
  }
}

void
startmodule (int argc, char *argv[])
{
  /* Intercept interupt signals to play "logout" sound */
  signal (SIGPIPE, DeadPipe);
  signal (SIGINT, DeadPipe);
  signal (SIGKILL, DeadPipe);
  signal (SIGTERM, DeadPipe);
  signal (SIGQUIT, DeadPipe);
  signal (SIGSTOP, DeadPipe);
  signal (SIGHUP, DeadPipe);

  /* Initiate pipe */
  fd[0] = atoi (argv[1]);
  fd[1] = atoi (argv[2]);

  audio_play (BUILTIN_STARTUP, 0);
  sendinfo (fd, "Nop", 0);
  Loop (fd);
}

char *
get_all_data (int i)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (play_sound_checkbutton)))
    sndcfg.playsnd = 1;
  else
    sndcfg.playsnd = 0;
  strcpy (sndcfg.playcmd, cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (command_entry))));
  strcpy (sndcfg.datafiles[i], cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (soundfile_entry))));
  return (sndcfg.datafiles[i]);
}

void
update_all_data (void)
{
  int i;

  if (prev >= 0)
    strcpy (sndcfg.datafiles[prev], cleanup ((char *) gtk_entry_get_text (GTK_ENTRY (soundfile_entry))));
  i = list_get_choice_selected ();
  gtk_entry_set_text (GTK_ENTRY (command_entry), sndcfg.playcmd);
  gtk_entry_set_text (GTK_ENTRY (soundfile_entry), sndcfg.datafiles[i]);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (play_sound_checkbutton), (sndcfg.playsnd != 0));
  prev = i;
}

GtkWidget *
create_xfsound ()
{
  GtkWidget *xfsound_mainframe;
  GtkWidget *vbox1;
  GtkWidget *frame2;
  GtkWidget *vbox2;
  GtkWidget *frame4;
  GtkWidget *table1;
  GtkWidget *label1;
  GtkWidget *label3;
  GtkWidget *test_button;
  GtkWidget *browse_button;
  GtkWidget *frame5;
  GtkWidget *hbox1;
  GtkWidget *command_label;
  GtkWidget *vbuttonbox1;
  GtkWidget *internal_player_button;
  GtkWidget *external_player_button;
  GtkWidget *frame3;
  GtkWidget *hbuttonbox2;
  GtkWidget *ok_button;
  GtkWidget *apply_button;
  GtkWidget *cancel_button;
  GtkAccelGroup *accel_group;

  xfsound = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (xfsound, "xfsound");
  gtk_object_set_data (GTK_OBJECT (xfsound), "xfsound", xfsound);
  gtk_widget_set_usize (xfsound, 0, 0);
  gtk_window_set_title (GTK_WINDOW (xfsound), _("XFSound - XFce Sound Manager"));
  gtk_window_position (GTK_WINDOW (xfsound), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (xfsound), TRUE, TRUE, FALSE);

  xfsound_mainframe = gtk_frame_new (NULL);
  gtk_widget_set_name (xfsound_mainframe, "xfsound_mainframe");
  gtk_object_set_data (GTK_OBJECT (xfsound), "xfsound_mainframe", xfsound_mainframe);
  gtk_widget_show (xfsound_mainframe);
  gtk_container_add (GTK_CONTAINER (xfsound), xfsound_mainframe);
#ifdef OLD_STYLE
  gtk_frame_set_shadow_type (GTK_FRAME (xfsound_mainframe), GTK_SHADOW_NONE);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (xfsound_mainframe), GTK_SHADOW_OUT);
#endif

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (xfsound), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (xfsound_mainframe), vbox1);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame2, "frame2");
  gtk_object_set_data (GTK_OBJECT (xfsound), "frame2", frame2);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox1), frame2, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame2), 5);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (xfsound), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame2), vbox2);

  frame4 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame4, "frame4");
  gtk_object_set_data (GTK_OBJECT (xfsound), "frame4", frame4);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (vbox2), frame4, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame4), 5);

  table1 = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (table1, "table1");
  gtk_object_set_data (GTK_OBJECT (xfsound), "table1", table1);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame4), table1);

  label1 = gtk_label_new (_("Event : "));
  gtk_widget_set_name (label1, "label1");
  gtk_object_set_data (GTK_OBJECT (xfsound), "label1", label1);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 1, 0.5);

  label3 = gtk_label_new (_("Sound file : "));
  gtk_widget_set_name (label3, "label3");
  gtk_object_set_data (GTK_OBJECT (xfsound), "label3", label3);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);

  test_button = gtk_button_new_with_label (_("Test"));
  gtk_widget_set_name (test_button, "test_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "test_button", test_button);
  gtk_widget_show (test_button);
  gtk_table_attach (GTK_TABLE (table1), test_button, 2, 3, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);

  browse_button = gtk_button_new_with_label (_("Browse ..."));
  gtk_widget_set_name (browse_button, "browse_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "browse_button", browse_button);
  gtk_widget_show (browse_button);
  gtk_table_attach (GTK_TABLE (table1), browse_button, 2, 3, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);

  soundfile_entry = gtk_entry_new ();
  gtk_widget_set_name (soundfile_entry, "soundfile_entry");
  gtk_object_set_data (GTK_OBJECT (xfsound), "soundfile_entry", soundfile_entry);
  /* gtk_widget_set_style(soundfile_entry, pal->cm[4]); */
  gtk_widget_show (soundfile_entry);
  gtk_table_attach (GTK_TABLE (table1), soundfile_entry, 1, 2, 1, 2, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 5, 5);

  list_event_optionmenu = gtk_option_menu_new ();
  gtk_widget_set_name (list_event_optionmenu, "list_event_optionmenu");
  gtk_object_set_data (GTK_OBJECT (xfsound), "list_event_optionmenu", list_event_optionmenu);
  gtk_widget_show (list_event_optionmenu);
  gtk_table_attach (GTK_TABLE (table1), list_event_optionmenu, 1, 2, 0, 1, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) 0, 5, 5);
  list_event_optionmenu_menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (list_event_optionmenu), list_event_optionmenu_menu);

  frame5 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame5, "frame5");
  gtk_object_set_data (GTK_OBJECT (xfsound), "frame5", frame5);
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox2), frame5, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame5), 5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_object_set_data (GTK_OBJECT (xfsound), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_container_add (GTK_CONTAINER (frame5), hbox1);

  play_sound_checkbutton = gtk_check_button_new_with_label (_("Play Sound"));
  gtk_widget_set_name (play_sound_checkbutton, "play_sound_checkbutton");
  gtk_object_set_data (GTK_OBJECT (xfsound), "play_sound_checkbutton", play_sound_checkbutton);
  gtk_widget_show (play_sound_checkbutton);
  gtk_box_pack_start (GTK_BOX (hbox1), play_sound_checkbutton, TRUE, TRUE, 5);
  gtk_container_border_width (GTK_CONTAINER (play_sound_checkbutton), 5);
  gtk_widget_grab_focus (play_sound_checkbutton);

  command_label = gtk_label_new (_("Command : "));
  gtk_widget_set_name (command_label, "command_label");
  gtk_object_set_data (GTK_OBJECT (xfsound), "command_label", command_label);
  gtk_widget_show (command_label);
  gtk_box_pack_start (GTK_BOX (hbox1), command_label, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (command_label), 1, 0.5);

  command_entry = gtk_entry_new ();
  gtk_widget_set_name (command_entry, "command_entry");
  gtk_object_set_data (GTK_OBJECT (xfsound), "command_entry", command_entry);
  /* gtk_widget_set_style(command_entry, pal->cm[4]); */
  gtk_widget_show (command_entry);
  gtk_box_pack_start (GTK_BOX (hbox1), command_entry, FALSE, FALSE, 5);

  vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_widget_set_name (vbuttonbox1, "vbuttonbox1");
  gtk_object_set_data (GTK_OBJECT (xfsound), "vbuttonbox1", vbuttonbox1);
  gtk_widget_show (vbuttonbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbuttonbox1, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (vbuttonbox1), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (vbuttonbox1), 0, 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (vbuttonbox1), 0, 0);

  internal_player_button = gtk_button_new_with_label (_("Internal"));
  gtk_widget_set_name (internal_player_button, "internal_player_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "internal_player_button", internal_player_button);
  gtk_widget_show (internal_player_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), internal_player_button);

  external_player_button = gtk_button_new_with_label (_("External"));
  gtk_widget_set_name (external_player_button, "external_player_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "external_player_button", external_player_button);
  gtk_widget_show (external_player_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), external_player_button);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame3, "frame3");
  gtk_object_set_data (GTK_OBJECT (xfsound), "frame3", frame3);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox1), frame3, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame3), 5);

  hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox2, "hbuttonbox2");
  gtk_object_set_data (GTK_OBJECT (xfsound), "hbuttonbox2", hbuttonbox2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox2), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox2), 0, 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox2), 0, 0);
  gtk_container_border_width (GTK_CONTAINER (hbuttonbox2), 5);
  gtk_widget_show (hbuttonbox2);
  gtk_container_add (GTK_CONTAINER (frame3), hbuttonbox2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_SPREAD);

  ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_set_name (ok_button, "ok_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (xfsound), accel_group);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_set_name (apply_button, "apply_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "apply_button", apply_button);
  gtk_widget_show (apply_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), apply_button);
  GTK_WIDGET_SET_FLAGS (apply_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (apply_button, "clicked", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_name (cancel_button, "cancel_button");
  gtk_object_set_data (GTK_OBJECT (xfsound), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (cancel_button, "clicked", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (ok_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (cancel_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (apply_button), "clicked", GTK_SIGNAL_FUNC (apply_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (play_sound_checkbutton), "clicked", GTK_SIGNAL_FUNC (doplay_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (external_player_button), "clicked", GTK_SIGNAL_FUNC (defaultcmd_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (internal_player_button), "clicked", GTK_SIGNAL_FUNC (internal_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (browse_button), "clicked", GTK_SIGNAL_FUNC (browsefile_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (test_button), "clicked", GTK_SIGNAL_FUNC (testfile_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (xfsound), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

  return xfsound;
}

void
private_update_menuentry_cb (GtkWidget * widget, gpointer data)
{
  private_menu_selected = (gint) ((long) data);
  update_all_data ();
}

GtkWidget *
list_addto_choice (char *name)
{
  static int position = 0;
  GtkWidget *menuitem = NULL;

  if (name && strlen (name))
  {
    menuitem = gtk_menu_item_new_with_label (name);
    gtk_menu_append (GTK_MENU (list_event_optionmenu_menu), menuitem);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (private_update_menuentry_cb), (gpointer) ((long) position++));
    gtk_widget_show (menuitem);
    gtk_widget_realize (menuitem);
  }
  return menuitem;
}

int
list_get_choice_selected (void)
{
  return private_menu_selected;
}

void
list_set_choice_selected (int index)
{
  gtk_option_menu_set_history (GTK_OPTION_MENU (list_event_optionmenu), index);
  private_menu_selected = index;
}

void
init_choice_str (void)
{
  int i;

  for (i = 0; i < (KNOWN_MESSAGES + KNOWN_BUILTIN); i++)
    list_addto_choice (_(messages[i].label));
}

int
main (int argc, char *argv[])
{
  signal_setup ();

  homedir = (char *) malloc ((MAXSTRLEN + 1) * sizeof (char));
  tempstr = (char *) malloc (16 * sizeof (char));
  allocXFSound (&sndcfg);
  loadcfg (&sndcfg);
  signal (SIGCHLD, reap);
  sound_init ();

  if ((argc != 6) && (argc != 7))
  {
    xfce_init (&argc, &argv);

    xfsound = create_xfsound ();
    init_choice_str ();
    list_set_choice_selected (0);
    gtk_entry_set_text (GTK_ENTRY (command_entry), sndcfg.playcmd);
    gtk_entry_set_text (GTK_ENTRY (soundfile_entry), sndcfg.datafiles[0]);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (play_sound_checkbutton), (sndcfg.playsnd != 0));
    gtk_widget_show (xfsound);
    set_icon (xfsound, "XFSound", xfsound_icon_xpm);
    gtk_main ();
    xfce_end ((gpointer) NULL, 0);
  }
  else
    startmodule (argc, argv);
  return (0);
}
