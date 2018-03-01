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

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <glib.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "xfce-common.h"
#include "my_string.h"
#include "gnome_protocol.h"
#include "xpmext.h"
#include "constant.h"

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#define empty_cursor_width 16
#define empty_cursor_height 16

static char *question[] = {
  "32 32 5 1",
  ". c none",
  "# c #183c59",
  "a c #bebebe",
  "b c #ffffff",
  "c c #0000ff",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  ".........##########.............",
  "........##aaaaaaaaab............",
  ".......##aaaaccccaaab...........",
  "......##aaaccccccccaab..........",
  "......#aaaccccccccccaab.........",
  "......#aaaccccccccccaab.........",
  "......#aaacccaaaacccaab.........",
  "......#aaacccaaaacccaaab........",
  "......#aaacccaaaccccaaaab.......",
  "......#aaaaaaaacccccaaabb.......",
  "......#aaaaaaacccccaaab.........",
  "......##aaaaacccccaaabb.........",
  ".......##aaaaccccaaaab..........",
  "........##aaacccaaaaab..........",
  ".........##aacccaaaaab..........",
  "..........#aaaaaaaaaab..........",
  "..........#aaaaaaabbb...........",
  "..........#aacccaab.............",
  "..........#aacccaab.............",
  "..........#aacccaab.............",
  ".........##aaaaaaab.............",
  ".........#bbbbbbbbbb............",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................"
};

static char *warning[] = {
  "32 32 5 1",
  ". c none",
  "# c #183c59",
  "a c #bebebe",
  "b c #ffffff",
  "c c #ff0000",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  ".........##########.............",
  "........##aaaaaaaaab............",
  ".......##aaaaacaaaaab...........",
  "......##aaaaacccaaaaab..........",
  "......#aaaaacccccaaaaab.........",
  "......#aaaaacccccaaaaab.........",
  "......#aaaaacccccaaa#ab.........",
  "......#aaaaacccccaa##aab........",
  "......#aaaaacccccaaa#aaab.......",
  "......#aaaaaacccaaaaaaabb.......",
  "......#aaaaaacccaaaaaab.........",
  "......##aaaaacccaaaaabb.........",
  ".......##aaaacccaaaaab..........",
  "........##aaacccaaaaab..........",
  ".........##aacccaaaaab..........",
  "..........#aaaaaaaaaab..........",
  "..........#aaaaaaabbb...........",
  "..........#aacccaab.............",
  "..........#aacccaab.............",
  "..........#aacccaab.............",
  ".........##aaaaaaab.............",
  ".........#bbbbbbbbbb............",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................"
};

static unsigned char empty_cursor_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char empty_cursormask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

GtkWidget *
get_widget (GtkWidget * widget, gchar * widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
  {
    if (GTK_IS_MENU (widget))
      parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
    else
      parent = widget->parent;
    if (parent == NULL)
      break;
    widget = parent;
  }

  found_widget = (GtkWidget *) gtk_object_get_data (GTK_OBJECT (widget), widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

/* This is an internally used function to set notebook tab widgets. */
void
set_notebook_tab (GtkWidget * notebook, gint page_num, GtkWidget * widget)
{
  GtkNotebookPage *page;
  GtkWidget *notebook_page;

  page = (GtkNotebookPage *) g_list_nth (GTK_NOTEBOOK (notebook)->children, page_num)->data;
  notebook_page = page->child;
  gtk_widget_ref (notebook_page);
  gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page_num);
  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), notebook_page, widget, page_num);
  gtk_widget_unref (notebook_page);
}

void
lock_size (GtkWidget * toplevel)
{
  GdkGeometry geometry;
  GdkWindowHints geometry_mask;
  gint uwidth = 0;
  gint uheight = 0;

  gdk_window_get_size (((GtkWidget *) toplevel)->window, &uwidth, &uheight);

  geometry_mask = GDK_HINT_MAX_SIZE | GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE;
  geometry.min_width = geometry.max_width = geometry.base_width = uwidth;
  geometry.min_height = geometry.max_height = geometry.base_height = uheight;
  gtk_window_set_geometry_hints (GTK_WINDOW (toplevel), toplevel, &geometry, geometry_mask);
}

void
set_icon (GtkWidget * toplevel, gchar * name, gchar ** data)
{
  GdkPixmap *icon = NULL;
  GdkBitmap *mask = NULL;

  if (!GTK_WIDGET_REALIZED (toplevel))
    gtk_widget_realize (toplevel);

  icon = MyCreateGdkPixmapFromData (data, toplevel, &mask, FALSE);
  gdk_window_set_icon (toplevel->window, NULL, icon, mask);
  gdk_window_set_icon_name (toplevel->window, name);
}

