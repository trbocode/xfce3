/*  gxfce
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <X11/Xlib.h>
#include "constant.h"
#include "configfile.h"
#include "my_string.h"
#include "xfce-common.h"
#include "xfce_main.h"
#include "selects.h"
#include "popup.h"
#include "xfce.h"
#include "xfwm.h"
#include "fileutil.h"
#include "my_intl.h"

#ifdef HAVE_LIBXML2
#  include "configtree.h"
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef XFCE_TASKBAR
  #include "taskbar.h"
#endif

char *rcfile = "xfce3rc";
int nl = 0;
int buffersize = 127;

#define my_strnSTARTS(__s,__m) \
   (!  g_strncasecmp ((__s),(__m),strlen(__m)))

void
syntax_error (char *s)
{
  fprintf (stderr, _("XFce : Syntax error in configuration file\n(%s)\n"), s);
  my_alert (_("Syntax error in configuration file\nAborting"));
  end_XFCE (2);
}

void
data_error (char *s)
{
  fprintf (stderr, _("XFce : Data mismatch error in config file\n(%s)\n"), s);
  my_alert (_("Data mismatch error in configuration file\nAborting"));
  end_XFCE (3);
}

char *
nextline (FILE * f, char *lineread)
{
  char *p;
  do
  {
    nl++;
    if (!fgets (lineread, MAXSTRLEN + 1, f))
    {
      return (NULL);
    }
    if (strlen (lineread))
    {
      lineread[strlen (lineread) - 1] = '\0';
    }
    p = skiphead (lineread);
  }
  while (!strlen (p) && !feof (f));
  if (strlen (p))
    skiptail (p);
  return ((!feof (f)) ? p : NULL);
}

config *
initconfig (config * newconf)
{
  char *value;
  if (!newconf)
    newconf = (config *) g_malloc (sizeof (config) + 1);
  /* 
     The following is intended to help sysdmins who don't want users to
     tweak their configuration (for building a set top box or a public
     terminal, for example). If DISABLE_XFCE_USER_CONFIG is set to
     "y" or "1", xfce won't read/modify or even save the user config...
   */
  value = getenv ("DISABLE_XFCE_USER_CONFIG");
  if (value && (my_strnSTARTS (value, "1") || my_strnSTARTS (value, "y")))
  {
    newconf->disable_user_config = TRUE;
  }
  else
  {
    newconf->disable_user_config = FALSE;
  }
  newconf->panel_x = -1;
  newconf->panel_y = -1;
  newconf->wm = 0;
  newconf->visible_screen = 4;
  newconf->visible_popup = 6;
  newconf->select_icon_size = 0;	/* Small size */
  newconf->popup_icon_size = 0;	/* Small size */
  newconf->colorize_root = FALSE;
  newconf->gradient_root = FALSE;
  newconf->detach_menu = TRUE;
  newconf->gradient_active_title = FALSE;
  newconf->gradient_inactive_title = TRUE;
  newconf->clicktofocus = TRUE;
  newconf->opaquemove = TRUE;
  newconf->opaqueresize = TRUE;
  newconf->show_diagnostic = FALSE;
  newconf->apply_xcolors = TRUE;
  newconf->mapfocus = TRUE;
  newconf->snapsize = 10;
  newconf->autoraise = 0;
  newconf->tooltipsdelay = 250;
  newconf->startup_flags = (F_SOUNDMODULE | F_MOUSEMODULE | 
			    F_BACKDROPMODULE | F_PAGERMODULE | 
			    F_GNOMEMODULE | F_GNOMEMENUMODULE | 
			    F_KDEMENUMODULE | F_DEBIANMENUMODULE);
  newconf->iconpos = 0;		/* Top of screen */
  newconf->fonts[0] = (char *) g_strdup (XFWM_TITLEFONT);
  newconf->fonts[1] = (char *) g_strdup (XFWM_MENUFONT);
  newconf->fonts[2] = (char *) g_strdup (XFWM_ICONFONT);
  newconf->digital_clock = 0;
  newconf->hrs_mode = 1;
  newconf->panel_layer = DEFAULT_LAYER;
  newconf->xfwm_engine = XFCE_ENGINE;

