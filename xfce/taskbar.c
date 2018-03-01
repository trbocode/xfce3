#define NO_LOG_FILE                         
/*
 *
 * AFECTED FILES: taskbar.[ch] xfce.c xfwm.c handle.h configfile.[ch] Makefile.am 
 *                ../libs/xfcolor.c ../libs/fileutil.[ch] ../libs/Makefile.am
 *                ../configure.in ../acconfig.h 
 *                ../other/taskbarrc ../other/Makefile.am
 *                ../scripts/xfce_setup.in
 *
 * TODO (NaB = Not a Bug):
 *  - (?)gtk types conversion (int<->pointer)
 *  - clean up widget structure 
 *  - (100%)set proper tb size according to 'standalone' state
 *  - (?) various taskbar height for different xfce panel sizes
 *  - (100%) proper disabling of 'toggled' signal handling
 *  - (100%) proper checking for XFCE window itself
 *  - (90%code,0%properties) internationalization
 *  - (?) long to gint/int
 *  - (100%/changing xfcolor.c concept) reliable indicator of current desk 
 *  - (NaB) cleanup init function(s) -- make single one
 *  - (?) remove g_xfce_taskbar and (100%) other global vars
 *  - (90%) function names clean-up
 *  - (100%) constants (DEFINEs) for all commands send to xfwm
 *  - comments
 *
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifdef XFCE_TASKBAR

  #include "taskbar.h"

  #include <stdio.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <stdarg.h>
  #include <string.h>
  #include <glib.h>
  #include <gtk/gtk.h>
  #include <math.h>

  #include "xfwm.h"
  #include "../xfwm/xfwm.h"
  #include "xfce.h"
  #include "fileutil.h"
  #include "utils.h"
  #include "configfile.h"
  #include "sendinfo.h"
  #include "xfce_cb.h"
  #include "selects.h"
  #include "constant.h"
  #include "my_intl.h"
  #include "move.h"
  #include "gnome_protocol.h"
  #include "xpmext.h"
  #include "handle.h"
  

#define TASKBAR_CMD_WIN_LIST  "Send_WindowList"
#define TASKBAR_CMD_CHANGE_FOCUS "Focus"
#define TASKBAR_CMD_ICONIFY  "Iconify 1"
#define TASKBAR_CMD_DEICONIFY "Iconify -1"
#define TASKBAR_CMD_RAISE "Raise"

#define TASKBAR_MIN_CELL_NO 5

#define TASKBAR_SORT_BY_NAME    1       
#define TASKBAR_SORT_BY_DESK    2       
#define TASKBAR_SORT_BY_WINID   3       
#define TASKBAR_SORT_UNSORTED   4       

#define TASKBAR_TB_HEIGHT       16

#define TASKBAR_TASK_FLAG_NONE       0x00000000L
#define TASKBAR_TASK_FLAG_VISIBLE    0x00000001L       
#define TASKBAR_TASK_FLAG_TASKJAR    0x00000002L

#define TASKBAR_RC_FILE   "taskbarrc"       
                      
typedef struct _taskbar_window {
  unsigned long window;
  char *name;
  int desk;
  unsigned long flags;
  unsigned long my_flags;
} taskbar_window;

#define TASKBAR_FLAG_NONE              0x00000000L
#define TASKBAR_FLAG_SINGLE_DESK       0x00000001L
#define TASKBAR_FLAG_STANDALONE        0x00000002L
#define TASKBAR_FLAG_PROC_LOAD         0x00000004L
#define TASKBAR_FLAG_OPENED            0x00000008L
#define TASKBAR_FLAG_WINLISTSKIP       0x00000010L
#define TASKBAR_FLAG_HIDE_XFCE_PANEL     0x00000020L
#define TASKBAR_FLAG_CONFIG_READ       0x80000000L

typedef struct _taskbar_xfce {
  GtkWidget *gtk_xfce_toplevel;
  GtkWidget *gtk_realtaskbar;
  GtkWidget *gtk_labels[TASKBAR_MIN_CELL_NO];
  GtkToggleButton *gtk_toggled_button;
  GtkWidget *gtk_proc_load_indicator;
  int gtk_pressed;
  int base_height;
  int base_width;
  GtkWidget *model_widget;
  int gtk_task_no;
  gint gtk_timeout_handler_id;
  long curr_desk;
  unsigned long curr_window;
  int sort_order;
  GList* windows;
  GtkWidget *gtk_stand_alone;
  GtkWidget *gtk_task_popup_menu;
  unsigned long flags;
  GtkWidget *gtk_taskjar_menu;
  int gtk_standalone_x,gtk_standalone_y;
  GtkWidget *gtk_desk_menu;
  int desk_menu_choice;
  char *cmd_to_execute;
  char **taskjar_pattern;
} taskbar_xfce;



/* 
 * where all taskbar structures are stored
 */
taskbar_xfce g_xfce_taskbar;

void taskbar_add_task_widget(long id,char *wname);
void taskbar_update_task_widget(long id,char *caption);
void taskbar_remove_task_widget(long id);
void taskbar_select_task_widget(long id);
void taskbar_synch_single_task_widget_with_desk(long id, taskbar_window *win);
void taskbar_synch_task_widget_with_desk();
void taskbar_synch_labels();

void taskbar_set_standalone_state();
gint tb_set_proc_load(gpointer data);
void taskbar_put_in_taskjar(taskbar_window *tbw);
void 
taskbar_remove_from_taskjar(taskbar_window *tbw);


  #ifdef LOG_FILE
FILE *_f;
  #endif

#ifdef LOG_FILE
void _print_gtk_widget_hierarchy(FILE *f,GtkWidget *widget, int level)
{
  int i;
  GList *l,*cl;
  if (!widget)
    return;
  for (i=0;i<level;i++)
    fprintf(f," ");
  if (GTK_IS_CONTAINER(widget)) {
    fprintf(f,"%s\n",widget->name);
    for (i=0;i<level;i++)
      fprintf(f," ");
    fprintf(f,"{\n");
    l=cl=gtk_container_children((GtkContainer*)widget);
    while (cl!=NULL) {
      _print_gtk_widget_hierarchy(f,(GtkWidget*)cl->data,level+3);
      cl=cl->next;
    }
    g_list_free(l);
    for (i=0;i<level;i++)
      fprintf(f," ");
    fprintf(f,"}\n");
  } else {
    fprintf(f,"%s\n",widget->name);
  }

}
#endif

void taskbar_xfwm_init()
{
#ifdef LOG_FILE
  if(!_f) {
    _f=fopen("/tmp/xfce_ms.log","a");
    fprintf(_f,"--------------------------------------\n");
  }
#endif
  g_xfce_taskbar.windows=NULL;


  if (current_config.wm)
    sendinfo (fd_internal_pipe,TASKBAR_CMD_WIN_LIST, 0);
}


gint taskbar_glist_comp_find_window(gconstpointer *a, gconstpointer *b)
{
  if (!a)
    return -1;
  return(((taskbar_window*)a)->window==(long)b) ? 0 : -1;
}

gint taskbar_glist_comp_sort_name(gconstpointer *a, gconstpointer *b)
{
  char *sa,*sb;
  sa=((taskbar_window*)a)->name;
  sb=((taskbar_window*)b)->name;
  return sa==NULL ? 1 : sb==NULL ? 1 : strcasecmp(sa,sb);
}

gint taskbar_glist_comp_sort_winid(gconstpointer *a, gconstpointer *b)
{
  long la,lb;
  la=((taskbar_window*)a)->window;
  lb=((taskbar_window*)b)->window;
  return la==lb ? 0 : la<lb ? -1 : 1;
}

