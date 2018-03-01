#include <gtk/gtk.h>


gboolean on_xfumed_window_delete_event (GtkWidget * widget, GdkEvent * event, gpointer user_data);

gboolean on_xfumed_window_destroy_event (GtkWidget * widget, GdkEvent * event, gpointer user_data);

void on_ctree_tree_collapse (GtkCTree * ctree, GList * node, gpointer user_data);

void on_ctree_tree_expand (GtkCTree * ctree, GList * node, gpointer user_data);

void on_ctree_tree_select_row (GtkCTree * ctree, GList * node, gint column, gpointer user_data);

void on_ctree_tree_unselect_row (GtkCTree * ctree, GList * node, gint column, gpointer user_data);

void on_ctree_drag_begin (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data);

void on_ctree_drag_data_delete (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data);

void on_ctree_drag_data_get (GtkWidget * widget, GdkDragContext * drag_context, GtkSelectionData * data, guint info, guint time, gpointer user_data);

void on_ctree_drag_data_received (GtkWidget * widget, GdkDragContext * drag_context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer user_data);

void on_ctree_drag_end (GtkWidget * widget, GdkDragContext * drag_context, gpointer user_data);

gboolean on_ctree_button_press_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data);

gboolean on_ctree_button_release_event (GtkWidget * widget, GdkEventButton * event, gpointer user_data);

void on_up_button_clicked (GtkButton * button, gpointer user_data);

void on_down_button_clicked (GtkButton * button, gpointer user_data);

void on_add_button_clicked (GtkButton * button, gpointer user_data);

void on_edit_button_clicked (GtkButton * button, gpointer user_data);

void on_remove_button_clicked (GtkButton * button, gpointer user_data);

gboolean on_ctree_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data);

void on_save_button_clicked (GtkButton * button, gpointer user_data);

void on_reset_button_clicked (GtkButton * button, gpointer user_data);

void on_quit_button_clicked (GtkButton * button, gpointer user_data);

gboolean on_dialog1_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data);

void on_fileselect_button_clicked (GtkButton * button, gpointer user_data);

void on_dlg_ok_button_clicked (GtkButton * button, gpointer user_data);

void on_dlg_cancel_button_clicked (GtkButton * button, gpointer user_data);

void on_item_entry_changed (GtkEditable * editable, gpointer user_data);

gboolean on_edit_dialog_key_press_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data);
