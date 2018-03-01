/*  Xfglob: GTK+ front-end to glob, a file find utility for XFCE 
*  
*  Copyright (C) 2000-2001 Edscott Wilson García 
*
* 
*  Xfglob is pattern on Olivier Fourdan's setup.c and diagnostic.c, and
*  uses other xfce modules. It also requires command line \"glob\" 
*  (which should be included with Xfglob). Furthermore, if the content
*   of files are to be searched, it requires GNU grep.
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
*  *  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#define COPYRIGHT "\n\
Xfglob is a GTK+ front-end to command line glob.\n\
\n\
Command line glob is the file find utility for XFCE.\n\
\"Glob\" is a smaller and faster version of \"GNU find\"\n\
and it takes advantage of the powerful \"GNU grep\"\n\
to do content searching using regular expresions.\n\
\n\
Please note: if you do not have grep in your $PATH, you will\n\
not be able to search the content of files.\n\
\n\
 Copyright (C) 2000-2001 Edscott Wilson Garcia \n\
 Distributed under GNU General Public License"

/* todo: 0.5.0:
*
* move DnD
* Double-click "open with" fork (defaults to type defined at xftree?)
* help tab. ctime, mtime atime options put your name here
* xfglob.rc with combo histories
*   */

#define XFGLOB_VERSION "0.5.0"
#define FIND_HEIGHT 400
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
/* for intl macro definition, it also includes config.h: */
#include "my_intl.h"
#include "constant.h"
#include "xfce-common.h"
#include "xpmext.h"
#include "fileselect.h"

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif


#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define DEFAULT_FILTER NULL
#define DEFAULT_PATH NULL

/* maximum amount of entries in combo boxes: */
#define HISTORY_LIMIT 20

/*#define DEFAULT_FILTER "*.c"*/


#define FRAME      1
#define BOX        2
#define NOTEBOOK   3
#define SCROLLED   4
#define LABEL      5
#define ENTRY      6
#define BUTTON     7
#define SPINBUTTON 8
#define TAB_LABEL  9
#define COMBO      0

#define ADD        0
#define PACK      -1
#define SCROLL     1
#define TAB        2
#define REPACK     3
#define FPACK      4

#define GLOB "glob"
#include "icons/help.xpm"
#include "icons/xfglob_icon.xpm"
#include "icons/block_dev.xpm"
#include "icons/char_dev.xpm"
#include "icons/dir_close.xpm"
#include "icons/dir_open.xpm"
#include "icons/go_to.xpm"
#include "icons/go_down.xpm"
#include "icons/exe.xpm"
#include "icons/sexe.xpm"
#include "icons/fifo.xpm"
#include "icons/page.xpm"
#include "icons/page_lnk.xpm"
#include "icons/socket.xpm"

/* glob options */
static const RECURSIVE = 0x01;
static const MTIME = 0x02;
static const CTIME = 0x04;
static const ATIME = 0x08;
static const MINUTES = 0x10;
static const HOURS = 0x20;
static const DAYS = 0x40;
static const MONTHS = 0x80;

/* grep options */
static const CASE_SENSITIVE = 0x400;
static const REGEXP = 0x800;
static const MATCH = 0x1000;
static const WORDS = 0x2000;
static const LINES = 0x4000;
static const INVERT = 0x8000;
static const NOBINARIES = 0x10000;
static const COUNT = 0x20000;
static const XDEV = 0x40000;

static char *ftypes[9];		/* watchout: array must be same length as in initialization */
static char *ft[] = {
  "any",
  "reg",
  "dir",
  "sym",
  "sock",
  "blk",
  "chr",
  "fifo",
  NULL
};

typedef struct
{
  GtkWidget *path_entry;
  GtkWidget *filter_entry;
  GtkWidget *grep_entry;
  char *user_id;
  char *group_id;
  GtkWidget *sizeG;
  GtkWidget *sizeM;
  GtkWidget *hours;
  GtkWidget *filetype;
  GtkWidget *FindLimit;
}
_find_options;

_find_options find_options;

static GdkPixmap *gPIX_block_dev, *gPIX_char_dev, *gPIX_dir_close, *gPIX_go_to, *gPIX_go_down, *gPIX_exe, *gPIX_sexe, *gPIX_fifo, *gPIX_page, *gPIX_page_lnk, *gPIX_socket, *gPIX_help;
static GdkBitmap *gPIM_block_dev, *gPIM_char_dev, *gPIM_dir_close, *gPIM_go_to, *gPIM_go_down, *gPIM_exe, *gPIM_sexe, *gPIM_fifo, *gPIM_page, *gPIM_page_lnk, *gPIM_socket, *gPIM_help;

static GtkWidget *text, *bottom_hbox1, *bottom_hbox2, *bottomframe;
static GtkWidget *find, *SizeVbox, *TimeVbox, *UserVbox, *PermVbox, *mainframe;
static GtkAdjustment *options_adj, *find_adj;
static GtkWidget *regexpScroll, *findTree, *filterScroll, *xdevScroll, *permScroll, *userScroll, *sizeScroll, *timeScroll;
static GList *itemsP = NULL, *itemsF = NULL, *itemsG = NULL;

static gboolean considerTime = FALSE, considerSize = FALSE, considerUser = FALSE, considerPerm = FALSE, cancelled = FALSE;

static GtkWidget *cat = NULL;
static int pfd[2];		/* the pipe */
static pid_t Gpid;		/* glob pid, to be able to cancel search */
static short int findCount;	/* how many files found */
GtkCTreeNode *findNode;
static short int fileLimit = 64;
static int type = 0x0, options = 0x0;

enum
{
  TARGET_URI_LIST,
  TARGET_PLAIN,
  TARGET_STRING,
  TARGET_XTREE_WIDGET,
  TARGET_XTREE_WINDOW,
  TARGET_ROOTWIN,
  TARGETS
};

static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URI_LIST},
  {"text/plain", 0, TARGET_PLAIN},
  {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

void
node_destroy (gpointer p)
{
  int *data = (int *) p;
  free (data);
}

void
on_drag_data_get (GtkWidget * widget, GdkDragContext * context, GtkSelectionData * selection_data, guint info, guint time, gpointer data)
{
  GtkCTreeNode *node = NULL;
  GtkCTree *ctree = GTK_CTREE (widget);
  GList *selection;
  char *path[2];
  int len;
  char *dnd_data;

  if (!(GTK_CLIST (ctree)->selection))
  {
    return;
  }

  selection = GTK_CLIST (ctree)->selection;
  len = 0;
  while (selection)		/* get length for total DnD string: */
  {
    node = selection->data;
    if ((!gtk_ctree_node_get_text (ctree, node, 2, path)) || (!gtk_ctree_node_get_text (ctree, node, 1, path + 1)))
    {
      return;
    }
    if (path[0][0] == '/')
    {
      len += (strlen (path[0]) + strlen (path[1]) + 1 + 5 + 2);
    }
    selection = selection->next;
  }
  dnd_data = g_malloc (len + 1);
  dnd_data[0] = '\0';
  selection = GTK_CLIST (ctree)->selection;
  while (selection)
  {
    node = selection->data;
    if ((!gtk_ctree_node_get_text (ctree, node, 2, path)) || (!gtk_ctree_node_get_text (ctree, node, 1, path + 1)))
    {
      return;
    }
    if (path[0][0] == '/')
    {
      strcat (dnd_data, "file:");
      strcat (dnd_data, path[0]);
      strcat (dnd_data, "/");
      strcat (dnd_data, path[1]);
      strcat (dnd_data, "\r\n");
    }
    selection = selection->next;
  }
  if (!len)
    return;
  gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) dnd_data, len);
}


static void
on_drag_data_received (GtkWidget * entry, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
  char *text, *file;

  if ((data->length == 0) || (data->format != 8) || (info != TARGET_STRING))
  {
    gtk_drag_finish (context, FALSE, TRUE, time);
  }

  text = (char *) malloc (data->length + 1);
  strncpy (text, (char *) data->data, data->length);
  text[data->length] = '\0';

  if (strstr (text, "\n"))
    text = strtok (text, "\n");
  if (strncmp (text, "file:", strlen ("file:")) == 0)
  {
    file = text + strlen ("file:");
    {
      int i;
      for (i = 0; i < strlen (file); i++)
	if (file[i] == 13)
	  file[i] = 0;
    }
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (find_options.path_entry)->entry), file);
    gtk_widget_set_usize (find_options.path_entry, -1, -1);
  }
  gtk_drag_finish (context, TRUE, TRUE, time);
  free (text);
}

static gint
compare (GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
  GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
  int i, *i1, *i2;

  i1 = row1->row.data;
  i2 = row2->row.data;
  switch (clist->sort_column)
  {
  case 3:
    i = 1;
    break;
  case 4:
    i = 2;
    break;
  default:
    i = 0;
  }

  return i1[i] - i2[i];
}


static gint
on_click_column (GtkCList * clist, gint column, gpointer data)
{
  GtkCTreeNode *node;
  GList *selection;

  if ((column == 0) || (column == 3) || (column == 4))
  {
    gtk_clist_set_compare_func (GTK_CLIST (findTree), compare);
  }
  else
  {
    gtk_clist_set_compare_func (GTK_CLIST (findTree), NULL);
  }

  if (column != clist->sort_column)
    gtk_clist_set_sort_column (clist, column);
  else
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else
      clist->sort_type = GTK_SORT_ASCENDING;
  }
  selection = clist->selection;
  if (selection)
  {
    do
    {
      node = selection->data;
      if (!GTK_CTREE_ROW (node)->children || (!GTK_CTREE_ROW (node)->expanded))
      {
	node = GTK_CTREE_ROW (node)->parent;
      }
      gtk_ctree_sort_node (GTK_CTREE (clist), node);
      selection = selection->next;
    }
    while (selection);
  }
  else
  {
    gtk_clist_sort (clist);
  }
  return TRUE;
}


