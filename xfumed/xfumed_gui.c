#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xfumed.h"
#include "xfumed_gui.h"
#include "xfumed_gui_cb.h"


GtkWidget *
create_xfumed_window (XFMENU * xfmenu)
{
  GtkWidget *xfumed_window;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GtkWidget *vbox2;
  GtkWidget *scrolledwindow1;
  GtkWidget *ctree = xfmenu->ctree;
  GtkWidget *caption_label;
  GtkWidget *cmd_label;
  GtkWidget *action_frame;
  GtkWidget *vbuttonbox1;
  GtkWidget *up_button;
  GtkWidget *down_button;
  GtkWidget *add_button;
  GtkWidget *edit_button;
  GtkWidget *remove_button;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox1;
  GtkWidget *save_button;
  GtkWidget *reset_button;
  GtkWidget *quit_button;

  GtkTargetEntry target_entry[1];

  xfumed_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (xfumed_window), "xfumed_window", xfumed_window);
  gtk_container_set_border_width (GTK_CONTAINER (xfumed_window), 6);
  gtk_window_set_title (GTK_WINDOW (xfumed_window), _("User Menu Editor"));

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "vbox1", vbox1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (xfumed_window), vbox1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "hbox1", hbox1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "vbox2", vbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "scrolledwindow1", scrolledwindow1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  gtk_widget_ref (ctree);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "ctree", ctree, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ctree);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), ctree);
  gtk_clist_column_titles_show (GTK_CLIST (ctree));

  caption_label = gtk_label_new (_("Caption"));
  gtk_widget_ref (caption_label);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "caption_label", caption_label, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (caption_label);
  gtk_clist_set_column_widget (GTK_CLIST (ctree), 0, caption_label);

  cmd_label = gtk_label_new (_("Command"));
  gtk_widget_ref (cmd_label);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "cmd_label", cmd_label, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cmd_label);
  gtk_clist_set_column_widget (GTK_CLIST (ctree), 1, cmd_label);

  action_frame = gtk_frame_new (_("Actions"));
  gtk_widget_ref (action_frame);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "action_frame", action_frame, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (action_frame);
  gtk_box_pack_start (GTK_BOX (hbox1), action_frame, FALSE, TRUE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (action_frame), 6);
  gtk_frame_set_label_align (GTK_FRAME (action_frame), 0.2, 0.5);

  vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_widget_ref (vbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "vbuttonbox1", vbuttonbox1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbuttonbox1);
  gtk_container_add (GTK_CONTAINER (action_frame), vbuttonbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbuttonbox1), 6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);

  up_button = gtk_button_new_with_label (_("Move Up"));
  gtk_widget_ref (up_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "up_button", up_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (up_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), up_button);
  GTK_WIDGET_SET_FLAGS (up_button, GTK_CAN_DEFAULT);

  down_button = gtk_button_new_with_label (_("Move Down"));
  gtk_widget_ref (down_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "down_button", down_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (down_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), down_button);
  GTK_WIDGET_SET_FLAGS (down_button, GTK_CAN_DEFAULT);

  add_button = gtk_button_new_with_label (_("Add"));
  gtk_widget_ref (add_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "add_button", add_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (add_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), add_button);
  GTK_WIDGET_SET_FLAGS (add_button, GTK_CAN_DEFAULT);

  edit_button = gtk_button_new_with_label (_("Edit"));
  gtk_widget_ref (edit_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "edit_button", edit_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (edit_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), edit_button);
  GTK_WIDGET_SET_FLAGS (edit_button, GTK_CAN_DEFAULT);

  remove_button = gtk_button_new_with_label (_("Remove"));
  gtk_widget_ref (remove_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "remove_button", remove_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_button);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), remove_button);
  GTK_WIDGET_SET_FLAGS (remove_button, GTK_CAN_DEFAULT);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "hseparator1", hseparator1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, TRUE, 6);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "hbuttonbox1", hbuttonbox1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbuttonbox1), 4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);

  save_button = gtk_button_new_with_label (_("Save"));
  gtk_widget_ref (save_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "save_button", save_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), save_button);
  GTK_WIDGET_SET_FLAGS (save_button, GTK_CAN_DEFAULT);

  reset_button = gtk_button_new_with_label (_("Reset"));
  gtk_widget_ref (reset_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "reset_button", reset_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (reset_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), reset_button);
  GTK_WIDGET_SET_FLAGS (reset_button, GTK_CAN_DEFAULT);

  quit_button = gtk_button_new_with_label (_("Quit"));
  gtk_widget_ref (quit_button);
  gtk_object_set_data_full (GTK_OBJECT (xfumed_window), "quit_button", quit_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (quit_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), quit_button);
  GTK_WIDGET_SET_FLAGS (quit_button, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (xfumed_window), "delete_event", GTK_SIGNAL_FUNC (on_xfumed_window_delete_event), xfmenu);
  gtk_signal_connect (GTK_OBJECT (xfumed_window), "destroy_event", GTK_SIGNAL_FUNC (on_xfumed_window_destroy_event), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_select_row", GTK_SIGNAL_FUNC (on_ctree_tree_select_row), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_unselect_row", GTK_SIGNAL_FUNC (on_ctree_tree_unselect_row), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_begin", GTK_SIGNAL_FUNC (on_ctree_drag_begin), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_get", GTK_SIGNAL_FUNC (on_ctree_drag_data_get), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_data_received", GTK_SIGNAL_FUNC (on_ctree_drag_data_received), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "drag_end", GTK_SIGNAL_FUNC (on_ctree_drag_end), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "button_press_event", GTK_SIGNAL_FUNC (on_ctree_button_press_event), xfmenu);
  gtk_signal_connect_after (GTK_OBJECT (ctree), "button_release_event", GTK_SIGNAL_FUNC (on_ctree_button_release_event), xfmenu);
  gtk_signal_connect (GTK_OBJECT (ctree), "key_press_event", GTK_SIGNAL_FUNC (on_ctree_key_press_event), xfmenu);
  gtk_signal_connect (GTK_OBJECT (up_button), "clicked", GTK_SIGNAL_FUNC (on_up_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (down_button), "clicked", GTK_SIGNAL_FUNC (on_down_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (add_button), "clicked", GTK_SIGNAL_FUNC (on_add_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (edit_button), "clicked", GTK_SIGNAL_FUNC (on_edit_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (remove_button), "clicked", GTK_SIGNAL_FUNC (on_remove_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (save_button), "clicked", GTK_SIGNAL_FUNC (on_save_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (reset_button), "clicked", GTK_SIGNAL_FUNC (on_reset_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (quit_button), "clicked", GTK_SIGNAL_FUNC (on_quit_button_clicked), xfmenu);

  target_entry[0].target = TARGET_NAME;
  target_entry[0].flags = GTK_TARGET_SAME_APP;
  target_entry[0].info = TARGET_ID;

  gtk_drag_dest_set (ctree, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP, target_entry, 1, GDK_ACTION_MOVE);
  gtk_drag_source_set (ctree, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK, target_entry, 1, GDK_ACTION_MOVE);

  gtk_clist_set_column_width (GTK_CLIST (xfmenu->ctree), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (xfmenu->ctree), 1, 200);
  gtk_clist_set_column_visibility (GTK_CLIST (xfmenu->ctree), 2, FALSE);
  gtk_ctree_set_line_style (GTK_CTREE (ctree), GTK_CTREE_LINES_NONE);
  gtk_ctree_set_expander_style (GTK_CTREE (ctree), GTK_CTREE_EXPANDER_CIRCULAR);
  gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), 0, TRUE);
  gtk_clist_column_titles_passive (GTK_CLIST (ctree));

  gtk_widget_grab_focus (ctree);

  if (xfmenu->first_node)
    gtk_ctree_select (GTK_CTREE (ctree), xfmenu->first_node);

  gtk_widget_set_sensitive (GTK_CLIST (ctree)->column[0].button, FALSE);
  gtk_widget_set_sensitive (GTK_CLIST (ctree)->column[1].button, FALSE);

  return xfumed_window;
}

GtkWidget *
create_edit_dialog (XFMENU * xfmenu)
{
  GtkWidget *edit_dialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *hbox2;
  GtkWidget *table1;
  GtkWidget *type_label;
  GtkWidget *caption_label;
  GtkWidget *command_label;
  GtkWidget *item_combo;
  GList *item_combo_items = NULL;
  GtkWidget *item_entry;
  GtkWidget *caption_entry;
  GtkWidget *hbox3;
  GtkWidget *cmd_entry;
  GtkWidget *fileselect_button;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox2;
  GtkWidget *dlg_ok_button;
  GtkWidget *dlg_cancel_button;

  XFDLG *dlg = xfmenu->dlg;

  edit_dialog = gtk_dialog_new ();
  gtk_object_set_data (GTK_OBJECT (edit_dialog), "edit_dialog", edit_dialog);

  if (dlg->update)
    gtk_window_set_title (GTK_WINDOW (edit_dialog), _("Edit Entry"));
  else
    gtk_window_set_title (GTK_WINDOW (edit_dialog), _("Add Entry"));

  gtk_window_set_modal (GTK_WINDOW (edit_dialog), TRUE);

  dialog_vbox1 = GTK_DIALOG (edit_dialog)->vbox;
  gtk_object_set_data (GTK_OBJECT (edit_dialog), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "hbox2", hbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox2, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 10);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "table1", table1, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (hbox2), table1, TRUE, TRUE, 0);

  type_label = gtk_label_new (_("Type: "));
  gtk_widget_ref (type_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "type_label", type_label, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (type_label);
  gtk_table_attach (GTK_TABLE (table1), type_label, 0, 1, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (type_label), 0, 0.5);

  caption_label = gtk_label_new (_("Caption: "));
  gtk_widget_ref (caption_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "caption_label", caption_label, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (caption_label);
  gtk_table_attach (GTK_TABLE (table1), caption_label, 0, 1, 1, 2, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (caption_label), 0, 0.5);

  command_label = gtk_label_new (_("Command: "));
  gtk_widget_ref (command_label);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "command_label", command_label, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (command_label);
  gtk_table_attach (GTK_TABLE (table1), command_label, 0, 1, 2, 3, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (command_label), 0, 0.5);

  item_combo = gtk_combo_new ();
  gtk_widget_ref (item_combo);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "item_combo", item_combo, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (item_combo);
  gtk_table_attach (GTK_TABLE (table1), item_combo, 1, 2, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_combo_set_value_in_list (GTK_COMBO (item_combo), TRUE, TRUE);
  item_combo_items = g_list_append (item_combo_items, (gpointer) DLG_ITEMTEXT);
  item_combo_items = g_list_append (item_combo_items, (gpointer) DLG_SUBMENUTEXT);
  item_combo_items = g_list_append (item_combo_items, (gpointer) DLG_NOPTEXT);
  gtk_combo_set_popdown_strings (GTK_COMBO (item_combo), item_combo_items);
  g_list_free (item_combo_items);

  item_entry = GTK_COMBO (item_combo)->entry;
  gtk_widget_ref (item_entry);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "item_entry", item_entry, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (item_entry);
  gtk_entry_set_editable (GTK_ENTRY (item_entry), FALSE);

  gtk_entry_set_text (GTK_ENTRY (item_entry), DLG_ITEMTEXT);

  caption_entry = gtk_entry_new ();
  gtk_widget_ref (caption_entry);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "caption_entry", caption_entry, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (caption_entry);
  gtk_table_attach (GTK_TABLE (table1), caption_entry, 1, 2, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "hbox3", hbox3, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_table_attach (GTK_TABLE (table1), hbox3, 1, 2, 2, 3, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

  cmd_entry = gtk_entry_new ();
  gtk_widget_ref (cmd_entry);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "cmd_entry", cmd_entry, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cmd_entry);
  gtk_box_pack_start (GTK_BOX (hbox3), cmd_entry, TRUE, TRUE, 0);

  fileselect_button = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (fileselect_button);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "fileselect_button", fileselect_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fileselect_button);
  gtk_box_pack_start (GTK_BOX (hbox3), fileselect_button, FALSE, FALSE, 0);

  dialog_action_area1 = GTK_DIALOG (edit_dialog)->action_area;
  gtk_object_set_data (GTK_OBJECT (edit_dialog), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox2);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "hbuttonbox2", hbuttonbox2, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox2);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbuttonbox2, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_END);

  dlg_ok_button = gtk_button_new_with_label (_("Ok"));
  gtk_widget_ref (dlg_ok_button);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "dlg_ok_button", dlg_ok_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dlg_ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), dlg_ok_button);
  GTK_WIDGET_SET_FLAGS (dlg_ok_button, GTK_CAN_DEFAULT);

  dlg_cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (dlg_cancel_button);
  gtk_object_set_data_full (GTK_OBJECT (edit_dialog), "dlg_cancel_button", dlg_cancel_button, (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dlg_cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), dlg_cancel_button);
  GTK_WIDGET_SET_FLAGS (dlg_cancel_button, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (edit_dialog), "key_press_event", GTK_SIGNAL_FUNC (on_edit_dialog_key_press_event), xfmenu);
  gtk_signal_connect (GTK_OBJECT (fileselect_button), "clicked", GTK_SIGNAL_FUNC (on_fileselect_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (dlg_ok_button), "clicked", GTK_SIGNAL_FUNC (on_dlg_ok_button_clicked), xfmenu);
  gtk_signal_connect (GTK_OBJECT (dlg_cancel_button), "clicked", GTK_SIGNAL_FUNC (on_dlg_cancel_button_clicked), xfmenu);

  gtk_signal_connect (GTK_OBJECT (item_entry), "changed", GTK_SIGNAL_FUNC (on_item_entry_changed), xfmenu);

  gtk_widget_grab_default (dlg_ok_button);

  dlg->window = edit_dialog;
  dlg->entry_type_entry = item_entry;
  dlg->caption_entry = caption_entry;
  dlg->cmd_entry = cmd_entry;
  dlg->fileselect_button = fileselect_button;

  if (dlg->update)
    {
      GtkCTreeNode *node = xfmenu->selected_node;
      char *caption, *cmd;

      gtk_ctree_node_get_pixtext (GTK_CTREE (xfmenu->ctree), node, 0, &caption, NULL, NULL, NULL);
      gtk_ctree_node_get_text (GTK_CTREE (xfmenu->ctree), node, 1, &cmd);

      gtk_entry_set_text (GTK_ENTRY (caption_entry), caption);
      gtk_entry_set_text (GTK_ENTRY (cmd_entry), cmd);

      if (!GTK_CTREE_ROW (node)->is_leaf)
	gtk_entry_set_text (GTK_ENTRY (item_entry), DLG_SUBMENUTEXT);

      gtk_widget_set_sensitive (item_combo, FALSE);
    }

  return edit_dialog;
}