gint taskbar_glist_comp_sort_desk(gconstpointer *a, gconstpointer *b)
{
  long la,lb;
  la=((taskbar_window*)a)->desk;
  lb=((taskbar_window*)b)->desk;
  return la==lb ? taskbar_glist_comp_sort_winid(a,b) : la<lb ? -1 : 1;
}

taskbar_window *taskbar_find_window(long id)
{
  GList *l;
  l=g_list_find_custom(g_xfce_taskbar.windows,(gpointer)id,(GCompareFunc)taskbar_glist_comp_find_window);
  return l ? (taskbar_window*) l->data : NULL;
}

void taskbar_set_active(long type)
{
  taskbar_window *tbw;
  tbw=taskbar_find_window(type);
  if (!tbw)
    return;
  if (current_config.wm) {
    sendinfo (fd_internal_pipe,TASKBAR_CMD_CHANGE_FOCUS, tbw->window);
    sendinfo (fd_internal_pipe,TASKBAR_CMD_DEICONIFY, tbw->window);
    sendinfo (fd_internal_pipe,TASKBAR_CMD_RAISE, tbw->window);
    tbw->flags&=!ICONIFIED;
  }
}

void taskbar_toggle_iconify(long type)
{
  taskbar_window *tbw;
  tbw=taskbar_find_window(type);
  if (!tbw)
    return;
  if (current_config.wm) {
    if (tbw->flags&ICONIFIED) {
      sendinfo (fd_internal_pipe,TASKBAR_CMD_CHANGE_FOCUS, tbw->window);
      sendinfo (fd_internal_pipe,TASKBAR_CMD_DEICONIFY, tbw->window);
      sendinfo (fd_internal_pipe,TASKBAR_CMD_RAISE, tbw->window);
      tbw->flags&=!ICONIFIED;
    } else {
      sendinfo (fd_internal_pipe,TASKBAR_CMD_ICONIFY, tbw->window);
      tbw->flags|=ICONIFIED;
    }
  }
}

GtkWidget* taskbar_get_widget(long id)
{
  GtkWidget *w;
  char sid[64];

  g_snprintf(sid,sizeof(sid)-1,"task%ld",id);
  w=g_xfce_taskbar.gtk_realtaskbar;
  return(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),sid);
}

void taskbar_adapt_widgets_to_tasks_order()
{
  taskbar_window *cwin;    
  GList *clist;
  GtkWidget *w;
  GtkBox  *box;
  int cpos=0;

  box=GTK_BOX(g_xfce_taskbar.gtk_realtaskbar);
  clist=g_list_first(g_xfce_taskbar.windows);
  while (clist) {
    cwin=(taskbar_window*)clist->data;
    w=taskbar_get_widget(cwin->window);
    if (w) {
      gtk_box_reorder_child(box,w,cpos++);    
    }
    clist=g_list_next(clist);
  }
}

void taskbar_sort(int new_order)
{
  g_xfce_taskbar.sort_order=new_order;
  switch (new_order) {
  case TASKBAR_SORT_BY_DESK:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_desk);
    break;
  case TASKBAR_SORT_BY_NAME:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_name);
    break;
  case TASKBAR_SORT_BY_WINID:
    g_xfce_taskbar.windows=g_list_sort(g_xfce_taskbar.windows,(GCompareFunc)taskbar_glist_comp_sort_winid);
    break;
  case TASKBAR_SORT_UNSORTED:
  default:
    return;
    ;
  } 
  taskbar_adapt_widgets_to_tasks_order();
}

int
taskbar_check_if_taskjar(taskbar_window *tbw)
{
  char **patt;

  if(!tbw->name)
    return FALSE;
  patt=g_xfce_taskbar.taskjar_pattern;
  if(!patt)
    return FALSE;
  while(*patt) {
    if(matchWildcards(*patt,tbw->name)) {
      taskbar_put_in_taskjar(tbw);
      return TRUE;
    }
    patt++;
  }
  return FALSE;
}

/* 
 * check for events interested for taskbar
 */
void taskbar_check_events(unsigned long type,unsigned long *body)
{
  taskbar_window *tbw;
  long desk_diff;
  long flags_diff;
  int is_new;

  switch (type) {
  case XFCE_M_CONFIGURE_WINDOW:
#ifdef LOG_FILE
    fprintf(_f,"CONFIGURE_WINDOW: %ld\n",body[0]);
#endif  
  case XFCE_M_ADD_WINDOW:
    is_new=FALSE;
    if (!(tbw=taskbar_find_window(body[0]))) {
#ifdef LOG_FILE
      fprintf(_f,"ADD_WINDOW: %ld\n",body[0]);
#endif      
      tbw=(taskbar_window*)malloc(sizeof(taskbar_window));
      if (!tbw) {
#ifdef LOG_FILE
        fprintf(_f,"Cannot malloc!\n");
#endif        
        return; /* TODO: error */
      }
      tbw->window=body[0];
      tbw->name=NULL;
      tbw->desk=body[7];
      tbw->flags=body[8];
      tbw->my_flags=TASKBAR_TASK_FLAG_NONE;
      g_xfce_taskbar.windows=g_list_append(g_xfce_taskbar.windows,tbw);
      taskbar_add_task_widget(tbw->window,tbw->name);
      taskbar_check_if_taskjar(tbw);
      is_new=TRUE;
    }

    desk_diff=tbw->desk!=body[7];
    tbw->desk=body[7];
    flags_diff=tbw->flags^body[8];
    tbw->flags=body[8];
    if (desk_diff||is_new)
      taskbar_sort(g_xfce_taskbar.sort_order);
    taskbar_synch_single_task_widget_with_desk(-1,tbw);
    taskbar_synch_labels();

    break;
  case XFCE_M_NEW_DESK:
#ifdef LOG_FILE
    fprintf(_f,"NEW_DESK: %ld\n",body[0]);
#endif    
    g_xfce_taskbar.curr_desk=body[0];
    taskbar_synch_task_widget_with_desk();
#ifdef LOG_FILE
  fprintf(_f,"synch ---------------------\n");
#endif
    break;
  case XFCE_M_DESTROY_WINDOW:
#ifdef LOG_FILE
    fprintf(_f,"DESTROY_WINDOW: %ld\n",body[0]);
#endif    
    if ((tbw=taskbar_find_window(body[0]))) {
      taskbar_remove_from_taskjar(tbw);
      taskbar_remove_task_widget(tbw->window);
      if(tbw->my_flags&TASKBAR_TASK_FLAG_VISIBLE) {
        g_xfce_taskbar.gtk_task_no--;
      }
      g_xfce_taskbar.windows=g_list_remove(g_xfce_taskbar.windows,tbw);
      if (tbw->name)
        free(tbw->name);
      free(tbw);
      taskbar_synch_labels();
    }
    break;
  case XFCE_M_FOCUS_CHANGE:
#ifdef LOG_FILE
    fprintf(_f,"FOCUS_CHNAGE: %ld\n",body[0]);
#endif    
    if ((tbw=taskbar_find_window(body[0]))) {
      taskbar_select_task_widget(tbw->window);
    }
    break;
  case XFCE_M_ICON_NAME:
#ifdef LOG_FILE
    fprintf(_f,"ICON NAME: %ld %s\n",body[0],(char*)(&body[3]));
#endif   
    if ((tbw=taskbar_find_window(body[0]))) {
      if (tbw->name)
        free(tbw->name);
      tbw->name=strdup((char*)(&body[3])); 
      taskbar_update_task_widget(tbw->window,tbw->name);
      taskbar_sort(g_xfce_taskbar.sort_order);
      if(taskbar_check_if_taskjar(tbw)) {
        taskbar_synch_single_task_widget_with_desk(-1,tbw);
        taskbar_synch_labels();
      }
    } else
      return; /* TODO: error */
    break;


  default:
    ;
  } /* switch */
#ifdef LOG_FILE
  fflush(_f);
#endif
}