void
set_widget (int pack, int kind, char *name, GtkWidget * widget, GtkWidget * parent, GtkWidget * top, int border, GList * list)
{
  gtk_widget_set_name (widget, name);
  gtk_object_set_data (GTK_OBJECT (top), name, widget);
  switch (kind)
  {
  case COMBO:			/* select input box */
    if (list)
      gtk_combo_set_popdown_strings (GTK_COMBO (widget), list);
    gtk_combo_set_case_sensitive (GTK_COMBO (widget), TRUE);
    break;
  case TAB_LABEL:		/* labels for notebook tabs */
    break;
  case FRAME:			/* frames */
    gtk_container_border_width (GTK_CONTAINER (widget), border);
    gtk_frame_set_shadow_type (GTK_FRAME (widget), GTK_SHADOW_ETCHED_IN);
    break;
  case BUTTON:			/* buttons */
    /*break; */
  case BOX:			/* boxes */
    /*break;        */
  case NOTEBOOK:		/* notebooks */
    gtk_container_border_width (GTK_CONTAINER (widget), border);
    break;
  case SCROLLED:		/* scrolled windows */
    gtk_container_border_width (GTK_CONTAINER (widget), border);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    break;
  case LABEL:			/* labels */
    gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_RIGHT);
    break;
  case ENTRY:			/* entries */
    gtk_entry_set_editable (GTK_ENTRY (widget), TRUE);
    break;
  case SPINBUTTON:		/* spinbuttons */
    break;
  default:
    break;

  }

  switch (pack)
  {
  case TAB:
    set_notebook_tab (parent, border, widget);
    break;
  case ADD:			/* container add */
    gtk_container_add (GTK_CONTAINER (parent), widget);
    break;
  case SCROLL:			/* parent is scrolled */
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (parent), widget);
    gtk_container_set_focus_vadjustment (GTK_CONTAINER (widget), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (parent)));
    break;

  case PACK:
    gtk_box_pack_start (GTK_BOX (parent), widget, TRUE, TRUE, 0);
    break;
  case REPACK:
    gtk_box_pack_start (GTK_BOX (parent), widget, FALSE, TRUE, 0);
    break;
  case FPACK:
    gtk_box_pack_start (GTK_BOX (parent), widget, FALSE, FALSE, 0);
    break;
  }
  gtk_widget_show (widget);
}

gboolean process_find_messages (gpointer client_data, gint source, GdkInputCondition condition);

static void show_cat (char *message);
static void
abort_glob (GtkWidget * widget, gpointer data)
{
  if (Gpid)
  {
    kill (Gpid, SIGTERM);	/* nonagressive */
    if (data)
    {
      char *message;
      message = (char *) malloc (strlen (_("Results limit reached")) + 64);
      sprintf (message, "%s (%d)\n", _("Results limit reached"), fileLimit);
      show_cat (message);
      /*sprintf (message,"  > %d",fileLimit);
         gtk_ctree_node_set_text((GtkCTree *)findTree,findNode,0,message); */
      free (message);
    }
    else
    {
      gtk_ctree_node_set_text ((GtkCTree *) findTree, findNode, 0, _("Cancelled"));
      show_cat (_("Search canceled!\n"));
    }
    cancelled = TRUE;
    gtk_widget_hide (bottom_hbox2);
    gtk_widget_show (bottom_hbox1);
    gtk_widget_realize (find);
    cursor_reset (GTK_WIDGET (cat));
    /*Gpid = 0; */
  }
}

static void
destroy (GtkWidget * widget, gpointer data)
{
  if (Gpid)
    kill (Gpid, SIGHUP);
  gtk_main_quit ();
}

static void
on_clear_show_diag (GtkWidget * widget, gpointer data)
{
  guint lg;
  if ((Gpid) && (!cancelled))
  {
    my_show_message (_("Glob run must finish or be cancelled first."));
    return;
  }
  if (Gpid)
    kill (Gpid, SIGHUP);
  lg = gtk_text_get_length (GTK_TEXT (text));
  gtk_text_backward_delete (GTK_TEXT (text), lg);
  gtk_clist_clear ((GtkCList *) findTree);
}
static void
raiseR_cb (GtkWidget * widget, gpointer data)
{
  if (!cat)
    show_cat (_("Find results.\n"));
  if (!GTK_WIDGET_VISIBLE (cat))
    gtk_widget_show (cat);
  gdk_window_raise (gtk_widget_get_parent_window (GTK_DIALOG (cat)->vbox));
}
static void
raiseG_cb (GtkWidget * widget, gpointer data)
{
  gdk_window_raise (gtk_widget_get_parent_window (mainframe));
}


static void
on_ok_show_diag (GtkWidget * widget, gpointer data)
{
  if ((Gpid) && (!cancelled))
  {
    my_show_message (_("Glob run must finish or be cancelled first."));
    return;
  }
  if (Gpid)
    kill (Gpid, SIGHUP);
  gtk_widget_hide (GTK_WIDGET (cat));
  gdk_window_withdraw ((GTK_WIDGET (cat))->window);
}

static void
delete_event_show_diag (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  on_ok_show_diag (widget, data);
}

static GtkWidget *
icon_button (int togg, char **data, char *tip)
{
  GtkWidget *button, *pixmap;
  GtkTooltips *tooltip;

/*  button = gtk_button_new ();*/
  if (togg)
    button = gtk_toggle_button_new ();
  else
    button = gtk_button_new ();


  gtk_button_set_relief ((GtkButton *) button, GTK_RELIEF_NONE);
  gtk_widget_set_usize (button, 30, 30);
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, tip, "ContextHelp/buttons/?");
  pixmap = MyCreateFromPixmapData (button, data);
  if (pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  else
  {
    gtk_widget_show (pixmap);
    gtk_container_add (GTK_CONTAINER (button), pixmap);
  }
  return button;
}


static void
show_cat (char *message)
{
  GtkWidget *bbox, *scrolled, *button, *vpaned;

  if ((!message) || (!strlen (message)))
    return;
  if (cat != NULL)
  {
    gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
    if (!GTK_WIDGET_VISIBLE (cat))
      gtk_widget_show (cat);
    return;
  }

  cat = gtk_dialog_new ();
  gtk_container_border_width (GTK_CONTAINER (cat), 5);

  gtk_window_position (GTK_WINDOW (cat), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (cat), _("Find results..."));
  gtk_widget_realize (cat);

  /* new stuff */

  vpaned = gtk_vpaned_new ();
  {
    gtk_widget_ref (vpaned);
    gtk_object_set_data (GTK_OBJECT (cat), "vpaned1", vpaned);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->vbox), vpaned, TRUE, TRUE, 0);
    gtk_widget_set_usize (vpaned, 400, 300);
    gtk_paned_set_position (GTK_PANED (vpaned), 230);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    {
      gchar *titles[6];
      titles[0] = "";
      titles[1] = _("Name");
      titles[2] = _("Directory");
      titles[3] = _("Size (bytes)");
      titles[4] = _("Last changed");
      titles[5] = _("Mode");
      gtk_object_set_data (GTK_OBJECT (cat), "scrolled1", scrolled);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_paned_pack1 (GTK_PANED (vpaned), scrolled, TRUE, TRUE);
      findTree = gtk_ctree_new_with_titles (6, 0, titles);
      {
	gtk_clist_set_auto_sort (GTK_CLIST (findTree), FALSE);
	gtk_clist_set_shadow_type (GTK_CLIST (findTree), GTK_SHADOW_IN);
	gtk_ctree_set_line_style (GTK_CTREE (findTree), GTK_CTREE_LINES_NONE);
	gtk_ctree_set_expander_style (GTK_CTREE (findTree), GTK_CTREE_EXPANDER_CIRCULAR);
	gtk_clist_set_reorderable (GTK_CLIST (findTree), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled), findTree);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 0, TRUE);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 1, TRUE);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 2, TRUE);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 3, TRUE);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 4, TRUE);
	gtk_clist_set_column_auto_resize ((GtkCList *) findTree, 5, TRUE);
	gtk_clist_set_selection_mode (GTK_CLIST (findTree), GTK_SELECTION_EXTENDED);
	gtk_signal_connect (GTK_OBJECT (findTree), "drag_data_get", GTK_SIGNAL_FUNC (on_drag_data_get), NULL);
	gtk_signal_connect (GTK_OBJECT (findTree), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
	gtk_drag_source_set (findTree, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
      }
      gtk_widget_show (findTree);
    }
    gtk_widget_show (scrolled);
  }
  gtk_widget_show (vpaned);

  /* old stuff (but now in a pane) */

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (cat), "scrolled2", scrolled);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_paned_pack2 (GTK_PANED (vpaned), scrolled, TRUE, TRUE);
  gtk_widget_show (scrolled);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  gtk_container_add (GTK_CONTAINER (scrolled), text);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cat)->action_area), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);
  button = gtk_button_new_with_label (_("Clear"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_clear_show_diag), (gpointer) GTK_WIDGET (cat));

  button = gtk_button_new_with_label ("Xfglob");
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (raiseG_cb), NULL);


  button = gtk_button_new_with_label (_("Dismiss"));
  gtk_box_pack_end (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  /*GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT); */
  /*gtk_widget_grab_default (button); */
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok_show_diag), (gpointer) GTK_WIDGET (cat));
  gtk_signal_connect (GTK_OBJECT (cat), "delete_event", GTK_SIGNAL_FUNC (delete_event_show_diag), (gpointer) GTK_WIDGET (cat));

  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
  gtk_widget_show (text);
  set_icon (cat, _("Find results..."), xfglob_icon_xpm);
  gtk_widget_show (cat);

  return;

}

