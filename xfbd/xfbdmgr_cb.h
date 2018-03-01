#ifndef __XFBDMGR_CB_H__
#define __XFBDMGR_CB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

void Myquit (GtkWidget *, gpointer);
void save_clicked (GtkWidget *, gpointer);
void load_clicked (GtkWidget *, gpointer);
void add_clicked (GtkWidget *, gpointer);
void remove_clicked (GtkWidget *, gpointer);
void edit_changed (GtkWidget *, gpointer);
void selectionMade (GtkWidget *, gint, gint, GdkEventButton *, gpointer);
void selectionLost (GtkWidget *, gint, gint, GdkEventButton *, gpointer);
void add_filename (gchar *);
void get_add_filename (GtkFileSelection *, gpointer);
void get_load_filename (GtkFileSelection *, gpointer);
void get_save_filename (GtkFileSelection *, gpointer);
void add_filename (gchar *);
void on_drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, gpointer cbdata);


gint selected;
GtkWidget *fileselector;
GtkWidget *fileselector1;
GtkWidget *fileselector2;

#endif
