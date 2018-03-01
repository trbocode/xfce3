GtkCTreeNode *add_rpm_tree(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en);
GtkCTreeNode *add_rpm_dummy(GtkCTree * ctree, GtkCTreeNode * parent,entry *p_en);

int rpm_install(GtkCTree *ctree,char *path);
