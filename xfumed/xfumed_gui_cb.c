#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "xfumed.h"
#include "xfumed_gui_cb.h"
#include "xfumed_gui.h"
#include "xfce-common.h"
#include "constant.h"
#include "fileselect.h"
#include <gdk/gdkkeysyms.h>

void
save (XFMENU * xfmenu)
{
  gtk_clist_freeze (GTK_CLIST (xfmenu->ctree));

  if (!xfmenu->saved)
    {
      write_menu (xfmenu);
      my_show_message (_("Your changes will take effect on next startup"));
    }

  gtk_clist_thaw (GTK_CLIST (xfmenu->ctree));
}

void
reset (XFMENU * xfmenu)
{
  gtk_clist_freeze (GTK_CLIST (xfmenu->ctree));

  xfmenu->ctree = read_menu (xfmenu);

  gtk_clist_thaw (GTK_CLIST (xfmenu->ctree));
}

static gboolean quit_or_not = FALSE;
static gboolean save_or_not = TRUE;

void
confirm_quit_yes_with_save (GtkWidget * widget, gpointer user_data)
{
  quit_or_not = TRUE;
  save_or_not = TRUE;
  gtk_main_quit ();
}

void
confirm_quit_yes_without_save (GtkWidget * widget, gpointer user_data)
{
  quit_or_not = TRUE;
  save_or_not = FALSE;
  gtk_main_quit ();
}

void
confirm_quit_no (GtkWidget * widget, gpointer user_data)
{
  quit_or_not = FALSE;
  save_or_not = TRUE;
  gtk_main_quit ();
}

gboolean confirm_quit (XFMENU * xfmenu)
{
  GtkWidget *quit_dialog;
  char *message = _("There are unsaved changes to the menu.\n\n" "Do you want to save the changes ?");
  char *buttons[3];
  int default_button = 1;
  GtkSignalFunc signal_handlers[3];

  buttons[0] = _("Yes");
  buttons[1] = _("No");
  buttons[2] = _("Don't Quit");

  signal_handlers[0] = GTK_SIGNAL_FUNC (confirm_quit_yes_with_save);
  signal_handlers[1] = GTK_SIGNAL_FUNC (confirm_quit_yes_without_save);
  signal_handlers[2] = GTK_SIGNAL_FUNC (confirm_quit_no);

  quit_dialog = my_show_dialog (message, 3, buttons, default_button, signal_handlers, xfmenu);
  gtk_signal_connect (GTK_OBJECT (quit_dialog), "delete_event", GTK_SIGNAL_FUNC (confirm_quit_no), NULL);

  gtk_widget_show (quit_dialog);
  gtk_main ();
  gtk_widget_destroy (quit_dialog);

  if (quit_or_not && save_or_not)
    save (xfmenu);

  return quit_or_not;
}


static gboolean delete_or_not = FALSE;

void
confirm_delete_yes (GtkWidget * widget, gpointer data)
{
  delete_or_not = TRUE;
  gtk_main_quit ();
}

void
confirm_delete_no (GtkWidget * widget, gpointer data)
{
  delete_or_not = FALSE;
  gtk_main_quit ();
}

gboolean confirm_delete (GtkCTreeNode * node)
{
  GtkWidget *delete_dialog;
  char *message;
  char *buttons[2];
  int default_button = 1;
  GtkSignalFunc signal_handlers[2];

  if (GTK_CTREE_ROW (node)->is_leaf)
    message = _("Do you want to delete this entry ?");
  else
    message = _("If you delete a submenu, all its entries and \n" "submenus will also be deleted.\n\n" "Do you want to delete this submenu ?");

  buttons[0] = _("Yes");
  buttons[1] = _("No");

  signal_handlers[1] = GTK_SIGNAL_FUNC (confirm_delete_no);
  signal_handlers[0] = GTK_SIGNAL_FUNC (confirm_delete_yes);

  delete_dialog = my_show_dialog (message, 2, buttons, default_button, signal_handlers, node);
  gtk_signal_connect (GTK_OBJECT (delete_dialog), "delete_event", GTK_SIGNAL_FUNC (confirm_delete_no), NULL);

  gtk_widget_show (delete_dialog);
  gtk_main ();
  gtk_widget_destroy (delete_dialog);

  return delete_or_not;
}

gboolean on_xfumed_window_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  if (xfmenu->saved || confirm_quit (xfmenu))
    {
      gtk_main_quit ();
      return TRUE;
    }
  else
    return FALSE;
}


gboolean on_xfumed_window_destroy_event (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  gtk_main_quit ();
  return TRUE;
}


void
on_ctree_tree_collapse (GtkCTree * ctree, GList * node, gpointer user_data)
{
}