GtkWidget *
create_help (GtkWidget * parent, char *help_text)
{
  GtkWidget *scroll, *helpText;

  scroll = gtk_scrolled_window_new (NULL, NULL);
  set_widget (PACK, SCROLLED, "scrolled_window_filter", scroll, parent, find, 0, NULL);
  if (GTK_WIDGET_VISIBLE (scroll))
    gtk_widget_hide (scroll);

  helpText = gtk_label_new (help_text);
  gtk_label_set_justify (GTK_LABEL (helpText), GTK_JUSTIFY_LEFT);
  gtk_scrolled_window_add_with_viewport ((GtkScrolledWindow *) scroll, helpText);
  gtk_widget_show (helpText);
  return scroll;
}


static void
toggle_view (GtkWidget * widget, gpointer data)
{
  int i;
  static int call = 0;
  gboolean *considerWhat;
  GdkEvent event;
  GdkEventConfigure *configure;

  GtkWidget *box;
  GtkAdjustment *adjust;
  i = (int) ((long) (data));
  switch (i)
  {
  case 0:
    adjust = options_adj;
    considerWhat = &considerUser;
    break;
  case 1:
    adjust = options_adj;
    considerWhat = &considerSize;
    break;
  case 2:
    adjust = options_adj;
    considerWhat = &considerTime;
    break;
  case 3:
    adjust = find_adj;
    considerWhat = NULL;
    break;
  case 5:
    adjust = options_adj;
    considerWhat = &considerPerm;
    break;
  default:
    adjust = NULL;
    considerWhat = NULL;
    break;
  }
  switch (i)
  {
  case 0:
    box = UserVbox;
    break;
  case 1:
    box = SizeVbox;
    break;
  case 2:
    box = TimeVbox;
    break;
  case 3:
    box = regexpScroll;
    break;
  case 4:
    box = filterScroll;
    break;
  case 5:
    box = PermVbox;
    break;
  case 6:
    box = permScroll;
    break;
  case 7:
    box = userScroll;
    break;
  case 8:
    box = sizeScroll;
    break;
  case 9:
    box = timeScroll;
    break;
  case 10:
    box = xdevScroll;
    break;
  default:
    return;
  }
  if (GTK_WIDGET_VISIBLE (box))
  {
    gtk_widget_hide (box);
    if (considerWhat)
      *considerWhat = FALSE;
  }
  else
  {
    gtk_widget_show (box);
    if (considerWhat)
      *considerWhat = TRUE;
  }
  /* GTK+ bug workaround (mostly): */
  if (adjust)
  {
    gtk_adjustment_set_value (adjust, FIND_HEIGHT);
    gtk_adjustment_value_changed (adjust);
  }
  configure = (GdkEventConfigure *) (&event);
  event.type = GDK_CONFIGURE;
  configure->width = find->allocation.width;
  configure->height = find->allocation.height;
  if ((call++) % 2)
    configure->height++;
  else
    configure->height--;
  gtk_propagate_event (find, &event);

}

static void
toggle_option (GtkWidget * widget, gpointer data)
{
  int *flag;
  flag = (int *) data;
  options ^= (*flag);
}
static void
toggle_perm (GtkWidget * widget, gpointer data)
{
  int flag;
  flag = (int) ((long) data);
  type ^= (flag & 07777);
}

static void
make_toggle (gchar * label, gchar * name, GtkWidget * parent, const *flag, gboolean state)
{
  GtkWidget *button;
  button = gtk_check_button_new_with_label (label);
  set_widget (PACK, BUTTON, name, button, parent, find, 2, NULL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_option), (gpointer) flag);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), state);
  if (state)
    options |= *flag;

}

#define RADIO_WIPE 0xfffffff1
static void
toggle_radio1 (GtkWidget * widget, gpointer data)
{
  int *flag;
  flag = (int *) data;
  options &= RADIO_WIPE;
  options |= (*flag);
/*	printf("option=0x%x\n",options);*/
}

#define RADIO_WIPE2 0xffff0fff
static void
toggle_radio2 (GtkWidget * widget, gpointer data)
{
  int *flag;
  flag = (int *) data;
  options &= RADIO_WIPE2;
  options |= (*flag);
/*	printf("option=0x%x\n",options);*/
}

#define RADIO_WIPE3 0xffffff0f
static void
toggle_radio3 (GtkWidget * widget, gpointer data)
{
  int *flag;
  flag = (int *) data;
  options &= RADIO_WIPE3;
  options |= (*flag);
}


static void
make_radio1 (gchar * label, gchar * name, GtkWidget * parent, const *flag)
{
  static GSList *radio1 = NULL;
  GtkWidget *button;
  button = gtk_radio_button_new_with_label (radio1, label);
  radio1 = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_radio1), (gpointer) flag);
  set_widget (PACK, BUTTON, name, button, parent, find, 2, NULL);
}
static void
make_radio2 (gchar * label, gchar * name, GtkWidget * parent, const *flag)
{
  static GSList *radio2 = NULL;
  GtkWidget *button;
  button = gtk_radio_button_new_with_label (radio2, label);
  radio2 = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_radio2), (gpointer) flag);
  set_widget (PACK, BUTTON, name, button, parent, find, 2, NULL);
}
static void
make_radio3 (gchar * label, gchar * name, GtkWidget * parent, const *flag)
{
  static GSList *radio3 = NULL;
  GtkWidget *button;
  button = gtk_radio_button_new_with_label (radio3, label);
  radio3 = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_radio3), (gpointer) flag);
  set_widget (PACK, BUTTON, name, button, parent, find, 2, NULL);
}

/* this MAX_ARG is ugly and should be fixed with malloc() and free() */
#define MAX_ARG 50

static char *
strip (char *s)
{
  int i, j;
  while ((s[0] == ' ') || (s[0] == '\t'))
    s++;
  j = strlen (s) - 1;
  for (i = j; i >= 0; i--)
  {
    if ((s[i] == ' ') || (s[i] == '\t'))
      s[i] = (char) 0;
    else
      break;
  }
  return s;
}

gint itemcompare (gconstpointer a, gconstpointer b)
{
  return strcmp ((char *) a, (char *) b);
}

gchar *
get_combo (GtkCombo * combo)
{
  gchar *s, *r;
  s = strip (gtk_entry_get_text (GTK_ENTRY (combo->entry)));
  r = (gchar *) malloc (strlen (s) + 1);
  strcpy (r, s);
  return r;
}

GList *
put_item (GList * items, gchar * r, GtkCombo * combo)
{
  GList *itemsR;
  if (!g_list_find_custom (items, r, itemcompare))
  {
    if (g_list_length (items) >= HISTORY_LIMIT)
    {
      GList *last;
      gchar *r;
      last = g_list_last (items);
      r = last->data;
      items = g_list_remove (items, last->data);
      free (r);
    }
    itemsR = g_list_insert (items, r, 0);
    gtk_combo_set_popdown_strings (combo, itemsR);
    return itemsR;
  }
  else
    return items;
}

static gint
GlobWait (gpointer data)
{
  int status;
  int childPID;
  childPID = (int) ((long) data);

/*  fprintf(stderr,"waiting\n");*/

  waitpid (childPID, &status, WNOHANG);
  if (WIFEXITED (status))
  {
/*  fprintf(stderr,"waiting done\n");*/
    return FALSE;
  }
  return TRUE;
}

static void
find_ok_cb (GtkWidget * widget, gpointer data)
{
  char *argument[MAX_ARG];
  char sizeG_s[64], sizeM_s[64], hours_s[64], permS[64];
  gchar *path, *filter, *token, *s;
  int i, j, sizeG, sizeM, hours;
  int childPID;

  cancelled = FALSE;
  if (Gpid)
  {
    kill (Gpid, SIGHUP);
    Gpid = 0;
  }
  findCount = 0;
/* get the parameters set by the user... *****/

/* limit */
  fileLimit = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (find_options.FindLimit));