/* This shows a simple dialog box with a label and an 'OK' button.
   Example usage:
    my_show_message ("Error saving file");
 */

static void
on_ok_show_message (GtkWidget * twidget, gpointer data)
{
  gtk_main_quit ();
}

static gboolean
delete_event_show_message (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_main_quit ();
  return (TRUE);
}

void
my_show_message (gchar * message)
{
  GtkWidget *dialog_show_message;
  GtkWidget *label, *button, *hbox, *pixmapwid;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;

  dialog_show_message = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog_show_message), GTK_WIN_POS_CENTER);
  gtk_container_border_width (GTK_CONTAINER (dialog_show_message), 5);

  gtk_widget_realize (dialog_show_message);
  gnome_sticky (dialog_show_message->window);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_show_message)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  pixmap = MyCreateGdkPixmapFromData (warning, dialog_show_message, &mask, FALSE);

  /* a pixmap widget to contain the pixmap */
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pixmapwid);
  gtk_box_pack_start (GTK_BOX (hbox), pixmapwid, FALSE, FALSE, 0);

  label = gtk_label_new (message);
  gtk_misc_set_padding (GTK_MISC (label), 20, 20);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  button = gtk_button_new_with_label (_("OK"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_show_message)->action_area), button, FALSE, FALSE, 14);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_ok_show_message), GTK_OBJECT (dialog_show_message));
  gtk_signal_connect (GTK_OBJECT (dialog_show_message), "delete_event", GTK_SIGNAL_FUNC (delete_event_show_message), GTK_OBJECT (dialog_show_message));
  gtk_window_set_modal (GTK_WINDOW (dialog_show_message), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog_show_message), _("Message"));
  gtk_widget_show (dialog_show_message);
  gtk_main ();
  gtk_widget_destroy (dialog_show_message);
}

/* This shows a dialog box with a message and a number of buttons.
 * Signal handlers can be supplied for any of the buttons.
 * NOTE: The dialog is automatically destroyed when any button is clicked.
 * default_button specifies the default button, numbered from 1..
 * data is passed to the signal handler.

   Example usage:
     GtkWidget *dialog;
     gchar *buttons[] = { "Yes", "No", "Cancel" };
     GtkSignalFunc signal_handlers[] = { on_yes, on_no, NULL };

     dialog = my_util_show_dialog ("Do you want to save the current project?", 3, buttons, 3, signal_handlers, NULL);
     gtk_widget_show (dialog);
 */
GtkWidget *
my_show_dialog (gchar * message, gint nbuttons, gchar * buttons[], gint default_button, GtkSignalFunc signal_handlers[], gpointer data)
{
  GtkWidget *dialog, *hbox, *label, *button, *bbox, *pixmapwid;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask = NULL;
  int i;

  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);

  gtk_widget_realize (dialog);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gnome_sticky (dialog->window);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  pixmap = MyCreateGdkPixmapFromData (question, dialog, &mask, FALSE);

  /* a pixmap widget to contain the pixmap */
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pixmapwid);
  gtk_box_pack_start (GTK_BOX (hbox), pixmapwid, FALSE, FALSE, 0);


  label = gtk_label_new (message);
  gtk_misc_set_padding (GTK_MISC (label), 20, 20);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);

  for (i = 0; i < nbuttons; i++)
  {
    button = gtk_button_new_with_label (_(buttons[i]));
    gtk_container_add (GTK_CONTAINER (bbox), button);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    if (i == default_button - 1)
      gtk_widget_grab_default (button);
    gtk_widget_show (button);

    if (signal_handlers[i])
      gtk_signal_connect (GTK_OBJECT (button), "clicked", signal_handlers[i], GTK_OBJECT (dialog));
    else
      gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (dialog));
  }
  return dialog;
}

static int rep_yesno_dialog = 0;

static void
on_yes_yesno_dialog (GtkWidget * twidget, gpointer data)
{
  gtk_main_quit ();
  rep_yesno_dialog = 1;
}

static void
on_no_yesno_dialog (GtkWidget * twidget, gpointer data)
{
  rep_yesno_dialog = 0;
  gtk_main_quit ();
}

static gboolean
delete_event_yesno_dialog (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  rep_yesno_dialog = 0;
  gtk_main_quit ();
  return (TRUE);
}

