#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "xfumed.h"
#include "xfumed_gui.h"
#include "xpmext.h"
#include "xfce-common.h"
#include "icons/dir_close.xpm"
#include "icons/dir_open.xpm"
#include "icons/exe.xpm"

static GdkPixmap *pix_exe, *pix_closed, *pix_open;
static GdkBitmap *bm_exe, *bm_closed, *bm_open;

int
main (int argc, char *argv[])
{
  XFMENU *xfmenu = xfmenu_new ();

  xfumed_init (&argc, &argv);

  read_menu (xfmenu);

  xfmenu->window = create_xfumed_window (xfmenu);
  gtk_widget_show (xfmenu->window);
  gtk_main ();

  return 0;
}

XFMENU *
xfmenu_new (void)
{
  XFMENU *new = g_malloc (sizeof (XFMENU));

  if (!new)
    g_error ("Could not allocate memory for xfmenu struct");

  new->window = NULL;
  new->ctree = NULL;
  new->menulist = NULL;
  new->selected_node = NULL;
  new->first_node = NULL;
  new->saved = TRUE;
  new->dlg = xfdlg_new ();

  return new;
}


XFDLG *
xfdlg_new (void)
{
  XFDLG *dlg = g_malloc (sizeof (XFDLG));

  if (!dlg)
    g_error ("xfumed: Could not allocate xfdlg struct");

  dlg->window = NULL;
  dlg->entry_type_combo = NULL;
  dlg->entry_type_entry = NULL;
  dlg->caption_entry = NULL;
  dlg->cmd_entry = NULL;
  dlg->entry = NULL;
  dlg->caption = NULL;
  dlg->cmd = NULL;
  dlg->fileselect_button = NULL;
  dlg->update = FALSE;
  return (dlg);
}

void
load_ctree_images (void)
{
  GtkWidget *w = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  pix_open = MyCreateGdkPixmapFromData (dir_open_xpm, w, &bm_open, FALSE);
  pix_closed = MyCreateGdkPixmapFromData (dir_close_xpm, w, &bm_closed, FALSE);
  pix_exe = MyCreateGdkPixmapFromData (exe_xpm, w, &bm_exe, FALSE);

  gtk_widget_destroy (w);
}

void
xfumed_init (int *argc, char **argv[])
{
  gtk_set_locale ();
  gtk_init (argc, argv);

  xfce_init (argc, argv);

  load_ctree_images ();
}

char *
get_next_word (char **remainder)
{
  int len, i, delimiter;
  char word[MAXSTRLEN];

  word[0] = '\0';

  while (**remainder == ' ')
    (*remainder)++;

  /* check if word is enclosed in quotes */
  if (**remainder == '\"')
    {
      (*remainder)++;
      delimiter = '\"';
    }
  else
    delimiter = ' ';

  len = strlen (*remainder);

  for (i = 0; i < len && i < MAXSTRLEN; i++, (*remainder)++)
    {
      if (**remainder == delimiter || **remainder == '\n')
	break;
      else
	{
	  word[i] = **remainder;
	  word[i + 1] = '\0';
	}
    }

  if (**remainder != '\n' && **remainder != '\0')
    (*remainder)++;

  g_strstrip (word);

  return g_strdup (word);
}

GtkWidget *
read_menu (XFMENU * xfmenu)
{
  FILE *fp;
  char *home, *filename;
  char line[MAXSTRLEN];
  gboolean have_menu = FALSE;

  GtkCTreeNode *current_node = NULL;
  GtkWidget *ctree;

  home = g_getenv ("HOME");

  filename = g_strconcat (home, "/", RCFILE, NULL);

  if (!(fp = fopen (filename, "r")))
    {
      if ((fp = fopen (filename, "w")))
	{
	  fclose (fp);
	  fp = fopen (filename, "r");
	}
      else
	g_error ("xfumed: Couldn't open file %s\n", filename);
    }

  g_free (filename);

  if (xfmenu->ctree)
    remove_node (xfmenu, NULL);
  else
    xfmenu->ctree = gtk_ctree_new (3, 0);

  ctree = xfmenu->ctree;
  xfmenu->menulist = NULL;
  xfmenu->first_node = NULL;

  gtk_widget_set_sensitive (ctree, FALSE);

  while (fgets (line, MAXSTRLEN - 1, fp) != NULL)
    {
      char *remainder = line;
      char *word;

      while (*remainder == ' ')
	remainder++;

      if (*remainder == '#' || *remainder == '\n')
	continue;

      word = get_next_word (&remainder);

      if (g_strcasecmp ("AddToMenu", word) == 0)
	{
	  char *menuname = get_next_word (&remainder);

	  if (!menuname)
	    {
	      g_free (word);
	      continue;
	    }

	  if (!(have_menu = find_node_by_name (xfmenu, menuname, &current_node)))
	    current_node = new_temp_node (xfmenu, menuname);

	  g_free (menuname);
	  entry_from_line (xfmenu, current_node, remainder);
	}
      else if (have_menu && strcmp ("+", word) == 0)
	{
	  entry_from_line (xfmenu, current_node, remainder);
	}
      else
	{
	  have_menu = FALSE;
	}

      g_free (word);
    }

  if (xfmenu->first_node)
    gtk_ctree_select (GTK_CTREE (ctree), xfmenu->first_node);

  fclose (fp);
  xfmenu->saved = TRUE;
  return xfmenu->ctree;
}