/*
 * User Interace stuff
 */

void gtk_set_bg_color(GtkWidget *w,GtkStateType type,int color)
{
  GtkStyle *style,*os;

  os=gtk_widget_get_style(w);
  if(os)
    style=gtk_style_copy(os);
  else 
    style=gtk_style_new();

  if (style->bg_pixmap[type]) {
    style->bg_pixmap[type]=NULL;
  }
  if(style->rc_style) {
    gtk_rc_style_unref(style->rc_style);
    style->rc_style=NULL;
  }
  style->bg[type].red=(color&0x00ff0000)>>8;
  style->bg[type].green=(color&0x0000ff00);
  style->bg[type].blue=(color&0x000000ff)<<8;
  gtk_widget_set_style(w,style);
  gtk_style_unref(style);
}

int gtk_get_bg_color(GtkWidget *w,GtkStateType type)
{
  GtkStyle *style;
  int r;

  style=gtk_widget_get_style (w);
  r=0;
  r=r|((style->bg[type].red&0xff00)<<8);
  r=r|(style->bg[type].green&0xff00);
  r=r|((style->bg[type].blue&0xff00)>>8);
  return r;
}



void taskbar_synch_labels()
{
  int i;

  for (i=0;i<(TASKBAR_MIN_CELL_NO-g_xfce_taskbar.gtk_task_no);i++) {
    gtk_widget_show(g_xfce_taskbar.gtk_labels[i]);
  }
  for (i=i;i<TASKBAR_MIN_CELL_NO;i++) {
    gtk_widget_hide(g_xfce_taskbar.gtk_labels[i]);
  }

}


void taskbar_set_labels()
{
  int i;
  for (i=0;i<TASKBAR_MIN_CELL_NO;i++) {
    g_xfce_taskbar.gtk_labels[i]=gtk_label_new("");
    gtk_box_pack_end_defaults(GTK_BOX(g_xfce_taskbar.gtk_realtaskbar),g_xfce_taskbar.gtk_labels[i]);
  }
}


void taskbar_synch_single_task_widget_with_desk(long id, taskbar_window *win)
{
GtkWidget *button;
  
  if(!win)
    win=taskbar_find_window(id);
  button=taskbar_get_widget(win->window);
  if(!button)
    return;
  if(((win->flags&WINDOWLISTSKIP)&&(g_xfce_taskbar.flags&TASKBAR_FLAG_WINLISTSKIP))||(win->my_flags&TASKBAR_TASK_FLAG_TASKJAR)) {
    gtk_widget_hide(button);
    if((win->my_flags&TASKBAR_TASK_FLAG_VISIBLE)) {
      g_xfce_taskbar.gtk_task_no--;
      win->my_flags&=~TASKBAR_TASK_FLAG_VISIBLE;
    }
    return;
  }
  if (!(g_xfce_taskbar.flags&TASKBAR_FLAG_SINGLE_DESK)||(win->desk==g_xfce_taskbar.curr_desk)||(win->flags&STICKY)) {
    if(!(win->my_flags&TASKBAR_TASK_FLAG_VISIBLE)) {
      g_xfce_taskbar.gtk_task_no++;
      gtk_widget_show(button);
      win->my_flags|=TASKBAR_TASK_FLAG_VISIBLE;
    }
  } else {
    gtk_widget_hide(button);
    if((win->my_flags&TASKBAR_TASK_FLAG_VISIBLE)) {
      g_xfce_taskbar.gtk_task_no--;
      win->my_flags&=~TASKBAR_TASK_FLAG_VISIBLE;
    }
  }
  gtk_widget_set_name(button,(win->desk==g_xfce_taskbar.curr_desk) ? "task_active" : "task");
}


void taskbar_synch_task_widget_with_desk()
{
GList *clist;
taskbar_window *win;

  clist=g_list_first(g_xfce_taskbar.windows);
  while (clist) {
    win=(taskbar_window*)clist->data;
    clist=g_list_next(clist);
    taskbar_synch_single_task_widget_with_desk(-1,win);
  }
  taskbar_synch_labels();
}





void taskbar_on_button_task_toggled(GtkToggleButton *button,
                                    gpointer user_data)
{
  long id=(long)user_data;
  int oldPressed;

  oldPressed=g_xfce_taskbar.gtk_pressed;
  g_xfce_taskbar.gtk_pressed=FALSE;

  if (!oldPressed||gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    taskbar_set_active(id);
  } else {
    taskbar_toggle_iconify(id);   
  }
}

gint taskbar_on_button_task_pressed(GtkToggleButton *button,GdkEventButton *event,
                                    gpointer user_data)
{
  g_xfce_taskbar.gtk_pressed=TRUE;
  if((g_xfce_taskbar.gtk_task_popup_menu)&&
    ((event->type == GDK_BUTTON_PRESS)&&(event->button >= 2))) {
      g_xfce_taskbar.curr_window=(unsigned long)user_data;
      gtk_menu_popup (GTK_MENU(g_xfce_taskbar.gtk_task_popup_menu), NULL, NULL, NULL, NULL,
                      event->button, event->time);
      return TRUE;
  }

  return FALSE;
}

void taskbar_add_task_widget(long id,char *wname)
{
  GtkWidget *w;
  GtkWidget *button;
  GtkWidget *label;
  char sid[64];
  guint handler_id;

  if ((GDK_WINDOW_XWINDOW(g_xfce_taskbar.gtk_xfce_toplevel->window)==id)||
      ((g_xfce_taskbar.gtk_stand_alone)&&(GDK_WINDOW_XWINDOW(g_xfce_taskbar.gtk_stand_alone->window)==id))) {
    return;
  }

  g_snprintf(sid,sizeof(sid)-1,"task%ld",id);

  w=g_xfce_taskbar.gtk_realtaskbar;
  button = gtk_toggle_button_new();
  label = gtk_label_new(wname ? wname : "(null)");
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_container_add(GTK_CONTAINER(button),label);

  gtk_object_set_data (GTK_OBJECT (w), strdup(sid), button);
  gtk_widget_set_name(button,"task");

  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
                      GTK_SIGNAL_FUNC (taskbar_on_button_task_pressed),
                      (gpointer)id);
  handler_id=gtk_signal_connect (GTK_OBJECT (button), "toggled",
                                 GTK_SIGNAL_FUNC (taskbar_on_button_task_toggled),
                                 (gpointer)id);
  gtk_object_set_data(GTK_OBJECT(button),"handler_id",(gpointer)handler_id);


#ifdef LOG_FILE
  fprintf(_f,"show button: %ld\n",id);
#endif
  gtk_box_pack_start (GTK_BOX (w), button, TRUE, TRUE, 0);

}

void taskbar_remove_task_widget(long id)
{
  GtkWidget *w;
  GtkWidget *button;
  char sid[64];

  g_snprintf(sid,sizeof(sid)-1,"task%ld",id);
  w=g_xfce_taskbar.gtk_realtaskbar;
  button=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),sid);
  if (!button)
    return;

  if ((GtkToggleButton*)button==g_xfce_taskbar.gtk_toggled_button)
    g_xfce_taskbar.gtk_toggled_button=NULL;


  gtk_container_remove(GTK_CONTAINER(w),button);
  gtk_object_remove_data(GTK_OBJECT(w),sid);
}

void taskbar_update_task_widget(long id,char *wname)
{
  GtkWidget *button;
  GList *gl;
  GtkLabel *label;

  button=taskbar_get_widget(id);
  if (!button)
    return;

  gl=gtk_container_children(GTK_CONTAINER(button));

  if (gl&&gl->data) {
    label=(GtkLabel*)gl->data;
    gtk_label_set_text(label,wname);
    gtk_widget_show(GTK_WIDGET(label));
  }

  /* set tool tip */
  gtk_tooltips_set_tip(gtk_tooltips_new(),button,wname,NULL);


}



