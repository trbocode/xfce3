#ifndef __XFBDMGR_H__
#define __XFBDMGR_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

GtkWidget *create_xfbdmgr ();

GtkWidget *xfbdmgr;
GtkWidget *clist_fileslist;
GtkWidget *edit_name;

void writeToList ();
void saveList ();

gint theNumber;
gboolean Changed;

#endif
