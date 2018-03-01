/*  xfmenu
 *  Copyright (C) 2001 Jasper Huijsmans (j.b.huijsmans@chem.rug.nl)
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

/* xfmenu
 * version 2.0
 *
 * Generates an xfwm menu structure from GNOME or KDE
 * "*.desktop" (or "*.kdelnk") files.
 * 
 * Regognized options:
 * -gnome
 * -kde
 * -debian
 * gnome is the default when no options are given. If both options are
 * given both menus are generated.
 * 
 * Environmental variables:
 * $GNOMEDIR
 * $KDEDIR
 * $LC_MESSAGES
 * $LANG
 * 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <glib.h>

#include <string.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif /* HAVE_ICONV */

#include "utils.h"
#include "module.h"
#include "constant.h"

#ifndef HAVE_SCANDIR
#include "my_scandir.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define GMENUPATH "/share/gnome/apps"
#define KMENUPATH "/share/applnk"
#define SYSPATH   "/etc/X11/applnk"
#define DEFAULT_CHARSET "ISO-8859-15"
#ifdef HAVE_ICONV
#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */
#define OUTBUF_SIZE	4096
#endif /* HAVE_ICONV */

/*------------------*
 * typedefs 
 *------------------*/

typedef struct menuoptions
{
  int gnome_menu;
  int kde_menu;
  int debian_menu;
}
MenuOptions;

typedef enum menutypes
{ GNOME, KDE, DEBIAN }
MenuType;

typedef enum entrytypes
{ ITEM, SUB }
EntryType;

typedef struct menuentry
{
  EntryType type;
  char *name;
  char *dirname;
  char *cmd;
  int term;
}
MenuEntry;

/*------------------*
 * prototypes 
 *------------------*/
void init_xfmenu (int argc, char **argv);
MenuOptions get_options (char **argv);
void init_nls (void);
int xfmenu (MenuOptions options);
void quit_xfmenu (void);

/*------------------*
 * global variables 
 *------------------*/

/* pipe file descriptor */
int fd[2];

/* needed to get the full name of a menu */
char *current_menu_root = NULL;

/* submenus should only be opened in one place
 * so we have to keep track of them */
GList *menunames_list = NULL;

/* used for translations */
char *lcmessages = NULL;
char *lang = NULL;
char *lcmessages_base = NULL;
char *lang_base = NULL;
char *charset = NULL;

/*------------------*
 * functions 
 *------------------*/

#ifdef HAVE_ICONV
static unsigned int isUtf8(const char *buf) {
  int i, n;
  register char c;
  unsigned int gotone = 0;

  static const char text_chars[256] = 
  {
  /*		      BEL BS HT LF    FF CR    */
	F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
	/*				ESC	     */
	F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
	/*	      NEL			     */
	X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
	X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
  };

  /* *ulen = 0; */
  for (i = 0; (c = buf[i]); i++) {
    if ((c & 0x80) == 0) {	  /* 0xxxxxxx is plain ASCII */
      /*
       * Even if the whole file is valid UTF-8 sequences,
       * still reject it if it uses weird control characters.
       */

      if (text_chars[(int) c] != (char) T)
	return 0;

    } else if ((c & 0x40) == 0) { /* 10xxxxxx never 1st byte */
      return 0;
    } else {			       /* 11xxxxxx begins UTF-8 */
      int following;

    if ((c & 0x20) == 0) {	       /* 110xxxxx */
      following = 1;
    } else if ((c & 0x10) == 0) {      /* 1110xxxx */
      following = 2;
    } else if ((c & 0x08) == 0) {      /* 11110xxx */
      following = 3;
    } else if ((c & 0x04) == 0) {      /* 111110xx */
      following = 4;
    } else if ((c & 0x02) == 0) {      /* 1111110x */
      following = 5;
    } else
      return 0;

      for (n = 0; n < following; n++) {
	i++;
	if (!(c = buf[i]))
	  goto done;

	if ((c & 0x80) == 0 || (c & 0x40))
	  return 0;
      }
      gotone = 1;
    }
  }
done:
  return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
}