void taskbar_select_task_widget(long id)
{
  GtkWidget *button;
  guint handler_id;

  button=taskbar_get_widget(id);    
  if (!button)
    return;

  handler_id=(guint)gtk_object_get_data(GTK_OBJECT(button),"handler_id");
  if (g_xfce_taskbar.gtk_toggled_button) {
    handler_id=(guint)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),"handler_id");
    gtk_signal_handler_block(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),handler_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_xfce_taskbar.gtk_toggled_button),FALSE);
    gtk_signal_handler_unblock(GTK_OBJECT(g_xfce_taskbar.gtk_toggled_button),handler_id);

  }

  if (button) {
    handler_id=(guint)gtk_object_get_data(GTK_OBJECT(button),"handler_id");
    gtk_signal_handler_block(GTK_OBJECT(button),handler_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
    gtk_signal_handler_unblock(GTK_OBJECT(button),handler_id);
  }

  g_xfce_taskbar.gtk_toggled_button=(GtkToggleButton*)button;

}



void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;

  if ( s == 0 ) {
    *r = *g = *b = v;
    return;
  }

  h /= 60;
  i = floor( h );
  f = h - i;
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );

  switch ( i ) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:
    *r = v;
    *g = p;
    *b = q;
    break;
  }

}



gint taskbar_set_proc_load(gpointer data)
{
  char stats_in[256];
  char *s;
  FILE *stats_f;
  int user,sys,nice,idle;
  static int o_user,o_sys,o_nice,o_idle;
  float over;
  int busy,total;
  float r,g,b;


  if (!g_xfce_taskbar.gtk_proc_load_indicator)
    return TRUE;

  stats_f=fopen("/proc/stat","r");
  if (!stats_f) {
    return TRUE;
  }
  while (fgets(stats_in,sizeof(stats_in),stats_f)!=NULL) {
    if ((s=strstr(stats_in,"cpu"))!=NULL) {
      s=s+strlen("cpu");
      sscanf(s,"%d %d %d %d",&user,&sys,&nice,&idle);
      busy=(user-o_user)+(sys-o_sys)+(nice-o_nice); 
      total=busy+(idle-o_idle);
      over=(float)busy/(float)total;
      HSVtoRGB(&r,&g,&b,0.0,over,1.0);
      gtk_set_bg_color(g_xfce_taskbar.gtk_proc_load_indicator,GTK_STATE_NORMAL,(((int)(r*0xff))<<16)|(((int)(g*0xff))<<8)|(((int)(b*0xff))<<0));
      o_user=user; o_sys=sys; o_nice=nice; o_idle=idle;
      break;
    }
  }
  fclose(stats_f);

  return TRUE;

}



void taskbar_on_button_open_clicked(GtkWidget *w,gpointer data)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_hrule;
  GtkWidget *tb_standalone;
  GtkWidget *tb_proc_load;
  GtkWidget *tb_single_desk;
  GtkWidget *tb_radio;

  GtkWidget *tb_button_close;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),"tb_panel");
  tb_standalone=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_standalone");
  tb_proc_load=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_sysload");
  tb_single_desk=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_single_desk");

  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");
  tb_button_close=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_close");


  g_xfce_taskbar.gtk_proc_load_indicator=tb_button_close;

  gtk_widget_show_all(tb_hbox1);
  gtk_widget_show(tb_hrule);
  gtk_widget_hide(w);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tb_standalone),g_xfce_taskbar.flags&TASKBAR_FLAG_STANDALONE);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tb_proc_load),g_xfce_taskbar.flags&TASKBAR_FLAG_PROC_LOAD);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tb_single_desk),g_xfce_taskbar.flags&TASKBAR_FLAG_SINGLE_DESK);
  switch (g_xfce_taskbar.sort_order) {
  case TASKBAR_SORT_BY_DESK:
    tb_radio=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_sort_desk");
    break;
  case TASKBAR_SORT_BY_NAME:
    tb_radio=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_sort_name");
    break;
  case TASKBAR_SORT_BY_WINID:
    tb_radio=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_sort_win_id");
    break;
  case TASKBAR_SORT_UNSORTED:
    tb_radio=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_sort_unsorted");
    break;
  default:
    tb_radio=NULL;
  } 
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tb_radio),1);
  
  

  g_xfce_taskbar.flags|=TASKBAR_FLAG_OPENED;
  taskbar_synch_task_widget_with_desk();

  taskbar_set_standalone_state();

}

gint taskbar_on_button_taskjar_clicked(GtkWidget *widget, GdkEventButton *event)
{
  GtkMenu *menu;
  
  menu = GTK_MENU (g_xfce_taskbar.gtk_taskjar_menu);
  if ((event->type == GDK_BUTTON_PRESS)) {
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
  }
  return FALSE;
}

void taskbar_on_button_close_clicked(GtkWidget *w,gpointer data)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_open;
  GtkWidget *tb_hrule;  
  GtkWidget *tb_standalone;  
  g_xfce_taskbar.gtk_proc_load_indicator=NULL;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(w),"tb_panel");
  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_button_open=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_open");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");

  tb_standalone=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_mcheck_standalone");

  g_xfce_taskbar.flags&=~TASKBAR_FLAG_OPENED;
   /* work around for size changing */
  tb_button_open->allocation.width=-1;
  tb_button_open->allocation.height=-1;
  taskbar_set_standalone_state();

  gtk_widget_hide_all(tb_hbox1);
  gtk_widget_hide(tb_hrule);
  gtk_widget_show(tb_button_open);
/*
  while(gtk_events_pending())
    gtk_main_iteration();
printf("--wait_3\n");
  sleep(4);
printf("---close clicked end\n");
*/
}

/*
 * pop-up menu
 */
gint taskbar_on_button_close_right_clicked(GtkWidget *widget, GdkEventButton *event)
{
  GtkMenu *menu;
  
  menu = GTK_MENU (widget);
  if ((event->type == GDK_BUTTON_PRESS)&&(event->button >= 2)) {
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
  }
  return FALSE;
}


void taskbar_on_radio_sort_toggled(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  int type;
  type=(int)user_data;
  if (!checkmenuitem->active)
    return;
  taskbar_sort(type);
}


GtkWidget* taskbar_create_standalone_frame(GtkWidget *to_be_added)
{
  GtkWidget *window,*frame;
  GtkWidget *hbox;
  GtkWidget *ebox;
  GtkWidget *move_pixmap;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
  gtk_widget_set_name (window, "tb_standalone");
  gtk_window_set_title (GTK_WINDOW (window), "XFce Taskbar");
  gtk_widget_realize (window);
  /* decorations !!!!! */
  gdk_window_set_decorations (window->window, 0);
  frame=gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(window),frame);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

  hbox=gtk_hbox_new(FALSE,1);
  gtk_container_add(GTK_CONTAINER(frame),hbox);
  ebox=gtk_event_box_new();
  create_move_button(ebox,window);
  move_pixmap = MyCreateFromPixmapData (ebox, handle);
  if (move_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_show(move_pixmap);
  gtk_widget_set_usize(move_pixmap,12,TASKBAR_TB_HEIGHT);
  gtk_container_add(GTK_CONTAINER(ebox),move_pixmap);

  gtk_box_pack_start(GTK_BOX(hbox),ebox,FALSE,FALSE,2);
  gtk_widget_show(ebox);
  gtk_widget_show(hbox);


  gtk_object_ref(GTK_OBJECT(to_be_added));
  if (to_be_added->parent)
    gtk_container_remove(GTK_CONTAINER(to_be_added->parent),to_be_added);
  gtk_container_add(GTK_CONTAINER(hbox),to_be_added);
  gtk_object_unref(GTK_OBJECT(to_be_added));


  ebox=gtk_event_box_new();
  create_move_button(ebox,window);
  move_pixmap = MyCreateFromPixmapData (ebox, handle);
  if (move_pixmap == NULL)
    g_error (_("Couldn't create pixmap"));
  gtk_widget_show(move_pixmap);
  gtk_widget_set_usize(move_pixmap,12,TASKBAR_TB_HEIGHT);
  gtk_container_add(GTK_CONTAINER(ebox),move_pixmap);
  gtk_box_pack_start(GTK_BOX(hbox),ebox,FALSE,FALSE,2);
  gtk_widget_show(ebox);

  gtk_widget_show(to_be_added);

  gnome_layer (window->window, current_config.panel_layer);

  gtk_widget_show(window);
  return window;
}