# ifdef HAVE_LIBXML2
  value = getenv ("DISABLE_XFCE_USER_XMLCONFIG");
  if (value && (my_strnSTARTS (value, "1") || my_strnSTARTS (value, "y")))
  {
    newconf->disable_xmlconfigs = TRUE;
  }
  else
  {
    newconf->disable_xmlconfigs = FALSE;
  }
  newconf->xmlconfigs = NULL; /* no extras here */
# endif 

  return newconf;
}

void
backupconfig (char *extension)
{
  char homedir[MAXSTRLEN + 1];
  char buffer[MAXSTRLEN + 1];
  char backname[MAXSTRLEN + 1];
  FILE *copyfile;
  FILE *backfile;
  int nb_read;
  
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  /*
     Backup any existing config file before creating a new one 
   */
  if (existfile (homedir))
  {
    snprintf (backname, MAXSTRLEN, "%s%s", homedir, extension);
    backfile = fopen (backname, "w");
    copyfile = fopen (homedir, "r");
    if ((backfile) && (copyfile))
    {
      while ((nb_read = fread (buffer, 1, MAXSTRLEN, copyfile)) > 0)
      {
	fwrite (buffer, 1, nb_read, backfile);
      }
      fflush (backfile);
    }
    if (backfile)
      fclose (backfile);
    if (copyfile)
      fclose (copyfile);
#  ifdef HAVE_LIBXML2
    gxfce_backup_configs (extension);
#  endif
  }
}

