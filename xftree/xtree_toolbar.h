/*
 * Edscott Wilson Garcia Copyright 2001-2002 gnu-gpl*/
#ifndef __XTREE_TOOLBAR_H__
#define __XTREE_TOOLBAR_H__

GtkWidget *
create_toolbar (GtkWidget * top, GtkWidget * ctree, cfg * win,gboolean large);
GdkPixmap *duplicate_xpm(GtkWidget *widget,char **xpm,GdkBitmap **mask);

void cb_config_toolbar(GtkWidget *widget,GtkWidget *ctree);
void toggle_toolbar(GtkWidget * widget, GtkWidget *ctree);

#endif
