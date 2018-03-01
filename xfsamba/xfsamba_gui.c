/*  gui modules for xfsamba
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* for _( definition, it also includes config.h : */
#include "my_intl.h"
#include "constant.h"
/* for pixmap creation routines : */
#include "xfce-common.h"
#include "xpmext.h"

/* local xfsamba includes : */
#undef XFSAMBA_MAIN
#include "xfsamba.h"
#include "xfsamba_dnd.h"
#include "../xftree/xtree_icons.h"
#include "tubo.h"

static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URI_LIST},
  {"text/plain", 0, TARGET_PLAIN},
  {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))


static GtkWidget *vpaned, *hpaned, *vpaned2, *show_links, *hide_links, *show_diag, *hide_diag;
/* diagnostics=0x01, links=0x10 :*/
int view_toggle = 0x10;
static GtkWidget * user, *passwd, *dialog;
static nmb_cache *current_cache;
static GdkFont *the_font; 
static char *custom_font=NULL;
GdkColor ctree_color;

int set_fontT(GtkWidget * ctree){
	GtkStyle  *Ostyle,*style;
	Ostyle=gtk_widget_get_style (ctree);
    	style = gtk_style_copy (Ostyle);
	if ((preferences & CUSTOM_FONT)&&(custom_font)) {
	  the_font = gdk_font_load (custom_font);
          if (the_font == NULL) {
           xf_dlg_error(smb_nav,_("Could not load specified font\n"),NULL);
           preferences &= (CUSTOM_FONT ^ 0xffffffff);
           return -1;
          }
	  style->font=the_font;
	}
	if (!(preferences & CUSTOM_FONT)) style->font=Ostyle->font;
	gtk_widget_set_style (ctree,style);gtk_widget_ensure_style (ctree);
	gtk_widget_set_style (workgroups,style);gtk_widget_ensure_style (workgroups);
	gtk_widget_set_style (servers,style);gtk_widget_ensure_style (servers);
   	return (style->font->ascent + style->font->descent + 5);	
}


void set_colors(GtkWidget * ctree){
	GtkStyle *Ostyle,*style;
	int red,green,blue;
	int Nred,Ngreen,Nblue;

	red = ctree_color.red & 0xffff;
	green = ctree_color.green & 0xffff;
	blue = ctree_color.blue & 0xffff;
	Ostyle=gtk_widget_get_style (ctree);
    	style = gtk_style_copy (Ostyle);

	if (!(preferences & CUSTOM_COLORS)){
		gtk_widget_set_style (ctree,style);gtk_widget_ensure_style (ctree);
		gtk_widget_set_style (workgroups,style);gtk_widget_ensure_style (workgroups);
		gtk_widget_set_style (servers,style);gtk_widget_ensure_style (servers);
		return;
	}
	
	style->base[GTK_STATE_ACTIVE].red=red;
	style->base[GTK_STATE_ACTIVE].green=green;
	style->base[GTK_STATE_ACTIVE].blue=blue;
	  
	style->base[GTK_STATE_NORMAL].red=red;
	style->base[GTK_STATE_NORMAL].green=green;
	style->base[GTK_STATE_NORMAL].blue=blue;
	  
	style->bg[GTK_STATE_NORMAL].red=red;
	style->bg[GTK_STATE_NORMAL].green=green;
	style->bg[GTK_STATE_NORMAL].blue=blue;
	  
	style->fg[GTK_STATE_SELECTED].red=red;
	style->fg[GTK_STATE_SELECTED].green=green;
	style->fg[GTK_STATE_SELECTED].blue=blue;
	  /* foregrounds */
	
	Nred=(red^0xffff)&(0xffff);
	Ngreen=(green^0xffff)&(0xffff);
	Nblue=(blue^0xffff)&(0xffff);

	
	if (red + green + blue < 3*32768) Nred=Ngreen=Nblue=65535;
	else Nred=Ngreen=Nblue=0;
	
	style->fg[GTK_STATE_NORMAL].red=Nred;
	style->fg[GTK_STATE_NORMAL].green=Ngreen;
	style->fg[GTK_STATE_NORMAL].blue=Nblue;
	  
	style->bg[GTK_STATE_SELECTED].red=Nred;
	style->bg[GTK_STATE_SELECTED].green=Ngreen;
	style->bg[GTK_STATE_SELECTED].blue=Nblue;
	
	gtk_widget_set_style (ctree,style);gtk_widget_ensure_style (ctree);
	gtk_widget_set_style (workgroups,style);gtk_widget_ensure_style (workgroups);
	gtk_widget_set_style (servers,style);gtk_widget_ensure_style (servers);
	return;
}

 
void
cb_select_colors (GtkWidget * widget, GtkWidget * ctree)
{
  gdouble colors[4];
  gdouble *newcolor;

  colors[0] = ((gdouble) ctree_color.red) / COLOR_GDK;
  colors[1] = ((gdouble) ctree_color.green) / COLOR_GDK;
  colors[2] = ((gdouble) ctree_color.blue) / COLOR_GDK;
  
  newcolor=xfdiff_colorselect (colors);
  if (newcolor){
      ctree_color.red   = ((guint) (newcolor[0] * COLOR_GDK));
      ctree_color.green = ((guint) (newcolor[1] * COLOR_GDK));
      ctree_color.blue  = ((guint) (newcolor[2] * COLOR_GDK));
      preferences |= CUSTOM_COLORS;
  } else {
      preferences &= (CUSTOM_COLORS ^ 0xffffffff);
  }
  set_colors(shares);
  set_colors(workgroups);
  set_colors(servers);
  save_defaults ();  
  return;
}


void
cb_select_font (GtkWidget * widget, GtkWidget *ctree)
{
  char *font_selected;
  void go_reload (GtkWidget * widget, gpointer data);
  font_selected = open_fontselection ("fixed");
  preferences ^= FONT_STATE;
  if (!font_selected) {
	  preferences &= (CUSTOM_FONT ^ 0xffffffff);
  } else  {
	preferences |= CUSTOM_FONT;
  	if (custom_font) free(custom_font);
	custom_font=(char *)malloc(strlen(font_selected)+1);
	if (custom_font) strcpy(custom_font,font_selected);
	else preferences &= (CUSTOM_FONT ^ 0xffffffff);
  }
  create_pixmaps(set_fontT(shares),shares);
  save_defaults ();
  go_reload(NULL,NULL);
  return;
}


void
node_destroy (gpointer p)
{
  smb_entry_free((smb_entry *) p);
}
 
static gint
compare (GtkCList * clist, gconstpointer ptr1, gconstpointer ptr2)
{
  GtkCTreeRow *row1 = (GtkCTreeRow *) ptr1;
  GtkCTreeRow *row2 = (GtkCTreeRow *) ptr2;
  int i;
  smb_entry *e1,*e2;

  

  e1 = row1->row.data;
  e2 = row2->row.data;

    /* I want to have the directories at the top  */
   if (e1->i[2] != e2->i[2]){
    return (e2->i[2] - e1->i[2]);
  }
  /*printf("dbg:%d--%d\n",i1[2],i2[2]);*/
  
  switch (clist->sort_column)
  {
  case 2:
    i = 0;
    break;
  case 3:
    i = 1;
    break;
  case 1:
    return (strcmp(e2->label,e1->label));
  default:
    i = 0;
  }

  return e2->i[i] - e1->i[i];
}

