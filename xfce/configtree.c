/*
 * Copyright (C) 2002 Guido Draheim (guidod@gmx.de)
 *
 * This program part is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * You shall treat rule 3 of the license as a dual license option herein.
 *
 * This program part is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

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

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_LIBXML2
#include <libxml/parser.h>

char *rcfile4 = "xfce4rc";

#define my_strnSTARTS(__s,__m) \
   (!  g_strncasecmp ((__s),(__m),strlen(__m)))

void
gxfce_backup_configs (char *extension)
{
  char homedir[MAXSTRLEN + 1];
  char buffer[MAXSTRLEN + 1];
  char backname[MAXSTRLEN + 1];
  FILE *copyfile;
  FILE *backfile;
  int nb_read;
  
  snprintf (homedir, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), 
	    rcfile4);
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
  }
}

static xmlNodePtr
find_section (xmlDocPtr doc, const char* sectname)
{
    xmlNodePtr root, sect; 

    if (! doc) 	return 0;

    root = xmlDocGetRootElement (doc);
    if (! root)
    {
	root = xmlNewDocRawNode (doc, 0, "options", 0);
	if (! root) return 0;
	xmlDocSetRootElement (doc, root);
    }

    if (strcmp (root->name, "options"))
	return 0; /* wrong type of xmldoc */

    for (sect = root->children; sect ; sect = sect->next)
    {
	if (sect->name && !strcasecmp (sect->name, sectname))
	    return sect;
    }
    sect = xmlNewDocRawNode (doc, 0, sectname, 0);
    if (! sect) return sect;
    xmlAddChild (root, xmlNewDocText(sect->doc, "\n"));
    return xmlAddChild (root, sect);
}

static xmlNodePtr
findnew_section (xmlDocPtr doc, const char* sectname)
{
    xmlNodePtr node;
    xmlNodePtr sect;

    sect = find_section (doc, sectname);
    if (! sect) return sect;

    node = sect->children;
    while (node)
    {
	xmlNodePtr next = node->next;
	xmlUnlinkNode (node);
	xmlFreeNode (node);
	node = next;
    }

    xmlAddChild (sect, xmlNewDocText(sect->doc, "\n "));
    return sect;
}

static xmlNodePtr
xmlNewTextChildNL (xmlNodePtr parent, 
		   xmlNsPtr ns,
		   xmlChar* name,
		   xmlChar* content)
{
    xmlNewTextChild (parent, 0, name, content);
    return xmlAddChild (parent, xmlNewDocText(parent->doc, "\n "))->prev;
}

