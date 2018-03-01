/*
 * gtk_prop.c
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia Copyright 2001-2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef HAVE_CONFIG_H   
#  include <config.h>   
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "my_intl.h"
#include "gtk_prop.h"
#include "gtk_dlg.h"
#include "xtree_mess.h"
#include "xtree_cfg.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


#define box_pack_start(box,w) \
	gtk_box_pack_start(GTK_BOX(box),w,TRUE,FALSE,0)
#define box_pack_end(box,w) \
	gtk_box_pack_end(GTK_BOX(box),w,TRUE,FALSE,0)

#define X_PAD 8
#define Y_PAD 1
#define TBL_XOPT GTK_EXPAND

typedef struct
{
  GtkWidget *top;
  GtkWidget *user;
  GtkWidget *group;
  fprop *prop;
  int result;
  int type;
}
dlg;

static dlg dl;

/*
 */
static GtkWidget *
label_new (char *text, GtkJustification j_type)
{
  GtkWidget *label;
  label = gtk_label_new (text);
  gtk_label_set_justify (GTK_LABEL (label), j_type);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  /* j_type == GTK_JUSTIFY_RIGHT? 1.0 : 0.0, 0.5); */
  return (label);
}

/*
 */
static void
on_cancel (GtkWidget * btn, gpointer * data)
{
  if ((int) ((long) data) != DLG_RC_DESTROY)
  {
    dl.result = (int) ((long) data);
    gtk_widget_destroy (dl.top);
  }
  gtk_main_quit ();
}

/*
 */
static void
on_ok (GtkWidget * ok, gpointer * data)
{
  char *val;
  struct passwd *pw;
  struct group *gr;

  val = gtk_entry_get_text (GTK_ENTRY (dl.user));
  if (val)
  {
    pw = getpwnam (val);
    if (pw)
    {
      dl.prop->uid = pw->pw_uid;
    }
  }
  val = gtk_entry_get_text (GTK_ENTRY (dl.group));
  if (val)
  {
    gr = getgrnam (val);
    if (gr)
    {
      dl.prop->gid = gr->gr_gid;
    }
  }
  gtk_widget_destroy (dl.top);

  dl.result = (int) ((long) data);
  gtk_main_quit ();
}

/*
 */
static void
cb_perm (GtkWidget * toggle, void *data)
{
  int bit = (int) ((long) data);
  if (GTK_TOGGLE_BUTTON (toggle)->active)
    dl.prop->mode |= (mode_t) bit;
  else
    dl.prop->mode &= (mode_t) ~ bit;
}

/*
 */
static gint
on_key_press (GtkWidget * w, GdkEventKey * event, void *data)
{
  if (event->keyval == GDK_Escape)
  {
    on_cancel ((GtkWidget *) data, (gpointer) ((long) DLG_RC_CANCEL));
    return (TRUE);
  }
  return (FALSE);
}

/*
 * create a modal dialog for properties and handle it
 * dlg_prop is deprecated. use xf_dlg_prop instead */

gint dlg_prop(char *path, fprop * prop, int flags)
{
 return (xf_dlg_prop (NULL,path,prop,flags));
}

static GtkWidget *create_text (GtkWidget * parent, char *text)
{
  GtkWidget *scroll, *helpText;
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_border_width (GTK_CONTAINER (scroll), 2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (parent), scroll, TRUE, TRUE, 0);
  helpText = gtk_label_new (text);
  gtk_label_set_justify (GTK_LABEL (helpText), GTK_JUSTIFY_LEFT);
  gtk_scrolled_window_add_with_viewport ((GtkScrolledWindow *) scroll, helpText);
  return scroll;
}