gint
on_click_column (GtkCList * clist, gint column, gpointer data)
{
  GtkCTreeNode *node;
  GList *selection;

  if (clist != GTK_CLIST (shares))
    gtk_clist_set_compare_func (clist, NULL);
  else
  {
    if ((column == 1) ||(column == 2) || (column == 3))
    {
      gtk_clist_set_compare_func (clist, compare);
    }
    else
    {
      gtk_clist_set_compare_func (clist, NULL);
    }
  }

  if (column != clist->sort_column)
    gtk_clist_set_sort_column (clist, column);
  else
  {
    if (clist->sort_type == GTK_SORT_ASCENDING)
      clist->sort_type = GTK_SORT_DESCENDING;
    else
      clist->sort_type = GTK_SORT_ASCENDING;
  }
  selection = clist->selection;
  if (selection)
  {
    do
    {
      node = selection->data;
      if (!GTK_CTREE_ROW (node)->children || (!GTK_CTREE_ROW (node)->expanded))
      {
	node = GTK_CTREE_ROW (node)->parent;
      }
      gtk_ctree_sort_node (GTK_CTREE (clist), node);
      selection = selection->next;
    }
    while (selection);
  }
  else
  {
    gtk_clist_sort (clist);
  }
  return TRUE;
}

static void
cb_xftree (GtkWidget * item, GtkWidget * ctree){
	char *argv[2];
	argv[0]="xftree";
	argv[1]=0;
	if (!sane(argv[0])){
		xf_dlg_error(smb_nav,"Could not find",argv[0]);
	} else io_system(argv,smb_nav);
}

extern void
cb_mount (GtkWidget * item, GtkWidget * ctree);

/*void
cb_paste (GtkWidget * item, GtkCTree * ctree){
	xf_dlg_warning(smb_nav,"This function is not yet active");
}*/
static void
cb_master (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("If you have win95 nodes on your network, xfsamba might not find\n" "a master browser. If you start smb services on your linux box,\n" "making it a samba-server, the problem will be fixed as long as\n" "the win95 box(es) are reset. You know the routine, reset wind*ws\n" "for changes to take effect. Otherwise,\n" "you must type in the node to be browsed and hit RETURN.\n"));
}
static void
cb_dnd (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("Full drag and drop capability is provided between xfsamba and\n"
			  "the companion file browser \"xftree\". Drag and drop may or not\n"
			  "work with other file browsers"));
}
static void
cb_mountH (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("In order to mount SMB share, the following samba files must be suid-root:\n"
			  "- smbmnt\n"
			  "- smbumount\n"
			  "these file are usually in /usr/bin, and root must perform:\n"
			  "$ chmod u+s smbmount smbumount\n"
	  		  "(This option is not available in FreeBSD4.4)"
			  ));
}

static void
cb_search (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("To search for files on remote SMB shares\n"
			  "first mount the share with the mount tool\n"
			  "and then use the xftree search utility to do the search\n"));
}
static void
cb_diff (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("To view differences on a remote SMB share\n"
			  "first mount the share with the mount tool\n"
			  "and then use the xftree diff utility to display the differences.\n"));
}

static void
cb_about (GtkWidget * item, GtkWidget * ctree)
{
  xf_dlg_warning (smb_nav,_("This is XFSamba " XFSAMBA_VERSION "\n(c) Edscott Wilson Garcia under GNU GPL"));
}

#ifdef OBSOLETE
static void
configure (void)
{
  static int call = 0;
  GdkEvent event;
  GdkEventConfigure *configure;
  configure = (GdkEventConfigure *) (&event);
  event.type = GDK_CONFIGURE;
  configure->width = smb_nav->allocation.width;
  configure->height = smb_nav->allocation.height;
  if ((call++) % 2)
    configure->height++;
  else
    configure->height--;	/*hack */
  gtk_propagate_event (smb_nav, &event);
}
#endif

static void
cb_view (GtkWidget * widget, gpointer data)
{
  int caso;
  caso = (int) ((long) data);
  preferences &= (0xffffffff ^ (VIEW_1|VIEW_2|VIEW_3|VIEW_4));
  if (caso & 0xf00)
  {
    switch (caso)
    {
     case 0x100: view_toggle = 0x0;  preferences |= VIEW_1; break;
     case 0x200: view_toggle = 0x10; preferences |= VIEW_2; break;
     case 0x400: view_toggle = 0x11; preferences |= VIEW_3; break;
     case 0x800: view_toggle = 0x01; preferences |= VIEW_4; break;
    }
  }
  else view_toggle ^= caso;
  save_defaults();
  
  if (view_toggle & 0x01)
  {
    gtk_widget_hide (show_diag);
    gtk_widget_show (hide_diag);
    gtk_paned_set_position (GTK_PANED (vpaned), vpaned->allocation.height * 0.75);

  }
  else
  {
    gtk_widget_hide (hide_diag);
    gtk_widget_show (show_diag);
    gtk_paned_set_position (GTK_PANED (vpaned), vpaned->allocation.height);
  }

  if (view_toggle & 0x10)
  {
    gtk_widget_hide (show_links);
    gtk_widget_show (hide_links);
    gtk_paned_set_position (GTK_PANED (vpaned2), 0.65 * vpaned2->allocation.height);
    gtk_paned_set_position (GTK_PANED (hpaned), 0.50 * hpaned->allocation.width);
  }
  else
  {
    gtk_widget_hide (hide_links);
    gtk_widget_show (show_links);
    gtk_paned_set_position (GTK_PANED (vpaned2), vpaned2->allocation.height);
  }
}



static void
destroy_dialog (GtkWidget * widget, gpointer data)
{
  if (SMBResult == CHALLENGED)
  {
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:Unflagging entry in primary cache\n");
#endif
    if (current_cache)
      current_cache->visited = 0;
          
      /*pop_cache(thisN->shares); */
     
    SMBResult = SUCCESS;
  }
  if (SMBResult == CHALLENGED)
  {
    SMBResult = SUCCESS;
  }
  gtk_widget_destroy ((GtkWidget *) data);
}

static int passwd_caso;

static void
ok_dialog (GtkWidget * widget, gpointer data)
{
  char *s, *t;
  int caso;

  caso = (int) ((long) data);

  s = gtk_entry_get_text (GTK_ENTRY (user));
  t = gtk_entry_get_text (GTK_ENTRY (passwd));
  if (!strlen(s)) s="Guest";

  if (thisN)
  {
    if (thisN->password)
      free (thisN->password);
    thisN->password = (unsigned char *) malloc (strlen (s) + strlen (t) + 2);
    if (strlen (t) > 0)
      sprintf (thisN->password, "%s%%%s", s, t);
    else
      sprintf (thisN->password, "%s%%", s);
  }

  if (passwd_caso == 1)
  {
    gtk_widget_destroy (dialog);
    if (SMBResult == CHALLENGED)
    {
      SMBResult = SUCCESS;
      if (selected.directory)
      {
	SMBList ();
      }
      else
      {				/* browsing has been done at netbios: */
	SMBrefresh (thisN->netbios, FORCERELOAD);
      }
    }
  }
  else
  {
    if (default_user)
      free (default_user);
    default_user = (char *) malloc (strlen (s) + strlen (t) + 2);
    if (strlen (t) > 0)
      sprintf (default_user, "%s%%%s", s, t);
    else
      sprintf (default_user, "%s%%", s);
    gtk_widget_destroy (dialog);
  }
}
static void
entry_keypress (GtkWidget * entry, GdkEventKey * event, gpointer data)
{
  if (event->keyval == GDK_Return)
  {
    if (entry == user)
      gtk_widget_grab_focus (passwd);
    if (entry == passwd)
      ok_dialog (NULL, NULL);
  }
  return;

}


