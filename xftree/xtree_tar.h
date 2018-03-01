GtkCTreeNode *add_tar_tree(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en);
GtkCTreeNode *add_tar_dummy(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en);
int tar_delete(GtkCTree *ctree,char *path);
int tar_extract(GtkCTree *ctree,char *tgt,char *src);

void cb_tar_open_with (GtkWidget * item, GtkCTree * ctree);
