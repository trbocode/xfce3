/* copywrite 2001 edscott wilson garcia under GNU/GPL 
* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* probably overkill with all these includes: */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
#include "xtree_gui.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_prop.h"
#include "gtk_dnd.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "../xfsamba/tubo.h"
#include "xtree_mess.h"
#include "xtree_functions.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static void *fork_obj;
static char path[PATH_MAX + 1];
static gboolean first;


/* function to process stdout produced by child */
static int
duStdout (int n, void *data)
{
  char *line;
  char *texto;
  char *du_txt=_("Disk usage:");
  int i;
  
  if (n) return TRUE; /* this would mean binary data */
  if (!first) return TRUE; /* bug avoider, since we can only create a single instance of dlg_new at a time  */
  first=FALSE;

  line = (char *) data;

  /* tabs don't show up well in gtk-1.2 */
  for (i=0;i<strlen(line);i++) if (line[i]=='\t') line[i]=' ';
  
 
  texto = (char *) malloc(strlen(line)+strlen(du_txt)+3);
  if (!texto) return TRUE; /*unprobable memory allocation trouble */
  sprintf(texto,"%s\n%s",du_txt,line); 
  show_cat(texto);
  /*fprintf(stdout,"%s\n",line);*/
  free(texto);
  return TRUE;
}


/* function to be run by parent after child has exited
*  and all data in pipe has been read : */
static void
duForkOver (pid_t pid)
{
/*  cursor_reset (GTK_WIDGET (smb_nav));*/
  fork_obj = 0;
 /* fprintf(stderr,"forkover\n");*/
}

/* function executed by child after all pipes
*  timeouts and inputs have been set up */
static void
duFork (void)
{
/*  fprintf(stderr,"childfork: du -s %s\n",path);*/
  fflush (NULL);
  execlp ("du", "du", "-s","-b", path,(char *) 0);
  fprintf (stderr,"error: du\n");
  _exit (127);
}

/* FIXME: path should be constructed with multiple path if
 * more than one file selected, to permit simultaneous
 * diskusage reports. Watch out for memory allocation
 * problems */
void 
cb_du (GtkWidget * item, GtkCTree * ctree)
{
  GtkCTreeNode *node;
  entry *en;
  int num;

  first=TRUE;
  
	/* here a fork (tubo) to du, capturing output to the window,
	*  above cancel button will close the window and kill the
	*  du if still running. */ 
  num = count_selection (ctree, &node);
  if (!num)
  {
    cfg *win;
    win = gtk_object_get_user_data (GTK_OBJECT (ctree));
    xf_dlg_warning (win->top,_("No directory selected !"));
    return;
  }
/*    node = GTK_CLIST (ctree)->selection->data;*/
    en = gtk_ctree_node_get_row_data (ctree, node);
    
    if (!io_is_valid (en->label) || (en->type & FT_DIR_UP))
    {
      /* we do not process ".." */
      gtk_ctree_unselect (ctree, node);
      return;
    }
    /* could use a non-blocking dialog window with cancel option.
     * to terminate the fork operation here. Easy to put in if necesary */

  memcpy(path,en->path,strlen(en->path)+1);

  fork_obj = Tubo (duFork, duForkOver, TRUE, duStdout, NULL);
  return;
}