GtkWidget *
passwd_dialog (int caso)
{
  GtkWidget *button, *hbox, *label;

  passwd_caso = caso;

  SMBabortdrop=TRUE;
  dialog = gtk_dialog_new ();
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_widget_realize (dialog);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  if (caso == 1)
    label = gtk_label_new (_("Please provide information for server "));
  else
    label = gtk_label_new (_("Please provide browsing preferences "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  if (caso == 1)
  {
    label = gtk_label_new (thisN->server);
    gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
    gtk_widget_show (label);
  }

  gtk_widget_show (hbox);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Username : "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  user = gtk_entry_new ();
  if ((thisN) && (thisN->password))
  {
    char *c;
    c=g_strdup(thisN->password);
    if (c){ 
     strtok (c, "\%");
     if (strstr (c, "Guest") == NULL) gtk_entry_set_text ((GtkEntry *) user, c);
     g_free(c);
    }
  }
  gtk_box_pack_start (GTK_BOX (hbox), user, EXPAND, NOFILL, 0);
  gtk_signal_connect (GTK_OBJECT (user), "key-press-event", GTK_SIGNAL_FUNC (entry_keypress), NULL);
  gtk_widget_show (user);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Password  : "));
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  passwd = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), passwd, EXPAND, NOFILL, 0);
  gtk_entry_set_visibility ((GtkEntry *) passwd, FALSE);
  gtk_signal_connect (GTK_OBJECT (passwd), "key-press-event", GTK_SIGNAL_FUNC (entry_keypress), NULL);
  gtk_widget_show (passwd);


  button = gtk_button_new_with_label (_("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ok_dialog), (gpointer) ((long) caso));
  button = gtk_button_new_with_label ("Cancel");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, EXPAND, NOFILL, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (destroy_dialog), (gpointer) dialog);
  gtk_widget_show (dialog);
  gtk_widget_grab_focus (user);
  return dialog;
}

static void
select_user (void)
{
  gtk_window_set_transient_for (GTK_WINDOW (passwd_dialog (0)), GTK_WINDOW (smb_nav));

}


void
cb_download (GtkWidget * widget, GtkWidget *ctree)
{
	/*FIXME: do multiple files and directories */
  int num;	
  GList *s;
  smb_entry *en;
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))!=1){
    xf_dlg_warning (smb_nav,_("A single file can be selected!"));
    return;
  }
  s = GTK_CLIST (ctree)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree, s->data);
  if (!(en->type&S_T_FILE)){
    xf_dlg_warning (smb_nav,_("A single file can be selected!"));
    return;
  }
  SMBGetFile ();
}

void
cb_upload (GtkWidget * widget, GtkWidget *ctree)
{
  int num;	
  GList *s;
  smb_entry *en;
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))!=1){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  s = GTK_CLIST (ctree)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree, s->data);
  if (!(en->type&S_T_DIRECTORY)){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  SMBPutFile ();
}

void
cb_new_dir (GtkWidget * widget, GtkWidget *ctree)
{
  int num;	
  smb_entry *en;
  GList *s;
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))!=1){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  s = GTK_CLIST (ctree)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree, s->data);
  if (!(en->type&(S_T_DIRECTORY|S_T_SHARE))){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  SMBmkdir ();
}

void
cb_delete (GtkWidget * widget, GtkWidget *ctree)
{
  GList *s,*e=NULL;
  smb_entry *en;
  int num;
  char *orig_share=NULL;   
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))==0){
    xf_dlg_warning (smb_nav,_("Something to be deleted must be selected!"));
    return;
  }
  /* if num > 1, unselect all directory entries 
   * (a recursive delete should be used, but not mentioned
   * in man smbclient)*/
  for (s = GTK_CLIST (ctree)->selection; s != NULL; s=s->next){
     en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree, s->data);
     if (!orig_share) g_strdup(en->share);
     if (orig_share && (strcmp(orig_share,en->share)!=0)) {
	     g_free(orig_share);
             xf_dlg_warning (smb_nav,_("Only elements from a single share may be selected!"));
	     return;
     }
     if ((num > 1) && (en->type & S_T_DIRECTORY)) e=g_list_append(e,(gpointer)s->data);
     if (en->type & (S_T_SHARE|S_T_PRINTER|S_T_IPC)) e=g_list_append(e,(gpointer)s->data);
   }
  if (orig_share) g_free(orig_share);
  for (s=e; s != NULL; s=s->next){
    gtk_ctree_unselect((GtkCTree *)ctree,s->data);
  } g_list_free(e);
     
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))==0){
    xf_dlg_warning (smb_nav,_("Something to be deleted must be selected!"));
    return;
  }  
  if (num) SMBrm ();
}

void
cb_tar (GtkWidget * widget, GtkWidget *ctree)
{
 int num;	
  smb_entry *en;
  GList *s;
  if ((num = g_list_length (GTK_CLIST (ctree)->selection))==0){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  s = GTK_CLIST (ctree)->selection;
  en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree, s->data);
  if (!(en->type&S_T_DIRECTORY)){
    xf_dlg_warning (smb_nav,_("A single directory must be selected!"));
    return;
  }
  SMBtar ();
}

extern GtkCTreeNode *DropNode;

char **get_select_share(GtkCTree * ctree, GList * node){
  static char *line[SHARE_COLUMNS];
  smb_entry *en;
  int i;
  for (i=0;i<SHARE_COLUMNS;i++) line[i]=NULL;
  if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) node, SHARE_NAME_COLUMN, line))
    return NULL;
  if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) node, COMMENT_COLUMN, line + 1))
    return NULL;
  /*printf("dbg:%s\n%s\n",line[0],line[1]);*/
  selected.directory = selected.file = FALSE;
  if (selected.share) free (selected.share);
  selected.share = NULL;
  if (selected.dirname) free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename) free (selected.filename);
  selected.filename = NULL;
  if (selected.comment) free (selected.comment);
  selected.comment = NULL;

  en = gtk_ctree_node_get_row_data ((GtkCTree *)ctree,(GtkCTreeNode *) node);
  if (en->type &(S_T_IPC|S_T_PRINTER)) return NULL;
  
  
  selected.comment = (char *) malloc (strlen (line[1]) + 1);
  strcpy (selected.comment, line[1]);

  selected.node = node;		/* this is leaf or node */
  selected.parent_node = NULL;

  if (!strncmp (selected.comment, "Disk", strlen ("Disk"))) 
	  selected.directory = TRUE;

  if ((selected.comment[0] == '/') && (selected.comment[1])) {
    selected.directory = TRUE;
    selected.parent_node = node;
  }

  if ((GTK_CTREE_ROW (node)->parent) && (line[1][0] != '/')) {
    selected.file = TRUE;
    selected.parent_node = (GList *) (GTK_CTREE_ROW (node)->parent);
  }

/* get the share and dirname: */
  if (selected.parent_node == NULL) {	/* top level node */
    selected.share = (char *) malloc (strlen (line[0]) + 1);
    sprintf (selected.share, "%s", line[0]);
    selected.dirname = (char *) malloc (3);
    sprintf (selected.dirname, "/");
    DropNode=(GtkCTreeNode *)node;
  } else {  /* something not top level */
    char *word;
    int i;
    DropNode=NULL;
    gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) node, COMMENT_COLUMN, line + 1);
    if (line[1][0]=='/') DropNode=(GtkCTreeNode *)node;
    if (!gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) selected.parent_node, COMMENT_COLUMN, line + 1))
    {
      print_diagnostics ("DBG:unable to get parent information\n");
      return FALSE;
    }
    if (!DropNode) DropNode=(GtkCTreeNode *)selected.parent_node;
    if (strncmp (line[1], "Disk", strlen ("Disk")) == 0)
    {
      gtk_ctree_node_get_text (ctree, (GtkCTreeNode *) selected.parent_node, SHARE_NAME_COLUMN, line + 1);
      selected.share = (char *) malloc (strlen (line[1]) + 1);
      strcpy (selected.share, line[1]);
      selected.dirname = (char *) malloc (strlen ("/") + 1);
      sprintf (selected.dirname, "/");
    }
    else
    {
      word = line[1] + 1;
      selected.share = (char *) malloc (strlen (word) + 1);
      i = 0;
      while ((word[i] != '/') && (word[i]))
      {
	selected.share[i] = word[i];
	i++;
      }
      selected.share[i] = 0;

      if (strstr (word, "/"))
      {
	word = word + i + 1;
	while (word[strlen (word) - 1] == ' ')
	  word[strlen (word) - 1] = 0;

	selected.dirname = (char *) malloc (strlen (word) + 2);
	sprintf (selected.dirname, "/%s", word);
      }
      else
      {
	selected.dirname = (char *) malloc (strlen ("/") + 1);
	sprintf (selected.dirname, "/");
      }
    }
  }

