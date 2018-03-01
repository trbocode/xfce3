/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * Routines to override original ones in xtree_gui.c
 *
 * original create_toolbar (GtkWidget * top, GtkWidget * ctree, cfg * win)
 * by Rasca, Berlin and Olivier Fourdan, very much changed here.
 * 
 * xfce project: http://www.xfce.org
 * 
 * xtree_toolbar.c: toolbar routines for xftree
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

/* probably overkill with all these includes: */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <utime.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xtree_functions.h"
#include "xtree_gui.h"
#include "gtk_dlg.h"
#include "gtk_exec.h"
#include "gtk_prop.h"
#include "gtk_dnd.h"
#include "xtree_cfg.h"
#include "xtree_dnd.h"
#include "entry.h"
#include "uri.h"
#include "io.h"
#include "top.h"
#include "reg.h"
#include "xfcolor.h"
#include "xfce-common.h"
#include "../xfsamba/tubo.h"
#include "../xfdiff/xfdiff_colorsel.h"
#include "xtree_go.h"
#include "xtree_cb.h"
#include "toolbar_icons.h"
#include "xpmext.h"

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* this file is for processing certain common messages.
 * override warning to begin with */

#include "xtree_mess.h"
#include "xtree_pasteboard.h"
/* only 32 elements allowed in TOOLBARICONS */
#define TOOLBARICONS \
  {_("New icon view"),	new_rox_xpm,	cb_rox},	\
  {_("New tree view"),	new_win_xpm,	cb_new_window},	\
  {_("New terminal"),	comp1_xpm,	cb_term},	\
  {_("Close window"),	closewin_xpm,	cb_destroy}, \
  {_("Run ..."), 	tb_macro_xpm,	cb_exec}, \
  {_(""),	 	NULL,		NULL}, \
  {_("Update ..."), 	reload_xpm,	cb_reload}, \
  {_("Go back ..."), 	go_back_xpm,	cb_go_back}, \
  {_("Go to ..."), 	go_to_xpm,	cb_go_to}, \
  {_("Go up"), 		go_up_xpm,	cb_go_up}, \
  {_("Go home"),	home_xpm,	cb_go_home}, \
  {_(""),	 	NULL,		NULL}, \
  {_("Cut ..."), 	tb_cut_xpm,	cb_cut}, \
  {_("Copy ..."), 	tb_copy_xpm,	cb_copy}, \
  {_("Paste ..."), 	tb_paste_xpm,	cb_paste}, \
  {_("Delete ..."),	delete_xpm,	cb_delete}, \
  {_(""),	 	NULL,		NULL}, \
  {_("Find ..."), 	tb_find_xpm,	cb_find}, \
  {_("Differences ..."),tb_vsplit_xpm,	cb_diff}, \
  {_("Browse SMB network ..."),wg1_xpm,	cb_samba}, \
  {_(""),	 	NULL,		NULL}, \
  {_("New Folder"),	new_dir_xpm,	cb_new_subdir}, \
  {_("New file ..."),	new_file_xpm,	cb_new_file}, \
  {_("Properties"),	appinfo_xpm,	cb_props}, \
  {_(""),	 	NULL,		NULL}, \
  {_("Open Trash"),	trash_xpm,	cb_open_trash}, \
  {_("Empty Trash"),	empty_trash_xpm,cb_empty_trash}, \
  {_("Toggle Dotfiles"),dotfile_xpm,	on_dotfiles}, \
  {NULL,NULL,NULL} 
  

typedef struct boton_icono {
	char *text;
	char **icon;
	gpointer function;
} boton_icono;
 

GdkPixmap *duplicate_xpm(GtkWidget *widget,char **xpm,GdkBitmap **mask){
	GdkPixmap *pixmap;
#ifdef HAVE_GDK_PIXBUF
  	GdkPixbuf *orig_pixbuf,*new_pixbuf;

  	orig_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) xpm);
  	new_pixbuf  = gdk_pixbuf_scale_simple (orig_pixbuf, 2*gdk_pixbuf_get_width (orig_pixbuf), 
		  2*gdk_pixbuf_get_height (orig_pixbuf),GDK_INTERP_TILES );
  	gdk_pixbuf_render_pixmap_and_mask (new_pixbuf, &pixmap, mask, gdk_pixbuf_get_has_alpha (new_pixbuf));
  	gdk_pixbuf_unref (orig_pixbuf);
  	gdk_pixbuf_unref (new_pixbuf);
  	return pixmap;