/* the rest */

  path = get_combo ((GtkCombo *) (find_options.path_entry));
  if (strlen (path) == 0)
    path = "/";
  /* tilde expansion */
  if (path[strlen (path) - 1] == '~')
    path = "~/";
  /* environment variables */
  if (path[0] == '$')
  {
    path = getenv (path + 1);
    if (path == NULL)
      path = "/";
  }

  itemsP = put_item (itemsP, path, (GtkCombo *) (find_options.path_entry));

  filter = get_combo ((GtkCombo *) (find_options.filter_entry));
  itemsF = put_item (itemsF, filter, (GtkCombo *) (find_options.filter_entry));

  token = get_combo ((GtkCombo *) (find_options.grep_entry));
  itemsG = put_item (itemsG, token, (GtkCombo *) (find_options.grep_entry));

  /* spinbuttons: */
  if (considerSize)
  {
    sizeG = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (find_options.sizeG));
    sizeM = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (find_options.sizeM));
    if ((sizeM <= sizeG) && (sizeM > 0))
    {
      my_show_message (_("Incoherent size considerations!"));
      /* fprintf(stderr,"xfglob: this should never happen\n"); */
      return;
    }

  }
  else
    sizeG = sizeM = 0;
  if (considerTime)
  {
    hours = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (find_options.hours));
  }
  else
    hours = 0;
  /* select list */
  s = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (find_options.filetype)->entry));
  for (j = -1, i = 0; ftypes[i] != NULL; i++)
  {
    if (strcmp (s, ftypes[i]) == 0)
    {
      j = i;
      break;
    }
  }
  if (j < 0)
    s = ftypes[0];
  i = 0;
  argument[i++] = GLOB;

  /*argument[i++] = "-v"; (verbose output from glob for debugging) */
  argument[i++] = "-P";
  if (options & NOBINARIES)
    argument[i++] = "-I";

  if (options & RECURSIVE)
    argument[i++] = "-r";
/* obsolete: replace with -o option
  if ((options & EXE) || (options & SUID))
    {
      argument[i++] = "-p";
      if (options & EXE)
	argument[i++] = "exe";
      if (options & SUID)
	argument[i++] = "suid";
    }
    */
  if (considerPerm)
  {
    argument[i++] = "-o";
    sprintf (permS, "0%o", type & 07777);
    argument[i++] = permS;
  }
  if (!(options & CASE_SENSITIVE))
    argument[i++] = "-i";
  if (options & COUNT)
    argument[i++] = "-c";
  if (options & INVERT)
    argument[i++] = "-L";

  if (options & WORDS)
    argument[i++] = "-w";
  else
  {
    if (options & LINES)
      argument[i++] = "-x";
  }
  if (j > 0)
  {
    argument[i++] = "-t";
    argument[i++] = ft[j];
  }

  if (considerTime)
  {
    if (options & MTIME)
      argument[i++] = "-M";
    if (options & ATIME)
      argument[i++] = "-A";
    if (options & CTIME)
      argument[i++] = "-C";
    if (hours > 0)
    {
      if (options & MINUTES)
	argument[i++] = "-k";
      if (options & HOURS)
	argument[i++] = "-h";
      if (options & DAYS)
	argument[i++] = "-d";
      if (options & MONTHS)
	argument[i++] = "-m";

      sprintf (hours_s, "%d", hours);
      argument[i++] = hours_s;
    }
  }

  if (considerSize)
  {
    if (sizeG > 0)
    {
      argument[i++] = "-s";
      sprintf (sizeG_s, "+%d", sizeG);
      argument[i++] = sizeG_s;
    }
    if (sizeM > 0)
    {
      argument[i++] = "-s";
      sprintf (sizeM_s, "-%d", sizeM);
      argument[i++] = sizeM_s;
    }
  }
  if (options & XDEV)
    argument[i++] = "-a";

  if (considerUser)
  {
    if (find_options.user_id)
    {
      argument[i++] = "-u";
      argument[i++] = find_options.user_id;
    }
    if (find_options.group_id)
    {
      argument[i++] = "-g";
      argument[i++] = find_options.group_id;
    }
  }

  /* don't apply filter if not specified and path is absolute!! */
  if (strlen (filter) > 0)
  {
    argument[i++] = "-f";
    argument[i++] = filter;
  }
  else
  {
    if (path[strlen (path) - 1] == '/')
    {
      argument[i++] = "-f";
      argument[i++] = "*";
    }
    else
    {
      struct stat st;
      if (stat (path, &st) == 0)
      {
	if (S_ISDIR (st.st_mode))
	{
	  argument[i++] = "-f";
	  argument[i++] = "*";
	}
      }
    }
  }

  if (strlen (token) > 0)
  {
    if (options & REGEXP)
      argument[i++] = "-E";
    else
      argument[i++] = "-e";
    argument[i++] = token;
  }


  /* last argument must be the path */
  argument[i++] = path;
  argument[i] = (char *) 0;
  /*for (j=0;j<i;j++) printf ("%s ",argument[j]);printf ("\n"); */

  show_cat (_("Result of "));
  /* raising after first show_cat */
  gtk_ctree_collapse_recursive ((GtkCTree *) findTree, NULL);
  gdk_window_raise (gtk_widget_get_parent_window (GTK_DIALOG (cat)->vbox));
  cursor_wait (GTK_WIDGET (cat));
  for (j = 0; j < i; j++)
  {
    show_cat (argument[j]);
    show_cat (" ");
  }
  show_cat (":\n---");
  show_cat (_("File type :"));
  show_cat (s);
  show_cat ("------------------\n");

  Gpid = 0;
  childPID = fork ();
  if (!childPID)
  {
    dup2 (pfd[1], 1);		/* assign child stdout to pipe */
    close (pfd[0]);		/* not used by child */
    execvp (GLOB, argument);
    perror ("exec");
    _exit (127);		/* child never get here */
  }
  gtk_timeout_add (2080, (GtkFunction) GlobWait, (gpointer) ((long) childPID));
  {
    char command[128];
    char *textos[6];
    strcpy (command, argument[0]);
    for (j = 1; j < i; j++)
    {
      strcat (command, " ");
      strcat (command, argument[j]);
    }
    /*textos[1] = command; */
    if (strlen (token))
      textos[0] = token;
    else
      textos[0] = "";
    if (strlen (filter))
      textos[1] = filter;
    else
      textos[1] = "";
    if (strlen (path))
      textos[2] = path;
    else
      textos[2] = "";
    textos[3] = textos[4] = textos[5] = "";
    findNode = gtk_ctree_insert_node ((GtkCTree *) findTree, NULL, NULL, textos, 3, gPIX_go_to, gPIM_go_to, gPIX_go_down, gPIM_go_down, FALSE, TRUE);	/* isleaf,expand */
    {
      int *data;
      data = (int *) malloc (3 * sizeof (int));
      data[0] = data[1] = data[2] = 0;
      gtk_ctree_node_set_row_data_full ((GtkCTree *) findTree, findNode, data, node_destroy);
    }

    /*gtk_clist_set_selectable((GtkCList *)findTree,GTK_CTREE_ROW(findNode),FALSE); */
  }





}

void
cb_user (GtkWidget * widget, gpointer data)
{
  find_options.user_id = (char *) (data);
}

void
cb_group (GtkWidget * widget, gpointer data)
{
  find_options.group_id = (char *) (data);
}

void
jam (char *file, GtkOptionMenu * optmen, gpointer func)
{
  FILE *archie;
  char line[256];
  char *s, *r, *t, *initial = _("Anyone");
  GtkWidget *optionmenu, *menuitem;

  archie = fopen (file, "r");
  if (archie == NULL)
    return;

  optionmenu = gtk_menu_new ();
  menuitem = gtk_menu_item_new_with_label (initial);
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (optionmenu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), NULL);

  while (!feof (archie) && (fgets (line, 255, archie)))
  {
    if (feof (archie))
      break;
    line[255] = 0;
    if ((line[0] == '#') || (strchr (line, ':') == NULL))
      continue;
    r = strtok (line, ":");
    if (!r)
      continue;
    s = strchr (r + strlen (r) + 1, ':') + 1;
    if (!s)
      continue;
    s = strtok (s, ":");
    if (!s)
      continue;
    t = (char *) malloc (strlen (s) + 1);
    strcpy (t, s);
    menuitem = gtk_menu_item_new_with_label (r);
    gtk_widget_show (menuitem);
    gtk_menu_append (GTK_MENU (optionmenu), menuitem);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), (gpointer) t);
  }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optmen), optionmenu);
  fclose (archie);
  return;
}

static void
get_path (GtkWidget * widget, gpointer data)
{
  char *fileS;
  fileS = open_fileselect (NULL);
  if (fileS)
  {
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (find_options.path_entry)->entry), fileS);
    gtk_widget_set_usize (find_options.path_entry, -1, -1);

  }
  return;
}

#define X_PAD 8