#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:comment=");
  print_diagnostics (selected.comment);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:share=");
  print_diagnostics (selected.share);
  print_diagnostics ("\n");
  print_diagnostics ("DBG:dirname=");
  print_diagnostics (selected.dirname);
  print_diagnostics ("\n");
#endif

  /* get file name, if applicable */
  if (selected.file)
  {
    char *word;
    word = line[0];
    while (word[strlen (word) - 1] == ' ')
      word[strlen (word) - 1] = 0;
    selected.filename = (char *) malloc (strlen (word) + 1);
    sprintf (selected.filename, "%s", word);
    /*      SMBGetFile(); */
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:filename=");
    print_diagnostics (selected.filename);
    print_diagnostics ("\n");
#endif
    return line; /* nothing else to do for simple file (in select_share) */
  }
  else
  {
    selected.filename = NULL;
  }
  return line;

}

void
select_share (GtkCTree * ctree, GList * node, gint column, gpointer user_data)
{
  nmb_cache *cache;
  char **line;

  /* only expand on single selections */
  if (g_list_length (GTK_CLIST (ctree)->selection) > 1) return;
  
  line=get_select_share(ctree,node);
  if (!line || selected.file) return;
  
/* determine whether or not to modify ctree: */

  cache = thisN->shares;

#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:Looking into cache for ");
  print_diagnostics (line[(GTK_CTREE_ROW (node)->parent) ? 1 : 0]);
  print_diagnostics ("\n");
#endif

  while (cache)
  {
    if (!(GTK_CTREE_ROW (node)->parent))
    {				/* look into first level cache */
      if (cache->textos[1])
      {
/*fprintf(stderr,"%s<->%s#1\n",cache->textos[1],line[0]); */
	if (strcmp (cache->textos[1], line[0]) == 0)
	{
	  if (cache->visited)
	  {
#ifdef DBG_XFSAMBA
	    print_diagnostics ("DBG:Found in cache. \n");
#endif
	    return;
	  }
	  cache->visited = 1;
	  current_cache = cache;
	  break;
	}
      }
    }
    else if (cache->textos[2])
    {				/* look into second level cache */
/* fprintf(stderr,"%s<->%s#2\n",cache->textos[2],line[1]); */
      if (strcmp (cache->textos[2], line[1]) == 0)
      {
#ifdef DBG_XFSAMBA
	print_diagnostics ("DBG:Found in cache. \n");
#endif
	current_cache = NULL;
	return;
      }
    }
    cache = cache->next;
  }
  if (!cache)
  {
    char *textos[3];
#ifdef DBG_XFSAMBA
    print_diagnostics ("DBG:Not found in cache.\n");
#endif
    textos[0] = textos[1] = NULL;
    textos[2] = line[1];
    push_nmb_cacheF (thisN->shares, textos);
/*    push_nmb_cache (thisN->shares, textos);*/
    current_cache = NULL;
  }


  gtk_ctree_expand (ctree, (GtkCTreeNode *) node);
#ifdef DBG_XFSAMBA
  print_diagnostics ("DBG:SMBList ");
#endif
  SMBList ();
}



void
select_server (GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer user_data)
{
  nmb_cache *cache;
  char *line[3];
  selected.directory = selected.file = FALSE;
  if (selected.share)
    free (selected.share);
  selected.share = NULL;
  if (selected.dirname)
    free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename)
    free (selected.filename);
  selected.filename = NULL;
  if (selected.comment)
    free (selected.comment);
  selected.comment = NULL;

  gtk_clist_get_text (clist, row, SERVER_NAME_COLUMN, line);
  if (line[0] == NULL)
    return;
  cache = thisN->servers;
  while (cache)
  {
    if (strcmp (cache->textos[SERVER_NAME_COLUMN], line[0]) == 0)
    {
      cache->visited = 1;
      break;
    }
    cache = cache->next;
  }
  SMBrefresh ((unsigned char *) (line[0]), RELOAD);
}

void
select_workgroup (GtkCList * clist, gint row, gint column, GdkEventButton * event, gpointer user_data)
{
  nmb_cache *cache;
  char *line[3];
  selected.directory = selected.file = FALSE;
  if (selected.share)
    free (selected.share);
  selected.share = NULL;
  if (selected.dirname)
    free (selected.dirname);
  selected.dirname = NULL;
  if (selected.filename)
    free (selected.filename);
  selected.filename = NULL;
  if (selected.comment)
    free (selected.comment);
  selected.comment = NULL;
  gtk_clist_get_text (clist, row, WG_MASTER_COLUMN, line);
  if (line[0] == NULL)
    return;
  cache = thisN->workgroups;
  while (cache)
  {
    if (strcmp (cache->textos[WG_MASTER_COLUMN], line[0]) == 0)
    {
      cache->visited = 1;
      break;
    }
    cache = cache->next;
  }
  SMBrefresh ((unsigned char *) (line[0]), RELOAD);
}

void
select_combo_server (GtkWidget * widget, gpointer data)
{
  char *s;
  s = gtk_entry_get_text (GTK_ENTRY (widget));
  /* is it in history list? */
  nonstop = TRUE;
  SMBrefresh ((unsigned char *) s, RELOAD);
}

void
go_reload (GtkWidget * widget, gpointer data)
{
  if (thisN)
    SMBrefresh (thisN->netbios, FORCERELOAD);
  else
    SMBrefresh (NULL, REFRESH);
}

void
go_home (GtkWidget * widget, gpointer data)
{
  print_status (_("Going to master browser..."));
  SMBCleanLevel2 ();
  SMBrefresh (NULL, REFRESH);
}

void
go_back (GtkWidget * widget, gpointer data)
{
  if ((thisH) && (thisH->previous))
  {
    print_status (_("Going back..."));
    SMBCleanLevel2 ();
    thisH = thisH->previous;
    thisN = thisH->record;
    SMBrefresh (thisN->netbios, REFRESH);
  }
}
void
go_forward (GtkWidget * widget, gpointer data)
{
  if ((thisH) && (thisH->next))
  {
    print_status (_("Going forward..."));
    SMBCleanLevel2 ();
    thisH = thisH->next;
    thisN = thisH->record;
    SMBrefresh (thisN->netbios, REFRESH);
  }
}

void
go_stop (GtkWidget * widget, gpointer data)
{

  if (fork_obj)
  {
    if (nonstop)
    {
      print_status (_("Attempting to stop query..."));	/* this is called atole */
      return;
    }

    print_status (_("Query stopped."));
    if (stopcleanup)
      TuboCancel (fork_obj, clean_nmb);
    else
      TuboCancel (fork_obj, NULL);
    cursor_reset (GTK_WIDGET (smb_nav));
    animation (FALSE);
    fork_obj = NULL;
  }				/* else print_status(_("Nothing to stop.")); */
  return;
}


static void
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  xfsamba_abort (1);
}

#define TOGGLENOT 4
#define TOGGLE 3
#define EMPTY_SUBMENU 2
#define SUBMENU 1
#define MENUBAR 0
#define RIGHT_MENU -1

