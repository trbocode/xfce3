#ifndef __XTREE_CB_H__
#define __XTREE_CB_H__

void cb_rox (GtkWidget * top,GtkWidget * ctree);
void 
cb_open_trash (GtkWidget * item, GtkCTree *ctree);
void
cb_new_window (GtkWidget * widget, GtkCTree * ctree);
void
cb_select (GtkWidget * item, GtkCTree * ctree);
void
cb_unselect (GtkWidget * widget, GtkCTree * ctree);
void
cb_diff (GtkWidget * widget, GtkCTree * ctree);
void
cb_patch (GtkWidget * widget, GtkCTree * ctree);
void
cb_empty_trash (GtkWidget * widget, GtkCTree * ctree);
void
cb_delete (GtkWidget * widget, GtkCTree * ctree);
void
cb_find (GtkWidget * item, GtkWidget * ctree);
void
cb_about (GtkWidget * item, GtkWidget * ctree);
void
cb_new_subdir (GtkWidget * item, GtkWidget * ctree);
void
cb_new_file (GtkWidget * item, GtkWidget * ctree);
void
cb_rename (GtkWidget * item, GtkCTree * ctree);
void
cb_open_with (GtkWidget * item, GtkCTree * ctree);
void
cb_props (GtkWidget * item, GtkCTree * ctree);
void
on_destroy (GtkWidget * top,  GtkCTree * ctree);
void
cb_destroy (GtkWidget * top, GtkCTree * ctree);
void 
cb_quit (GtkWidget * top,  GtkCTree * ctree);
void
cb_term (GtkWidget * item, GtkWidget * ctree);
void
cb_exec (GtkWidget * top, GtkWidget * ctree);
void 
cb_refresh (GtkWidget * item, GtkCTree * ctree);
/* at xtree_du.c: */
void 
cb_du (GtkWidget * item, GtkCTree * ctree);
void 
cb_samba (GtkWidget * item, GtkCTree * ctree);
void 
cb_autotype(GtkWidget * item, GtkCTree * ctree);
void 
cb_autotar(GtkWidget * item, GtkCTree * ctree);
char *valid_path(GtkCTree *ctree,gboolean expand);
char *file_path(char *path);
#endif