void
entry_from_line (XFMENU * xfmenu, GtkCTreeNode * parent, char *string)
{
  char *word, *end;
  char *text[] = { NULL, NULL, NULL };

  while (*string == ' ')
    string++;

  word = get_next_word (&string);

  if (strlen (word) == 0 && *string != '\n')
    string++;
  else
    text[0] = g_strdup (word);

  g_free (word);

  if (*string == '\n' || *string == '\0')
    return;

  while (*string == ' ')
    string++;

  word = get_next_word (&string);

  if (g_strcasecmp ("Nop", word) == 0)
    {
      g_free (word);
      text[0] = g_strdup (NOP_CAPTION);

      insert_node (xfmenu, parent, NULL, text, TRUE);
      return;
    }
  else if (g_strcasecmp ("Exec", word) == 0)
    {
      char *remainder = g_strdup (string);

      g_free (word);

      if ((end = strchr (remainder, '\n')))
	*end = '\0';

      text[1] = remainder;

      insert_node (xfmenu, parent, NULL, text, TRUE);
      return;
    }
  else if (g_strcasecmp ("PopUp", word) == 0)
    {
      g_free (word);

      text[2] = get_next_word (&string);

      insert_node (xfmenu, parent, NULL, text, FALSE);
      return;
    }
}

gboolean
find_node_by_name (XFMENU * xfmenu, char *name, GtkCTreeNode ** return_node)
{
  GList *list;

  if (!name)
    return FALSE;

  if (strcmp ("user_menu", name) == 0)
    {
      (*return_node) = NULL;
      return TRUE;
    }

  for (list = xfmenu->menulist; list && list->data; list = g_list_next (list))
    {
      GtkCTreeNode *node = (GtkCTreeNode *) list->data;
      char *menuname;

      if (!node)
	continue;

      gtk_ctree_node_get_text (GTK_CTREE (xfmenu->ctree), node, 2, &menuname);

      if (!menuname)
	continue;

      if (strcmp (name, menuname) == 0)
	{
	  (*return_node) = node;
	  return TRUE;
	}
    }

  return FALSE;
}

GtkCTreeNode *
update_node (XFMENU * xfmenu, char **text)
{
  GtkCTreeNode *node = xfmenu->selected_node;
  gboolean is_leaf = node ? GTK_CTREE_ROW (node)->is_leaf : FALSE;

  if (!node)
    return NULL;

  gtk_ctree_node_set_text (GTK_CTREE (xfmenu->ctree), node, 1, text[1]);
  gtk_ctree_node_set_text (GTK_CTREE (xfmenu->ctree), node, 2, text[2]);

  if (is_leaf)
    {
      GdkPixmap *pm;
      GdkBitmap *bm;

      if (strcmp (NOP_CAPTION, text[0]) != 0)
	{
	  pm = pix_exe;
	  bm = bm_exe;
	}
      else
	{
	  pm = NULL;
	  bm = NULL;
	}

      gtk_ctree_set_node_info (GTK_CTREE (xfmenu->ctree), node, text[0], SPACING, pm, bm, NULL, NULL, is_leaf, 0);
    }
  else
    {
      gtk_ctree_set_node_info (GTK_CTREE (xfmenu->ctree), node, text[0], SPACING, pix_closed, bm_closed, pix_open, bm_open, is_leaf, 0);
    }

  xfmenu->saved = FALSE;
  return node;
}


GtkCTreeNode *
insert_node (XFMENU * xfmenu, GtkCTreeNode * parent, GtkCTreeNode * sibling, char **text, gboolean is_leaf)
{
  GtkCTreeNode *node = NULL;

  if (is_leaf)
    {
      if (strcmp (NOP_CAPTION, text[0]) == 0)
	node = gtk_ctree_insert_node (GTK_CTREE (xfmenu->ctree), parent, sibling, text, SPACING, NULL, NULL, NULL, NULL, is_leaf, 0);
      else
	node = gtk_ctree_insert_node (GTK_CTREE (xfmenu->ctree), parent, sibling, text, SPACING, pix_exe, bm_exe, NULL, NULL, is_leaf, 0);
    }
  else
    {
      if (find_node_by_name (xfmenu, text[2], &node))
	{
	  if (strcmp (TEMPNODETEXT, text[0]) == 0)
	    {

	      if (gtk_ctree_node_is_visible (GTK_CTREE (xfmenu->ctree), node) != GTK_VISIBILITY_FULL)
		gtk_ctree_node_moveto (GTK_CTREE (xfmenu->ctree), node, 0.5, 0, 0);

	      gtk_ctree_select (GTK_CTREE (xfmenu->ctree), node);
	      xfmenu->saved = FALSE;
	      return node;
	    }

	  gtk_ctree_move (GTK_CTREE (xfmenu->ctree), node, parent, sibling);
	  gtk_ctree_set_node_info (GTK_CTREE (xfmenu->ctree), node, text[0], SPACING, pix_closed, bm_closed, pix_open, bm_open, is_leaf, 0);
	}
      else
	{
	  node = gtk_ctree_insert_node (GTK_CTREE (xfmenu->ctree), parent, sibling, text, SPACING, pix_closed, bm_closed, pix_open, bm_open, is_leaf, 1);

	  xfmenu->menulist = g_list_append (xfmenu->menulist, (gpointer) node);
	}
    }


  if (gtk_ctree_node_is_visible (GTK_CTREE (xfmenu->ctree), node) != GTK_VISIBILITY_FULL)
    gtk_ctree_node_moveto (GTK_CTREE (xfmenu->ctree), node, 0.5, 0, 0);

  if (!xfmenu->first_node)
    {
      xfmenu->first_node = node;
      gtk_widget_set_sensitive (xfmenu->ctree, TRUE);
    }

  gtk_ctree_select (GTK_CTREE (xfmenu->ctree), node);
  gtk_widget_grab_focus (xfmenu->ctree);

  xfmenu->saved = FALSE;
  return node;
}