static GtkWidget *
shortcut_menu (int submenu, GtkWidget * parent, char *txt, gpointer func, gpointer data)
{
  GtkWidget *menuitem;
  static GtkWidget *menu;
  int togglevalue;

  switch (submenu)
  {
  case TOGGLE:
  case TOGGLENOT:
    togglevalue = (int) data;
    menuitem = gtk_check_menu_item_new_with_label (txt);
    GTK_CHECK_MENU_ITEM (menuitem)->active = (submenu == TOGGLENOT) ? (!togglevalue) : togglevalue;
    gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menuitem), 1);
    break;
  case EMPTY_SUBMENU:
    menuitem = gtk_menu_item_new ();
    break;
  case RIGHT_MENU:
    menuitem = gtk_menu_item_new_with_label (txt);
    gtk_menu_item_right_justify (GTK_MENU_ITEM (menuitem));
    break;
  case SUBMENU:
  case MENUBAR:
  default:
    menuitem = gtk_menu_item_new_with_label (txt);
    break;
  }
  if (submenu > 0)
  {
    gtk_menu_append (GTK_MENU (parent), menuitem);
    if ((submenu) && (submenu != EMPTY_SUBMENU) && (func))
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (func), (gpointer) data);
  }
  else
    gtk_menu_bar_append (GTK_MENU_BAR (parent), menuitem);
  gtk_widget_show (menuitem);

  if (submenu <= 0)
  {
    menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);
    return menu;

  }
  return menuitem;
}

static GtkWidget *
newbox (gboolean pack, int vertical, GtkWidget * parent, gboolean expand, gboolean fill, int spacing)
{
  GtkWidget *box;
  switch (vertical)
  {
  case HORIZONTAL:
    box = gtk_hbox_new (FALSE, 0);
    break;
  case VERTICAL:
    box = gtk_vbox_new (FALSE, 0);
    break;
  case HANDLEBOX:
    box = gtk_handle_box_new ();
    break;
  default:
    box = NULL;
    return box;
  }

  if (pack)
    gtk_box_pack_start (GTK_BOX (parent), box, expand, fill, spacing);
  else
  {
    gtk_container_add (GTK_CONTAINER (parent), box);
    gtk_container_set_border_width (GTK_CONTAINER (parent), spacing);
  }

  gtk_widget_show (box);
  return box;
}

/*FIXME: zap these from CVS */
#if 0
#include "icons/page.xpm"
#include "icons/dir_close_lnk.xpm"
#include "icons/dir_open_lnk.xpm"
#include "icons/dir_close.xpm"
#include "icons/dir_open.xpm"
#include "icons/dotfile.xpm"
#include "icons/rdotfile.xpm"
#endif
/*FIXME: but leave these on CVS */
#if 0
#include "icons/rpage.xpm"
#include "icons/comp1.xpm"
#include "icons/comp2.xpm"
#include "icons/wg1.xpm"
#include "icons/wg2.xpm"
#include "icons/print.xpm"
#endif

#include "icons/go_to.xpm"
#include "icons/go_back.xpm"
#include "icons/home.xpm"
#if 0
#include "icons/stop.xpm"
#endif
#include "icons/reload.xpm"
#include "icons/help.xpm"
#include "icons/ip.xpm"
#include "icons/download.xpm"
#include "icons/upload.xpm"
#include "icons/view1.xpm"
#include "icons/view2.xpm"
#include "icons/view3.xpm"
#include "icons/view4.xpm"
#include "icons/delete.xpm"
#include "icons/new_dir.xpm"
#include "icons/new_win.xpm"
#include "icons/tar.xpm"
#include "icons/tb_cut.xpm"
#include "icons/tb_copy.xpm"
#include "icons/tb_paste.xpm"
#include "icons/mount.xpm"

static GtkWidget *
icon_button (char **data, char *tip)
{
  GtkWidget *button, *pixmap;
  GtkTooltips *tooltip;

  button = gtk_button_new ();
  gtk_button_set_relief ((GtkButton *) button, GTK_RELIEF_NONE);
  gtk_widget_set_usize (button, 30, 30);
  tooltip = gtk_tooltips_new ();
  gtk_tooltips_set_tip (tooltip, button, tip, "ContextHelp/buttons/?");
  pixmap = MyCreateFromPixmapData (button, data);
  if (pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  else
  {
    gtk_widget_show (pixmap);
    gtk_container_add (GTK_CONTAINER (button), pixmap);
  }
  return button;
}

static void make_menu(GtkWidget *handlebox){
	GtkWidget *hbox,*menu, *submenu, *menubar;
	hbox = newbox (ADD, HORIZONTAL, handlebox, NOTUSED, NOTUSED, 3);
	/* menu bar */
	menubar = gtk_menu_bar_new ();
	gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (hbox), menubar);
	gtk_widget_show (menubar);

/* FILE */
	menu = shortcut_menu (MENUBAR, menubar, _("File"), NULL, NULL);
	/* (s): multiple file upload/download enabled by DnD mostly */
	submenu = shortcut_menu (SUBMENU, menu, _("Download..."), GTK_SIGNAL_FUNC (cb_download),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Upload..."), GTK_SIGNAL_FUNC (cb_upload),(gpointer) shares);

	submenu = shortcut_menu (SUBMENU, menu, _("New folder..."), GTK_SIGNAL_FUNC (cb_new_dir),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Delete..."), GTK_SIGNAL_FUNC (cb_delete),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Tar..."), GTK_SIGNAL_FUNC (cb_tar),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Exit"), GTK_SIGNAL_FUNC (delete_event), NULL);

/* EDIT */
	menu = shortcut_menu (MENUBAR, menubar, _("Edit"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Paste..."), GTK_SIGNAL_FUNC (cb_paste),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Cut..."), GTK_SIGNAL_FUNC (cb_cut),(gpointer) shares);
	submenu = shortcut_menu (SUBMENU, menu, _("Copy..."), GTK_SIGNAL_FUNC (cb_copy),(gpointer) shares);
	
/* GOTO */
	menu = shortcut_menu (MENUBAR, menubar, _("Go"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Home..."), GTK_SIGNAL_FUNC (go_home), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Reload..."), GTK_SIGNAL_FUNC (go_reload), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Forward..."), GTK_SIGNAL_FUNC (go_forward), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Back..."), GTK_SIGNAL_FUNC (go_back), NULL);
/* TOOLS */
	menu = shortcut_menu (MENUBAR, menubar, _("Tools"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Local file browser"), GTK_SIGNAL_FUNC (cb_xftree), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Mount remote share"), GTK_SIGNAL_FUNC (cb_mount), NULL);
/* VIEW */
	menu = shortcut_menu (MENUBAR, menubar, _("View"), NULL, NULL);

	show_diag = submenu = shortcut_menu (SUBMENU, menu, _("Show diagnostics"), 
			GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x01));
	hide_diag = submenu = shortcut_menu (SUBMENU, menu, _("Hide diagnostics"), 
			GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x1));
	show_links = submenu = shortcut_menu (SUBMENU, menu, _("Show browser links"), 
			GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x10));
	hide_links = submenu = shortcut_menu (SUBMENU, menu, _("Hide browser links"), 
			GTK_SIGNAL_FUNC (cb_view), (gpointer) ((long) 0x10));
/* PREFERENCES */
	menu = shortcut_menu (MENUBAR, menubar, _("Preferences"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Set background color"), GTK_SIGNAL_FUNC (cb_select_colors), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Set font"), GTK_SIGNAL_FUNC (cb_select_font), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Browse as..."), GTK_SIGNAL_FUNC (select_user), NULL);

/* HELP */

	menu = shortcut_menu (RIGHT_MENU, menubar, _("Help"), NULL, NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Drag and drop"), GTK_SIGNAL_FUNC (cb_dnd), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Mounting SMB shares"), GTK_SIGNAL_FUNC (cb_mountH), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Searching"), GTK_SIGNAL_FUNC (cb_search), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Differences"), GTK_SIGNAL_FUNC (cb_diff), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("Master browsers"), GTK_SIGNAL_FUNC (cb_master), NULL);
	submenu = shortcut_menu (SUBMENU, menu, _("About xfsamba"), GTK_SIGNAL_FUNC (cb_about), NULL);
}

#if 0
   {_("Stop ..."), 	stop_xpm,	go_stop,	0x0}, 
#endif
   