GtkWidget *
create_find (char *path)
{
  GtkAccelGroup *accel_group;
  GtkObject *Fa;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *note_vbox;
  GtkWidget *find_notebook;
  GtkWidget *scrolled_window;
  GtkWidget *page_vbox;
  GtkWidget *upframe, *table, *perm[15];
  GtkWidget *vbox;
  GtkWidget *hbox1, *hbox2, *label;
  GList *list_ftypes = NULL;
  int i;

  if (path == NULL)
    path = DEFAULT_PATH;
  /* translation of filetypes */
  ftypes[0] = _("Any kind"), ftypes[1] = _("Regular"), ftypes[2] = _("Directory"), ftypes[3] = _("Symlink"), ftypes[4] = _("Socket"), ftypes[5] = _("Block device"), ftypes[6] = _("Character device"), ftypes[7] = _("FIFO"), ftypes[8] = NULL;
  /* Initialization of filetypes */
  i = 0;
  while (ftypes[i])
    list_ftypes = g_list_append (list_ftypes, ftypes[i++]);

/* first circle : main window  */
  find = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (find, "Xfglob");
  gtk_object_set_data (GTK_OBJECT (find), "Xfglob", find);
  gtk_window_set_title (GTK_WINDOW (find), _("Xfglob"));
  gtk_window_position (GTK_WINDOW (find), GTK_WIN_POS_MOUSE);
  gtk_widget_set_usize (find, -2, FIND_HEIGHT);
  gtk_window_set_policy (GTK_WINDOW (find), FALSE, TRUE, FALSE);
  gtk_widget_realize (find);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (find), accel_group);
  gPIX_go_down = MyCreateGdkPixmapFromData (go_down_xpm, find, &gPIM_go_down, FALSE);
  gPIX_go_to = MyCreateGdkPixmapFromData (go_to_xpm, find, &gPIM_go_to, FALSE);
  gPIX_block_dev = MyCreateGdkPixmapFromData (block_dev_xpm, find, &gPIM_block_dev, FALSE);
  gPIX_char_dev = MyCreateGdkPixmapFromData (char_dev_xpm, find, &gPIM_char_dev, FALSE);
  gPIX_dir_close = MyCreateGdkPixmapFromData (dir_close_xpm, find, &gPIM_dir_close, FALSE);
  gPIX_exe = MyCreateGdkPixmapFromData (exe_xpm, find, &gPIM_exe, FALSE);
  gPIX_sexe = MyCreateGdkPixmapFromData (sexe_xpm, find, &gPIM_sexe, FALSE);
  gPIX_fifo = MyCreateGdkPixmapFromData (fifo_xpm, find, &gPIM_fifo, FALSE);
  gPIX_page = MyCreateGdkPixmapFromData (page_xpm, find, &gPIM_page, FALSE);
  gPIX_page_lnk = MyCreateGdkPixmapFromData (page_lnk_xpm, find, &gPIM_page_lnk, FALSE);
  gPIX_socket = MyCreateGdkPixmapFromData (socket_xpm, find, &gPIM_socket, FALSE);


  gPIX_help = MyCreateGdkPixmapFromData (help_xpm, find, &gPIM_help, FALSE);
/* second circle: frame */
  {
    mainframe = gtk_frame_new (NULL);
    set_widget (ADD, FRAME, "mainframe", mainframe, find, find, 0, NULL);
#ifdef OLD_STYLE
    gtk_frame_set_shadow_type (GTK_FRAME (mainframe), GTK_SHADOW_NONE);
#else
    gtk_frame_set_shadow_type (GTK_FRAME (mainframe), GTK_SHADOW_OUT);
#endif
/* third circle: vbox */
    {
      note_vbox = gtk_vbox_new (FALSE, 0);
      set_widget (ADD, BOX, "note_vbox", note_vbox, mainframe, find, 5, NULL);
/* fourth circle: notebook*/
      {
	find_notebook = gtk_notebook_new ();
	set_widget (ADD, NOTEBOOK, "find_notebook", find_notebook, note_vbox, find, 0, NULL);

/* first notebook page *******************/
/* fifth circle : scrolled window */
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  set_widget (ADD, SCROLLED, "scrolled_window1", scrolled_window, find_notebook, find, 5, NULL);
	  find_adj = gtk_scrolled_window_get_vadjustment ((GtkScrolledWindow *) scrolled_window);
/* sixth circle : vbox */
	  {
	    page_vbox = gtk_vbox_new (FALSE, 0);
	    set_widget (SCROLL, BOX, "page_vbox", page_vbox, scrolled_window, find, 5, NULL);
/* seventh circle : frame  */
	    {
	      upframe = gtk_frame_new (_("Find"));
	      set_widget (PACK, FRAME, "find_upframe", upframe, page_vbox, find, 5, NULL);
/* eigth circle : vbox  */
	      {
		vbox = gtk_vbox_new (FALSE, 0);
		set_widget (ADD, BOX, "find_vbox", vbox, upframe, find, 5, NULL);
/* ninth circle : hboxes */
		{
/* path box */
		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (ADD, BOX, "path_hbox", hbox1, vbox, find, 5, NULL);

		  label = gtk_label_new (_("Path : "));
		  set_widget (PACK, LABEL, "path_label", label, hbox1, find, 5, NULL);

		  find_options.path_entry = gtk_combo_new ();
		  set_widget (PACK, COMBO, "find_options.path_entry", find_options.path_entry, hbox1, find, 5, NULL);
		  if (path)
		    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (find_options.path_entry)->entry), path);
		  gtk_combo_disable_activate ((GtkCombo *) find_options.path_entry);
		  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (find_options.path_entry)->entry), "activate", GTK_SIGNAL_FUNC (find_ok_cb), NULL);
		  button = icon_button (0, dir_open_xpm, _("Browse"));
		  gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
		  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (get_path), NULL);
		  gtk_widget_show (button);

/* Filter box : */
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (ADD, BOX, "filter_hbox", hbox2, vbox, find, 5, NULL);

		  label = gtk_label_new (_("File filter : "));
		  set_widget (PACK, LABEL, "filter_label", label, hbox2, find, 5, NULL);

		  find_options.filter_entry = gtk_combo_new ();
		  set_widget (PACK, COMBO, "find_options.filter_entry", find_options.filter_entry, hbox2, find, 5, NULL);
		  gtk_combo_disable_activate ((GtkCombo *) find_options.filter_entry);
		  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (find_options.filter_entry)->entry), "activate", GTK_SIGNAL_FUNC (find_ok_cb), NULL);

		  button = icon_button (1, help_xpm, _("What's a file filter?"));
		  gtk_box_pack_end (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
		  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 4));
		  gtk_widget_show (button);

		  if (DEFAULT_FILTER)
		    gtk_entry_set_text (GTK_ENTRY (find_options.filter_entry), DEFAULT_FILTER);
		  filterScroll = create_help (vbox, _("Basic rules:\n" "\n" "*  Will match any character zero or more times.\n" "?  Will match any character exactly one time\n"));


		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "recursive_hbox2", hbox2, vbox, find, 5, NULL);

		  make_toggle (_("Search subdirectories too"), "find_options.recursive", hbox2, &RECURSIVE, TRUE);

		  label = gtk_label_new (_("Limit results to first : "));
		  set_widget (PACK, LABEL, "limit_label", label, hbox2, find, 5, NULL);

		  Fa = gtk_adjustment_new (64, 1, 1024, 1, 64, 64);
		  find_options.FindLimit = gtk_spin_button_new (GTK_ADJUSTMENT (Fa), 64, 0);
		  gtk_widget_set_usize (find_options.FindLimit, -2, -2);
		  set_widget (PACK, SPINBUTTON, "find_options.FindLimit", find_options.FindLimit, hbox2, find, 5, NULL);
/****/
		}		/* end - circle 9 */
	      }			/* end - circle 8 */
	    }			/* end - circle 7 */
/* seventh circle : frame */
	    {
	      upframe = gtk_frame_new (_("Content search"));
	      set_widget (PACK, FRAME, "grep_frame", upframe, page_vbox, find, 5, NULL);
/* eighth circle : vbox  */
	      {
		vbox = gtk_vbox_new (FALSE, 0);
		set_widget (ADD, BOX, "grep_vbox", vbox, upframe, find, 5, NULL);
/* ninth circle : hboxes */
		{
/* grep_hbox1 */
		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "grep_hbox1", hbox1, vbox, find, 5, NULL);

		  label = gtk_label_new (_("Containing : "));
		  set_widget (PACK, LABEL, "grep_label", label, hbox1, find, 5, NULL);

		  find_options.grep_entry = gtk_combo_new ();
		  set_widget (PACK, COMBO, "find_options.grep_entry", find_options.grep_entry, hbox1, find, 5, NULL);
		  gtk_combo_disable_activate ((GtkCombo *) find_options.grep_entry);
		  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (find_options.grep_entry)->entry), "activate", GTK_SIGNAL_FUNC (find_ok_cb), NULL);
/* grep_hbox2 */
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "grep_hbox2", hbox2, vbox, find, 5, NULL);

		  make_toggle (_("Case sensitive search"), "find_options.casesense", hbox2, &CASE_SENSITIVE, FALSE);
		  make_toggle (_("Extended regexp"), "find_options.regexp", hbox2, &REGEXP, FALSE);


		  button = icon_button (1, help_xpm, _("What's a regexp?"));
		  gtk_box_pack_end (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
		  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 3));
		  gtk_widget_show (button);
		  regexpScroll =
		    create_help (vbox,
				 _("Reserved characters for extended regexp are\n" ". ^ $ [ ] ? * + { } | \\ ( ) : \n" "In  basic regular expressions the metacharacters\n" "?, +, {, |, (, and ) lose their special meaning.\n" "\n" "The  period  .   matches  any  single  character.\n" "The caret ^ matches at the start of line.\n" "The dollar $ matches at the end of line.\n" "\n" "Characters within [ ] matches any single \n" "       character in the list.\n" "Characters within [^ ] matches any single\n" "       character *not* in the list.\n" "Characters inside [ - ] matches a range of\n" "       characters (ie [0-9] or [a-z]).\n" "\n" "A regular expression may be followed by one\n" "       of several repetition operators:\n" "?      The preceding item is optional and matched\n" "       at most once.\n" "*      The preceding item will be matched zero\n" "       or more times.\n" "+      The preceding item will be matched one or\n" "       more times.\n" "{n}    The preceding item is matched exactly n times.\n"
				    "{n,}   The preceding item is matched n or more times.\n" "{n,m}  The preceding item is matched at least n times,\n" "       but not more than m times.\n" "\n" "To match any reserved character, precede it with \\. \n" "\n" "Two regular expressions may be joined by the logical or\n" "       operator |.\n" "Two regular expressions may be concatenated.\n" "\n" "More information is available by typing \"man grep\"\n" "       at the command prompt.\n"));

		}		/* end - circle 9 */
	      }			/* end - circle 8 */
	    }			/* end - circle 7 */
	  }			/* end - circle 6 */
	}			/* end - circle 5 */