void
on_ctree_tree_expand (GtkCTree * ctree, GList * node, gpointer user_data)
{

}


void
on_ctree_tree_select_row (GtkCTree * ctree, GList * node, gint column, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  xfmenu->selected_node = (GtkCTreeNode *) node;
}


void
on_ctree_tree_unselect_row (GtkCTree * ctree, GList * node, gint column, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  xfmenu->selected_node = NULL;
}


void
on_ctree_drag_begin (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  gtk_signal_handler_block_by_func (GTK_OBJECT (xfmenu->ctree), (GtkSignalFunc) on_ctree_button_release_event, xfmenu);
}


void
on_ctree_drag_data_delete (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data)
{
}


void
on_ctree_drag_data_get (GtkWidget * widget, GdkDragContext * drag_context, GtkSelectionData * data, guint info, guint time, gpointer user_data)
{
  /* Not really used, but something needs to be send
   * for DND to work (I think) */
  char ddn_buf[] = "    ";

  gtk_selection_data_set (data, GDK_SELECTION_TYPE_STRING, 8,	/* 8 bits per character. */
			  ddn_buf, strlen (ddn_buf));
}

void
on_ctree_drag_data_received (GtkWidget * widget, GdkDragContext * drag_context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  int row;
  GtkCTreeNode *parent, *sibling, *node;

  if (!GTK_IS_CTREE (widget) || widget != gtk_drag_get_source_widget (drag_context))
    return;

  row = -1;
  y -= GTK_CLIST (xfmenu->ctree)->column_title_area.height;

  if (gtk_clist_get_selection_info (GTK_CLIST (xfmenu->ctree), x, y, &row, NULL) && info == TARGET_ID && row >= 0)
    {
      sibling = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), row);
      parent = GTK_CTREE_ROW (sibling)->parent;
      node = xfmenu->selected_node;

      if (node != sibling)
	{
	  /* code to be able to drag into empty subdirs */
	  if (!GTK_CTREE_ROW (sibling)->is_leaf && 
	      GTK_CTREE_ROW (sibling)->children==NULL)
	  {
	    parent=sibling;
	    sibling=NULL;
	  }
	  gtk_ctree_move (GTK_CTREE (xfmenu->ctree), node, parent, sibling);
	  xfmenu->first_node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), 0);
	  xfmenu->saved = FALSE;
	}
    }
}


void
on_ctree_drag_end (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  gtk_signal_handler_unblock_by_func (GTK_OBJECT (xfmenu->ctree), (GtkSignalFunc) on_ctree_button_release_event, xfmenu);
}


gboolean on_ctree_button_press_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  int x = event->x, y = event->y, row;

  if (event->button <= 3 && gtk_clist_get_selection_info (GTK_CLIST (xfmenu->ctree), x, y, &row, NULL))
    {
      GtkCTreeNode *node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), row);

      gtk_ctree_select (GTK_CTREE (xfmenu->ctree), node);
    }

  return TRUE;
}

gboolean on_ctree_button_release_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  int x = event->x, y = event->y, row;

  if (event->button <= 3 && gtk_clist_get_selection_info (GTK_CLIST (xfmenu->ctree), x, y, &row, NULL))
    {
      GtkCTreeNode *node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), row);

      gtk_ctree_select (GTK_CTREE (xfmenu->ctree), node);
    }

  return TRUE;
}


void
on_up_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  GtkCTreeNode *node, *parent, *prevnode, *prevparent, *first;

  node = xfmenu->selected_node;
  first = xfmenu->first_node;

  if (!node || node == first)
    return;

  parent = GTK_CTREE_ROW (node)->parent;
  prevnode = GTK_CTREE_NODE_PREV (node);
  prevparent = GTK_CTREE_ROW (prevnode)->parent;

  if (prevnode == parent)
    return;

  while (prevparent && prevparent != parent)
    {
      prevnode = prevparent;
      prevparent = GTK_CTREE_ROW (prevnode)->parent;
    }
  if (prevnode)
    {
      gtk_ctree_move (GTK_CTREE (xfmenu->ctree), node, prevparent, prevnode);
      xfmenu->first_node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), 0);

      if (gtk_ctree_node_is_visible (GTK_CTREE (xfmenu->ctree), node) != GTK_VISIBILITY_FULL)
	gtk_ctree_node_moveto (GTK_CTREE (xfmenu->ctree), node, 0.0, 0, 0);
    }
}