#else /* fallback to xpm basics */
#define TMP_FILE_NAME "/tmp/xftree_tmp.xpm"
	char *w,*ww;
	static gchar *wx=NULL;
	FILE *tmpfile;
	int i,j,x,y,numcols,numchar;

	if (!xpm) return NULL;
	if (!xpm[0]) return NULL;
	ww = (char *)malloc(strlen(xpm[0])+32);
	strncpy(ww,xpm[0],strlen(xpm[0])+31);
	ww[strlen(xpm[0])+31]=0;
	
	w=strtok(ww," "); if (!w) return NULL;
	x=atoi(w);
	w=strtok(NULL," ");   if (!w) return NULL;
	y=atoi(w);
	w=strtok(NULL," ");   if (!w) return NULL;
	numcols=atoi(w);
	w=strtok(NULL," ");   if (!w) return NULL;
	numchar=atoi(w); 
	if (numchar != 1 ) return NULL; /* only 256 colors */
	free(ww);

	tmpfile=fopen(TMP_FILE_NAME,"w");
	fprintf(tmpfile,"/* XPM */\nstatic char *dupped_xpm[]={\n");
	fprintf(tmpfile,"\"%d %d %d %d\",\n",2*x,2*y,numcols,numchar);
	/* write colors */
        for (i=0;i<numcols;i++){
		fprintf(tmpfile,"\"%s\",\n",xpm[i+1]);
	}
	/* write data */
	wx = (char *)malloc(1+2*x);
	for (i=0;i<2*y;i++) {
	    for (j=0;j<2*x;j++) wx[j] = xpm[i/2+numcols+1][j/2];
	    wx[2*x] = 0;
	    fprintf(tmpfile,"\"%s\",\n",wx);
	}
        fprintf(tmpfile,"};\n");
	fclose(tmpfile);
	free(wx);

  	pixmap=gdk_pixmap_create_from_xpm (widget->window,mask,NULL,TMP_FILE_NAME);
	unlink(TMP_FILE_NAME);
        return pixmap;
#endif /* !HAVE_GDK_PIXBUF */
}		
  

GtkWidget *
create_toolbar (GtkWidget * top, GtkWidget * ctree, cfg * win,gboolean large)
{
  GdkPixmap *pixmap=NULL;
  GdkBitmap *pixmask=NULL;
  GtkWidget *widget;
  int i,k;
  unsigned int mask;
  gboolean do_sep=FALSE;
  GtkWidget *toolbar;
  boton_icono toolbarIcon[]={TOOLBARICONS};

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_space_style ((GtkToolbar *) toolbar, GTK_TOOLBAR_SPACE_LINE);
  gtk_toolbar_set_button_relief ((GtkToolbar *) toolbar, GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 2);
  gtk_widget_realize(top);
  if (large) mask=stateTB[1]; else mask=stateTB[0];

  for (k=i=0;toolbarIcon[i].text != NULL;i++) {
     if (toolbarIcon[i].icon==NULL) {
	  if (do_sep) gtk_toolbar_append_space ((GtkToolbar *)toolbar);
	  do_sep=FALSE;
     }
     else {
      if (mask & (0x01<<k)) {
        if (large) {
		pixmap=duplicate_xpm(top,toolbarIcon[i].icon,&pixmask);
		widget = gtk_pixmap_new (pixmap, pixmask);
	} else widget=MyCreateFromPixmapData (toolbar, toolbarIcon[i].icon);
       gtk_toolbar_append_item ((GtkToolbar *) toolbar,
	NULL,
	toolbarIcon[i].text,toolbarIcon[i].text,
	widget, 
	GTK_SIGNAL_FUNC (toolbarIcon[i].function),
	(gpointer) ctree);
       do_sep=TRUE;
      }
      k++;
     }
  }
  for (i=0;i<2;i++) win->stateTB[i]=stateTB[i];
  return toolbar;
}