void
writeconfig (void)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile = NULL;
  int i, j;
  gint x, y;

  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
  {
    return;
  }
  
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  /*
     Backup any existing config file before creating a new one 
   */
  if (existfile (homedir))
    backupconfig (BACKUPEXT);

  configfile = fopen (homedir, "w");

  if (!configfile)
    my_alert (_("Cannot create file"));
  else
  {
    fprintf (configfile, "%s\n", XFCE3SIG);
    fprintf (configfile, "[Coords]\n");
    gdk_window_get_root_origin (gxfce->window, &x, &y);
    fprintf (configfile, "\t%i\n", x);
    fprintf (configfile, "\t%i\n", y);
    fprintf (configfile, "[ButtonLabels]\n");
    for (i = 0; i < NBSCREENS; i++)
      fprintf (configfile, "\t%s\n", get_gxfce_screen_label (i));
    fprintf (configfile, "[External_Icons]\n");
    for (i = 0; i < NBSELECTS; i++)
      if (get_exticon_str (i) && (strlen (get_exticon_str (i))))
	fprintf (configfile, "\t%s\n", get_exticon_str (i));
      else
	fprintf (configfile, "\tNone\n");
    fprintf (configfile, "[Popups]\n");
    fprintf (configfile, "\t%i\n", (int) current_config.visible_popup);
    fprintf (configfile, "[Icons]\n");
    fprintf (configfile, "\t%s\n", save_icon_str ());
    fprintf (configfile, "[WorkSpace]\n");
    fprintf (configfile, current_config.colorize_root ? "\tRepaint\n" : "\tNoRepaint\n");
    fprintf (configfile, current_config.gradient_root ? "\tGradient\n" : "\tSolid\n");
    fprintf (configfile, "[Lock]\n");
    fprintf (configfile, "\t%s\n", get_command (NBSELECTS));
    fprintf (configfile, "[MenuOption]\n");
    fprintf (configfile, current_config.detach_menu ? "\tDetach\n" : "\tNoDetach\n");
    fprintf (configfile, "[XFwmOption]\n");
    fprintf (configfile, current_config.clicktofocus ? "\tClickToFocus\n" : "\tFollowMouse\n");
    fprintf (configfile, current_config.opaquemove ? "\tOpaqueMove\n" : "\tNoOpaqueMove\n");
    fprintf (configfile, current_config.opaqueresize ? "\tOpaqueResize\n" : "\tNoOpaqueResize\n");
    fprintf (configfile, "\t%i\n", (int) current_config.snapsize);
    fprintf (configfile, "\t%i\n", (int) current_config.startup_flags);
    fprintf (configfile, current_config.autoraise ? "\tAutoraise\n" : "\tNoAutoraise\n");
    fprintf (configfile, current_config.gradient_active_title ? "\tGradientActive\n" : "\tOpaqueActive\n");
    fprintf (configfile, current_config.gradient_inactive_title ? "\tGradientInactive\n" : "\tOpaqueInactive\n");
    switch (current_config.iconpos)
    {
    case 1:
      fprintf (configfile, "\tIconsOnLeft\n");
      break;
    case 2:
      fprintf (configfile, "\tIconsOnBottom\n");
      break;
    case 3:
      fprintf (configfile, "\tIconsOnRight\n");
      break;
    default:
      fprintf (configfile, "\tIconsOnTop\n");
    }
    fprintf (configfile, "\t%s\n", current_config.fonts[0]);
    fprintf (configfile, "\t%s\n", current_config.fonts[1]);
    fprintf (configfile, "\t%s\n", current_config.fonts[2]);
    fprintf (configfile, current_config.mapfocus ? "\tMapFocus\n" : "\tNoMapFocus\n");
    switch (current_config.xfwm_engine)
    {
    case MOFIT_ENGINE:
      fprintf (configfile, "\tMofit_engine\n");
      break;
    case TRENCH_ENGINE:
      fprintf (configfile, "\tTrench_engine\n");
      break;
    case GTK_ENGINE:
      fprintf (configfile, "\tGtk_engine\n");
      break;
    case LINEA_ENGINE:
      fprintf (configfile, "\tLinea_engine\n");
      break;
    default:
      fprintf (configfile, "\tXfce_engine\n");
      break;
    }
    fprintf (configfile, "[Screens]\n");
    fprintf (configfile, "\t%i\n", current_config.visible_screen);
    fprintf (configfile, "[Tooltips]\n");
    fprintf (configfile, "\t%i\n", (int) current_config.tooltipsdelay);
    fprintf (configfile, "[Clock]\n");
    fprintf (configfile, current_config.digital_clock ? "\tDigital\n" : "\tAnalog\n");
    fprintf (configfile, current_config.hrs_mode ? "\t24hrs\n" : "\t12hrs\n");
    fprintf (configfile, "[Sizes]\n");
    switch (current_config.select_icon_size)
    {
    case 0:
      fprintf (configfile, "\tSmallPanelIcons\n");
      break;
    case 2:
      fprintf (configfile, "\tLargePanelIcons\n");
      break;
    default:
      fprintf (configfile, "\tMediumPanelIcons\n");
    }
    switch (current_config.popup_icon_size)
    {
    case 0:
      fprintf (configfile, "\tSmallMenuIcons\n");
      break;
    case 2:
      fprintf (configfile, "\tLargeMenuIcons\n");
      break;
    default:
      fprintf (configfile, "\tMediumMenuIcons\n");
    }
    fprintf (configfile, "[XColors]\n");
    fprintf (configfile, current_config.apply_xcolors ? "\tApply\n" : "\tIgnore\n");
    fprintf (configfile, "[Diagnostic]\n");
    fprintf (configfile, current_config.show_diagnostic ? "\tShow\n" : "\tIgnore\n");
    fprintf (configfile, "[Layer]\n");
    fprintf (configfile, "\t%d\n", current_config.panel_layer);
    fprintf (configfile, "[Commands]\n");
    for (i = 0; i < NBSELECTS; i++)
      if (strlen (selects[i].command))
	fprintf (configfile, "\t%s\n", get_command (i));
      else
	fprintf (configfile, "\tNone\n");
    for (i = 0; i < NBPOPUPS; i++)
    {
      fprintf (configfile, "[Menu%u]\n", i + 1);
      for (j = 0; j < get_popup_menu_entries (i); j++)
      {
	fprintf (configfile, "\t%s\n", get_popup_entry_label (i, j));
	fprintf (configfile, "\t%s\n", get_popup_entry_icon (i, j));
	fprintf (configfile, "\t%s\n", get_popup_entry_command (i, j));
      }

    }
#ifdef XFCE_TASKBAR
    fprintf (configfile, "[Taskbar]\n");
    taskbar_save_config(configfile);
    taskbar_applay_xfce_config(&current_config);
#endif
    fflush (configfile);
    fclose (configfile);
#  ifdef HAVE_LIBXML2
    if (! current_config.disable_xmlconfigs)
	gxfce_write_configs ();
#  endif
  }
}