void* /*xmlDocPtr*/
gxfce_set_configs (config * newconf, int reset)
{
  char content[MAXSTRLEN + 1];
  int i, j;
  gint x, y;
  xmlNodePtr sect;
  xmlNodePtr node;

  if (!newconf) return 0;

  if (! newconf->xmlconfigs)
      newconf->xmlconfigs = xmlNewDoc ("1.0");
  if (! newconf->xmlconfigs)
      return 0;

  /** writeconfig **/

  sect = findnew_section (newconf->xmlconfigs, "Coord");
  if (sect) 
  {
      if (!reset) 
	  gdk_window_get_root_origin (gxfce->window, &x, &y);
      else  
	  x = y = -1; 
      g_snprintf (content, MAXSTRLEN, "%i", x);
      xmlNewTextChildNL (sect, 0, "xoffset", content);
      g_snprintf (content, MAXSTRLEN, "%i", y);
      xmlNewTextChildNL (sect, 0, "yoffset", content);
  }
  sect = findnew_section (newconf->xmlconfigs, "ButtonLabels");
  if (sect) for (i = 0; i < NBSCREENS; i++)
  {
      if (!reset)
	  xmlNewTextChildNL (sect, 0, "label", 
			     get_gxfce_screen_label (i));
      else
	  xmlNewTextChildNL (sect, 0, "label", 
			     screen_names[i]);
  }
  sect = findnew_section (newconf->xmlconfigs, "External_Icons");
  if (sect) for (i = 0; i < NBSELECTS; i++)
  {
      if (!reset && get_exticon_str (i) && (strlen (get_exticon_str (i))))
	  xmlNewTextChildNL (sect, 0, "icon", get_exticon_str (i));
      else
	  xmlNewTextChildNL (sect, 0, "icon", "None");
  }
  sect = findnew_section (newconf->xmlconfigs, "Popups");
  if (sect)
  {
      g_snprintf (content, MAXSTRLEN, "%i", 
		  reset ? 6 : (int) current_config.visible_popup);
      xmlNewTextChildNL (sect, 0, "visible", content);
  }
  sect = findnew_section (newconf->xmlconfigs, "Icons");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "list", 
			 reset ? DEFAULT_ICON_SEQ : save_icon_str ());
  }
  sect = findnew_section (newconf->xmlconfigs, "WorkSpace");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "colorize_root", 
			 reset || current_config.colorize_root 
			 ? "Repaint" : "NoRepaint");
      xmlNewTextChildNL (sect, 0, "gradient_root", 
			 reset || current_config.gradient_root 
			 ? "Gradient" : "Solid");
  }
  sect = findnew_section (newconf->xmlconfigs, "Lock");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "command", 
			 reset ? "None" :  get_command (NBSELECTS));
  }
  sect = findnew_section (newconf->xmlconfigs, "MenuOption");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "detach_menu", 
			 reset || current_config.detach_menu 
			 ? "Detach" : "NoDetach");
  }
  sect = findnew_section (newconf->xmlconfigs, "XFwmOption");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "clicktofocus", 
			 reset || current_config.clicktofocus 
			 ? "\tClickToFocus\n" : "\tFollowMouse\n");
      xmlNewTextChildNL (sect, 0, "opaquemove", 
			 reset || current_config.opaquemove 
			 ? "\tOpaqueMove\n" : "\tNoOpaqueMove\n");
      xmlNewTextChildNL (sect, 0, "opaqueresize", 
			 reset || current_config.opaqueresize 
			 ? "\tOpaqueResize\n" : "\tNoOpaqueResize\n");
      g_snprintf (content, MAXSTRLEN, "%i", 
		  reset ? 10 : (int) current_config.snapsize);
      xmlNewTextChildNL (sect, 0, "opaqueresize", content);
      g_snprintf (content, MAXSTRLEN, "%i", 
		  reset ? (F_SOUNDMODULE | F_MOUSEMODULE | 
			   F_BACKDROPMODULE | F_PAGERMODULE)
		  : (int) current_config.startup_flags);
      xmlNewTextChildNL (sect, 0, "startup_flags", content);

      xmlNewTextChildNL (sect, 0, "autoraise", 
			 !reset && current_config.autoraise 
			 ? "Autoraise" : "NoAutoraise");
      xmlNewTextChildNL (sect, 0, "gradient_active_title", 
			 reset || current_config.gradient_active_title 
			 ? "GradientActive" : "OpaqueActive");
      xmlNewTextChildNL (sect, 0, "gradient_inactive_title", 
			 reset || current_config.gradient_inactive_title 
			 ? "GradientInactive" : "OpaqueInactive");
      xmlNewTextChildNL (sect, 0, "iconpos", 
			 reset ? "IconsOnTop" :
			 current_config.iconpos == 1 ? "IconsOnLeft" :
			 current_config.iconpos == 2 ? "IconsOnBottom" :
			 current_config.iconpos == 3 ? "IconsOnRight" :
			 "IconsOnTop");
      xmlNewTextChildNL (sect, 0, "titlefont",  
			 reset ? XFWM_TITLEFONT : current_config.fonts[0]);
      xmlNewTextChildNL (sect, 0, "menufont",   
			 reset ? XFWM_MENUFONT  : current_config.fonts[1]);
      xmlNewTextChildNL (sect, 0, "iconfont",   
			 reset ? XFWM_ICONFONT  : current_config.fonts[2]);
      xmlNewTextChildNL (sect, 0, "mapfocus", 
			 reset || current_config.mapfocus 
			 ? "MapFocus" : "NoMapFocus");
      xmlNewTextChildNL (sect, 0, "engine", 
          reset ? "Xfce_engine" :
	  current_config.xfwm_engine == MOFIT_ENGINE ? "Mofit_engine" :
	  current_config.xfwm_engine == TRENCH_ENGINE ? "Trench_engine" :
	  current_config.xfwm_engine == GTK_ENGINE ? "Gtk_engine" :
	  current_config.xfwm_engine == LINEA_ENGINE ? "Linea_engine" :
	  "Xfce_engine" );
  }
  sect = findnew_section (newconf->xmlconfigs, "Screens");
  if (sect)
  {
      g_snprintf (content, MAXSTRLEN, "%i", 
		  reset ? 4 : current_config.visible_screen);
      xmlNewTextChildNL (sect, 0, "visible", content);
  }
  sect = findnew_section (newconf->xmlconfigs, "Tooltips");
  if (sect)
  {
      g_snprintf (content, MAXSTRLEN, "%i", 
		  reset ? 250 : current_config.tooltipsdelay);
      xmlNewTextChildNL (sect, 0, "delay", content);
  }
  sect = findnew_section (newconf->xmlconfigs, "Clock");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "clockface", 
			 !reset && current_config.digital_clock 
			 ? "Digital" : "Analog");
      xmlNewTextChildNL (sect, 0, "hrs_mode", 
			 reset || current_config.hrs_mode 
			 ? "24hrs" : "12hrs");
  }
  sect = findnew_section (newconf->xmlconfigs, "Sizes");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "select_icon", 
	  reset ? "MediumPanelIcons" :
	  current_config.select_icon_size == 0 ?  "SmallPanelIcons" :
	  current_config.select_icon_size == 2 ?  "LargePanelIcons" :
	  "MediumPanelIcons");
      xmlNewTextChildNL (sect, 0, "popup_icon", 
	  reset ? "MediumMenuIcons" :
	  current_config.popup_icon_size == 0 ?  "SmallMenuIcons" :
	  current_config.popup_icon_size == 2 ?  "LargeMenuIcons" :
	  "MediumMenuIcons");
  }
  sect = findnew_section (newconf->xmlconfigs, "XColors");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "oncall", 
			 reset || current_config.apply_xcolors 
			 ? "Apply" : "Ignore");
  }
  sect = findnew_section (newconf->xmlconfigs, "Diagnostic");
  if (sect)
  {
      xmlNewTextChildNL (sect, 0, "oncall", 
			 !reset && current_config.show_diagnostic 
			 ? "Show" : "Ignore");
  }
  sect = findnew_section (newconf->xmlconfigs, "Layer");
  if (sect)
  {
      g_snprintf (content, MAXSTRLEN, "%d", 
		  reset ? DEFAULT_LAYER : current_config.panel_layer);
      xmlNewTextChildNL (sect, 0, "panel", content);
  }
  sect = findnew_section (newconf->xmlconfigs, "Commands");
  if (sect) for (i = 0; i < NBSELECTS; i++)
  {
      if (!reset && strlen (selects[i].command))
	  xmlNewTextChildNL (sect, 0, "command", get_command (i));
      else
	  xmlNewTextChildNL (sect, 0, "command", "None");
  }

  for (i = 0; i < NBPOPUPS; i++)
  {
      g_snprintf (content, MAXSTRLEN, "Menu%u", i + 1);
      sect = findnew_section (newconf->xmlconfigs, content);
      if (reset) continue;
      if (sect) for (j = 0; j < get_popup_menu_entries (i); j++)
      {
	  node = xmlNewTextChildNL (sect, 0, "app", 0);
	  xmlNewTextChildNL (node, 0, "label",   get_popup_entry_label (i, j));
	  xmlNewTextChildNL (node, 0, "icon",    get_popup_entry_icon (i, j));
	  xmlNewTextChildNL (node, 0, "command", get_popup_entry_command (i, j));
      }
  }
  sect = findnew_section (newconf->xmlconfigs, "SwitchDesk");
  if (sect)
  {
      g_snprintf (content, MAXSTRLEN, "%d", 
		  reset ? 4 : current_config.switchdeskfactor);
      xmlNewTextChildNL (sect, 0, "factor", content);
  }
  return newconf->xmlconfigs;
}