/*static unsigned int initial_preferences,initial_mask[2];*/
static GtkWidget *config_toolbar_dialog=NULL;

static void destroy_config_toolbar(GtkWidget * widget,gpointer data){
  gtk_widget_destroy (widget);
}

static void cancel_config_toolbar(GtkWidget * widget,GtkWidget *ctree){
  destroy_config_toolbar(config_toolbar_dialog,NULL);
  /*for (i=0;i<2;i++) stateTB[i]=initial_mask[i];*/
  save_defaults(NULL);
  return;
}

static void regen_toolbar(GtkWidget * widget,GtkWidget *ctree){
  cfg *win;
  GtkWidget *toolbar,*parent,*toolbarN;

  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  if (win->preferences & LARGE_TOOLBAR) {
	  toolbar=win->toolbarO;
	  toolbarN=win->toolbar;
  } else {
	  toolbar=win->toolbar;
	  toolbarN=win->toolbarO;
  }
  
  parent=toolbar->parent;
  gtk_widget_destroy(toolbar);
  
  toolbar = create_toolbar (win->top, ctree, win,win->preferences & LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (parent), toolbar);
  gtk_widget_show (toolbar);
  if (!GTK_WIDGET_VISIBLE(parent)&&(!(win->preferences&HIDE_TOOLBAR))) {
	  
	  gtk_widget_show(parent);
	  gtk_widget_hide(toolbarN->parent);
  }
 
  if (win->preferences & LARGE_TOOLBAR) {
	  win->toolbarO=toolbar;
  } else {
	  win->toolbar=toolbar;
  }
}

static void ok_Lconfig_toolbar(GtkWidget * widget,GtkWidget *ctree){
  cfg *win;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  win->preferences ^= LARGE_TOOLBAR;	
  preferences = win->preferences; 
  regen_toolbar(widget,ctree);
  save_defaults(NULL);
}


static void toggle_toolbars(unsigned int mask,int which){
	/*fprintf(stderr,"dbg:toggling toolbar %d\n",which);*/
	stateTB[which] ^= mask;
	save_defaults (NULL);
	return;
}

static void toggle_toolbars0(GtkWidget * widget,gpointer data){
	unsigned int mask;
	mask=(unsigned int)((long)data);
	toggle_toolbars(mask,0);
	  save_defaults(NULL);
return;
}

static void toggle_toolbars1(GtkWidget * widget,gpointer data){
	unsigned int mask;
	mask=(unsigned int)((long)data);
	toggle_toolbars(mask,1);
	  save_defaults(NULL);
return;
}

void toggle_toolbar(GtkWidget * widget, GtkWidget *ctree){
	cfg *win;
	GtkWidget *tb;
  	win = gtk_object_get_user_data (GTK_OBJECT (ctree));
			
	win->preferences ^= HIDE_TOOLBAR;
 	preferences = win->preferences;
	
	if (win->preferences & LARGE_TOOLBAR) tb=(win->toolbarO)->parent;
	else tb=(win->toolbar)->parent;

	if (win->preferences & HIDE_TOOLBAR) {
		if (GTK_WIDGET_VISIBLE(tb)) gtk_widget_hide(tb);
	}
	else if (!GTK_WIDGET_VISIBLE(tb)) gtk_widget_show(tb);
  save_defaults(NULL);
	
}