/* second notebook page *********************/
/* fifth circle : scrolled window */
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  set_widget (ADD, SCROLLED, "scrolled_window2", scrolled_window, find_notebook, find, 5, NULL);
	  options_adj = gtk_scrolled_window_get_vadjustment ((GtkScrolledWindow *) scrolled_window);
/* sixth circle : vbox */
	  {
	    page_vbox = gtk_vbox_new (FALSE, 0);
	    set_widget (SCROLL, BOX, "page_vbox2", page_vbox, scrolled_window, find, 5, NULL);
/* seventh circle : frame  */
	    {
	      upframe = gtk_frame_new (_("File considerations"));
	      set_widget (PACK, FRAME, "upframe3", upframe, page_vbox, find, 5, NULL);
/* eigth circle : vbox  */
	      {
		vbox = gtk_vbox_new (FALSE, 0);
		set_widget (ADD, BOX, "vbox3", vbox, upframe, find, 5, NULL);
/* ninth circle : hboxes */
		{

/* file-type box */
		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox5", hbox1, vbox, find, 5, NULL);

		  label = gtk_label_new (_("File type :"));
		  set_widget (PACK, LABEL, "Ftype_label", label, hbox1, find, 5, NULL);

		  find_options.filetype = gtk_combo_new ();
		  set_widget (PACK, COMBO, "Ftypes", find_options.filetype, hbox1, find, 5, list_ftypes);
		  g_list_free (list_ftypes);

/* Permissions box */
/* -xdev box */
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox6b", hbox2, vbox, find, 5, NULL);
		  make_toggle (_("Stay on single filesystem"), "find_options.xdev", hbox2, &XDEV, FALSE);
		  button = icon_button (1, help_xpm, _("What's a single filesystem?"));
		  gtk_box_pack_end (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
		  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 10));
		  gtk_widget_show (button);
		  xdevScroll = create_help (vbox, _("Staying on a single filesystem will hinder\n" "searching on different physical disk partitions.\n" "This is especially useful if you do not want\n" "to search into nfs or smb mounts.\n"));




		}		/* end - circle 9 */
	      }			/* end - circle 8 */
	    }			/* end - circle 7 */
/* seventh circle : frame  */
	    {
	      /*upframe = gtk_frame_new (_("Size considerations")); */
	      upframe = gtk_frame_new (NULL);
	      set_widget (PACK, FRAME, "upframe1", upframe, page_vbox, find, 5, NULL);

/* eigth circle : vbox  */
	      {
		vbox = gtk_vbox_new (FALSE, 0);
		set_widget (ADD, BOX, "vbox1", vbox, upframe, find, 5, NULL);

/* perm stuff */
		hbox = gtk_hbox_new (FALSE, 0);
		set_widget (ADD, BOX, "hbox", hbox, vbox, find, 5, NULL);
		button = gtk_check_button_new_with_label (_("Permission considerations"));
		set_widget (PACK, BUTTON, "toggleSize", button, hbox, find, 0, NULL);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 5));

		button = icon_button (1, help_xpm, _("How does permission filtering work?"));
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 6));
		gtk_widget_show (button);
		permScroll = create_help (vbox, _("When permission filtering is active:\n" "-If a file has one of the chosen permission bits,\n" "  it will match.\n" "-If no permission bit is chosen, files with \n" "  no set permission bits will match.\n"));

		PermVbox = gtk_vbox_new (FALSE, 0);
		set_widget (PACK, BOX, "vbox2x", PermVbox, vbox, find, 0, NULL);
/* ninth circle : hboxes */
		{
/*  */
		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox1y", hbox1, PermVbox, find, 5, NULL);

		  /*************/
		  {
		    table = gtk_table_new (3, 5, FALSE);
		    gtk_box_pack_start (GTK_BOX (hbox1), table, TRUE, TRUE, 0);
		    perm[0] = gtk_label_new (_("Owner :"));
		    perm[1] = gtk_check_button_new_with_label (_("Read"));
		    if (type & S_IRUSR)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[1]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[1]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IRUSR));
		    perm[2] = gtk_check_button_new_with_label (_("Write"));
		    if (type & S_IWUSR)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[2]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[2]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IWUSR));
		    perm[3] = gtk_check_button_new_with_label (_("Execute"));
		    if (type & S_IXUSR)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[3]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[3]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IXUSR));
		    perm[4] = gtk_check_button_new_with_label (_("Set UID"));
		    if (type & S_ISUID)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[4]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[4]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_ISUID));

		    gtk_table_attach (GTK_TABLE (table), perm[0], 0, 1, 0, 1, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[1], 1, 2, 0, 1, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[2], 2, 3, 0, 1, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[3], 3, 4, 0, 1, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[4], 4, 5, 0, 1, 0, 0, X_PAD, 0);

		    perm[5] = gtk_label_new (_("Group :"));
		    perm[6] = gtk_check_button_new_with_label (_("Read"));
		    if (type & S_IRGRP)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[6]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[6]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IRGRP));
		    perm[7] = gtk_check_button_new_with_label (_("Write"));
		    if (type & S_IWGRP)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[7]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[7]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IWGRP));
		    perm[8] = gtk_check_button_new_with_label (_("Execute"));
		    if (type & S_IXGRP)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[8]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[8]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IXGRP));
		    perm[9] = gtk_check_button_new_with_label (_("Set GID"));
		    if (type & S_ISGID)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[9]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[9]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_ISGID));
		    gtk_table_attach (GTK_TABLE (table), perm[5], 0, 1, 1, 2, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[6], 1, 2, 1, 2, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[7], 2, 3, 1, 2, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[8], 3, 4, 1, 2, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[9], 4, 5, 1, 2, 0, 0, X_PAD, 0);

		    perm[10] = gtk_label_new (_("Other :"));
		    perm[11] = gtk_check_button_new_with_label (_("Read"));
		    if (type & S_IROTH)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[11]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[11]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IROTH));
		    perm[12] = gtk_check_button_new_with_label (_("Write"));
		    if (type & S_IWOTH)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[12]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[12]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IWOTH));
		    perm[13] = gtk_check_button_new_with_label (_("Execute"));
		    if (type & S_IXOTH)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[13]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[13]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_IXOTH));
		    perm[14] = gtk_check_button_new_with_label (_("Sticky"));
		    if (type & S_ISVTX)
		      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[14]), 1);
		    gtk_signal_connect (GTK_OBJECT (perm[14]), "clicked", GTK_SIGNAL_FUNC (toggle_perm), (gpointer) ((long) S_ISVTX));
		    gtk_table_attach (GTK_TABLE (table), perm[10], 0, 1, 2, 3, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[11], 1, 2, 2, 3, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[12], 2, 3, 2, 3, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[13], 3, 4, 2, 3, 0, 0, X_PAD, 0);
		    gtk_table_attach (GTK_TABLE (table), perm[14], 4, 5, 2, 3, 0, 0, X_PAD, 0);
		    gtk_widget_show_all (hbox1);

		  }

		 /*************/
		}		/* end - circle 9 */
		if (!considerPerm && (GTK_WIDGET_VISIBLE (PermVbox)))
		  gtk_widget_hide (PermVbox);

/* user/group stuff */

		hbox = gtk_hbox_new (FALSE, 0);
		set_widget (ADD, BOX, "hbox", hbox, vbox, find, 5, NULL);
		button = gtk_check_button_new_with_label (_("User/group considerations"));
		set_widget (PACK, BUTTON, "toggleSize", button, hbox, find, 0, NULL);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 0));

		button = icon_button (1, help_xpm, _("How does user/group filtering work?"));
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 7));
		gtk_widget_show (button);
		userScroll = create_help (vbox, _("If user and group are both set to any \n" "value other than anyone:\n" "-files must meet both criteria in order\n" "  to match\n"));


		UserVbox = gtk_vbox_new (FALSE, 0);
		set_widget (PACK, BOX, "vbox2", UserVbox, vbox, find, 0, NULL);
/* ninth circle : hboxes */
		{
/* user box */
		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox1", hbox1, UserVbox, find, 5, NULL);

		  label = gtk_label_new (_("User ID: "));
		  set_widget (PACK, LABEL, "U_label", label, hbox1, find, 5, NULL);

		  button = gtk_option_menu_new ();
		  set_widget (PACK, BUTTON, "find_options.user_id", button, hbox1, find, 5, NULL);
		  jam ("/etc/passwd", (GtkOptionMenu *) button, GTK_SIGNAL_FUNC (cb_user));
		  find_options.user_id = NULL;

		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox2", hbox2, UserVbox, find, 5, NULL);

		  label = gtk_label_new (_("Group ID: "));
		  set_widget (PACK, LABEL, "G_label", label, hbox2, find, 5, NULL);
		  button = gtk_option_menu_new ();
		  set_widget (PACK, BUTTON, "find_options.group_id", button, hbox2, find, 5, NULL);
		  jam ("/etc/group", (GtkOptionMenu *) button, GTK_SIGNAL_FUNC (cb_group));
		  find_options.group_id = NULL;

		}		/* end - circle 9 */
		if (!considerUser && (GTK_WIDGET_VISIBLE (UserVbox)))
		  gtk_widget_hide (UserVbox);