void taskbar_set_standalone_state()
{
  GtkWidget *tb_panel;
  GtkWidget *tb_parent;
  GtkWidget *tb_hrule;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_open;
  GtkRequisition req;
  GtkWidget *toggle;
  gboolean state;
  int x,y;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_realtaskbar),"tb_panel");

  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");
  tb_button_open=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_open");

  toggle=gtk_toggle_button_new_with_label("A");
/*  
  toggle_style=gtk_widget_get_style(toggle);
  if(toggle_style->font) {
    base_height=toggle_style->
  } else {
    base_height=TASKBAR_TB_HEIGHT;
  }
*/  
  gtk_widget_size_request(GTK_BIN(toggle)->child,&req);
  g_xfce_taskbar.base_height=req.height;
  gtk_widget_unref(toggle);

  state=g_xfce_taskbar.flags&TASKBAR_FLAG_STANDALONE ? TRUE : FALSE;
  if ((state==TRUE)&&(g_xfce_taskbar.flags&TASKBAR_FLAG_OPENED)) {
    if (!g_xfce_taskbar.gtk_stand_alone) {
      g_xfce_taskbar.gtk_stand_alone=taskbar_create_standalone_frame(tb_panel);
      gtk_widget_hide(tb_hrule);
    }
    gtk_widget_set_usize(g_xfce_taskbar.gtk_stand_alone,gdk_screen_width(),g_xfce_taskbar.base_height+8);
    x=g_xfce_taskbar.gtk_standalone_x>=0 ? g_xfce_taskbar.gtk_standalone_x : 0;
    y=g_xfce_taskbar.gtk_standalone_y>=0 ? g_xfce_taskbar.gtk_standalone_y : gdk_screen_height()-(g_xfce_taskbar.base_height+8);
    gtk_widget_set_uposition(g_xfce_taskbar.gtk_stand_alone,x,y);

    if(g_xfce_taskbar.flags&TASKBAR_FLAG_HIDE_XFCE_PANEL) {
      gtk_widget_hide(g_xfce_taskbar.gtk_xfce_toplevel);
    }

  } else {
    if (g_xfce_taskbar.gtk_stand_alone) {
      gdk_window_get_position(g_xfce_taskbar.gtk_stand_alone->window,
                              &g_xfce_taskbar.gtk_standalone_x,&g_xfce_taskbar.gtk_standalone_y);
      gtk_object_ref(GTK_OBJECT(tb_panel));
      if (tb_panel->parent)
        gtk_container_remove(GTK_CONTAINER(tb_panel->parent),tb_panel);
      tb_parent=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_parent");
      gtk_container_add(GTK_CONTAINER(tb_parent),tb_panel);
        gtk_widget_show(tb_hrule);
        gtk_widget_show(tb_panel);
      gtk_object_unref(GTK_OBJECT(tb_panel));
      gtk_widget_destroy(g_xfce_taskbar.gtk_stand_alone);
      g_xfce_taskbar.gtk_stand_alone=NULL;
    }
    gtk_widget_set_usize(tb_hbox1,g_xfce_taskbar.base_width,g_xfce_taskbar.base_height);
    gtk_widget_set_usize(tb_hrule,g_xfce_taskbar.base_width,2); 
    
    gtk_widget_show(g_xfce_taskbar.gtk_xfce_toplevel);
  }
}
void taskbar_on_mcheck_standalone_toggled(GtkWidget *w, gpointer user_data)
{
  GtkWidget *tb_panel;

  tb_panel=(GtkWidget*)user_data;
  if (GTK_CHECK_MENU_ITEM(w)->active) {
    g_xfce_taskbar.flags|=TASKBAR_FLAG_STANDALONE;
  } else {
    g_xfce_taskbar.flags&=~TASKBAR_FLAG_STANDALONE;
  }
  taskbar_set_standalone_state();
}

void taskbar_on_mcheck_sysload_toggled(GtkWidget *w, gpointer user_data)
{
  GtkWidget *tb_panel,*cb;

  tb_panel=(GtkWidget*)user_data;
  if (!(GTK_CHECK_MENU_ITEM(w)->active)) {
    if (g_xfce_taskbar.gtk_timeout_handler_id>=0) {
      gtk_timeout_remove(g_xfce_taskbar.gtk_timeout_handler_id);
      g_xfce_taskbar.gtk_timeout_handler_id=-1;
      cb=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_button_close");
      if (cb) {
        gtk_widget_restore_default_style(cb);
      }
      g_xfce_taskbar.flags&=~TASKBAR_FLAG_PROC_LOAD;
    }
  } else if (g_xfce_taskbar.gtk_timeout_handler_id<0) {
    g_xfce_taskbar.gtk_timeout_handler_id=(gint)gtk_timeout_add(900,taskbar_set_proc_load,NULL);
    g_xfce_taskbar.flags|=TASKBAR_FLAG_PROC_LOAD;
  }
}


void taskbar_on_mcheck_single_desk_toggled(GtkWidget *w, gpointer user_data)
{
  GtkWidget *tb_panel;

  tb_panel=(GtkWidget*)user_data;
  if (GTK_CHECK_MENU_ITEM(w)->active) {
    g_xfce_taskbar.flags|=TASKBAR_FLAG_SINGLE_DESK;
  } else {
    g_xfce_taskbar.flags&=~TASKBAR_FLAG_SINGLE_DESK;
  }
  taskbar_synch_task_widget_with_desk();
}


typedef enum
{
  XFWM_CMD_TYPE_NORMAL=1,
  XFWM_CMD_TYPE_SEP,
  XFWM_CMD_TYPE_TASKJAR

} xfwm_cmd_type;

#define XFWM_CMD_FLAG_NONE  0x0000
#define XFWM_CMD_FLAG_DESK  0x0001

typedef struct _xfwm_command {
  char *name;
  char *command;
  int type;
  int flags;
} xfwm_cmd;


#define MAX_XFWM_CMDS 256


gint taskbar_on_taskjar_task_menu_clicked(GtkWidget *widget, GdkEventButton *event,gpointer user_data)
{
long id;
taskbar_window *tbw;
#ifdef LOG_FILE
    fprintf(_f," taskjar mouse pressed button: %d\n",event->button);
#endif  
  id=(long)user_data;
  if (event->type==GDK_BUTTON_PRESS) {
    if (event->button>1) {
       /* put back into taksbar */
      tbw=taskbar_find_window(id);
      if(!tbw)
        return FALSE;
      taskbar_remove_from_taskjar(tbw);
      taskbar_synch_single_task_widget_with_desk(-1,tbw);
      taskbar_synch_labels();
      gtk_menu_popdown(GTK_MENU(g_xfce_taskbar.gtk_taskjar_menu));

    } else {
      taskbar_set_active(id);
    }
  }
  return TRUE;
}



