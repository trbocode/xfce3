/* copywrite 2001 edscott wilson garcia under GNU/GPL 
 * 
 * xtree_pasteboard.c: pasteboard routines for xftree
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
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "constant.h"
#include "my_intl.h"
#include "my_string.h"
#include "xpmext.h"
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
#include "xtree_pasteboard.h"
#include "xtree_cpy.h"
#include "xtree_cb.h"
#include "xtree_functions.h"
#include "xtree_mess.h"


#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define CUT_BUFFER 0

extern gboolean tar_extraction;
extern char *src_host;

/* get rid of any old version pasteboard hanging around */
static void zap_old_pasteboard(){
  static gboolean zapped=FALSE;
  char *oldpasteboard=NULL;
  int len;

  XStoreBuffer(GDK_DISPLAY(),"",1,CUT_BUFFER); /* store a null string */
  if (zapped) return;
  zapped=TRUE;
  len = strlen ((char *) getenv ("HOME")) + 
	  strlen ("/.xfce/") + strlen (XFTREE_PASTEBOARD) + 1;
  oldpasteboard = (char *) malloc ((len) * sizeof (char));
  if (!oldpasteboard) return;
  sprintf (oldpasteboard, "%s/.xfce/%s", (char *) getenv ("HOME"), XFTREE_PASTEBOARD);
  unlink(oldpasteboard);
  g_free(oldpasteboard);
  return ;
}

void cb_clean_pasteboard(GtkWidget * widget, GtkCTree * ctree){
  XStoreBuffer(GDK_DISPLAY(),"",1,CUT_BUFFER); /* store a null string */
  return;
}

static void copy_cut(GtkWidget * widget, GtkCTree * ctree,gboolean cut)
{
  GtkCTreeNode *node;
  GList *selection;
  entry *en;

  int len;
  char *buffer;
  
  zap_old_pasteboard();  
  if (!(g_list_length (GTK_CLIST (ctree)->selection))) return;
  len=1+strlen("#xfvalid_buffer:copy:%%:\n");
  len += strlen(our_host_name((GtkWidget *)ctree));
  for (selection=GTK_CLIST (ctree)->selection;selection;selection=selection->next){
    node = selection->data;
    en = gtk_ctree_node_get_row_data (ctree, node);
    if (!io_is_valid (en->label) || (en->type & FT_DIR_UP)) continue;
    len += (1+strlen(en->path));
  }
  buffer=(char *)malloc(len+sizeof(char));
  if (!buffer){
	  fprintf(stderr,"xftree: unable to allocate paste buffer\n");
	  return;
  }
  sprintf(buffer,"#xfvalid_buffer:%s:%s:\n",(cut)?"cut":"copy",our_host_name((GtkWidget *)ctree));
  for (selection=GTK_CLIST (ctree)->selection;selection;selection=selection->next){
    node = selection->data;
    en = gtk_ctree_node_get_row_data (ctree, node);
    if (!io_is_valid (en->label) || (en->type & FT_DIR_UP)) continue;
    strcat(buffer,en->path);   strcat(buffer,"\n");
  }
  /*printf("dbg:len=%d,data=%s\n",len,buffer);*/
  XStoreBuffer(GDK_DISPLAY(),buffer,len,CUT_BUFFER);  
  g_free(buffer);  
  gtk_ctree_unselect_recursive (GTK_CTREE (ctree), NULL);
}

void cb_copy(GtkWidget * widget, GtkCTree * ctree){
  copy_cut(widget,ctree,FALSE);
}

void cb_cut(GtkWidget * widget, GtkCTree * ctree){
  copy_cut(widget,ctree,TRUE);
}

/* this is equivalent to copying by dnd */
void cb_paste(GtkWidget * widget, GtkCTree * ctree){
  uri *u;
  cfg *win;
  gboolean cut;
  GList *list,*flist;
  entry *t_en;
  char *tmpfile,*b,*word,*path;
  int i,len=-1;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  b=XFetchBuffer(GDK_DISPLAY(),&len,0);
  /*printf("dbg:bytes=%d,buffer0=%s\n",len,b);*/

  if ((!b) || (!strlen(b))) {
	  xf_dlg_info(win->top,_("The pasteboard is currently empty."));
	  if (b) XFree(b); 
	  return;
  }
	  
  if ((word=strtok(b,":"))==NULL){ XFree(b); return;}
  if (!strstr(word,"#xfvalid_buffer")){
 	  xf_dlg_info(win->top,_("Not a valid file transfer pasteboard."));
	  return;
  }
  if ((word=strtok(NULL,":"))==NULL) { XFree(b); return;}  
  if (strstr(word,"cut")) cut=TRUE; else cut=FALSE;
  if ((word=strtok(NULL,":"))==NULL) { XFree(b); return;}  
  if (!word) {
	  fprintf(stderr,"xftree: source host was not specified.\n");
	  XFree(b); return;
  }
  src_host=g_strdup(word);

  word = word + strlen(word) +1;
  if (word[0]=='\n') {
	  word++;
	  if (word[0]==0) { XFree(b); return;}
  } else {
    if ((word=strtok(NULL,"\n"))==NULL) { XFree(b); return;}  
    word = word + strlen(word) +1;
  }
  
  	 
  /* create list to send to CreateTmpList */
  i = uri_parse_list (word, &list);
  flist=list; /* keep initial pointer to later free the list */
  u = list->data;
  XFree(b); /* no longer needed here */
  if (!i) return;
  /* create a tmpfile */
  path=valid_path(ctree,TRUE);
  t_en = entry_new_by_path(path);
  if (u->type == URI_SMB){
	    extern void SMBGetFile (GtkCTree *,char *,GList *);
	    /*fprintf(stderr,"dbg: SMB type received.\n");*/
	    SMBGetFile ((GtkCTree *)ctree,t_en->path,list);
  } else if (strcmp(src_host,our_host_name((GtkWidget *)ctree)) != 0) {
     for (;list!=NULL;list=list->next){
        u = list->data;
        if (!rsync((GtkCTree *)ctree,u->url,t_en->path)) break;
     }
  } else {
      tmpfile=CreateTmpList(win->top,list,t_en);
      /*fprintf(stderr,"dbg:tmpfile=%s\n",tmpfile);*/
      if (tmpfile) {
       if (tar_extraction) DirectTransfer((GtkWidget *)ctree,(cut)?TR_MOVE:TR_COPY,tmpfile);
       else {
 	    if ((cut)&&(on_same_device())) DirectTransfer((GtkWidget *)ctree,TR_MOVE,tmpfile);
	    else IndirectTransfer((GtkWidget *)ctree,(cut)?TR_MOVE:TR_COPY,tmpfile);
       }
       unlink(tmpfile); 
      }
  }
  list=uri_free_list (flist);

  entry_free(t_en);
  if (cut) XStoreBuffer(GDK_DISPLAY(),"",1,CUT_BUFFER); /* store a null string */
  update_timer(ctree);

  return;
}

void cb_paste_show(GtkWidget * widget, GtkCTree * ctree){
  cfg *win;
  char *b;
  int len=-1;
  win = gtk_object_get_user_data (GTK_OBJECT (ctree));
  b=XFetchBuffer(GDK_DISPLAY(),&len,0);
  /*printf("dbg:bytes=%d,buffer0=%s\n",len,b);*/
  if (b) {
   clear_cat(widget,ctree);
   if (strlen(b)) show_cat(b);
   else show_cat( _("The pasteboard is currently empty.\n"));
   XFree(b);
  } else xf_dlg_info(win->top,_("No pasteboard available."));
  return;
}