char outbuf[OUTBUF_SIZE];
char *convert_code (char *fromcode)
{
  char *outptr;
  int outlen=0;
  int len=0;

  unsigned int utf8 = isUtf8(fromcode);

  memset(outbuf, 0, OUTBUF_SIZE);

  len = strlen(fromcode);

  outptr = outbuf;
  outlen = OUTBUF_SIZE;

  if (utf8) {
       iconv_t cd;
       cd = iconv_open(charset, "UTF8");
       if (cd != (iconv_t) (-1))
       {
         iconv (cd, &fromcode, &len, &outptr, &outlen);
         iconv_close(cd);
       }
       else
       {
         /* unsupported conversion */
	 g_snprintf (outbuf, OUTBUF_SIZE - 1, "%s", fromcode);
         outlen = strlen (fromcode);
       }
  }
  outbuf[outlen] = '\0';
  return outbuf;
}
#endif /* HAVE_ICONV */

/* initialization and 
 * communication with xfwm */

void
init_xfmenu (int argc, char **argv)
{
  if (argc < 7)
  {
    fprintf (stderr, "This module should be executed by xfwm\n");
    exit (0);
  }

  fd[0] = atoi (argv[1]);
  fd[1] = atoi (argv[2]);
  SetMessageMask (fd, 0);
}


void
quit_xfmenu (void)
{
  close (fd[0]);
  close (fd[1]);
}

void
DeadPipe (int nonsense)
{
  exit (1);
}

MenuOptions get_options (char **argv)
{
  MenuOptions options;

  options.gnome_menu = 0;
  options.kde_menu = 0;
  options.debian_menu = 0;

  if (argv[7] && strlen (argv[7]) > 1)
  {
    if (g_strncasecmp ("-g", argv[7], 2) == 0)
      options.gnome_menu = 1;
    else if (g_strncasecmp ("-k", argv[7], 2) == 0)
      options.kde_menu = 1;
    else if (g_strncasecmp ("-d", argv[7], 2) == 0)
      options.debian_menu = 1;

    if (argv[8] && strlen (argv[8]) > 1)
    {
      if (g_strncasecmp ("-g", argv[8], 2) == 0)
	options.gnome_menu = 1;
      else if (g_strncasecmp ("-k", argv[8], 2) == 0)
	options.kde_menu = 1;
      else if (g_strncasecmp ("-d", argv[8], 2) == 0)
            options.debian_menu = 1;	
      
      if (argv[9] && strlen (argv[9]) > 1)
	{
	  if (g_strncasecmp ("-g", argv[9], 2) == 0)
	    options.gnome_menu = 1;
	  else if (g_strncasecmp ("-k", argv[9], 2) == 0)
	    options.kde_menu = 1;
	  else if (g_strncasecmp ("-d", argv[9], 2) == 0)
	    options.debian_menu = 1;
	}
    }
  }
  else
  {
    /* default to only debian menu */
    options.debian_menu = 1;
    options.gnome_menu = 0;
    options.kde_menu = 0;
  }

  return options;
}

void
init_nls (void)
{
  char *temp;

  temp = g_getenv ("CHARSET");
  if (temp)
  {
    charset = g_strdup (temp);
  }
  
  /* these are global variables so they can be used easily in many places */
  temp = g_getenv ("LC_MESSAGES");
  if (temp)
  {
    char *end;
    char *dot;

    lcmessages = g_strndup (temp, 5);
    lcmessages_base = g_strdup (lcmessages);

    if ((!charset) && (dot = strrchr (lcmessages , '.')))
    {
      dot++;
      charset = (char *) g_malloc (strlen (dot) + 1);
      strcpy (charset, dot);
    }

    if ((end = strchr (lcmessages_base, '_')))
    {
      *end = '\0';
    }
  }
  temp = g_getenv ("LANG");
  if (!temp)
  {
    temp = g_getenv ("LANGUAGE");
  }

  if (temp)
  {
    char *end;
    char *dot;

    lang = g_strndup (temp, 5);
    lang_base = g_strdup (lang);

    if ((!charset) && (dot = strrchr (lang , '.')))
    {
      dot++;
      charset = (char *) g_malloc (strlen (dot) + 1);
      strcpy (charset, dot);
    }

    if ((end = strchr (lang_base, '_')))
    {
      *end = '\0';
    }
  }

  if (!charset)
  {
    charset = (char *) g_malloc (strlen (DEFAULT_CHARSET) + 1);
    strcpy (charset, DEFAULT_CHARSET);
  }
}

/*------------------*/

/* files and directories */