void 
taskbar_on_popup_desk_menu_select(GtkWidget *w, gpointer user_data)
{
  char exec[1024];

  if(!g_xfce_taskbar.cmd_to_execute)
    return;
  g_snprintf(exec,sizeof(exec)-1,g_xfce_taskbar.cmd_to_execute,(int)user_data);
#ifdef LOG_FILE
    fprintf(_f," execute pop-up with desk |%s|\n",exec);
#endif  
  sendinfo (fd_internal_pipe,exec,g_xfce_taskbar.curr_window);
  g_xfce_taskbar.cmd_to_execute=NULL;
}

void 
taskbar_remove_from_taskjar(taskbar_window *tbw)
{
  GtkWidget *widget;
  char sid[64];

  if(!tbw->my_flags&TASKBAR_TASK_FLAG_TASKJAR)
    return;
  g_snprintf(sid,sizeof(sid)-1,"task%ld",tbw->window);
  widget=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_taskjar_menu),sid);
  if(!widget)
    return;
  tbw->my_flags&=~TASKBAR_TASK_FLAG_TASKJAR;
  gtk_container_remove(GTK_CONTAINER(g_xfce_taskbar.gtk_taskjar_menu),widget);
}

void
taskbar_put_in_taskjar(taskbar_window *tbw)
{
  GtkWidget *menu_item;
  char sid[64];

  if(tbw->my_flags&TASKBAR_TASK_FLAG_TASKJAR)
    return;
  g_snprintf(sid,sizeof(sid)-1,"task%ld",tbw->window);
  tbw->my_flags|=TASKBAR_TASK_FLAG_TASKJAR;
  g_xfce_taskbar.curr_window=-1;
  menu_item=gtk_menu_item_new_with_label(tbw->name);
  gtk_menu_append(GTK_MENU(g_xfce_taskbar.gtk_taskjar_menu),menu_item);
  gtk_object_set_data (GTK_OBJECT (g_xfce_taskbar.gtk_taskjar_menu), strdup(sid), menu_item);

  gtk_widget_show(menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "button_press_event",
                      GTK_SIGNAL_FUNC (taskbar_on_taskjar_task_menu_clicked),
                      (gpointer)tbw->window);
}

void 
taskbar_on_popup_menu_select(GtkWidget *w, gpointer user_data)
{
  xfwm_cmd *cmd;
  taskbar_window *tbw;

    cmd=(xfwm_cmd*)user_data;
    g_xfce_taskbar.cmd_to_execute=NULL;
    tbw=taskbar_find_window(g_xfce_taskbar.curr_window);
    if(!tbw)
      return;
    if(cmd->type==XFWM_CMD_TYPE_TASKJAR) {
      taskbar_put_in_taskjar(tbw);
      taskbar_synch_single_task_widget_with_desk(-1,tbw);
      taskbar_synch_labels();
    }
    if(cmd->flags&XFWM_CMD_FLAG_DESK) {
       /* delay execution to when desk number is known*/
      g_xfce_taskbar.cmd_to_execute=cmd->command;
      return;
    }
#ifdef LOG_FILE
    fprintf(_f," popup_menu_exec: |%s|\n",(char*)cmd->command);
#endif  
    sendinfo (fd_internal_pipe,(char*)cmd->command, g_xfce_taskbar.curr_window);
}




GtkWidget*
taskbar_read_taskbarrc()
{
  GtkWidget *popup_menu;
  GtkWidget *menu_item;
  gchar *fname;
  FILE *f;
  char buff[1024],*s,*ps;
  char *token;
  int i,cmd_no,line_no;
  static xfwm_cmd cmds[MAX_XFWM_CMDS]; 
  char *patterns[MAX_XFWM_CMDS];
  int patt_no;
  /* parse taskbarrc file */
  fname=getlocalizedconffilename(TASKBAR_RC_FILE,FALSE);
  if (!fname||((f=fopen(fname,"r"))==NULL)) {
    if(fname)
      g_free(fname);
    return NULL;
  }
  cmd_no=0; line_no=0; 
  patt_no=0;
  while((cmd_no<MAX_XFWM_CMDS)&&((s=fgets(buff,sizeof(buff)-1,f))!=NULL)) {
    line_no++;
    while(isspace(s[strlen(s)-1]))
      s[strlen(s)-1]='\000';
    if((strlen(s)<2)||(s[0]=='#'))
      continue;
    s=GetNextToken(s,&token);
    if(strlen(token)==0) {
      fprintf(stderr," error parsing taskbarrc at line %d. Not a valid type name: %s\n",line_no,token);
      free(token);
      return NULL;
    }
    if(strcasecmp(token,"Menu")==0) {
      s=GetNextToken(s,&token);
      if(strlen(token)==0) {
        fprintf(stderr," error parsing taskbarrc at line %d. Not a valid cmd name: %s\n",line_no,token);
        free(token);
        return NULL;
      }
      if(strcasecmp(token,"SEP")==0) {
        cmds[cmd_no].name=NULL;
        cmds[cmd_no].command=NULL;
        cmds[cmd_no].type=XFWM_CMD_TYPE_SEP;
        cmd_no++;
        continue;
      }
       /* add name and remove double quotes */
      cmds[cmd_no].name=token;
  
       /* add xfwm command */
      s=GetNextToken(s,&token);
      if(strlen(token)==0) {
        fprintf(stderr," error parsing taskbarrc at line %d. Not a valid cmd cmd: %s\n",line_no,token);
        free(token);
        return NULL;
      }
      if(strcasecmp(token,"TASKJAR")==0) {
        cmds[cmd_no].command=NULL;
        cmds[cmd_no].type=XFWM_CMD_TYPE_TASKJAR;
        cmd_no++;
        continue;
      }
      
      cmds[cmd_no].flags=XFWM_CMD_FLAG_NONE;
      if((ps=strstr(token,"%%"))!=NULL) {
        *(ps+1)='d'; /* trick for using sprinf */
        cmds[cmd_no].flags=XFWM_CMD_FLAG_DESK;
      }
      
      cmds[cmd_no].command=token;
      cmds[cmd_no].type=XFWM_CMD_TYPE_NORMAL;
      cmd_no++;
    } else  if(strcasecmp(token,"Taskjar")==0) {
      s=GetNextToken(s,&token);
      if(strlen(token)==0) {
        fprintf(stderr," error parsing taskbarrc at line %d. Not a valid 'Taskjar' pattern\n",line_no);
        free(token);
        return NULL;
      }
      patterns[patt_no++]=token;
    } else  if(strcasecmp(token,"Option")==0) {
      s=GetNextToken(s,&token);
      if(strlen(token)==0) {
        fprintf(stderr," error parsing taskbarrc at line %d. Not a valid 'Option' parameter\n",line_no);
        free(token);
        return NULL;
      }
      if(strcasecmp(token,"NO_WindowListSkip")==0) {
        g_xfce_taskbar.flags&=~TASKBAR_FLAG_WINLISTSKIP;
      } else if(strcasecmp(token,"WindowListSkip")==0) {
        g_xfce_taskbar.flags|=TASKBAR_FLAG_WINLISTSKIP;
      } else if(strcasecmp(token,"HidePanel")==0) {
        g_xfce_taskbar.flags|=TASKBAR_FLAG_HIDE_XFCE_PANEL;
      }

    }
  }
  fclose(f);
  g_xfce_taskbar.taskjar_pattern=(char**)malloc((patt_no+1)*sizeof(char*));
  for(i=0;i<patt_no;i++) {
    g_xfce_taskbar.taskjar_pattern[i]=patterns[i];
  }
  g_xfce_taskbar.taskjar_pattern[i]=NULL;
  
  if(cmd_no==0)
    return NULL;


  /* pop-up menu */
  popup_menu=gtk_menu_new();
  for(i=0;i<cmd_no;i++) {
    if(cmds[i].type==XFWM_CMD_TYPE_SEP) {
      menu_item=gtk_menu_item_new();
    } else {
      menu_item=gtk_menu_item_new_with_label(cmds[i].name);
      gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
                         GTK_SIGNAL_FUNC(taskbar_on_popup_menu_select),(gpointer)(&cmds[i]));
      if(cmds[i].flags&XFWM_CMD_FLAG_DESK) {
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),g_xfce_taskbar.gtk_desk_menu);
      }
    }
    gtk_menu_append(GTK_MENU(popup_menu),menu_item);
    gtk_widget_show(menu_item);
  }
  return popup_menu;
}