static GtkWidget *toolbar_config(GtkWidget *ctree){
  GtkWidget *scrolledwindow,*table,*viewport,*widget,*hbox;
  int i,k;
  cfg *win;
  boton_icono toolbarIcon[]={TOOLBARICONS};
  char *labels[]={
	  _("Action"),
	  _("Small icons. "),
	  _("Large icons. ")
  };
  char *button_label[]={
	  _("Regen"),/* this is an invisible button */
	  _("OK")
  };
  gpointer button_function[]={
	regen_toolbar,
	cancel_config_toolbar
  };
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));


  
  config_toolbar_dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (config_toolbar_dialog), _("Configure toolbar"));
  gtk_signal_connect (GTK_OBJECT (config_toolbar_dialog), "destroy", 
		  GTK_SIGNAL_FUNC (destroy_config_toolbar), NULL);
  gtk_window_position (GTK_WINDOW (config_toolbar_dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_realize (config_toolbar_dialog);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
		  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_ref (scrolledwindow);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "scrolledwindowT", scrolledwindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_toolbar_dialog)->vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "viewportT", viewport,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);

  table = gtk_table_new (4, 4, FALSE);
  gtk_widget_ref (table);
  gtk_object_set_data_full (GTK_OBJECT (config_toolbar_dialog), "table", table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (viewport), table);

  /* table titles */
  for (i=0;i<3;i++) {
    widget=gtk_label_new(labels[i]);gtk_widget_show (widget);
    gtk_table_attach (GTK_TABLE (table), widget, i+1,i+2,0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  }
  
  /* table entries */
  for (k=i=0;toolbarIcon[i].text != NULL;i++) {
     int j;
     gpointer toolbar_func[]={
	     toggle_toolbars0,
	     toggle_toolbars1
     };
     if (!toolbarIcon[i].icon) continue; /* skip separators */
     widget = MyCreateFromPixmapData (config_toolbar_dialog, toolbarIcon[i].icon); 
     gtk_widget_show (widget);
     gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     0,1,k+1, k+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     
     widget=gtk_label_new( toolbarIcon[i].text);
     gtk_widget_show (widget);
     gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     1,2, k+1, k+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     for (j=0;j<2;j++) {
       widget=gtk_check_button_new ();
       if (stateTB[j] & (1<<k)) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
       gtk_widget_show (widget);
       gtk_signal_connect (GTK_OBJECT (widget), "toggled", GTK_SIGNAL_FUNC (toolbar_func[j]), 
		     (gpointer)((long) (1 << k)));
       gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (regen_toolbar),
		   (gpointer) ctree);
       gtk_table_attach (GTK_TABLE (table), 
		     widget, 
		     j+2,j+3, k+1, k+2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
     }
     k++;
  }
   
  /* hide show toolbar */
  hbox=gtk_hbox_new(TRUE,5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_toolbar_dialog)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  widget = gtk_check_button_new_with_label (_("Hide toolbar"));
  gtk_box_pack_end (GTK_BOX (hbox),widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  if (win->preferences & HIDE_TOOLBAR) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
  gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (toggle_toolbar),
		   (gpointer) ctree);
  
  widget = gtk_check_button_new_with_label (_("Large buttons"));
  gtk_box_pack_end (GTK_BOX (hbox),widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  if (win->preferences & LARGE_TOOLBAR) gtk_toggle_button_set_active ((GtkToggleButton *)widget,TRUE);
  gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (ok_Lconfig_toolbar),
		   (gpointer) ctree);
 
   /* buttons */
  for (i=0;i<2;i++) { 
    widget = gtk_button_new_with_label (button_label[i]);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(config_toolbar_dialog)->action_area),widget, FALSE, FALSE, 0);
    /*gtk_widget_show (widget);*/
    gtk_signal_connect (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (button_function[i]),
		  (gpointer) ctree);
  }
  gtk_widget_show (widget); /* only show last button. */
  
  gtk_widget_set_usize(config_toolbar_dialog,333,333);
  gtk_window_set_modal (GTK_WINDOW (config_toolbar_dialog), TRUE);
  gtk_widget_show (config_toolbar_dialog);
  return config_toolbar_dialog;

}

void cb_config_toolbar(GtkWidget *widget,GtkWidget *ctree){ 
  cfg *win;
  int i;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  for (i=0;i<2;i++) stateTB[i]=win->stateTB[i];
  if (win && win->top) 
    gtk_window_set_transient_for (GTK_WINDOW (toolbar_config(ctree)), GTK_WINDOW (win->top)); 	
}


