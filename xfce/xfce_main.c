/*  gxfce
 *  Copyright (C) 1999 Olivier Fourdan <fourdan@xfce.org>
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

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#include "my_intl.h"
#include "xfce.h"
#include "setup.h"
#include "fileutil.h"
#include "modify.h"
#include "action.h"
#include "screen.h"
#include "info.h"
#include "popup.h"
#include "startup.h"
#include "xfce_main.h"
#include "xfce-common.h"
#include "xfce_cb.h"
#include "move.h"
#include "xfcolor.h"
#include "configfile.h"
#include "selects.h"
#include "xfwm.h"
#include "xpmext.h"
#include "gnome_protocol.h"
#include "session.h"
#include "diagnostic.h"
#include "my_tooltips.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern gint check_mail (gpointer data);

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

int
main (int argc, char *argv[])
{
  char gtkrc_file[MAXSTRLEN + 1];
  static int dg;
  char *homedir;
/* ??? patch 3.8.12c
  int ruid, euid, suid;

  getresgid(&ruid, &euid, &suid);
  fprintf(stderr, "aaaa: %d %d %d\n", ruid, euid, suid);
 ??? patch 3.8.12c */

  xfce_init (&argc, &argv);

  /* Build path to gtkrc file in user's home directory */
  if (!(homedir = getenv ("HOME")))
    {
      fprintf (stderr, "Can't fetch $HOME. Aborting\n");
      exit (-1);
    }
  snprintf (gtkrc_file, MAXSTRLEN, "%s/.gtkrc", (char *) homedir);

  signal (SIGCHLD, reap);

  /* The GC for the outline move */
  CreateDrawGC (GDK_ROOT_PARENT ());

  /* Create atoms for a GNOME complaint Window Manager */
  create_gnome_atoms ();

  OpenICEConn ();

  initconfig (&current_config);

  /* Initialisation of GtkStyle palettes */
  pal = newpal ();
  defpal (pal);
  loadpal (pal);

  current_config.wm = initwm (argc, argv);

  startup = create_startup ();
  open_startup (startup);
  update_events ();

  setup = create_setup (pal);
  modify = create_modify ();
  action = create_action ();
  screen = create_screen ();
  info_scr = create_info ();
  gxfce = create_gxfce (pal);

  create_all_popup_menus (gxfce);
  alloc_selects ();

  readconfig ();

  apply_xpalette (pal, current_config.apply_xcolors);
  update_gxfce_screen_buttons (current_config.visible_screen);
  gnome_set_desk_count (current_config.visible_screen ? current_config.visible_screen : 1);
  update_gxfce_popup_buttons (current_config.visible_popup);
  update_gxfce_size ();
  update_gxfce_coord (gxfce, &current_config.panel_x, &current_config.panel_y);
  update_delay_tooltips (current_config.tooltipsdelay);
  update_gxfce_clock ();
  gtk_widget_set_uposition (gxfce, current_config.panel_x, current_config.panel_y);

  gtk_widget_show (gxfce);
  gnome_sticky (gxfce->window);

  gnome_layer (gxfce->window, current_config.panel_layer);

  if (!current_config.wm)
    gnome_set_root_event (GTK_SIGNAL_FUNC (handle_gnome_workspace_event_cb));

  switch_to_screen (pal, 0);
  cursor_wait (gxfce);
  init_watchpipe ();

  reg_xfce_app (gxfce, pal);
  close_startup (startup);

  /* Update all running GTK+ apps if ~/.gtkrc is not present */

  if (!existfile (gtkrc_file))
  {
    create_gtkrc_file (pal, NULL);
    applypal_to_all ();
  }

  /* Apply user's preferences to the Window Manager */

  if (current_config.wm == XFWM)
  {
    apply_wm_colors (pal);
    apply_wm_fonts ();
    apply_wm_iconpos ();
    apply_wm_options ();
    apply_wm_snapsize ();
    apply_wm_engine ();
    startup_wm_modules ();
  }

  /* Initialize the pipe for subprocess diagnostics */
  dg = initdiag ();

  /* And display the diag box every time a message comes out */
  gdk_input_add (dg, GDK_INPUT_READ, (GdkInputFunction) process_diag_messages, (gpointer) ((long) dg));

  /* Add a hook for the mail-check function. */
  gtk_timeout_add (MAILCHECK_PERIOD, check_mail, NULL);

  cursor_reset (gxfce);

  /* Main event loop */
  gtk_main ();

  /* Cleanup */
  signal (SIGCHLD, SIG_DFL);
  free_selects ();
  free_tooltips_list ();
  free_internals_setup ();

  quit_wm ();
  FreeDrawGC ();

  /* Ciao ! */
  xfce_end ((gpointer) NULL, 0);
  return (0);
}