void
resetconfig (void)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile;
  int i;

  /* Return if DISABLE_XFCE_USER_CONFIG was set */
  if (current_config.disable_user_config)
  {
    return;
  }
  
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile);
  configfile = fopen (homedir, "w");
  if (!configfile)
    my_alert (_("Cannot reset configuration file"));
  else
  {
    fprintf (stderr, _("Creating new config file...\n"));
    fprintf (configfile, "%s\n", XFCE3SIG);
    fprintf (configfile, "[Coords]\n");
    fprintf (configfile, "\t%i\n", -1);
    fprintf (configfile, "\t%i\n", -1);
    fprintf (configfile, "[ButtonLabels]\n");
    for (i = 0; i < NBSCREENS; i++)
      fprintf (configfile, "\t%s\n", screen_names[i]);
    fprintf (configfile, "[External_Icons]\n");
    for (i = 0; i < NBSELECTS; i++)
      fprintf (configfile, "\tNone\n");
    fprintf (configfile, "[Popups]\n");
    fprintf (configfile, "\t6\n");
    fprintf (configfile, "[Icons]\n");
    fprintf (configfile, "\t%s\n", DEFAULT_ICON_SEQ);
    fprintf (configfile, "[WorkSpace]\n");
    fprintf (configfile, "\tRepaint\n");
    fprintf (configfile, "\tGradient\n");
    fprintf (configfile, "[Lock]\n");
    fprintf (configfile, "\tNone\n");
    fprintf (configfile, "[MenuOption]\n");
    fprintf (configfile, "\tDetach\n");
    fprintf (configfile, "[XFwmOption]\n");
    fprintf (configfile, "\tClickToFocus\n");
    fprintf (configfile, "\tOpaqueMove\n");
    fprintf (configfile, "\tOpaqueResize\n");
    fprintf (configfile, "\t10\n");
    fprintf (configfile, "\t%i\n", (F_SOUNDMODULE | F_MOUSEMODULE | F_BACKDROPMODULE | F_PAGERMODULE));
    fprintf (configfile, "\tNoAutoRaise\n");
    fprintf (configfile, "\tGradientActive\n");
    fprintf (configfile, "\tGradientInactive\n");
    fprintf (configfile, "\tIconsOnTop\n");
    fprintf (configfile, "\t%s\n", XFWM_TITLEFONT);
    fprintf (configfile, "\t%s\n", XFWM_MENUFONT);
    fprintf (configfile, "\t%s\n", XFWM_ICONFONT);
    fprintf (configfile, "\tMapFocus\n");
    fprintf (configfile, "\tXfce_engine\n");
    fprintf (configfile, "[Screens]\n");
    fprintf (configfile, "\t4\n");
    fprintf (configfile, "[Tooltips]\n");
    fprintf (configfile, "\t250\n");
    fprintf (configfile, "[Clock]\n");
    fprintf (configfile, "\tAnalog\n");
    fprintf (configfile, "\t24hrs\n");
    fprintf (configfile, "[Sizes]\n");
    fprintf (configfile, "\tMediumPanelIcons\n");
    fprintf (configfile, "\tSmallMenuIcons\n");
    fprintf (configfile, "[XColors]\n");
    fprintf (configfile, "\tApply\n");
    fprintf (configfile, "[Diagnostic]\n");
    fprintf (configfile, "\tIgnore\n");
    fprintf (configfile, "[Layer]\n");
    fprintf (configfile, "\t%d\n", DEFAULT_LAYER);
    fprintf (configfile, "[Commands]\n");
    for (i = 0; i < NBSELECTS; i++)
      fprintf (configfile, "\tNone\n");
    for (i = 0; i < NBPOPUPS; i++)
    {
      fprintf (configfile, "[Menu%u]\n", i + 1);
    }
#ifdef XFCE_TASKBAR
/* Reset taskbar config by removie [TAskbar] entry in xfce3rc file    
    fprintf (configfile, "[Taskbar]\n");
    taskbar_save_config(configfile);
*/    
    taskbar_applay_xfce_config(&current_config);
#endif
    fflush (configfile);
    fclose (configfile);
#  ifdef HAVE_LIBXML2
    if (! current_config.disable_xmlconfigs)
	gxfce_reset_configs ();
#  endif
  }
}