void
gxfce_write_configs (void)
{
  char homedir[MAXSTRLEN + 1];

  /* Return if DISABLE_XFCE_USER_XMLCONFIG was set */
  if (current_config.disable_xmlconfigs)
  {
    return;
  }
  
  g_snprintf (homedir, MAXSTRLEN, 
	      "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile4);
  /*
     Backup any existing config file before creating a new one 
   */
  if (existfile (homedir))
    backupconfig (BACKUPEXT);

  {
      xmlDocPtr xmlconfigs = gxfce_set_configs (&current_config, 0);
      fprintf (stderr, __FUNCTION__" saving xfce4rc (%p)\n", xmlconfigs);
      if (xmlconfigs)
          xmlSaveFile (homedir, xmlconfigs);
  }
}

void
gxfce_reset_configs (void)
{
  char homedir[MAXSTRLEN + 1];
  FILE *configfile;

  /* Return if DISABLE_XFCE_USER_XMLCONFIG was set */
  if (current_config.disable_xmlconfigs)
  {
    return;
  }

  snprintf (homedir, MAXSTRLEN, 
	    "%s/.xfce/%s", (char *) getenv ("HOME"), rcfile4);
  configfile = fopen (homedir, "w");
  if (!configfile)
      /* my_alert (_("Cannot reset configuration file")); */
      return;
  else
      fclose (configfile);
  /*fallthrough*/
  {
      xmlDocPtr xmlconfigs = gxfce_set_configs (&current_config, 1);
      fprintf (stderr, __FUNCTION__" resetting xfce4rc (%p)\n", xmlconfigs);
      if (xmlconfigs)
          xmlSaveFile (homedir, xmlconfigs);
  }
}

void
gxfce_read_configs (void)
{
  extern char* localize_rcfilename (int); /*FIXME: import via header... */
  char *filename;
  FILE *configfile = NULL;

  filename = localize_rcfilename (current_config.disable_user_config);
  filename[strlen(filename)-3] = '4'; /* turn xfce3rc into xfce4rc */
  configfile = fopen (filename, "r");
  if (!configfile)
  {
    g_free (filename);
    /* my_alert (_("Cannot open configuration file")); */
    if (!(current_config.disable_user_config))
    {
      resetconfig ();
      filename = localize_rcfilename (FALSE);
      filename[strlen(filename)-3] = '4'; /* turn xfce3rc into xfce4rc */
      configfile = fopen (filename, "r");
    }
  }
  if (!configfile)
      /* my_alert (_("Cannot open configuration file")); */ 
      return;
  else
  {
      fclose (configfile);
      current_config.xmlconfigs = xmlParseFile (filename);
  }
  g_free (filename);

  /* ... the complete configfile has been parsed and stored in memory
   * now we need to search for the items we recognize and place them
   * into the appopriate local variables of the program. Most stuff
   * however is still done in the old xfce3rc routines. Here we just
   * give an example on how to do it.
   */
  {
    xmlNodePtr root, sect; 

    if (! current_config.xmlconfigs) return;

    root = xmlDocGetRootElement (current_config.xmlconfigs);
    if (! root || strcmp (root->name, "options"))
    {
	if (!root)
	    root = xmlNewDocRawNode (current_config.xmlconfigs, 
				     0, "options", 0);
	else
	    root = find_section (current_config.xmlconfigs, "options");

	if (! root) return;
	sect = xmlDocSetRootElement (current_config.xmlconfigs, root);
	if (sect)
	    xmlAddSibling (root, sect);
    }

    sect = find_section (current_config.xmlconfigs, "SwitchDesk");
    if (sect)
    {
	register xmlNodePtr node;
	for (node = sect->children; node ; node = node->next)
	{
	    if (!strcmp (node->name, "factor"))
	    {
		current_config.switchdeskfactor = 
		    strtol (xmlNodeGetContent (node), 0, 0); /* memleak? */
		if (current_config.switchdeskfactor < 1)
		    current_config.switchdeskfactor = 1;
		if (current_config.switchdeskfactor > 99)
		    current_config.switchdeskfactor = 99;
	    }
	}
    }
  }
#if 0
  {
    char lineread[MAXSTRLEN + 1];
    char dummy[16];
    char *pixfile;
    char *command;
    char *label;
    char *p;
    int i, j;

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
	filename[strlen(filename)-3] = '4'; /* turn xfce3rc into xfce4rc */
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
      while ((p) && (!my_strnSTARTS (p, dummy)))
      {
        label = g_strdup (p ? p : "None");
	p = nextline (configfile, lineread);
        pixfile = g_strdup (p ? p : "Default icon");
	p = nextline (configfile, lineread);
	if (strcmp (p, "None"))
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
    fclose (configfile);
  }
#endif
}

#endif
/* HAVE_LIBXML2 */