#define TOOLBARICONS \
   {_("Xftree ..."),	new_win_xpm,	cb_xftree,	0x0}, \
   {_(""),		NULL ,		NULL,		0x0}, \
   {_("Back ..."),	go_back_xpm ,	go_back,	0x0}, \
   {_("Forward ..."),	go_to_xpm ,	go_forward,	0x0}, \
   {_("Reload ..."), 	reload_xpm,	go_reload,	0x0}, \
   {_("Home ..."), 	home_xpm,	go_home,	0x0}, \
   {_(""),		NULL ,		NULL,		0x0}, \
   {_("Cut ..."),	tb_cut_xpm ,	cb_cut,		0x0}, \
   {_("Copy ..."),	tb_copy_xpm ,	cb_copy,	0x0}, \
   {_("Paste ..."),	tb_paste_xpm ,	cb_paste,	0x0}, \
   {_("New folder ..."),new_dir_xpm ,	cb_new_dir,	0x0}, \
   {_("Delete ..."),	delete_xpm ,	cb_delete,	0x0}, \
   {_(""),		NULL ,		NULL,		0x0}, \
   {_("Mount ..."), 	mount_xpm,	cb_mount,	0x0}, \
   {_("Download ..."),download_xpm,	cb_download,	0x0}, \
   {_("Upload ..."),	upload_xpm ,	cb_upload,	0x0}, \
   {_("Tar ..."), 	tar_xpm,	cb_tar,		0x0}, \
   {_(""),		NULL ,		NULL,		0x0}, \
   {_("Set View 1"),	view1_xpm ,	cb_view,	0x100}, \
   {_("Set View 2"),	view2_xpm ,	cb_view,	0x200}, \
   {_("Set View 3"),	view3_xpm ,	cb_view,	0x400}, \
   {_("Set View 4"),	view4_xpm ,	cb_view,	0x800}, \
   {_(""),		NULL ,		NULL,		0x0}, \
   {_("Help ..."),	help_xpm ,	cb_about,	0x0}, \
   {NULL,NULL,NULL,0} 
typedef struct boton_icono {
	char *text;
	char **icon;
	gpointer function;
	long data;
} boton_icono;
 

static void make_toolbar(GtkWidget *handlebox){
  boton_icono toolbarIcon[]={TOOLBARICONS};
  int i;
  /* other pixmaps */
  GtkWidget *toolbar,*button;
  gPIX_ip = MyCreateGdkPixmapFromData (ip_xpm, smb_nav, &gPIM_ip, FALSE);

#if 0  
  gPIX_page = MyCreateGdkPixmapFromData (page_xpm, smb_nav, &gPIM_page, FALSE);
  /* FIXME: read only icon missing in xtree_icons.c */
  gPIX_rpage = MyCreateGdkPixmapFromData (rpage_xpm, smb_nav, &gPIM_rpage, FALSE);
  gPIX_dir_open = MyCreateGdkPixmapFromData (dir_open_xpm, smb_nav, &gPIM_dir_open, FALSE);
  gPIX_dir_close = MyCreateGdkPixmapFromData (dir_close_xpm, smb_nav, &gPIM_dir_close, FALSE);
  gPIX_dir_open_lnk = MyCreateGdkPixmapFromData (dir_open_lnk_xpm, smb_nav, &gPIM_dir_open_lnk, FALSE);
  gPIX_dir_close_lnk = MyCreateGdkPixmapFromData (dir_close_lnk_xpm, smb_nav, &gPIM_dir_close_lnk, FALSE);
  gPIX_comp1 = MyCreateGdkPixmapFromData (comp1_xpm, smb_nav, &gPIM_comp1, FALSE);
  gPIX_comp2 = MyCreateGdkPixmapFromData (comp2_xpm, smb_nav, &gPIM_comp2, FALSE);
  gPIX_wg1 = MyCreateGdkPixmapFromData (wg1_xpm, smb_nav, &gPIM_wg1, FALSE);
  gPIX_wg2 = MyCreateGdkPixmapFromData (wg2_xpm, smb_nav, &gPIM_wg2, FALSE);
  gPIX_dotfile = MyCreateGdkPixmapFromData (dotfile_xpm, smb_nav, &gPIM_dotfile, FALSE);
  gPIX_rdotfile = MyCreateGdkPixmapFromData (rdotfile_xpm, smb_nav, &gPIM_rdotfile, FALSE);
  gPIX_print = MyCreateGdkPixmapFromData (print_xpm, smb_nav, &gPIM_print, FALSE);
#endif
  
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_space_style ((GtkToolbar *) toolbar, GTK_TOOLBAR_SPACE_LINE);
  gtk_toolbar_set_button_relief ((GtkToolbar *) toolbar, GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 2);
  gtk_widget_realize(smb_nav);	
  /* icon bar */
  gtk_container_add (GTK_CONTAINER (handlebox), toolbar);
  
  for (i=0;(toolbarIcon[i].text != NULL)&&(i<32);i++) {
       if (toolbarIcon[i].icon==NULL) gtk_toolbar_append_space ((GtkToolbar *)toolbar);
       else {
        button=MyCreateFromPixmapData (toolbar, toolbarIcon[i].icon);
        gtk_toolbar_append_item ((GtkToolbar *) toolbar,NULL,
			toolbarIcon[i].text,toolbarIcon[i].text,
			button, GTK_SIGNAL_FUNC (toolbarIcon[i].function),
	(!toolbarIcon[i].data)?(gpointer) shares:(gpointer)(toolbarIcon[i].data));
       }
  }
  gtk_widget_show_all(toolbar);
}
	