gint
my_yesno_dialog (gchar * message)
{
  /* FIXME : how make this short and ANSI ? */
  GtkWidget *dialog_yesno_dialog;
  gchar *buttons[2];
  GtkSignalFunc signal_handlers[2];
  
  signal_handlers[0] = GTK_SIGNAL_FUNC (on_yes_yesno_dialog);
  signal_handlers[1] = GTK_SIGNAL_FUNC (on_no_yesno_dialog);

  buttons[0] = _("Yes");
  buttons[1] = _("No");

  dialog_yesno_dialog = my_show_dialog (message, 2, buttons, 2, signal_handlers, NULL);
  gtk_signal_connect (GTK_OBJECT (dialog_yesno_dialog), "delete_event", GTK_SIGNAL_FUNC (delete_event_yesno_dialog), GTK_OBJECT (dialog_yesno_dialog));
  gtk_window_set_modal (GTK_WINDOW (dialog_yesno_dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog_yesno_dialog), _("Question"));
  gtk_widget_show (dialog_yesno_dialog);
  gtk_main ();
  gtk_widget_destroy (dialog_yesno_dialog);
  return rep_yesno_dialog;
}

void
refresh_screen (void)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  Window w;

  valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder | CWBackPixmap;
  attributes.override_redirect = True;
  attributes.save_under = False;
  attributes.background_pixmap = None;
  attributes.backing_store = NotUseful;
  w = XCreateWindow (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), 0, 0, (unsigned int) gdk_screen_width (), (unsigned int) gdk_screen_height (), (unsigned int) 0, CopyFromParent, (unsigned int) CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);
  XMapWindow (GDK_DISPLAY (), w);
  XDestroyWindow (GDK_DISPLAY (), w);
  XFlush (GDK_DISPLAY ());
}

char *
build_path (char *value)
{
  static char filename[MAXSTRLEN];
  char *path;

  path = (char *) getenv ("XFCE_DATA");
  if (path)
    strcpy (filename, path);
  else
    strcpy (filename, XFCE_DIR);

  if ((value) && strlen (value))
    strcat (filename, value);

  return filename;
}

void
update_events (void)
{
  while (gtk_events_pending ())
    gtk_main_iteration_do (TRUE);
}

/*** the next three routines are taken straight from gnome-libs so that the
     gtk-only version can receive drag and drops as well ***/
/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void
gnome_uri_list_free_strings (GList * list)
{
  g_list_foreach (list, (GFunc) g_free, NULL);
  g_list_free (list);
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList *
gnome_uri_list_extract_uris (const gchar * uri_list)
{
  const gchar *p, *q;
  gchar *retval;
  GList *result = NULL;

  g_return_val_if_fail (uri_list != NULL, NULL);

  p = uri_list;

  /* We don't actually try to validate the URI according to RFC
   * 2396, or even check for allowed characters - we just ignore
   * comments and trim whitespace off the ends.  We also
   * allow LF delimination as well as the specified CRLF.
   */
  while (p)
  {
    if (*p != '#')
    {
      while (isspace ((int) (*p)))
	p++;

      q = p;
      while (*q && (*q != '\n') && (*q != '\r'))
	q++;

      if (q > p)
      {
	q--;
	while (q > p && isspace ((int) (*q)))
	  q--;

	retval = (char *) g_malloc (q - p + 2);
	strncpy (retval, p, q - p + 1);
	retval[q - p + 1] = '\0';

	result = g_list_prepend (result, retval);
      }
    }
    p = strchr (p, '\n');
    if (p)
      p++;
  }

  return g_list_reverse (result);
}


/**
 * gnome_uri_list_extract_filenames:
 * @uri_list: an uri-list in the standard format
 *
 * Returns a GList containing strings allocated with g_malloc
 * that contain the filenames in the uri-list.
 *
 * Note that unlike gnome_uri_list_extract_uris() function, this
 * will discard any non-file uri from the result value.
 */
GList *
gnome_uri_list_extract_filenames (const gchar * uri_list)
{
  GList *tmp_list, *node, *result;

  g_return_val_if_fail (uri_list != NULL, NULL);

  result = gnome_uri_list_extract_uris (uri_list);

  tmp_list = result;
  while (tmp_list)
  {
    gchar *s = (char *) tmp_list->data;

    node = tmp_list;
    tmp_list = tmp_list->next;

    if (!strncmp (s, "file:", 5))
    {
      node->data = cleanup (g_strdup (s + 5));
    }
    else
    {
      node->data = cleanup (g_strdup (s));
    }
    g_free (s);
  }
  return result;
}

void
my_flush_events (void)
{
  gdk_flush ();
  while (gtk_events_pending ())
    gtk_main_iteration_do (TRUE);
  gdk_flush ();
}

void
cursor_wait (GtkWidget * widget)
{
  GdkCursor *cursor;
  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (widget->window, cursor);
  gdk_flush ();
  gdk_cursor_destroy (cursor);
}

void
cursor_reset (GtkWidget * widget)
{
  gdk_window_set_cursor (widget->window, NULL);
}

void
cursor_hide(GtkWidget * widget)
{
  GdkCursor *cursor;
  GdkPixmap *source, *mask;
  GdkColor fg = { 0, 0, 0, 0 }; /* Transparent. */
  GdkColor bg = { 0, 0, 0, 0 }; /* Transparent. */   

  g_return_if_fail (widget->window != NULL);
  source = gdk_bitmap_create_from_data (NULL, empty_cursor_bits, empty_cursor_width, empty_cursor_height);
  mask = gdk_bitmap_create_from_data (NULL, empty_cursormask_bits, empty_cursor_width, empty_cursor_height);
  cursor = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, 8, 8);
  gdk_pixmap_unref (source);
  gdk_pixmap_unref (mask);   
  gdk_window_set_cursor (widget->window, cursor);
}