void
taskbar_applay_xfce_config(config *config)
{
GtkWidget *menu_item;
GList *l;
char buff[1024];
int i;

  if(g_xfce_taskbar.gtk_desk_menu) {
    l=gtk_container_children(GTK_CONTAINER(g_xfce_taskbar.gtk_desk_menu));
    while(l) {
      gtk_widget_destroy(GTK_WIDGET(l->data));
      l=l->next;
    }
    g_list_free(l);
    gtk_widget_destroy(GTK_WIDGET(g_xfce_taskbar.gtk_desk_menu));
  }  
  g_xfce_taskbar.gtk_desk_menu=gtk_menu_new();
  for(i=0;i<config->visible_screen;i++) {
    g_snprintf(buff,sizeof(buff)-1,"%d: %s",i+1,get_gxfce_screen_label(i));
    menu_item=gtk_menu_item_new_with_label(buff);
    gtk_widget_show(menu_item);
    gtk_menu_append(GTK_MENU(g_xfce_taskbar.gtk_desk_menu),menu_item);
    gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
                       GTK_SIGNAL_FUNC(taskbar_on_popup_desk_menu_select),(gpointer)i);
  }
  if(!g_xfce_taskbar.gtk_task_popup_menu);
    g_xfce_taskbar.gtk_task_popup_menu=taskbar_read_taskbarrc();
/*
  if(g_xfce_taskbar.flags&TASKBAR_FLAG_OPENED) {
    GTK_WIDGET(_open)->allocation.width=-1;
    GTK_WIDGET(_open)->allocation.height=-1;
    taskbar_set_standalone_state();
printf("APPLAY\n");
printf(" close\n");
    gtk_button_clicked(_close);
printf(" open\n");
    gtk_button_clicked(_open);
  }
*/    

}


gboolean taskbar_on_model_widget_configure (GtkWidget *widget,GtkRequisition *event,
                                            gpointer user_data)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hrule;
  GtkWidget *tb_hbox1;

  tb_panel=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_realtaskbar),"tb_panel");

  tb_hbox1=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hbox1");
  tb_hrule=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(tb_panel),"tb_hrule");
  
  if(event->width==g_xfce_taskbar.base_width)
    return FALSE;
  g_xfce_taskbar.base_width=event->width;
  
  taskbar_set_standalone_state();

  return FALSE;
}

/* 
 * create taskbar frame (to be added to xfce main window) 
 */