GtkWidget *
create_smb_window (void)
{
  int lineH;
  GtkWidget * vbox, *vbox1, *vbox2, *hbox, *widget, *toolbar,*menubar,
            *handlebox, *scrolled, *button;


  smb_nav = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW (smb_nav), TRUE, TRUE, FALSE);

  gtk_signal_connect (GTK_OBJECT (smb_nav), "destroy", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (smb_nav));
  gtk_signal_connect (GTK_OBJECT (smb_nav), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (smb_nav));
  /* boxes: */

  vbox = newbox (ADD, VERTICAL, smb_nav, FILL, EXPAND, 0);
  widget = gtk_label_new ("GET FONT INFO");
  lineH = widget->style->font->ascent + widget->style->font->descent + 5;

  {
    vbox1 = newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0);
    {
      menubar = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      toolbar = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      handlebox = newbox (PACK, HANDLEBOX, vbox1, NOEXPAND, NOFILL, 0);
      {
	hbox = newbox (ADD, HORIZONTAL, handlebox, NOTUSED, NOTUSED, 3);
	/* location entry */
	gtk_widget_set_usize (hbox, WINDOW_WIDTH, lineH);
	widget = gtk_label_new (_("Location : "));
	gtk_box_pack_start (GTK_BOX (hbox), widget, NOEXPAND, NOFILL, 0);
	gtk_widget_show (widget);

	location = gtk_combo_new ();
	gtk_box_pack_start (GTK_BOX (hbox), location, EXPAND, FILL, 0);
	gtk_widget_show (location);

	gtk_combo_disable_activate ((GtkCombo *) location);
	/*
	   gtk_signal_connect (GTK_OBJECT (GTK_COMBO (location)->entry), "changed",
	 */
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (location)->entry), "activate", GTK_SIGNAL_FUNC (select_combo_server), NULL);

	button = icon_button (ip_xpm, _("Show IP"));
	gtk_box_pack_start (GTK_BOX (hbox), button, NOEXPAND, NOFILL, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (NMBLookup), NULL);
	gtk_widget_show (button);

	locationIP = gtk_label_new ("-");
	gtk_box_pack_start (GTK_BOX (hbox), locationIP, NOEXPAND, NOFILL, 0);
	gtk_widget_show (locationIP);
      }
    }
    vpaned = gtk_vpaned_new ();
    gtk_widget_ref (vpaned);
    gtk_object_set_data (GTK_OBJECT (smb_nav), "vpaned1", vpaned);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
    vbox1 = gtk_vbox_new (FALSE, 0);	/* newbox (PACK, VERTICAL, vbox, EXPAND, FILL, 3); */
    gtk_paned_pack1 (GTK_PANED (vpaned), vbox1, TRUE, TRUE);
    gtk_widget_show (vbox1);
    {
      vpaned2 = gtk_vpaned_new ();
      gtk_object_set_data (GTK_OBJECT (smb_nav), "vpaned2", vpaned2);
      gtk_box_pack_start (GTK_BOX (vbox1), vpaned2, TRUE, TRUE, 0);
      vbox2 = gtk_vbox_new (FALSE, 0);	/* = newbox (PACK, VERTICAL, vbox1, EXPAND, FILL, 0); */
      gtk_paned_pack1 (GTK_PANED (vpaned2), vbox2, TRUE, TRUE);
      gtk_widget_show (vbox2);
      {
	/* location shares */
	sharesL = gtk_label_new (_("Location shares : "));
	gtk_box_pack_start (GTK_BOX (vbox2), sharesL, NOEXPAND, NOFILL, 0);
	gtk_widget_show (sharesL);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	gtk_widget_set_usize (scrolled, WINDOW_WIDTH, lineH * 6);
	gtk_widget_show (scrolled);
	{
	  int i;
	  gchar *titles[SHARE_COLUMNS];
	  for (i = 0; i < SHARE_COLUMNS; i++)
	    titles[i] = "";
	  titles[SHARE_NAME_COLUMN] = _("Name");
	  titles[SHARE_SIZE_COLUMN] = _("Size");
	  titles[SHARE_DATE_COLUMN] = _("Date");
	  titles[COMMENT_COLUMN] = _("Comment");
	  shares = gtk_ctree_new_with_titles (SHARE_COLUMNS, 0, titles);

	  /*
	     shares=gtk_ctree_new_with_titles(2,0,titles);
	   */
	  gtk_clist_set_auto_sort (GTK_CLIST (shares), FALSE);
	  gtk_clist_set_shadow_type (GTK_CLIST (shares), GTK_SHADOW_IN);
	  gtk_ctree_set_line_style (GTK_CTREE (shares), GTK_CTREE_LINES_NONE);
	  gtk_ctree_set_expander_style (GTK_CTREE (shares), GTK_CTREE_EXPANDER_TRIANGLE);
          gtk_clist_set_selection_mode (GTK_CLIST (shares), GTK_SELECTION_EXTENDED);
	  gtk_clist_set_reorderable (GTK_CLIST (shares), FALSE);
	  gtk_signal_connect (GTK_OBJECT (shares), "tree-select-row", GTK_SIGNAL_FUNC (select_share), (gpointer) GTK_WIDGET (shares));
	  gtk_signal_connect (GTK_OBJECT (shares), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
          gtk_signal_connect (GTK_OBJECT (shares), "drag_data_get", GTK_SIGNAL_FUNC (on_drag_data_get), NULL);
  	  gtk_signal_connect (GTK_OBJECT (shares), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data), NULL);
  	  gtk_signal_connect (GTK_OBJECT (shares), "drag_motion", GTK_SIGNAL_FUNC (on_drag_motion), NULL);
	  gtk_container_add (GTK_CONTAINER (scrolled), shares);
	  (GTK_CLIST (shares))->sort_type = GTK_SORT_ASCENDING;
          for (i = 0; i < SHARE_COLUMNS; i++)
	    gtk_clist_set_column_auto_resize ((GtkCList *) shares, i, TRUE);
	  gtk_widget_show (shares);
	}
      }
      gtk_drag_source_set (shares, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK, 
			target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY );
      gtk_drag_dest_set (shares, GTK_DEST_DEFAULT_DROP|GTK_DEST_DEFAULT_MOTION, 
		      target_table, NUM_TARGETS,  GDK_ACTION_MOVE | GDK_ACTION_COPY );
	  

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_paned_pack2 (GTK_PANED (vpaned2), hbox, TRUE, TRUE);
      gtk_widget_show (hbox);

      /*hbox = newbox (PACK, HORIZONTAL, vbox3, EXPAND, FILL, 0); */

      {
	/* hbox=newbox(PACK,HORIZONTAL,vbox1,EXPAND,FILL,0);{ */
	hpaned = gtk_hpaned_new ();
	gtk_widget_ref (hpaned);
	gtk_object_set_data (GTK_OBJECT (smb_nav), "hpaned1", hpaned);
	gtk_box_pack_start (GTK_BOX (hbox), hpaned, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
				       /*= newbox (PACK, VERTICAL, hbox, EXPAND, FILL, 0);*/
	gtk_paned_pack1 (GTK_PANED (hpaned), vbox2, TRUE, TRUE);
	gtk_widget_show (vbox2);
	{
	  /* location computer links */
	  serversL = gtk_label_new (_("Location known servers : "));
	  gtk_box_pack_start (GTK_BOX (vbox2), serversL, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (serversL);

	  scrolled = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	  gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	  gtk_widget_set_usize (scrolled, WINDOW_WIDTH / 2, lineH * 6);
	  gtk_widget_show (scrolled);

	  {
	    int i;
	    gchar *titles[SERVER_COLUMNS] = { "", "Server", "Comment" };
	    servers = gtk_clist_new_with_titles (SERVER_COLUMNS, titles);
	    gtk_container_add (GTK_CONTAINER (scrolled), servers);
	    for (i = 0; i < SERVER_COLUMNS; i++)
	      gtk_clist_set_column_auto_resize ((GtkCList *) servers, i, TRUE);
	    gtk_widget_show (servers);
	    gtk_signal_connect (GTK_OBJECT (servers), "select-row", GTK_SIGNAL_FUNC (select_server), (gpointer) GTK_WIDGET (servers));
	    gtk_signal_connect (GTK_OBJECT (servers), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
	  }
	}
	vbox2 = gtk_vbox_new (FALSE, 0);
				       /*= newbox (PACK, VERTICAL, hbox, EXPAND, FILL, 0);*/
	gtk_paned_pack2 (GTK_PANED (hpaned), vbox2, TRUE, TRUE);
	gtk_widget_show (vbox2);
	{
	  /* location workgroup links */
	  workgroupsL = gtk_label_new (_("Location known workgroups : "));
	  gtk_box_pack_start (GTK_BOX (vbox2), workgroupsL, NOEXPAND, NOFILL, 0);
	  gtk_widget_show (workgroupsL);

	  scrolled = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	  gtk_box_pack_start (GTK_BOX (vbox2), scrolled, EXPAND, FILL, 0);
	  gtk_widget_show (scrolled);

	  {
	    int i;
	    gchar *titles[WG_COLUMNS] = { "", "Workgroup", "Master" };
	    workgroups = gtk_clist_new_with_titles (WG_COLUMNS, titles);
	    gtk_container_add (GTK_CONTAINER (scrolled), workgroups);
	    for (i = 0; i < WG_COLUMNS; i++)
	      gtk_clist_set_column_auto_resize ((GtkCList *) workgroups, i, TRUE);
	    gtk_widget_show (workgroups);
	    gtk_signal_connect (GTK_OBJECT (workgroups), "select-row", GTK_SIGNAL_FUNC (select_workgroup), (gpointer) GTK_WIDGET (workgroups));
	    gtk_signal_connect (GTK_OBJECT (workgroups), "click_column", GTK_SIGNAL_FUNC (on_click_column), NULL);
	  }
	}
	gtk_widget_show (hpaned);
      }
      gtk_widget_show (vpaned2);
    }
      
    make_menu(menubar);
    make_toolbar(toolbar);
    gtk_widget_realize(smb_nav);
    vbox1 = gtk_vbox_new (FALSE, 0);	/*newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0); */
    gtk_paned_pack2 (GTK_PANED (vpaned), vbox1, TRUE, TRUE);

    {
      scrolled = gtk_scrolled_window_new (NULL, NULL);
      {
	/* diagnostics */
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolled, EXPAND, FILL, 3);
	diagnostics = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT (diagnostics), FALSE);
	gtk_text_set_word_wrap (GTK_TEXT (diagnostics), TRUE);
	gtk_text_set_line_wrap (GTK_TEXT (diagnostics), TRUE);
	gtk_container_add (GTK_CONTAINER (scrolled), diagnostics);
	gtk_widget_set_usize (diagnostics, WINDOW_WIDTH, lineH * 5);
	gtk_widget_show (diagnostics);
	gtk_widget_show (scrolled);

      }
      gtk_widget_show (vbox1);
    }
    gtk_widget_show (vpaned);

    vbox1 = newbox (PACK, VERTICAL, vbox, NOEXPAND, NOFILL, 0);
    {
      hbox = newbox (PACK, HORIZONTAL, vbox1, EXPAND, FILL, 0);
      {
	/* status line */
	progress = gtk_progress_bar_new ();
	/* gtk_progress_bar_set_activity_step((GtkProgressBar *)progress,5); */
	gtk_box_pack_start (GTK_BOX (hbox), progress, NOEXPAND, NOFILL, 3);
	gtk_widget_show (progress);

	statusline = gtk_label_new (_("Welcome to xfsamba."));
	gtk_box_pack_start (GTK_BOX (hbox), statusline, NOEXPAND, NOFILL, 3);
	gtk_widget_show (statusline);
	gtk_widget_set_usize (statusline, WINDOW_WIDTH, lineH);
      }
    }
  }
  /* gtk_widget_set_usize (smb_nav, 640, 480); */
  if (preferences & CUSTOM_COLORS) set_colors(shares); 
  if (preferences & CUSTOM_FONT) create_pixmaps(set_fontT(shares),shares);
  else create_pixmaps(-1,shares);