/* FIXME: change memory for strings from static stack to dynamic heap */
gint
xf_dlg_prop (GtkWidget *ctree,char *path, fprop * prop, int flags)
{
  GtkWidget *ok = NULL, *cancel = NULL, *label, *skip, *all, *notebook, *table, *owner[4], *perm[15], *info[12];
  struct tm *t;
  struct passwd *pw;
  struct group *gr;
  char buf[PATH_MAX + 1];
  char cmd[PATH_MAX + 1];
  GList *g_user = NULL;
  GList *g_group = NULL, *g_tmp;
  int n, len;
  FILE *pipe;
#ifndef LINE_MAX
#define LINE_MAX	1024
#endif
  char line[LINE_MAX + 1];
  cfg *win;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));

  dl.result = 0;
  dl.prop = prop;
  dl.top = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dl.top), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (dl.top), _("Properties"));
  gtk_signal_connect (GTK_OBJECT (dl.top), "destroy", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_DESTROY));
  gtk_window_set_modal (GTK_WINDOW (dl.top), TRUE);
  if (win->top) gtk_window_set_transient_for (GTK_WINDOW (dl.top), GTK_WINDOW (win->top)); 

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->vbox), notebook, TRUE, TRUE, 0);

  /* ok and cancel buttons */
  ok = gtk_button_new_with_label (_("Ok"));
  cancel = gtk_button_new_with_label (_("Cancel"));

  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), ok, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), cancel, TRUE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_OK));
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_CANCEL));
  gtk_widget_grab_default (cancel);

  if (flags & IS_MULTI)
  {
    skip = gtk_button_new_with_label (_("Skip"));
    all = gtk_button_new_with_label (_("All"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), skip, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dl.top)->action_area), all, TRUE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT (skip), "clicked", GTK_SIGNAL_FUNC (on_cancel), (gpointer) ((long) DLG_RC_SKIP));
    gtk_signal_connect (GTK_OBJECT (all), "clicked", GTK_SIGNAL_FUNC (on_ok), (gpointer) ((long) DLG_RC_ALL));
    GTK_WIDGET_SET_FLAGS (skip, GTK_CAN_DEFAULT);
    GTK_WIDGET_SET_FLAGS (all, GTK_CAN_DEFAULT);
  }


  /* date and size page */
  label = gtk_label_new (_("Info"));
  table = gtk_table_new (6, 2, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  n = 0;
  info[n] = label_new (_("Name :"), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
  info[n + 1] = label_new (path, GTK_JUSTIFY_LEFT);
  gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
  n += 2;

  sprintf (cmd, "file \"%s\"", path);
  pipe = popen (cmd, "r");
  if (pipe)
  {
    char *p;
    fgets (line, LINE_MAX, pipe);
    len = strlen (line);
    line[len - 1] = '\0';
    pclose (pipe);
    if ((p = strstr (line, ": ")) != NULL)
    {
      p += 2;
      info[n + 1] = label_new (p, GTK_JUSTIFY_LEFT);
      info[n] = label_new (_("Type :"), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
      gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
      n += 2;
    }
  }

    {
      char *tag="bytes";
      unsigned long long tama;
      tama =  prop->size;
      if (tama >= (long long)1024*1024*1024*10) {tama /= (long long)1024*1024*1024; tag="Gbytes";}
      else if (tama >= 1024*1024*10) {tama /= 1024*1024; tag="Mbytes";}
      else if (tama >= 1024*10) {tama /= 1024; tag="Kbytes";}
      sprintf (buf, " %llu %s", tama,tag);
    }
/*  if ((win->preferences & SIZE_IN_KB) && (prop->size >= 1024)) 
	  sprintf (buf, _("%lld KBytes"), (long long) ((unsigned long long) prop->size/1024));
  else sprintf (buf, _("%lld Bytes"), (long long) prop->size);*/
  info[n + 1] = label_new (buf, GTK_JUSTIFY_LEFT);
  info[n] = label_new (_("Size :"), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
  n += 2;

  t = localtime (&prop->ctime);
  sprintf (buf, "%04d/%02d/%02d  %02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
  info[n + 1] = gtk_label_new (buf);
  info[n] = gtk_label_new (_("Creation Time :"));
  gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
  n += 2;

  t = localtime (&prop->mtime);
  sprintf (buf, "%02d/%02d/%02d  %02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
  info[n + 1] = gtk_label_new (buf);
  info[n] = gtk_label_new (_("Modification Time :"));
  gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
  n += 2;

  t = localtime (&prop->atime);
  sprintf (buf, "%04d/%02d/%02d  %02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
  info[n + 1] = gtk_label_new (buf);
  info[n] = gtk_label_new (_("Access Time :"));
  gtk_table_attach (GTK_TABLE (table), info[n], 0, 1, n, n + 1, TBL_XOPT, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), info[n + 1], 1, 2, n, n + 1, TBL_XOPT, 0, 0, 0);
  n += 2;

  /* permissions page */
  if (!(flags & IS_STALE_LINK))
  {
    label = gtk_label_new (_("Permissions"));
    table = gtk_table_new (3, 5, FALSE);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

    perm[0] = gtk_label_new (_("Owner :"));
    perm[1] = gtk_check_button_new_with_label (_("Read"));
    if (prop->mode & S_IRUSR)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[1]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[1]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IRUSR));
    perm[2] = gtk_check_button_new_with_label (_("Write"));
    if (prop->mode & S_IWUSR)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[2]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[2]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IWUSR));
    perm[3] = gtk_check_button_new_with_label (_("Execute"));
    if (prop->mode & S_IXUSR)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[3]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[3]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IXUSR));
    perm[4] = gtk_check_button_new_with_label (_("Set UID"));
    if (prop->mode & S_ISUID)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[4]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[4]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_ISUID));

    gtk_table_attach (GTK_TABLE (table), perm[0], 0, 1, 0, 1, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[1], 1, 2, 0, 1, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[2], 2, 3, 0, 1, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[3], 3, 4, 0, 1, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[4], 4, 5, 0, 1, 0, 0, X_PAD, 0);

    perm[5] = gtk_label_new (_("Group :"));
    perm[6] = gtk_check_button_new_with_label (_("Read"));
    if (prop->mode & S_IRGRP)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[6]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[6]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IRGRP));
    perm[7] = gtk_check_button_new_with_label (_("Write"));
    if (prop->mode & S_IWGRP)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[7]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[7]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IWGRP));
    perm[8] = gtk_check_button_new_with_label (_("Execute"));
    if (prop->mode & S_IXGRP)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[8]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[8]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IXGRP));
    perm[9] = gtk_check_button_new_with_label (_("Set GID"));
    if (prop->mode & S_ISGID)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[9]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[9]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_ISGID));
    gtk_table_attach (GTK_TABLE (table), perm[5], 0, 1, 1, 2, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[6], 1, 2, 1, 2, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[7], 2, 3, 1, 2, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[8], 3, 4, 1, 2, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[9], 4, 5, 1, 2, 0, 0, X_PAD, 0);

    perm[10] = gtk_label_new (_("Other :"));
    perm[11] = gtk_check_button_new_with_label (_("Read"));
    if (prop->mode & S_IROTH)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[11]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[11]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IROTH));
    perm[12] = gtk_check_button_new_with_label (_("Write"));
    if (prop->mode & S_IWOTH)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[12]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[12]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IWOTH));
    perm[13] = gtk_check_button_new_with_label (_("Execute"));
    if (prop->mode & S_IXOTH)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[13]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[13]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_IXOTH));
    perm[14] = gtk_check_button_new_with_label (_("Sticky"));
    if (prop->mode & S_ISVTX)
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (perm[14]), 1);
    gtk_signal_connect (GTK_OBJECT (perm[14]), "clicked", GTK_SIGNAL_FUNC (cb_perm), (gpointer) ((long) S_ISVTX));
    gtk_table_attach (GTK_TABLE (table), perm[10], 0, 1, 2, 3, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[11], 1, 2, 2, 3, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[12], 2, 3, 2, 3, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[13], 3, 4, 2, 3, 0, 0, X_PAD, 0);
    gtk_table_attach (GTK_TABLE (table), perm[14], 4, 5, 2, 3, 0, 0, X_PAD, 0);
  }

  /* owner/group page */
  while ((pw = getpwent ()) != NULL)
  {
    g_user = g_list_append (g_user, g_strdup (pw->pw_name));
  }
  g_user = g_list_sort (g_user, (GCompareFunc) strcmp);
  endpwent ();

  while ((gr = getgrent ()) != NULL)
  {
    g_group = g_list_append (g_group, g_strdup (gr->gr_name));
  }
  endgrent ();
  g_group = g_list_sort (g_group, (GCompareFunc) strcmp);

  label = gtk_label_new (_("Owner"));
  table = gtk_table_new (2, 2, FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  pw = getpwuid (prop->uid);
  sprintf (buf, "%s", pw ? pw->pw_name : _("unknown"));
  owner[1] = gtk_combo_new ();
  dl.user = GTK_WIDGET (GTK_COMBO (owner[1])->entry);
  if (g_user)
    gtk_combo_set_popdown_strings (GTK_COMBO (owner[1]), g_user);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (owner[1])->entry), buf);
  owner[0] = label_new (_("Owner :"), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (table), owner[0], 0, 1, 0, 1, 0, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), owner[1], 1, 2, 0, 1, 0, 0, X_PAD, 0);

  gr = getgrgid (prop->gid);
  sprintf (buf, "%s", gr ? gr->gr_name : _("unknown"));
  owner[3] = gtk_combo_new ();
  dl.group = GTK_WIDGET (GTK_COMBO (owner[3])->entry);
  if (g_group)
    gtk_combo_set_popdown_strings (GTK_COMBO (owner[3]), g_group);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (owner[3])->entry), buf);
  owner[2] = label_new (_("Group :"), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (GTK_TABLE (table), owner[2], 0, 1, 1, 2, 0, 0, X_PAD, Y_PAD);
  gtk_table_attach (GTK_TABLE (table), owner[3], 1, 2, 1, 2, 0, 0, X_PAD, 0);

/* for rpm files and only if rpm can be execd */
  {
    char *loc=NULL;
    loc=strrchr(path,'.');
    if ((loc) &&  (strcmp(loc,".rpm")==0)){
     sprintf (cmd, "rpm -qip \"%s\"", path);
     pipe = popen (cmd, "r");
     if (pipe){
      char *p;
      p=(char *)malloc(1);
      p[0]=0;
      while (fgets(line,LINE_MAX-1,pipe)){
	line[LINE_MAX-1]=0;
        p=(char *)realloc((void *)p,strlen(line)+strlen(p)+1);	       
	strcat(p,line);    
      }
      if (strlen(p)) {
       label = gtk_label_new (_("RPM info"));
       table = gtk_vbox_new (FALSE, 0);
       gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);
       create_text (table, p);
      }
      pclose (pipe);
      if (p) free(p);
     }
    }
  }
 
  

  gtk_signal_connect (GTK_OBJECT (dl.top), "key_press_event", GTK_SIGNAL_FUNC (on_key_press), (gpointer) cancel);
  gtk_widget_show_all (dl.top);
  gtk_main ();

  /* free the lists */
  g_tmp = g_user;
  while (g_tmp)
  {
    g_free (g_tmp->data);
    g_tmp = g_tmp->next;
  }
  g_list_free (g_user);
  g_tmp = g_group;
  while (g_tmp)
  {
    g_free (g_tmp->data);
    g_tmp = g_tmp->next;
  }
  g_list_free (g_group);
  return (dl.result);
}