/* size stuff */

		hbox = gtk_hbox_new (FALSE, 0);
		set_widget (ADD, BOX, "hbox", hbox, vbox, find, 5, NULL);
		button = gtk_check_button_new_with_label (_("Size considerations"));
		set_widget (PACK, BUTTON, "toggleSize", button, hbox, find, 0, NULL);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 1));

		button = icon_button (1, help_xpm, _("How does size filtering work?"));
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 8));
		gtk_widget_show (button);
		sizeScroll = create_help (vbox, _("-If a size of 0 (zero) is specified for \n" " size-less-than, this will be ignored.\n" "-If both size-greater-than and size-less-than\n" " are specified, both criteria must be met\n" " in order for a file to match.\n"));

		SizeVbox = gtk_vbox_new (FALSE, 0);
		set_widget (PACK, BOX, "vbox2", SizeVbox, vbox, find, 0, NULL);
/* ninth circle : hboxes */
		{
/* sizeG box */


		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox1", hbox1, SizeVbox, find, 5, NULL);

		  label = gtk_label_new (_("Size greater than (KB): "));
		  set_widget (PACK, LABEL, "SizeM_label", label, hbox1, find, 5, NULL);

		  Fa = gtk_adjustment_new (0, 0, 1024 * 1024 * 1024, 1, 1024, 1024);
		  find_options.sizeG = gtk_spin_button_new (GTK_ADJUSTMENT (Fa), 1, 0);

		  /*  gtk_signal_connect (GTK_OBJECT (find_options.sizeG), "changed",
		     GTK_SIGNAL_FUNC (toggle_radio3), (gpointer)((long)0x10)); */

		  gtk_widget_set_usize (find_options.sizeG, 100, -2);
		  set_widget (PACK, SPINBUTTON, "find_options.sizeG", find_options.sizeG, hbox1, find, 5, NULL);
/* sizeM box */
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox2", hbox2, SizeVbox, find, 5, NULL);

		  label = gtk_label_new (_("Size less than (KB): "));
		  set_widget (PACK, LABEL, "SizeM_label", label, hbox2, find, 5, NULL);

		  Fa = gtk_adjustment_new (0, 0, 1024 * 1024 * 1024, 1, 1024, 1024);
		  find_options.sizeM = gtk_spin_button_new (GTK_ADJUSTMENT (Fa), 1, 0);

		  /*  gtk_signal_connect (GTK_OBJECT (find_options.sizeM), "changed",
		     GTK_SIGNAL_FUNC (toggle_radio3), (gpointer)((long)0x20)); */

		  gtk_widget_set_usize (find_options.sizeM, 100, -2);
		  set_widget (PACK, SPINBUTTON, "find_options.sizeM", find_options.sizeM, hbox2, find, 5, NULL);


		}		/* end - circle 9 */
		if (!considerSize && (GTK_WIDGET_VISIBLE (SizeVbox)))
		  gtk_widget_hide (SizeVbox);


		hbox = gtk_hbox_new (FALSE, 0);
		set_widget (ADD, BOX, "hbox", hbox, vbox, find, 5, NULL);
		button = gtk_check_button_new_with_label (_("Time considerations"));
		set_widget (PACK, BUTTON, "toggleTime", button, hbox, find, 0, NULL);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 2));

		button = icon_button (1, help_xpm, _("How does time filtering work?"));
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_view), (gpointer) ((long) 9));
		gtk_widget_show (button);
		timeScroll = create_help (vbox, _("Three different time modes may be selected:\n" "*modified:\n" "  This changes when the file is modified, e.g.\n" "  by mknod(2), truncate(2), utime(2) and write(2)\n" " (of  more than  zero  bytes). Moreover, modification\n" "  time of a directory is changed by the creation or \n" "  deletion of  files  in  that directory. Modification\n" "  time is not changed for changes in owner, group,\n" "  hard link count, or mode.\n" "\n" "*changed:\n" "  This field is changed by writing or setting inode\n" "  information (i.e., owner, group, link count, mode).\n" "\n" "*accessed:\n" "  This field is changed by file accesses, e.g. by\n" "  exec(2), mknod(2), pipe(2), utime(2) and read(2)\n" "  (of more than zero bytes). Other routines, like\n" "  mmap(2), may or may not update access time.\n" "\n" "Please note: after filtering a file through glob, it\n" "will have its access time altered, so two consecutive\n" "runs will not produce identical results.\n"));

		TimeVbox = gtk_vbox_new (FALSE, 0);
		set_widget (PACK, BOX, "vbox2", TimeVbox, vbox, find, 0, NULL);
/* ninth circle : hboxes */

		{

/* hours box */
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox6", hbox2, TimeVbox, find, 5, NULL);

		  make_radio1 (_("Modified"), "Modified", hbox2, &MTIME);
		  make_radio1 (_("Changed"), "Changed", hbox2, &CTIME);
		  make_radio1 (_("Accessed"), "Accessed", hbox2, &ATIME);
		  options |= MTIME;

		  hbox1 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox3", hbox1, TimeVbox, find, 5, NULL);

		  label = gtk_label_new (_("In the previous : "));
		  set_widget (PACK, LABEL, "h_label", label, hbox1, find, 5, NULL);

		  Fa = gtk_adjustment_new (0, 0, 72, 1, 4, 4);
		  find_options.hours = gtk_spin_button_new (GTK_ADJUSTMENT (Fa), 1, 0);
		  set_widget (PACK, SPINBUTTON, "find_options.hours", find_options.hours, hbox1, find, 5, NULL);

		  hbox2 = gtk_hbox_new (FALSE, 0);
		  set_widget (PACK, BOX, "hbox6b", hbox2, TimeVbox, find, 5, NULL);

		  make_radio3 (_("Minutes"), "Minutes", hbox2, &MINUTES);
		  make_radio3 (_("Hours"), "Hours", hbox2, &HOURS);
		  make_radio3 (_("Days"), "Days", hbox2, &DAYS);
		  make_radio3 (_("Months"), "Months", hbox2, &MONTHS);
		  options |= MINUTES;

		  /*   gtk_signal_connect (GTK_OBJECT (find_options.hours), "changed",
		     GTK_SIGNAL_FUNC (toggle_radio3), (gpointer)((long)0x01));
		     label = gtk_label_new (_("hours"));
		     set_widget (PACK, LABEL, "h_label2", label, hbox1, find,
		     5, NULL); */

		}		/* end - circle 9 */
		if (!considerTime && (GTK_WIDGET_VISIBLE (TimeVbox)))
		  gtk_widget_hide (TimeVbox);
	      }			/* end - circle 8 */
	    }			/* end - circle 7 */

	  }			/* end - circle 6 */
	}			/* end - circle 5 */

/* third notebook page *********************/
/* fifth circle : scrolled window */
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  set_widget (ADD, SCROLLED, "scrolled_window2", scrolled_window, find_notebook, find, 5, NULL);

/* sixth circle : vbox */
	  {
	    page_vbox = gtk_vbox_new (FALSE, 0);
	    set_widget (SCROLL, BOX, "page_vbox2", page_vbox, scrolled_window, find, 5, NULL);

/* seventh circle : frame  */
	    {
	      upframe = gtk_frame_new (_("General considerations"));
	      set_widget (PACK, FRAME, "upframe1", upframe, page_vbox, find, 5, NULL);

/* eigth circle : vbox  */
	      {
		vbox = gtk_vbox_new (FALSE, 0);
		set_widget (ADD, BOX, "vbox1", vbox, upframe, find, 5, NULL);

/* ninth circle : checkbuttons */
		{
		  make_toggle (_("Do not look into binary files"), "find_options.I", vbox, &NOBINARIES, TRUE);

		  make_toggle (_("Output count of matching lines"), "find_options.count", vbox, &COUNT, TRUE);

		  make_radio2 (_("Match anywhere"), "find_options.match", vbox, &MATCH);
		  make_radio2 (_("Match whole words only"), "find_options.words", vbox, &WORDS);
		  make_radio2 (_("Match whole lines only"), "find_options.lines", vbox, &LINES);
		  make_radio2 (_("Output files where no match is found"), "find_options.invert", vbox, &INVERT);
		  options |= MATCH;	/* not used, really */

		}		/* end - circle 9 */
	      }			/* end - circle 8 */
	    }			/* end - circle 7 */
	  }			/* end - circle 6 */
	}			/* end - circle 5 */

/* about */
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  set_widget (ADD, SCROLLED, "scrolled_window4", scrolled_window, find_notebook, find, 5, NULL);
	  label = gtk_label_new (COPYRIGHT);
	  gtk_scrolled_window_add_with_viewport ((GtkScrolledWindow *) scrolled_window, label);
	  gtk_widget_show (label);



	}

/*********************************/

/* show the notebook pages: */

/* fourth circle : notebook tabs stuff */
	label = gtk_label_new (_("Find"));
	set_widget (TAB, TAB_LABEL, "find_tab_label", label, find_notebook, find, 0, NULL);

	label = gtk_label_new (_("Filter options"));
	set_widget (TAB, TAB_LABEL, "glob_label", label, find_notebook, find, 1, NULL);

	label = gtk_label_new (_("Content options"));
	set_widget (TAB, TAB_LABEL, "grep_label", label, find_notebook, find, 2, NULL);

	label = gtk_label_new (_("About"));
	set_widget (TAB, TAB_LABEL, "about_label", label, find_notebook, find, 3, NULL);
      }				/* end - circle 4 */