GList *
get_menu_dirs (MenuType mtype)
{
  char *home = g_getenv ("HOME");
  char *menupath;

  if (mtype == KDE)
  {
    GList *kdedirs = NULL;
    char *kdedir = g_getenv ("KDEDIR");

    /* make kde dirlist */
    if (kdedir)
    {
      menupath = g_strconcat (kdedir, KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (!(kdedir) || (strcmp ("/usr", kdedir) != 0))
    {
      menupath = g_strconcat ("/usr", KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (!(kdedir) || (strcmp ("/usr/local", kdedir) != 0))
    {
      menupath = g_strconcat ("/usr/local", KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (!(kdedir) || (strcmp ("/opt/kde", kdedir) != 0))
    {
      menupath = g_strconcat ("/opt/kde", KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (!(kdedir) || (strcmp ("/opt/kde2", kdedir) != 0))
    {
      menupath = g_strconcat ("/opt/kde2", KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (!(kdedir) || (strcmp ("/opt/kde3", kdedir) != 0))
    {
      menupath = g_strconcat ("/opt/kde3", KMENUPATH, NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    if (home)
    {
      menupath = g_strconcat (home, "/.kde/applnk", NULL);
      kdedirs = g_list_append (kdedirs, menupath);
      menupath = g_strconcat (home, "/.kde2/applnk", NULL);
      kdedirs = g_list_append (kdedirs, menupath);
      menupath = g_strconcat (home, "/.kde3/applnk", NULL);
      kdedirs = g_list_append (kdedirs, menupath);
    }
    kdedirs = g_list_append (kdedirs, SYSPATH);

    return kdedirs;
  }
  else
  {
    GList *gnomedirs = NULL;
    char *gnomedir = g_getenv ("GNOMEDIR");

    /* make gnome dirlist */
    if (gnomedir)
    {
      menupath = g_strconcat (gnomedir, GMENUPATH, NULL);
      gnomedirs = g_list_append (gnomedirs, menupath);
    }
    if (!(gnomedir) || (strcmp ("/usr", gnomedir) != 0))
    {
      menupath = g_strconcat ("/usr", GMENUPATH, NULL);
      gnomedirs = g_list_append (gnomedirs, menupath);
    }
    if (!(gnomedir) || (strcmp ("/usr/local", gnomedir) != 0))
    {
      menupath = g_strconcat ("/usr/local", GMENUPATH, NULL);
      gnomedirs = g_list_append (gnomedirs, menupath);
    }
    if (!(gnomedir) || (strcmp ("/opt/gnome", gnomedir) != 0))
    {
      menupath = g_strconcat ("/opt/gnome", GMENUPATH, NULL);
      gnomedirs = g_list_append (gnomedirs, menupath);
    }
    if (home)
    {
      menupath = g_strconcat (home, "/.gnome/apps", NULL);
      gnomedirs = g_list_append (gnomedirs, menupath);
    }
    gnomedirs = g_list_append (gnomedirs, SYSPATH);

    return gnomedirs;
  }
}

static int
select_subs (const struct dirent *dentry)
{
  struct stat filestat;
  char *name = dentry->d_name;

  if ((strcmp (name, ".") == 0) || (strcmp (name, "..") == 0) || (stat (name, &filestat) == -1))
  {
    return 0;
  }
  if (S_ISDIR (filestat.st_mode) && (*name != '.'))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

static int
select_items (const struct dirent *dentry)
{
  struct stat filestat;

  if (stat (dentry->d_name, &filestat) == -1)
    return 0;

  if (S_ISREG (filestat.st_mode) && (strstr (dentry->d_name, ".desktop") || strstr (dentry->d_name, ".kdelnk")))
    return 1;
  else
    return 0;
}

static int
select_all (const struct dirent *dentry)
{
  return 1;
}

GList *
order_entries (GList * mentries)
{
  FILE *fp;
  char line[MAXSTRLEN];
  GList *order, *templist;

  if ((fp = fopen (".order", "r")) == NULL)
    return mentries;

  order = NULL;
  while (fgets (line, MAXSTRLEN - 1, fp))
  {
    char *end;

    if ((end = strchr (line, '\n')))
      *end = '\0';

    if (strlen (line))
      order = g_list_append (order, g_strdup (line));
  }

  fclose (fp);

  /* order list might still be empty */
  if (order == NULL)
    return mentries;

  templist = NULL;
  for (; order; order = g_list_next (order))
  {
    if (g_list_find_custom (mentries, order->data, (GCompareFunc) strcmp) != NULL)
      templist = g_list_append (templist, order->data);
  }

  /* put additional items of entries in templist */
  for (; mentries; mentries = g_list_next (mentries))
  {
    if (g_list_find_custom (templist, mentries->data, (GCompareFunc) strcmp) == NULL)
      templist = g_list_append (templist, mentries->data);
  }

  mentries = templist;

  return mentries;
}

GList *
get_dir_entries (EntryType etype)
{
  GList *entries = NULL;
  struct dirent **direntrylist;
  int i, n;

  if (etype == SUB)
    n = scandir (".", &direntrylist, select_subs, alphasort);
  else
    n = scandir (".", &direntrylist, select_items, alphasort);


  for (i = 0; i < n; i++)
  {
    char *entry = g_strdup (direntrylist[i]->d_name);
    struct dirent **de_list;

    if (etype == SUB && scandir (entry, &de_list, select_all, NULL) < 3)
    {
      g_free (entry);
      continue;
    }

    entries = g_list_append (entries, entry);
  }

  entries = order_entries (entries);

  return entries;
}

void
free_direntry_list (GList * dentrylist)
{
  GList *list = dentrylist;

  for (; list; list = g_list_next (list))
    g_free (list->data);

  g_list_free (dentrylist);
}

/*------------------*/

/* menu entries */

MenuEntry *
new_menu_entry (EntryType etype)
{
  MenuEntry *entry = g_malloc (sizeof (MenuEntry));

  if (!entry)
    return NULL;

  entry->type = etype;
  entry->dirname = NULL;
  entry->name = NULL;
  entry->cmd = NULL;
  entry->term = 0;

  return entry;
}

int
parse_line (const char *line, char **key, char **locale, char **value)
{
  char *token = strchr (line, '=');
  char *temp;
  char *bracket_open, *bracket_close;

  if (token == NULL)
    return 0;

  temp = g_strndup (line, token - line);
  g_strstrip (temp);

  if ((bracket_open = strchr (temp, '[')) && (bracket_close = strchr (bracket_open, ']')))
  {
    *key = g_strndup (temp, bracket_open - temp);
    bracket_open++;
    *locale = g_strndup (bracket_open, MIN (bracket_close - bracket_open, 5));
  }
  else
  {
    *key = g_strdup (temp);
    *locale = NULL;
  }
  g_free (temp);

  *value = g_strdup (token + 1);
  g_strstrip (*value);

  return 1;
}

MenuEntry *
make_menu_entry (char *filename, MenuType etype)
{
  FILE *fp;
  char line[MAXSTRLEN];
  char *name = NULL, *cmd = NULL, *terminal = NULL;
  char *tname_lcmessages = NULL, *tname_lang = NULL;
  MenuEntry *entry = new_menu_entry (etype);

  if ((fp = fopen (filename, "r")) == NULL)
  {
    if (etype == ITEM)
      return NULL;
    else
    {
      /* No .directory file found. Use dirname instead */
      char *dir = g_dirname (filename);

      entry->dirname = dir;
      entry->name = g_strdup (g_basename (entry->dirname));
      return entry;
    }
  }
  /* We now have an open file */

  if (etype == SUB)
    entry->dirname = g_dirname (filename);

  while (fgets (line, MAXSTRLEN - 1, fp))
  {
    char *key, *value, *locale;
    char *end;

    if ((end = strchr (line, '\n')) != NULL)
      *end = '\0';

    if (parse_line (line, &key, &locale, &value) == 0)
      continue;

    if ((!name) && (strcmp ("Name", key) == 0) && (!locale))
    {
      name = value;
      g_free (key);
      g_free (locale);
      continue;
    }

    /* Translation stuff 
     * Name[LC_MESSAGES] takes precedent, but
     * if only Name[LANG] is found this will 
     * be used
     */
    if ((strcmp ("Name", key) == 0) && (locale))
    {
      if (!tname_lcmessages)
      {
	if (lcmessages)
	{
	  if (strcmp (lcmessages, locale) == 0)
	  {
	    tname_lcmessages = value;
	    g_free (key);
	    g_free (locale);
	    continue;
	  }
	}

	if (lcmessages_base)
	{
	  if (strcmp (lcmessages_base, locale) == 0)
	  {
	    tname_lcmessages = value;
	    g_free (key);
	    g_free (locale);
	    continue;
	  }
	}
      }
      if (!tname_lang)
      {
	if (lang)
	{
	  if (strcmp (lang, locale) == 0)
	  {
	    tname_lang = value;
	    g_free (key);
	    g_free (locale);
	    continue;
	  }
	}

	if (lang_base)
	{
	  if (strcmp (lang_base, locale) == 0)
	  {
	    tname_lang = value;
	    g_free (key);
	    g_free (locale);
	    continue;
	  }
	}
      }
    }

    if ((etype == ITEM) && (!cmd) && (strcmp ("Exec", key) == 0))
    {
      cmd = value;
      if (key)
	g_free (key);
      if (locale)
	g_free (locale);
      continue;
    }

    if ((etype == ITEM) && (!terminal) && (strcmp ("Terminal", key) == 0))
    {
      terminal = value;
      if (key)
	g_free (key);
      if (locale)
	g_free (locale);
      continue;
    }

    /* are we there yet ? */
    if ((name) && ((tname_lcmessages) || (tname_lang)))
    {
      if (etype == SUB)
	break;
      else if ((cmd) && (terminal))
	break;
    }
  }

  fclose (fp);

  /* We now have all the information I hope */
  if (etype == SUB)
  {
    if (!(name) && !(tname_lcmessages) && !(tname_lang))
    {
      entry->name = g_strdup (g_basename (entry->dirname));
    }
    else if (tname_lcmessages)
    {
      entry->name = tname_lcmessages;
      if (tname_lang)
	g_free (tname_lang);
      if (name)
	g_free (name);
    }
    else if (tname_lang)
    {
      entry->name = tname_lang;
      if (name)
	g_free (name);
    }
    else
    {
      entry->name = name;
    }

    return entry;
  }

  if (!(name) && !(tname_lcmessages) && !(tname_lang))
    return NULL;

  if (!cmd)
    return NULL;

  if (tname_lcmessages)
  {
    entry->name = tname_lcmessages;
    if (tname_lang)
      g_free (tname_lang);
    if (name)
      g_free (name);
  }
  else if (tname_lang)
  {
    entry->name = tname_lang;
    if (name)
      g_free (name);
  }
  else
    entry->name = name;

  entry->cmd = cmd;

  if (terminal && ((strcmp (terminal, "0") == 1) || (mystrcasecmp (terminal, "true") == 0)))
    entry->term = 1;
  else
    entry->term = 0;

  return entry;
}

GList *
get_menu_entries (GList * subdirlist, GList * itemlist)
{
  GList *entries = NULL;
  char *cwd = g_get_current_dir ();

  if (!(subdirlist) && !(itemlist))
    return NULL;

  for (; subdirlist; subdirlist = g_list_next (subdirlist))
  {
    char *basename = (char *) subdirlist->data;
    char *filename = g_strconcat (cwd, "/", basename, "/.directory", NULL);
    MenuEntry *menu_entry = NULL;

    if (filename)
      menu_entry = make_menu_entry (filename, SUB);

    g_free (filename);

    if (menu_entry)
      entries = g_list_append (entries, menu_entry);
  }

  for (; itemlist; itemlist = g_list_next (itemlist))
  {
    char *filename = (char *) itemlist->data;
    MenuEntry *menu_entry = NULL;

    if (filename)
      menu_entry = make_menu_entry (filename, ITEM);

    if (menu_entry)
      entries = g_list_append (entries, menu_entry);
  }

  return entries;
}

void
free_menu_entry (MenuEntry * mentry)
{
  if (mentry)
  {
    g_free (mentry->name);
    g_free (mentry->dirname);
    g_free (mentry->cmd);
  }
}

void
free_menu_list (GList * entry_list)
{
  GList *list = entry_list;

  for (; list; list = g_list_next (list))
    free_menu_entry ((MenuEntry *) list->data);

  g_list_free (entry_list);
}

/*------------------*/

/* writing menus */

char *
get_menu_name (const char *dirname, MenuType mtype)
{
  char *name;
  char buffer[MAXSTRLEN];
  char *s;

  /* current_menu_root is a global variable */
  if (strcmp (dirname, current_menu_root) == 0)
  {
    switch (mtype)
      {
      case GNOME:
        name = g_strdup ("__gnome_menu__");
        break;
      case KDE:
        name = g_strdup ("__kde_menu__");
        break;
      case DEBIAN:
        name = g_strdup ("__debian_menu__");
        break;
      default:
        name = g_strdup ("__debian_menu__");	
      }
    return name;
  }

  s = strstr (dirname, current_menu_root);
  if (s)
    s += strlen (current_menu_root);
  else
    return NULL;

  while (*s == '/')
    s++;

  if (strlen (s) == 0)
  {
    switch (mtype)
      {
      case GNOME:
        name = g_strdup ("__gnome_menu__");
        break;
      case KDE:
        name = g_strdup ("__kde_menu__");
        break;
      case DEBIAN:
        name = g_strdup ("__debian_menu__");
        break;
      default:
        name = g_strdup ("__debian_menu__");
      }
    return name;
  }

  if (!(name = g_strdup (s)))
    return NULL;

  g_strchomp (name);
  name = g_strdelimit (name, " /", '_');

  g_snprintf (buffer, MAXSTRLEN - 1, "__%s_%s__", ((mtype == GNOME) ? "gnome" : ((mtype == KDE) ? "kde" : "debian" )), name);

  g_free (name);
  return g_strdup (buffer);
}

void
add_to_user_menu (MenuType mtype)
{
  if (mtype == KDE)
  {
    fprintf (stderr, "Loading KDE menus\n");
    SendInfo (fd, "AddToMenu \"user_menu\" KDE popup \"__kde_menu__\"\n", 0);
  }
  else if (mtype == GNOME)
  {
    fprintf (stderr, "Loading GNOME menus\n");
    SendInfo (fd, "AddToMenu \"user_menu\" GNOME popup \"__gnome_menu__\"\n", 0);
  }
  else
    {
      fprintf (stderr, "Loading DEBIAN menus\n");
      SendInfo(fd, "Read \"/etc/X11/xfce/debian.menu\" Quiet\n", 0);
      SendInfo (fd, "AddToMenu \"user_menu\" Debian popup \"__debian_menu__\"\n", 0);
    }
}

int
is_executable (char *cmd)
{
  char bin[MAXSTRLEN];
  char *end;

  if (cmd == NULL || strlen (cmd) == 0)
    return 0;

  strcpy (bin, cmd);

  /* remove command line switches */
  if ((end = strchr (bin, ' ')) != NULL)
    *end = '\0';

  if (g_path_is_absolute (bin))
  {
    if (access (bin, F_OK) == -1)
      return 0;

    else if (access (bin, X_OK) == 0)
      return 1;

    else
      return 0;
  }
  else
  {
    char *path, *dir;

    path = g_strdup (g_getenv ("PATH"));
    dir = strtok (path, ":");

    if (dir == NULL)
      return 0;

    do
    {
      path = g_strconcat (dir, "/", bin, NULL);

      if (access (path, F_OK) == 0 && access (path, X_OK) == 0)
      {
	g_free (path);
	return 1;
      }

      g_free (path);
    }
    while ((dir = strtok (NULL, ":")) != NULL);

    return 0;
  }
}

void
add_entry_to_menu (char *menu_name, MenuEntry * entry, MenuType mtype)
{
  char buffer[MAXSTRLEN];
  char buf[4096];
#ifdef HAVE_ICONV
  unsigned int utf8;
#endif /* HAVE_ICONV */

  if (!(entry) || !(entry->name))
    return;

#ifdef HAVE_ICONV
  utf8 = isUtf8(entry->name);
  if(utf8) { 
	 mymemset(buf, 0, 4096);
	 strcpy(buf, convert_code(entry->name));
	 entry->name = g_realloc (entry->name, (size_t) ((strlen (buf) + 1) * sizeof(char)));
	 strcpy(entry->name, buf);
  }
#endif /* HAVE_ICONV */

  if (entry->type == ITEM && is_executable (entry->cmd))
  {
    char *end;
    char temp[MAXSTRLEN];

    strcpy (temp, entry->cmd);

    /* Get rid of the %f, %n, %c, etc. directives
     * and remove the commandline switches just to 
     * be sure.
     */
    if ((end = strstr (temp, "%")))
    {
      *end = '\0';

      if ((end = strchr (temp, ' ')))
      {
	*end = '\0';
      }

      g_free (entry->cmd);
      entry->cmd = g_strdup (temp);
    }

    if (entry->term)
    {
      g_snprintf (buffer, MAXSTRLEN - 1, "AddToMenu \"%s\" \"%s\" Exec xfterm -e \"%s\"\n", menu_name, entry->name, entry->cmd);
    }
    else
    {
      g_snprintf (buffer, MAXSTRLEN - 1, "AddToMenu \"%s\" \"%s\" Exec %s\n", menu_name, entry->name, entry->cmd);
    }

    SendInfo (fd, buffer, 0);
    return;
  }

  if (entry->type == SUB)
  {
    char *submenu_name = get_menu_name (entry->dirname, mtype);

    if (!(submenu_name))
      return;

    /* Only add submenu if it doesn't exist somewhere else */
    if (!(menunames_list) || !(g_list_find_custom (menunames_list, submenu_name, (GCompareFunc) strcmp)))
    {
      menunames_list = g_list_append (menunames_list, g_strdup (submenu_name));

      g_snprintf (buffer, MAXSTRLEN - 1, "AddToMenu \"%s\" \"%s\" PopUp %s\n", menu_name, entry->name, submenu_name);

      SendInfo (fd, buffer, 0);
    }

    g_free (submenu_name);
    return;
  }
}

/*------------------*/

/* making the menus */

int
make_recursive_menu (const char *dirname, MenuType mtype)
{
  char *cwd;
  GList *subdirs = NULL, *items = NULL;
  GList *menu_entries = NULL;
  int n = 1;

  if (!(cwd = g_get_current_dir ()))
    return 0;

  if (chdir (dirname) != 0)
  {
    g_free (cwd);
    return 0;
  }

  subdirs = get_dir_entries (SUB);
  items = get_dir_entries (ITEM);

  menu_entries = get_menu_entries (subdirs, items);

  free_direntry_list (items);

  if (menu_entries)
  {
    GList *list = menu_entries;
    char *menu_name = get_menu_name (dirname, mtype);

    if (!menu_name)
    {
      chdir (cwd);
      g_free (cwd);
      return 0;
    }

    for (; list; list = g_list_next (list))
    {
      MenuEntry *mentry = (MenuEntry *) list->data;

      if (mentry)
	add_entry_to_menu (menu_name, mentry, mtype);
    }

    free_menu_list (menu_entries);
    g_free (menu_name);
  }

  if (subdirs)
  {
    GList *list = subdirs;

    for (; list; list = g_list_next (list))
    {
      char *subdir = (char *) list->data;
      char *fullpath = g_strconcat (dirname, "/", subdir, NULL);

      if (subdir && fullpath)
	n += make_recursive_menu (fullpath, mtype);

      g_free (fullpath);
    }

    free_direntry_list (subdirs);
  }

  chdir (cwd);
  g_free (cwd);
  return n;
}

int
xfmenu (MenuOptions options)
{
  int start, stop, i;

  /* loop parameters */
  start = (options.gnome_menu) ? 0 : 1;
  stop = (options.kde_menu) ? 1 : 0;

  /* main loop */
  for (i = start; i <= stop; i++)
  {
    MenuType mtype = (i == 0) ? GNOME : KDE;
    GList *menudirs = NULL;
    int n = 0;

    /* find menu locations */
    menudirs = get_menu_dirs (mtype);

    /* make recursive menu for each menu root */
    for (; menudirs; menudirs = g_list_next (menudirs))
    {
      char *menudir = (char *) menudirs->data;

      if (menudir)
      {
	current_menu_root = menudir;
	n += make_recursive_menu (menudir, mtype);
      }
    }

    /* add to user menu if menus were found */
    if (n)
      add_to_user_menu (mtype);
  }

  if (options.debian_menu)
    add_to_user_menu(DEBIAN);
  
  return 0;
}

/*------------------*
 * main 
 *------------------*/
int
main (int argc, char **argv)
{
  MenuOptions options;
  int n = 0;

  /* initialize communication with xfwm */
  init_xfmenu (argc, argv);

  /* parse command line options */
  options = get_options (argv);

  /* native language support */
  init_nls ();

  /* generate the menu(s) */
  n = xfmenu (options);

  /* close communication with xfwm */
  quit_xfmenu ();

  if (lcmessages)
    g_free (lcmessages);
  if (lcmessages_base)
    g_free (lcmessages_base);
  if (lang)
    g_free (lcmessages);
  if (lang_base)
    g_free (lang_base);
  if (charset)
    g_free (charset);

  return n;
}

/*------------------*
 * The End
 *------------------*/