GtkCTreeNode *
new_temp_node (XFMENU * xfmenu, char *menuname)
{
  char *text[3];
  
  text[0] = TEMPNODETEXT;
  text[1] = NULL;
  text[2] = menuname;

  return insert_node (xfmenu, NULL, NULL, text, FALSE);
}

void
remove_from_list (GtkCTree * ctree, GtkCTreeNode * node, gpointer data)
{
  XFMENU *xfmenu = (XFMENU *) data;

  if (!GTK_CTREE_ROW (node)->is_leaf)
    xfmenu->menulist = g_list_remove (xfmenu->menulist, (gpointer) node);
}

void
remove_node (XFMENU * xfmenu, GtkCTreeNode * node)
{
  gtk_ctree_post_recursive (GTK_CTREE (xfmenu->ctree), node, (GtkCTreeFunc) remove_from_list, (gpointer) xfmenu);

  if (node)
    {
      GtkCTreeNode *node2 = NULL;

      if (GTK_CTREE_NODE_NEXT (node))
	node2 = GTK_CTREE_NODE_NEXT (node);
      else if (GTK_CTREE_NODE_PREV (node))
	node2 = GTK_CTREE_NODE_PREV (node);
      else
	gtk_widget_set_sensitive (xfmenu->ctree, FALSE);

      if (node2)
	gtk_ctree_select (GTK_CTREE (xfmenu->ctree), node2);

      if (node == xfmenu->first_node)
	xfmenu->first_node = node2;
    }

  gtk_ctree_remove_node (GTK_CTREE (xfmenu->ctree), node);
  xfmenu->saved = FALSE;
}

void
write_menu (XFMENU * xfmenu)
{
  char *home, *filename, *backup;
  FILE *fp, *backup_fp;

  if (xfmenu->saved)
    return;

  home = g_getenv ("HOME");
  filename = g_strconcat (home, "/", RCFILE, NULL);
  backup = g_strconcat (filename, ".bak", NULL);

  if ((backup_fp = fopen (backup, "w")) && (fp = fopen (filename, "r")))
    {
      char c;

      while ((c = fgetc (fp)) != EOF)
	fputc (c, backup_fp);

      fclose (fp);
      fclose (backup_fp);
    }

  g_free (backup);

  if (!(fp = fopen (filename, "w")))
    {
      g_free (filename);
      return;
    }

  fprintf (fp, "%s\n", INTROTEXT);

  gtk_ctree_post_recursive (GTK_CTREE (xfmenu->ctree), NULL, (GtkCTreeFunc) write_node, (gpointer) fp);

  fclose (fp);
  g_free (filename);
  xfmenu->saved = TRUE;
}

void
write_node (GtkCTree * ctree, GtkCTreeNode * node, gpointer data)
{
  FILE *fp = (FILE *) data;
  char *caption, *parentname;
  GtkCTreeRow *row = GTK_CTREE_ROW (node);
  GtkCTreeNode *parent = row->parent;

  gtk_ctree_node_get_pixtext (ctree, node, 0, &caption, NULL, NULL, NULL);

  if (!parent)
    parentname = "user_menu";
  else
    gtk_ctree_node_get_text (ctree, parent, 2, &parentname);

  if (row->is_leaf)
    {
      if (strcmp (NOP_CAPTION, caption) == 0)
	fprintf (fp, "AddToMenu \"%s\" \"\" Nop\n", parentname);
      else
	{
	  char *cmd;

	  gtk_ctree_node_get_text (ctree, node, 1, &cmd);
	  fprintf (fp, "AddToMenu \"%s\" \"%s\" Exec %s\n", parentname, caption, cmd);
	}
    }
  else
    {
      char *menuname;

      gtk_ctree_node_get_text (ctree, node, 2, &menuname);
      fprintf (fp, "AddToMenu \"%s\" \"%s\" PopUp \"%s\"\n", parentname, caption, menuname);
    }
}