gint 
my_get_adjustment_as_int (GtkAdjustment * adjustment)
{
  gfloat val;

  g_return_val_if_fail (adjustment != NULL, 0);
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), 0);

  val = adjustment->value;
  if (val - floor (val) < ceil (val) - val)
    return floor (val);
  else
    return ceil (val);
}

void
my_set_adjustment_bounds (GtkAdjustment * adjustment, gfloat lower, gfloat upper)
{
  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  adjustment->lower = lower;
  adjustment->upper = upper + 1.0;
}

void
terminate (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf (_("Terminated: "));
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  gdk_exit (1);
}

void
on_signal (int sig_num)
{
  switch (sig_num)
  {
  case SIGHUP:
    terminate ("sighup caught");
    break;
  case SIGINT:
    terminate ("sigint caught");
    break;
  case SIGQUIT:
    terminate ("sigquit caught");
    break;
  case SIGABRT:
    terminate ("sigabrt caught");
    break;
  case SIGPIPE:
    terminate ("sigpipe caught");
    break;
  case SIGBUS:
    terminate ("sigbus caught");
    break;
  case SIGSEGV:
    terminate ("sigsegv caught");
    break;
  case SIGTERM:
    terminate ("sigterm caught");
    break;
  case SIGFPE:
    terminate ("sigfpe caught");
    break;
  default:
    terminate ("unknown signal");
    break;
  }
}

void
signal_setup (void)
{
  /* Handle some signals */
#ifndef DEBUG
  signal (SIGHUP, on_signal);
  signal (SIGINT, on_signal);
  signal (SIGQUIT, on_signal);
  signal (SIGABRT, on_signal);
  signal (SIGBUS, on_signal);
  signal (SIGSEGV, on_signal);
  signal (SIGTERM, on_signal);
  signal (SIGFPE, on_signal);
#endif
}

void
xfce_init (int *argc, char **argv[])
{
#ifdef HAVE_GDK_IMLIB
  int pixmaps, images;
#endif
  signal_setup ();

  gtk_set_locale ();
  bindtextdomain (PACKAGE, XFCE_LOCALE_DIR);
  textdomain (PACKAGE);
  init_xfce_rcfile ();
  gtk_init (argc, argv);
#ifdef HAVE_GDK_IMLIB
  gdk_imlib_init ();
/* Get gdk to use imlib's visual and colormap */
  gtk_widget_push_visual (gdk_imlib_get_visual ());
  gtk_widget_push_colormap (gdk_imlib_get_colormap ());

  /*
   * Taken from gnome-libs where it is explained.
   */
  gdk_imlib_get_cache_info (&pixmaps, &images);
  if (!((pixmaps == -1) || (images == -1)))
  {
    gdk_imlib_set_cache_info (0, images);
  }
#else
#ifdef HAVE_GDK_PIXBUF
  gdk_rgb_init ();
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
  gtk_widget_set_default_visual (gdk_rgb_get_visual ());
#endif
#endif
}

void
xfce_end (gpointer data, int status)
{
  /* We'll be able to add more stuff here, but for the moment,
   * that's all we do here ...
   */
  gdk_exit (status);
}
