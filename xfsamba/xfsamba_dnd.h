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
void
on_drag_data (GtkWidget * ctree, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client);
gboolean
on_drag_motion (GtkWidget * ctree, GdkDragContext * dc, gint x, gint y, guint t, gpointer data);
void
on_drag_data_get (GtkWidget * ctree, GdkDragContext * context, GtkSelectionData * selection_data, guint info, guint time, gpointer data);