void
on_down_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  GtkCTreeNode *node, *parent, *nextnode, *nextparent, *nextnext;

  if (!(node = xfmenu->selected_node))
    return;

  parent = GTK_CTREE_ROW (node)->parent;
  nextnode = GTK_CTREE_NODE_NEXT (node);
  nextparent = nextnode ? GTK_CTREE_ROW (nextnode)->parent : NULL;

  while (nextnode && gtk_ctree_is_ancestor (GTK_CTREE (xfmenu->ctree), node, nextnode))
    {
      nextnode = GTK_CTREE_NODE_NEXT (nextnode);
      nextparent = nextnode ? GTK_CTREE_ROW (nextnode)->parent : NULL;
    }

  if (parent && nextparent == GTK_CTREE_ROW (parent)->parent)
    {
      nextparent = parent;
      nextnode = NULL;
      gtk_ctree_move (GTK_CTREE (xfmenu->ctree), node, nextparent, nextnode);
      xfmenu->first_node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), 0);

      if (gtk_ctree_node_is_visible (GTK_CTREE (xfmenu->ctree), node) != GTK_VISIBILITY_FULL)
	gtk_ctree_node_moveto (GTK_CTREE (xfmenu->ctree), node, 1.0, 0, 0);

      xfmenu->saved = FALSE;
      return;
    }

  nextnext = nextnode ? GTK_CTREE_NODE_NEXT (nextnode) : NULL;
  nextparent = nextnext ? GTK_CTREE_ROW (nextnext)->parent : NULL;

  while (nextnext && gtk_ctree_is_ancestor (GTK_CTREE (xfmenu->ctree), nextnode, nextnext))
    {
      nextnext = GTK_CTREE_NODE_NEXT (nextnext);
      nextparent = nextnext ? GTK_CTREE_ROW (nextnext)->parent : NULL;
    }

  if (parent && nextparent == GTK_CTREE_ROW (parent)->parent)
    {
      nextparent = parent;
      nextnext = NULL;
    }

  gtk_ctree_move (GTK_CTREE (xfmenu->ctree), node, nextparent, nextnext);
  xfmenu->first_node = gtk_ctree_node_nth (GTK_CTREE (xfmenu->ctree), 0);

  if (gtk_ctree_node_is_visible (GTK_CTREE (xfmenu->ctree), node) != GTK_VISIBILITY_FULL)
    gtk_ctree_node_moveto (GTK_CTREE (xfmenu->ctree), node, 1.0, 0, 0);

  xfmenu->saved = FALSE;
}


void
on_add_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;

  dlg->update = FALSE;

  dlg->window = create_edit_dialog (xfmenu);
  gtk_widget_show (dlg->window);
}


void
on_edit_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;
  GtkCTreeNode *node = xfmenu->selected_node;
  char *caption;

  if (!node)
    return;

  if (GTK_CTREE_ROW (xfmenu->selected_node)->is_leaf)
    {
      gtk_ctree_node_get_pixtext (GTK_CTREE (xfmenu->ctree), node, 0, &caption, NULL, NULL, NULL);
      if (strcmp (NOP_CAPTION, caption) == 0)
	return;
    }

  dlg->update = TRUE;

  dlg->window = create_edit_dialog (xfmenu);
  gtk_widget_show (dlg->window);
}


void
on_remove_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  GtkCTreeNode *node = xfmenu->selected_node;

  if (!node)
    return;

  if (GTK_CTREE_ROW (node)->is_leaf || confirm_delete (node))
    remove_node (xfmenu, node);
}


gboolean on_ctree_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  GtkCTreeNode *node = xfmenu->selected_node;
  GtkCTreeRow *row = GTK_CTREE_ROW (node);

  if (!node)
    return TRUE;

  switch (event->keyval)
    {
    case GDK_Return:
      if (!GTK_CTREE_ROW (node)->is_leaf)
	gtk_ctree_toggle_expansion (GTK_CTREE (xfmenu->ctree), node);
      break;
    case GDK_Delete:
      on_remove_button_clicked (NULL, user_data);
      break;
    case GDK_Up:
      if (GTK_CTREE_NODE_PREV (node))
	gtk_ctree_select (GTK_CTREE (xfmenu->ctree), GTK_CTREE_NODE_PREV (node));
      break;
    case GDK_Down:
      if (GTK_CTREE_NODE_NEXT (node))
	gtk_ctree_select (GTK_CTREE (xfmenu->ctree), GTK_CTREE_NODE_NEXT (node));
      break;
    case GDK_Left:
      if (!row->is_leaf && row->expanded)
	gtk_ctree_collapse (GTK_CTREE (xfmenu->ctree), node);
      else if (row->parent)
	gtk_ctree_select (GTK_CTREE (xfmenu->ctree), row->parent);
      break;
    case GDK_Right:
      if (!row->is_leaf)
	{
	  if (!row->expanded)
	    gtk_ctree_expand (GTK_CTREE (xfmenu->ctree), node);
	  else if (row->children)
	    gtk_ctree_select (GTK_CTREE (xfmenu->ctree), row->children);
	}
      break;
    default:
      return FALSE;
    }

  return TRUE;
}