/* bottom frame stuff *******/

/* fourth circle:  bottom frame */
      {
	bottomframe = gtk_frame_new (NULL);
	set_widget (REPACK, FRAME, "mainframe", bottomframe, note_vbox, find, 5, NULL);
/* fifth circle : hbox and contents */
	{
	  hbox = gtk_hbutton_box_new ();
	  set_widget (ADD, BOX, "bottomframe1", hbox, bottomframe, find, 0, NULL);

/* visible part */
	  bottom_hbox1 = gtk_hbutton_box_new ();
	  set_widget (PACK, BOX, "bottomframe1", bottom_hbox1, hbox, find, 0, NULL);

	  button = gtk_button_new_with_label (_("Find"));
	  set_widget (PACK, BUTTON, "find_button", button, bottom_hbox1, find, 5, NULL);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  gtk_widget_grab_default (button);
	  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (find_ok_cb), (gpointer) NULL);

	  button = gtk_button_new_with_label (_("Results"));
	  set_widget (PACK, BUTTON, "raiseR_button", button, bottom_hbox1, find, 5, NULL);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  gtk_widget_grab_default (button);
	  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (raiseR_cb), (gpointer) NULL);

	  button = gtk_button_new_with_label (_("Quit"));
	  set_widget (PACK, BUTTON, "cancel_button", button, bottom_hbox1, find, 5, NULL);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (destroy), (gpointer) NULL);

/* invisible part: */
	  bottom_hbox2 = gtk_hbutton_box_new ();
	  set_widget (PACK, BOX, "bottomframe2", bottom_hbox2, hbox, find, 0, NULL);

	  label = gtk_label_new (_("working..."));
	  set_widget (PACK, LABEL, "abort_tab_label", label, bottom_hbox2, find, 0, NULL);

	  button = gtk_button_new_with_label (_("Cancel Search"));
	  set_widget (PACK, BUTTON, "abort_button", button, bottom_hbox2, find, 5, NULL);
	  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (abort_glob), (gpointer) NULL);

	  gtk_widget_hide (bottom_hbox2);
	}			/* end - circle 5 */
      }				/* end - circle 4 */
    }				/* end - circle 3 */
  }				/* end - second circle */


/* and finally, the signals */
  gtk_drag_dest_set (find, GTK_DEST_DEFAULT_ALL, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
  gtk_signal_connect (GTK_OBJECT (find), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), (gpointer) NULL);

  gtk_signal_connect (GTK_OBJECT (find), "destroy", GTK_SIGNAL_FUNC (destroy), (gpointer) NULL);

/* toggle options: */


  set_icon (find, "Xfglob", xfglob_icon_xpm);
  return find;
}

gboolean process_find_messages (gpointer client_data, gint source, GdkInputCondition condition)
{
  static char *buffer, line[256];
  static gboolean nothing_found;
  char *filename;
/*printf("input found\n");fflush(NULL);*/

  buffer = line;
  while (1)
  {
    if (!read (pfd[0], buffer, 1))
      return TRUE;
    if (buffer[0] == '\n')
    {
      buffer[1] = (char) 0;
      if (strncmp (line, "GLOB DONE=", strlen ("GLOB DONE=")) == 0)
      {
	/* char message[32]; */
	/* eliminate cancel-search button */
	gtk_widget_hide (bottom_hbox2);
	gtk_widget_show (bottom_hbox1);
	gtk_widget_realize (find);
	Gpid = 0;
	if (nothing_found)
	  show_cat (_("Nothing found...\n"));

	/* sprintf (message,"%d",findCount);
	   gtk_ctree_node_set_text((GtkCTree *)findTree,findNode,0,message); */

	if (findCount)
	{
	  char mess[128];
	  show_cat (_("Files found="));
	  sprintf (mess, "%d ", findCount);
	  if (findCount >= fileLimit)
	    strcat (mess, _("(interrupted because limit exceded)"));
	  strcat (mess, "\n");
	  show_cat (mess);
	}

	/*printf("%s",line);fflush(NULL); */
	cursor_reset (GTK_WIDGET (cat));
	return TRUE;
      }
      if ((strncmp (line, "PID=", 4) == 0))
      {
	Gpid = atoi (line + 4);
	/*printf("Glob PID=%d\n",Gpid);fflush(NULL); */
	/* show cancel-search button */
	gtk_widget_hide (bottom_hbox1);
	gtk_widget_show (bottom_hbox2);
	gtk_widget_realize (find);
	nothing_found = TRUE;
	return (TRUE);
      }
      if (cancelled)
	return (TRUE);
      if (line[0] == '/')
      {				/* strstr for : and strtok and send to cuenta */
	if (findCount >= fileLimit)
	  abort_glob (NULL, (gpointer) 1);
	else
	{
	  char *path, *linecount = NULL, *textos[6], cuenta[32], sizeF[64], permF[16];
	  struct stat st;
	  GdkPixmap *gPIM;
	  GdkBitmap *gPIX;
	  int *data;

	  path = line;
	  if (strstr (path, "\n"))
	    path = strtok (path, "\n");
	  if (strstr (path, ":"))
	  {
	    path = strtok (path, ":");
	    linecount = strtok (NULL, ":");
	    if (strcmp (linecount, "0") == 0)
	    {
	      linecount = NULL;
	      return TRUE;
	    }
	  }


	  findCount++;
	  data = (int *) malloc (3 * sizeof (int));
	  data[0] = findCount;
	  data[1] = data[2] = 0;
	  if (linecount)
	    sprintf (cuenta, "%d (%s %s)", findCount, linecount, _("lines"));
	  else
	    sprintf (cuenta, "%d", findCount);
	  gPIM = gPIM_help;
	  gPIX = gPIX_help;
	  textos[2] = path;
	  textos[0] = cuenta;
	  textos[1] = filename = strrchr (path, '/') + 1;
	  if (lstat (path, &st) == 0)
	  {
	    data[1] = st.st_size;
	    data[2] = st.st_ctime;

	    sprintf (sizeF, "%lld",(long long) st.st_size);
	    sprintf (permF, "0%o", st.st_mode & 07777);
	    textos[3] = sizeF;
	    textos[4] = ctime (&(st.st_ctime));
	    textos[5] = permF;
	    if (S_ISREG (st.st_mode))
	    {
	      gPIM = gPIM_page;
	      gPIX = gPIX_page;
	    }
	    if ((st.st_mode & 0100) || (st.st_mode & 010) || (st.st_mode & 01))
	    {
	      gPIM = gPIM_exe;
	      gPIX = gPIX_exe;
	    }
	    if (st.st_mode & 04000)
	    {
	      gPIM = gPIM_sexe;
	      gPIX = gPIX_sexe;
	    }
	    if (S_ISDIR (st.st_mode))
	    {
	      gPIM = gPIM_dir_close;
	      gPIX = gPIX_dir_close;
	    }
	    if (S_ISCHR (st.st_mode))
	    {
	      gPIM = gPIM_char_dev;
	      gPIX = gPIX_char_dev;
	    }
	    if (S_ISBLK (st.st_mode))
	    {
	      gPIM = gPIM_block_dev;
	      gPIX = gPIX_block_dev;
	    }
	    if (S_ISFIFO (st.st_mode))
	    {
	      gPIM = gPIM_fifo;
	      gPIX = gPIX_fifo;
	    }
	    if (S_ISLNK (st.st_mode))
	    {
	      gPIM = gPIM_page_lnk;
	      gPIX = gPIX_page_lnk;
	    }
	    if (S_ISSOCK (st.st_mode))
	    {
	      gPIM = gPIM_socket;
	      gPIX = gPIX_socket;
	    }
	  }
	  else
	  {
	    show_cat (_("unable to stat:"));
	    textos[2] = textos[3] = textos[4] = "-";
	  }
	  {
	    GtkCTreeNode *node;
	    *(strrchr (path, '/')) = 0;	/* leave just directory */
	    if (!strlen (path))
	      textos[2] = "/";
	    node = gtk_ctree_insert_node ((GtkCTree *) findTree, findNode, NULL, textos, 3, gPIX, gPIM, NULL, NULL, TRUE, FALSE);	/* isleaf,expand */
	    gtk_ctree_node_set_row_data_full ((GtkCTree *) findTree, node, data, node_destroy);
	  }
	  /* path was zapped to directory 5 lines above */
	  show_cat (path);
	  show_cat ("/");
	  show_cat (filename);
	  show_cat ("\n");
	}

      }
      else
	show_cat (line);
      nothing_found = FALSE;
      buffer = line;
      return (TRUE); /* continue here causes main loop blocking */ ;
    }
    buffer++;
  }
  return (1);
}


int
main (int argc, char **argv)
{
  xfce_init (&argc, &argv);
  if (argc > 1)
    find = create_find (argv[1]);
  else
    find = create_find (NULL);
  gtk_widget_show (find);
  /* open the pipes... */
  if (pipe (pfd) < 0)
  {
    perror ("pipe");
    return 1;
  }
  /* call process_message everytime the child's pipe is flushed */
  gdk_input_add (pfd[0], GDK_INPUT_READ, (GdkInputFunction) process_find_messages, (gpointer) ((long) pfd[0]));
  gtk_main ();
  close (pfd[0]);
  close (pfd[1]);		/* close the pipes */
  return (0);
}