#if 0
  set_fontT(shares);
#endif
  gtk_widget_show (smb_nav);
  /* default: */
  cb_view (NULL, (gpointer) ((long) 0x200));
  
  if (preferences & VIEW_1) cb_view (NULL, (gpointer) ((long) 0x100));
  if (preferences & VIEW_2) cb_view (NULL, (gpointer) ((long) 0x200));
  if (preferences & VIEW_3) cb_view (NULL, (gpointer) ((long) 0x400));
  if (preferences & VIEW_4) cb_view (NULL, (gpointer) ((long) 0x800));
  return smb_nav;
}

static gboolean anim;
static gboolean
animate_bar (gpointer data)
{
  static gboolean direction = TRUE;
  static gfloat fraction = 0.0, delta = 0.01;

  /* should be gtk_progress_set_fraction() */
  fraction += delta;
  if (fraction >= 1.0)
  {
    fraction = 1.0;
    if (direction)
      gtk_progress_bar_set_orientation ((GtkProgressBar *) progress, GTK_PROGRESS_RIGHT_TO_LEFT);
    else
      gtk_progress_bar_set_orientation ((GtkProgressBar *) progress, GTK_PROGRESS_LEFT_TO_RIGHT);
    direction = !direction;
    delta = -0.01;
  }
  if (fraction <= 0.0)
  {
    delta = 0.01;
    fraction = 0.0;
  }
  gtk_progress_set_percentage ((GtkProgress *) progress, fraction);
  /* if (!anim) gtk_progress_set_percentage((GtkProgress *)progress,1.0); */
  return anim;
}

void
animation (gboolean state)
{
  if (!state)
  {
    anim = FALSE;
  }
  else
  {
    anim = TRUE;
    gtk_timeout_add (100, (GtkFunction) animate_bar, (gpointer) ((long) anim));
  }
}
	
gboolean sane (char *bin)
{
  char *spath, *path, *globstring;
  glob_t dirlist;

  /* printf("getenv=%s\n",getenv("PATH")); */
  if (getenv ("PATH"))
  {
    path = (char *) malloc (strlen (getenv ("PATH")) + 2);
    strcpy (path, getenv ("PATH"));
    strcat (path, ":");
  }
  else
  {
    path = (char *) malloc (4);
    strcpy (path, "./:");
  }

  globstring = (char *) malloc (strlen (path) + strlen (bin) + 1);

/* printf("path=%s\n",path);*/

  if (strstr (path, ":"))
    spath = strtok (path, ":");
  else
    spath = path;

  while (spath)
  {
    sprintf (globstring, "%s/%s", spath, bin);
/*	 printf("checking for %s...\n",globstring);*/
    if (glob (globstring, GLOB_ERR, NULL, &dirlist) == 0)
    {
      /*       printf("found at %s\n",globstring); */
      free (globstring);
      globfree (&dirlist);
      free (path);
      return TRUE;
    }
    globfree (&dirlist);
    spath = strtok (NULL, ":");
  }
  free (globstring);
  free(path);
  return FALSE;
}

void save_defaults (void)
{
  FILE *defaults;
  char *homedir;
  int len;
  struct stat h_stat;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFSAMBA_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
failed_rc:
    fprintf(stderr,"xfsamba: xfsambarc file cannot be created\n");
    return;
  }
  snprintf (homedir, len, "%s/.xfce", (char *) getenv ("HOME"));
  if (stat(homedir,&h_stat) < 0){
	if (errno!=ENOENT) goto failed_rc;
	if (mkdir(homedir,0770) < 0) goto failed_rc;
  }

  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"), XFSAMBA_CONFIG_FILE);
  defaults = fopen (homedir, "w");
  free (homedir);

  if (!defaults)goto failed_rc;
  fprintf (defaults, "# file created by xfsamba, if removed xfsamba returns to defaults.\n");
  fprintf (defaults, "# do a bitwise or for preferences with %u (0x%x) to enable smaller dialogs.\n",
		  (unsigned int)SMALL_DIALOGS,SMALL_DIALOGS);
  
  fprintf (defaults, "preferences : %d\n", preferences);
  fprintf (defaults, "custom_font :%s\n",(custom_font)?custom_font:"fixed");
  fprintf (defaults, "ctree_color : %d,%d,%d\n",ctree_color.red,ctree_color.green,ctree_color.blue);
  
  fclose (defaults);
  return;
}

void read_defaults(void){
  FILE *defaults;
  char *homedir,*word;
  int len;

  /* default custom colors: */
  ctree_color.red = ctree_color.green = ctree_color.blue = 10000;
  
  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFSAMBA_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir) {
    fprintf(stderr,"xfsamba: xfsambarc file cannot be read\n");
    return;
  }
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"),XFSAMBA_CONFIG_FILE );
  defaults = fopen (homedir, "r");
  free (homedir);

  if (!defaults) {
          /* default view for first run to set initial rc file: */
	  preferences = VIEW_1;
	  return;
  }
  
  homedir = (char *)malloc(256);
  while (!feof(defaults)){
	fgets(homedir,255,defaults);
	if (feof(defaults))break;
	if (strstr(homedir,"preferences :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");
		if (!word) break;
		preferences=atoi(word);
	}
	if (strstr(homedir,"custom_font :")){
		strtok(homedir,":");
		word=strtok(NULL,"\n");if (!word) break;
		if (custom_font) free(custom_font);
		custom_font=(char *)malloc(strlen(word)+1);
		if (custom_font) strcpy(custom_font,word);
	}
	if (strstr(homedir,"ctree_color :")){
		strtok(homedir,":");
		word=strtok(NULL,",");if (!word) break;
		ctree_color.red=atoi(word);
		word=strtok(NULL,",");if (!word) break;
		ctree_color.green=atoi(word);
		word=strtok(NULL,"\n");if (!word) break;
		ctree_color.blue=atoi(word);
	}
  }
  free(homedir);
  fclose(defaults);  
 
}