void
on_save_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  save (xfmenu);
}


void
on_reset_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  reset (xfmenu);
}


void
on_quit_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;

  if (xfmenu->saved || confirm_quit (xfmenu))
    gtk_main_quit ();
}



void
on_fileselect_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;
  char *selected_command = open_fileselect (NULL);

  if (selected_command)
    {
      gtk_entry_set_text (GTK_ENTRY (dlg->cmd_entry), selected_command);
      gtk_editable_set_position (GTK_EDITABLE (dlg->cmd_entry), -1);
    }
}


char *
make_menuname (char *name)
{
  char *menuname = g_strdup (name);

  g_strdown (menuname);
  menuname = g_strdelimit (menuname, " ", '_');

  return menuname;
}

void
on_dlg_ok_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;
  char *text[3], *type;
  gboolean is_leaf;

  type = gtk_editable_get_chars (GTK_EDITABLE (dlg->entry_type_entry), 0, -1);

  text[0] = gtk_editable_get_chars (GTK_EDITABLE (dlg->caption_entry), 0, -1);
  text[1] = gtk_editable_get_chars (GTK_EDITABLE (dlg->cmd_entry), 0, -1);

  gtk_widget_destroy (dlg->window);

  if (strcmp (type, DLG_SUBMENUTEXT) == 0)
    {
      text[2] = make_menuname (text[0]);
      is_leaf = FALSE;
    }
  else
    {
      text[2] = NULL;
      is_leaf = TRUE;
    }

  if (dlg->update)
    {
      update_node (xfmenu, text);
    }
  else
    {
      GtkCTreeNode *node = xfmenu->selected_node;
      GtkCTreeRow *row = node ? GTK_CTREE_ROW (node) : NULL;
      GtkCTreeNode *parent = row ? row->parent : NULL;
      GtkCTreeNode *sibling = row ? row->sibling : NULL;

      if (row && !row->is_leaf && (row->expanded || !row->children))
	{
	  parent = node;
	  sibling = row->children;
	}

      insert_node (xfmenu, parent, sibling, text, is_leaf);
    }
}


void
on_dlg_cancel_button_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;

  gtk_widget_destroy (dlg->window);
}


void
on_item_entry_changed (GtkEditable * editable, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;
  char *type = gtk_editable_get_chars (GTK_EDITABLE (dlg->entry_type_entry), 0, -1);

  if (g_strcasecmp (type, DLG_NOPTEXT) == 0)
    {
      gtk_entry_set_text (GTK_ENTRY (dlg->caption_entry), NOP_CAPTION);
      gtk_entry_set_text (GTK_ENTRY (dlg->cmd_entry), "");
      gtk_widget_set_sensitive (dlg->caption_entry, FALSE);
      gtk_widget_set_sensitive (dlg->cmd_entry, FALSE);
      gtk_widget_set_sensitive (dlg->fileselect_button, FALSE);
    }
  else if (g_strcasecmp (type, DLG_SUBMENUTEXT) == 0)
    {
      gtk_entry_set_text (GTK_ENTRY (dlg->cmd_entry), "");
      gtk_widget_set_sensitive (dlg->caption_entry, TRUE);
      gtk_widget_set_sensitive (dlg->cmd_entry, FALSE);
      gtk_widget_set_sensitive (dlg->fileselect_button, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (dlg->caption_entry, TRUE);
      gtk_widget_set_sensitive (dlg->cmd_entry, TRUE);
      gtk_widget_set_sensitive (dlg->fileselect_button, TRUE);
    }

  g_free (dlg->entry);
  dlg->entry = type;
}

void
on_button_fileselect_clicked (GtkButton * button, gpointer user_data)
{
  XFMENU *xfmenu = (XFMENU *) user_data;
  XFDLG *dlg = xfmenu->dlg;
  char *selected_command = open_fileselect (NULL);

  if (selected_command)
    {
      gtk_entry_set_text (GTK_ENTRY (dlg->cmd_entry), selected_command);
      gtk_editable_set_position (GTK_EDITABLE (dlg->cmd_entry), -1);
    }
}

gboolean on_edit_dialog_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
  switch (event->keyval)
    {
    case GDK_Return:
      on_dlg_ok_button_clicked (NULL, user_data);
      break;
    case GDK_Escape:
      on_dlg_cancel_button_clicked (NULL, user_data);
    default:
      return FALSE;
    }

  return TRUE;
}