GtkWidget *taskbar_create_gxfce_with_taskbar (GtkWidget *parent,GtkWidget *size_model,GtkWidget *top_level_window)
{
  GtkWidget *tb_panel;
  GtkWidget *tb_hbox1;
  GtkWidget *tb_button_close;
  GtkWidget *tb_button_taskjar;
  GtkWidget *tb_real;
  GtkWidget *tb_button_open;
  GtkWidget *tb_hrule;
  GtkWidget *popup_menu;
  GtkWidget *taskjar_menu;
  GtkWidget *menu_item;
  GtkWidget *label;
  GSList    *radio_list;
/*  
  GtkWidget *taskjar_pixmap;
  GdkBitmap *bitmap;
*/
  FILE *stats_f;
  int i;

#ifdef LOG_FILE
  if(!_f) {
    _f=fopen("/tmp/xfce_ms.log","a");
    fprintf(_f,"--------------------------------------\n");
  }
#endif
  tb_panel=gtk_vbox_new(FALSE,0);
  tb_button_open=gtk_button_new();
  gtk_object_set_data(GTK_OBJECT(tb_panel),"tb_parent",(gpointer)parent);
  gtk_object_set_data(GTK_OBJECT(tb_panel),"tb_size_model",(gpointer)size_model);
  gtk_signal_connect(GTK_OBJECT(size_model),"size_request",GTK_SIGNAL_FUNC(taskbar_on_model_widget_configure),NULL);
  gtk_widget_set_usize(tb_button_open,-1,4); 
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_button_open", tb_button_open);
  gtk_object_set_data (GTK_OBJECT (tb_button_open), "tb_panel", tb_panel);

  gtk_container_add(GTK_CONTAINER(tb_panel),tb_button_open);
  gtk_signal_connect(GTK_OBJECT(tb_button_open),"clicked",GTK_SIGNAL_FUNC(taskbar_on_button_open_clicked),NULL);
  gtk_widget_show(tb_panel);
  gtk_widget_show(tb_button_open);

  tb_hrule=gtk_hseparator_new();
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_hrule", tb_hrule);
  gtk_container_add(GTK_CONTAINER(tb_panel),tb_hrule);

  tb_hbox1=gtk_hbox_new(FALSE,1);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_hbox1", tb_hbox1);
  tb_button_close=gtk_button_new();
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_button_close", tb_button_close);
  gtk_object_set_data (GTK_OBJECT (tb_button_close), "tb_panel", tb_panel);

  /* pop-up menu */
  popup_menu=gtk_menu_new();
  radio_list=NULL;

  menu_item=gtk_check_menu_item_new_with_label(_("Stand-alone"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_mcheck_standalone", menu_item);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_mcheck_standalone_toggled),(gpointer)tb_panel);
  menu_item=gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);


  menu_item=gtk_menu_item_new_with_label(_("Sort order:"));
gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);



  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by desk"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_sort_desk", menu_item);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_DESK);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by name"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_sort_name", menu_item);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_NAME);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  by window ID"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_sort_win_id", menu_item);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_BY_WINID);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);
  menu_item=gtk_radio_menu_item_new_with_label(radio_list,_("  unsorted"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_sort_unsorted", menu_item);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_radio_sort_toggled),(gpointer)TASKBAR_SORT_UNSORTED);
  radio_list=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_check_menu_item_new_with_label(_("System load"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_mcheck_sysload", menu_item);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);

   /* check if /proc/stat present */
  stats_f=fopen("/proc/stat","r");
  if (!stats_f)
    gtk_widget_set_sensitive(menu_item,FALSE);
  else 
    fclose(stats_f);


  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_mcheck_sysload_toggled),(gpointer)tb_panel);
  
  menu_item=gtk_check_menu_item_new_with_label(_("Only current desk"));
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_mcheck_single_desk", menu_item);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),FALSE);
  gtk_menu_append(GTK_MENU(popup_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"toggled",
                     GTK_SIGNAL_FUNC(taskbar_on_mcheck_single_desk_toggled),(gpointer)tb_panel);

  /* menu end */

  gtk_signal_connect_object(GTK_OBJECT(tb_button_close), "button_press_event",
                            GTK_SIGNAL_FUNC (taskbar_on_button_close_right_clicked), GTK_OBJECT(popup_menu));


  gtk_signal_connect(GTK_OBJECT(tb_button_close),"clicked",GTK_SIGNAL_FUNC(taskbar_on_button_close_clicked),NULL);

  tb_real=gtk_hbox_new(TRUE,3);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_real", tb_real);
  gtk_object_set_data (GTK_OBJECT (tb_real), "tb_button_open", tb_button_open);

  /* pack close button */
  gtk_box_pack_start(GTK_BOX(tb_hbox1),tb_button_close,FALSE,TRUE,0);
  
  tb_button_taskjar=gtk_button_new();
  gtk_tooltips_set_tip(gtk_tooltips_new(),tb_button_taskjar,_("TaskJar"),NULL);
  gtk_object_set_data (GTK_OBJECT (tb_panel), "tb_button_taskjar", tb_button_taskjar);
  gtk_object_set_data (GTK_OBJECT (tb_button_taskjar), "tb_panel", tb_panel);
  gtk_object_set_data (GTK_OBJECT (tb_real), "tb_panel", tb_panel);
  
  gtk_signal_connect_object(GTK_OBJECT(tb_button_taskjar), "button_press_event",
                            GTK_SIGNAL_FUNC (taskbar_on_button_taskjar_clicked), NULL);
  
   /* pack task buttons area */
  gtk_box_pack_start(GTK_BOX(tb_hbox1),tb_real,TRUE,TRUE,0);
   /* pack taskjar button */
  gtk_box_pack_start(GTK_BOX(tb_hbox1),tb_button_taskjar,FALSE,TRUE,0);

  gtk_container_add(GTK_CONTAINER(tb_panel),tb_hbox1);
  /* taskjar menu */
  taskjar_menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label(_("Jared tasks"));
gtk_widget_set_sensitive(menu_item,FALSE);
  label=GTK_WIDGET(gtk_container_children(GTK_CONTAINER(menu_item))->data);
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_menu_append(GTK_MENU(taskjar_menu),menu_item);
  gtk_widget_show(menu_item);
  menu_item=gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(taskjar_menu),menu_item);
  gtk_widget_show(menu_item);
  
  
  /* init g_xfce_taskbar */
  g_xfce_taskbar.gtk_xfce_toplevel=top_level_window;
  g_xfce_taskbar.gtk_realtaskbar=tb_real;

  for (i=0;i<TASKBAR_MIN_CELL_NO;i++) {
    g_xfce_taskbar.gtk_labels[i]=NULL;
  }
  g_xfce_taskbar.gtk_task_no=0;
  g_xfce_taskbar.gtk_toggled_button=NULL;
  g_xfce_taskbar.curr_desk=0; 
  g_xfce_taskbar.model_widget=tb_button_open;
  g_xfce_taskbar.gtk_stand_alone=NULL;
  g_xfce_taskbar.gtk_pressed=FALSE;
  g_xfce_taskbar.gtk_timeout_handler_id=-1;
  g_xfce_taskbar.gtk_proc_load_indicator=NULL;
  g_xfce_taskbar.gtk_taskjar_menu=taskjar_menu;
  g_xfce_taskbar.gtk_desk_menu=NULL;
  g_xfce_taskbar.gtk_task_popup_menu=NULL;
  g_xfce_taskbar.cmd_to_execute=NULL;

  if(!(g_xfce_taskbar.flags&TASKBAR_FLAG_CONFIG_READ)) {
    g_xfce_taskbar.flags=TASKBAR_FLAG_NONE|TASKBAR_FLAG_WINLISTSKIP;
    g_xfce_taskbar.sort_order=TASKBAR_SORT_BY_DESK;
    g_xfce_taskbar.gtk_standalone_x=-1;
    g_xfce_taskbar.gtk_standalone_y=-1;
  }


  taskbar_set_labels();
  if(g_xfce_taskbar.flags&TASKBAR_FLAG_OPENED) {
    gtk_button_clicked(GTK_BUTTON(tb_button_open));

  }

  return tb_panel;
}


/*
 * TODO: invoke applay_changes directly form proper  xfce function
 */
void
taskbar_save_config(FILE *f)
{

  if(g_xfce_taskbar.flags&TASKBAR_FLAG_OPENED)
    fprintf(f,"\tOpened\n");
  if(g_xfce_taskbar.flags&TASKBAR_FLAG_PROC_LOAD)
    fprintf(f,"\tProcLoad\n");
  if(g_xfce_taskbar.flags&TASKBAR_FLAG_SINGLE_DESK)
    fprintf(f,"\tSingleDesk\n");
  if(g_xfce_taskbar.flags&TASKBAR_FLAG_STANDALONE)
    fprintf(f,"\tStandAlone\n");
  if(g_xfce_taskbar.gtk_stand_alone) {
    gdk_window_get_position(g_xfce_taskbar.gtk_stand_alone->window,
                            &g_xfce_taskbar.gtk_standalone_x,&g_xfce_taskbar.gtk_standalone_y);
  }
  fprintf(f,"\tStandAlonePos %d %d\n",g_xfce_taskbar.gtk_standalone_x,g_xfce_taskbar.gtk_standalone_y);
  
  switch (g_xfce_taskbar.sort_order) {
  case TASKBAR_SORT_BY_DESK:
    fprintf(f,"\tSortByDesk\n");
    break;
  case TASKBAR_SORT_BY_NAME:
    fprintf(f,"\tSortByName\n");
    break;
  case TASKBAR_SORT_BY_WINID:
    fprintf(f,"\tSortByWinId\n");
    break;
  case TASKBAR_SORT_UNSORTED:
    fprintf(f,"\tSortByUnsorted\n");
    break;
  default:
  } 
}


char*
taskbar_read_config(FILE *f, char *lineread,int max)
{
char *p;
GtkWidget *tb_button_open;

  g_xfce_taskbar.flags=TASKBAR_FLAG_NONE|TASKBAR_FLAG_WINLISTSKIP;
  g_xfce_taskbar.sort_order=TASKBAR_SORT_UNSORTED;

  while(fgets(lineread,max-1,f)!=NULL) {
    p=lineread;
    while(isspace(*p))
      p++;
    if(*p=='[')
      return p;
    if(strncasecmp(p,"Opened",strlen("Opened"))==0)
      g_xfce_taskbar.flags|=TASKBAR_FLAG_OPENED;
    else if (strncasecmp(p,"ProcLoad",strlen("ProcLoad"))==0)
      g_xfce_taskbar.flags|=TASKBAR_FLAG_PROC_LOAD;
    else if (strncasecmp(p,"SingleDesk",strlen("SingleDesk"))==0)
      g_xfce_taskbar.flags|=TASKBAR_FLAG_SINGLE_DESK;
    else if (strncasecmp(p,"StandAlonePos",strlen("StandAlonePos"))==0) {
      p+=strlen("StandAlonePos");
      sscanf(p,"%d %d",&g_xfce_taskbar.gtk_standalone_x,&g_xfce_taskbar.gtk_standalone_y);
    }
    else if (strncasecmp(p,"StandAlone",strlen("StandAlone"))==0)
      g_xfce_taskbar.flags|=TASKBAR_FLAG_STANDALONE;
    else if (strncasecmp(p,"SortByDesk",strlen("SortByDesk"))==0)
      g_xfce_taskbar.sort_order=TASKBAR_SORT_BY_DESK;
    else if (strncasecmp(p,"SortByName",strlen("SortByName"))==0)
      g_xfce_taskbar.sort_order=TASKBAR_SORT_BY_NAME;
    else if (strncasecmp(p,"SortByWinId",strlen("SortByWinId"))==0)
      g_xfce_taskbar.sort_order=TASKBAR_SORT_BY_WINID;
    else if (strncasecmp(p,"SortByUnsorted",strlen("SortByUnsorted"))==0)
      g_xfce_taskbar.sort_order=TASKBAR_SORT_UNSORTED;
    else {
      /* TODO: sig ERROR */
    }
  }
  g_xfce_taskbar.flags|=TASKBAR_FLAG_CONFIG_READ;
  if((g_xfce_taskbar.flags&TASKBAR_FLAG_OPENED)&&(g_xfce_taskbar.gtk_realtaskbar)) {
    tb_button_open=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(g_xfce_taskbar.gtk_realtaskbar),"tb_button_open");
    if(tb_button_open)
      gtk_button_clicked(GTK_BUTTON(tb_button_open));
  }
  return NULL;
}



#endif