/*static*/ char *
localize_rcfilename (gboolean disable_user_config)
{
  return getlocalizedconffilename(rcfile,disable_user_config==TRUE);
}

void
readconfig (void)
{
  char lineread[MAXSTRLEN + 1];
  char dummy[16];
  char *filename;
  char *pixfile;
  char *command;
  char *label;
  char *p;
  int i, j;
  FILE *configfile = NULL;

  nl = 0;
  filename = localize_rcfilename (current_config.disable_user_config);
  configfile = fopen (filename, "r");
  g_free (filename);
  if (!configfile)
  {
    my_alert (_("Cannot open configuration file"));
    if (!(current_config.disable_user_config))
    {
      resetconfig ();
      filename = localize_rcfilename (FALSE);
      configfile = fopen (filename, "r");
      g_free (filename);
    }
  }
  if (!configfile)
    my_alert (_("Cannot open configuration file"));
  else
  {
    p = nextline (configfile, lineread);
    if (! my_strnSTARTS (p, XFCE3SIG))
    {
      my_alert (_("Does not looks like an XFce 3 configuration file !"));
      if (my_yesno_dialog (_("Do you want to reset the configuration file ?\n \
	(The previous file will be saved with a \".orig\" extension)")))
      {
	backupconfig (".orig");
	fclose (configfile);
	resetconfig ();
        filename = localize_rcfilename (FALSE);
	configfile = fopen (filename, "r");
	g_free (filename);
	if (!configfile)
	{
	  my_alert (_("Cannot open new config, Giving up..."));
	  data_error (_("Cannot load configuration file"));
	}
	/* Skipping first line */
	p = nextline (configfile, lineread);
      }
      else
	syntax_error (_("Cannot use old version of XFce configuration files"));
    }
    p = nextline (configfile, lineread);
    if (!my_strnSTARTS (p, "[Coords]"))
      syntax_error (p);
    p = nextline (configfile, lineread);
    current_config.panel_x = atoi (p);
    p = nextline (configfile, lineread);
    current_config.panel_y = atoi (p);
    p = nextline (configfile, lineread);
    if (! my_strnSTARTS (p, "[ButtonLabels]"))
      syntax_error (p);
    i = 0;
    p = nextline (configfile, lineread);
    while ((i < NBSCREENS) && (!my_strnSTARTS (p, "[External_Icons]")))
    {
      set_gxfce_screen_label (i, p);
      p = nextline (configfile, lineread);
      i++;
    }
    if (my_strnSTARTS (p, "[External_Icons]"))
    {
      for (i = 0; i < NBSELECTS + 1; i++)
      {
	p = nextline (configfile, lineread);
	if ((!my_strnSTARTS (p, "[Icons]")) && 
	    (!my_strnSTARTS (p, "[Popups]")))
	{
	  set_exticon_str (i, p);
	}
	else
	{
	  break;
	}
      }
    }
    if (my_strnSTARTS (p, "[Popups]"))
    {
      p = nextline (configfile, lineread);
      current_config.visible_popup = atoi (p);
      if ((current_config.visible_popup) > NBPOPUPS)
	current_config.visible_popup = NBPOPUPS;
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Icons]"))
    {
      p = nextline (configfile, lineread);
      load_icon_str (p);
      p = nextline (configfile, lineread);
    }
    else
    {
      default_icon_str ();
    }
    if (my_strnSTARTS (p, "[WorkSpace]"))
    {
      p = nextline (configfile, lineread);
      current_config.colorize_root = my_strnSTARTS (p, "Repaint");
      p = nextline (configfile, lineread);
      current_config.gradient_root = my_strnSTARTS (p, "Gradient");
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Lock]"))
    {
      p = nextline (configfile, lineread);
      set_command (NBSELECTS, p);
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[MenuOption]"))
    {
      p = nextline (configfile, lineread);
      current_config.detach_menu = my_strnSTARTS (p, "Detach");
      p = nextline (configfile, lineread);
    }
    if (!my_strncasecmp (p, "[XFwmOption]", strlen ("[XFwmOption]")))
    {
      p = nextline (configfile, lineread);
      current_config.clicktofocus = my_strnSTARTS (p, "ClickToFocus");
      p = nextline (configfile, lineread);
      current_config.opaquemove = my_strnSTARTS (p, "OpaqueMove");
      p = nextline (configfile, lineread);
      if ((my_strnSTARTS (p, "OpaqueResize")) || 
	  (my_strnSTARTS (p, "NoOpaqueResize")))
      {
	current_config.opaqueresize = my_strnSTARTS (p, "OpaqueResize");
	p = nextline (configfile, lineread);
	current_config.snapsize = atoi (p);
	p = nextline (configfile, lineread);
	current_config.startup_flags = atoi (p);
	p = nextline (configfile, lineread);
      }
      current_config.autoraise = my_strnSTARTS (p, "AutoRaise");
      p = nextline (configfile, lineread);
      current_config.gradient_active_title = my_strnSTARTS (p, "GradientActive");
      p = nextline (configfile, lineread);
      current_config.gradient_inactive_title = my_strnSTARTS (p, "GradientInactive");
      p = nextline (configfile, lineread);
      if (my_strnSTARTS (p, "IconsOnLeft"))
	current_config.iconpos = 1;
      else if (my_strnSTARTS (p, "IconsOnBottom"))
	current_config.iconpos = 2;
      else if (my_strnSTARTS (p, "IconsOnRight"))
	current_config.iconpos = 3;
      else
	current_config.iconpos = 0;
      p = nextline (configfile, lineread);
      if (current_config.fonts[0])
	g_free (current_config.fonts[0]);
      current_config.fonts[0] = g_strdup (p);
      p = nextline (configfile, lineread);
      if (current_config.fonts[1])
	g_free (current_config.fonts[1]);
      current_config.fonts[1] = g_strdup (p);
      p = nextline (configfile, lineread);
      if (current_config.fonts[2])
	g_free (current_config.fonts[2]);
      current_config.fonts[2] = g_strdup (p);
      p = nextline (configfile, lineread);
      if (my_strnSTARTS (p, "MapFocus"))
      {
	current_config.mapfocus = TRUE;
	p = nextline (configfile, lineread);
      }
      else if (my_strnSTARTS (p, "NoMapFocus"))
      {
	current_config.mapfocus = FALSE;
	p = nextline (configfile, lineread);
      }
      if (my_strnSTARTS (p, "X"))
      {
	current_config.xfwm_engine = XFCE_ENGINE;
	p = nextline (configfile, lineread);
      }
      else if (my_strnSTARTS (p, "M"))
      {
	current_config.xfwm_engine = MOFIT_ENGINE;
	p = nextline (configfile, lineread);
      }
      else if (my_strnSTARTS (p, "T"))
      {
	current_config.xfwm_engine = TRENCH_ENGINE;
	p = nextline (configfile, lineread);
      }
      else if (my_strnSTARTS (p, "G"))
      {
	current_config.xfwm_engine = GTK_ENGINE;
	p = nextline (configfile, lineread);
      }
      else if (my_strnSTARTS (p, "L"))
      {
	current_config.xfwm_engine = LINEA_ENGINE;
	p = nextline (configfile, lineread);
      }
      else if (!my_strnSTARTS (p, "["))
      {
	/*
	 * Need to read a new line in case the theme is something else...
	 */
	p = nextline (configfile, lineread);
      }
    }
    if (my_strnSTARTS (p, "[Screens]"))
    {
      p = nextline (configfile, lineread);
      current_config.visible_screen = atoi (p);
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Tooltips]"))
    {
      p = nextline (configfile, lineread);
      current_config.tooltipsdelay = atoi (p);
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Clock]"))
    {
      p = nextline (configfile, lineread);
      current_config.digital_clock = my_strnSTARTS (p, "Digital");
      p = nextline (configfile, lineread);
      current_config.hrs_mode = my_strnSTARTS (p, "24hrs");
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Sizes]"))
    {
      p = nextline (configfile, lineread);
      if (my_strnSTARTS (p, "SmallPanelIcons"))
	current_config.select_icon_size = 0;
      else if (my_strnSTARTS (p, "LargePanelIcons"))
	current_config.select_icon_size = 2;
      else
	current_config.select_icon_size = 1;
      p = nextline (configfile, lineread);
      if (my_strnSTARTS (p, "SmallMenuIcons"))
	current_config.popup_icon_size = 0;
      else if (my_strnSTARTS (p, "LargeMenuIcons"))
	current_config.popup_icon_size = 2;
      else
	current_config.popup_icon_size = 1;
      update_popup_size ();
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[XColors]"))
    {
      p = nextline (configfile, lineread);
      current_config.apply_xcolors = my_strnSTARTS (p, "Apply");
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Diagnostic]"))
    {
      p = nextline (configfile, lineread);
      current_config.show_diagnostic = my_strnSTARTS (p, "Show");
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Layer]"))
    {
      p = nextline (configfile, lineread);
      current_config.panel_layer = atoi (p);
      p = nextline (configfile, lineread);
    }
    if (my_strnSTARTS (p, "[Commands]"))
    {
      p = nextline (configfile, lineread);
      if (p)
      {
	for (i = 0; i < NBSELECTS; i++)
	{
          set_command (i, p);
	  p = nextline (configfile, lineread);
	}
      }
    }
    i = 0;
    sprintf (dummy, "[Menu%u]", i + 1);
    while ((p) && (my_strnSTARTS (p, dummy)) && (i++ < NBPOPUPS))
    {
      sprintf (dummy, "[Menu%u]", i + 1);
      p = nextline (configfile, lineread);
      j = 0;
      while ((p) && !my_strnSTARTS (p, dummy)&& !my_strnSTARTS (p, "[Taskbar"))
      {
        label = g_strdup (p ? p : "None");
        p = nextline (configfile, lineread);
        pixfile = g_strdup (p ? p : "Default icon");
        p = nextline (configfile, lineread);
        if ((p) && strcmp (p, "None"))
        {
          command = g_strdup (p ? p : "None");
          if (j++ < NBMAXITEMS)
            add_popup_entry (i - 1, label, pixfile, command, -1);
          g_free (command);
        }
        g_free (label);
        g_free (pixfile);
        p = nextline (configfile, lineread);
      }
    }
#ifdef XFCE_TASKBAR
 if(p&&my_strnSTARTS(p,"[Taskbar]")) {
  p=taskbar_read_config(configfile,lineread,MAXSTRLEN);
  taskbar_applay_xfce_config(&current_config);
 }
#endif
    fclose (configfile);
#  ifdef HAVE_LIBXML2
    if (! current_config.disable_xmlconfigs)
	gxfce_read_configs ();
#  endif
  }
}

void
update_config_screen_visible (int i)
{
  if (i <= NBSCREENS)
    current_config.visible_screen = i;
  else
    current_config.visible_screen = NBSCREENS;
  apply_wm_desk_names (current_config.visible_screen);
}
