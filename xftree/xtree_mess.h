/*
 * Edscott Wilson Garcia Copyright 2001-2002 GNU-GPL
 * */
/*BYTE 1 */
#define SAVE_GEOMETRY 		0x01
#define DOUBLE_CLICK_GOTO 	0x02
#define SHORT_TITLES	 	0x04
#define DRAG_DOES_COPY		0x08
#define CUSTOM_COLORS		0x10
#define LARGE_TOOLBAR		0x20
#define HIDE_TOOLBAR		0x40
#define CUSTOM_FONT		0x80
/* BYTE 2 */
#define HIDE_SIZE		0x100
#define HIDE_DATE		0x200
#define HIDE_MENU		0x400
#define HIDE_TITLES		0x800
#define SUBSORT_BY_FILETYPE	0x1000
#define FILTER_OPTION		0x2000
#define ABREVIATE_PATHS		0x4000
#define SORT_SIZE		0x8000
/* BYTE 3 */
#define SORT_DATE		0x10000
#define SORT_NAME		0x20000
#define STATUS_FOLLOWS_EXPAND	0x40000
#define SHOW_STATUS		0x80000
#define SMALL_DIALOGS		0x100000
#define FONT_STATE		0x200000
#define HIDE_MODE		0x400000
#define HIDE_UID		0x800000
/* BYTE 4 */
#define HIDE_GID		0x1000000
#define HIDE_DD			0x2000000
#define USE_RSYNC		0x4000000



#define UNSORT_MASK		(0x38000 ^ 0xffffffff)


#define FILTER_DIRS		0x01
#define FILTER_FILES		0x02

#define XFTREE_CONFIG_FILE "xftreerc"

#ifdef XTREE_MESS_MAIN  
unsigned int preferences,stateTB[2];
int geometryX,geometryY;
char *custom_home_dir=NULL;
GdkColor ctree_color;
#else /* XTREE_MESS_MAIN */
extern unsigned int preferences,stateTB[2];
extern int geometryX,geometryY;
extern char *custom_home_dir;
extern GdkColor ctree_color;
#endif /* XTREE_MESS_MAIN */

#ifndef __XTREE_MESS_H__
#define __XTREE_MESS_H__
#include <sys/types.h>
#include <dirent.h>
#include <glob.h>
#ifndef GLOB_PERIOD
#define GLOB_PERIOD 0
#endif


typedef struct xf_dirent {
	DIR *dir;
	char *globstring;
	char *globstar;
	glob_t dirlist;
	int glob_count;
}xf_dirent;


gboolean sane (char *bin);
void read_defaults(void);
void save_defaults(GtkWidget *parent);
char * override_txt(char *new_file,char *old_file);
GtkWidget *
 shortcut_menu (GtkWidget * parent, char *txt, gpointer func, gpointer data);
void 
 cb_toggle_preferences (GtkWidget * widget, gpointer data);
void cb_custom_SCK(GtkWidget * item, GtkWidget * ctree);
void cb_default_SCK(GtkWidget * item, GtkWidget * ctree );
void show_cat (char *message);
void cb_select_colors (GtkWidget * widget, GtkWidget * ctree);
void cb_select_font (GtkWidget * widget, GtkWidget * ctree);
void cb_hide_date (GtkWidget * widget, GtkWidget * ctree);
void cb_hide_size (GtkWidget * widget, GtkWidget * ctree);
void set_colors(GtkWidget * ctree);
int set_fontT(GtkWidget * ctree);
void cb_dnd_help(GtkWidget * item, GtkWidget * ctree);
void cb_show_status (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_dd (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_mode (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_uid (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_gid (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_menu (GtkWidget * widget, GtkWidget *ctree);
void cb_hide_titles (GtkWidget * widget, GtkWidget *ctree);
void cb_subsort(GtkWidget * widget, GtkWidget *ctree);
void cb_short_titles(GtkWidget * widget, GtkWidget *ctree);
void cb_filter(GtkWidget * widget, GtkWidget *ctree);
void cb_filter_dirs(GtkWidget * widget, GtkWidget *ctree);
void cb_filter_files(GtkWidget * widget, GtkWidget *ctree);
void cb_abreviate(GtkWidget * widget, GtkWidget *ctree);
xf_dirent *xf_opendir(char *path,GtkWidget *ctree);
xf_dirent *xf_closedir(xf_dirent *diren);
char *xf_readdir(xf_dirent *diren,GtkWidget *ctree);
char *abreviate(char *path);
char *abreviateP(char *path);
void clear_cat (GtkWidget * widget, gpointer data);
void cb_custom_home(GtkWidget *widget,gpointer data);
void cb_status_follows_expand(GtkWidget * widget, GtkWidget *ctree);
void cb_registered(GtkWidget * item, GtkWidget * ctree);
void cb_help_iv(GtkWidget * item, GtkWidget * ctree);
void cb_use_rsync (GtkWidget * widget, gpointer ctree);
void cb_rsync(GtkWidget * item, GtkWidget * ctree);

void cb_drag_copy (GtkWidget * widget, gpointer ctree);
void cb_doubleC_goto (GtkWidget * widget, gpointer ctree);
void cb_save_geo (GtkWidget * widget, gpointer ctree);
void cb_sizeKB (GtkWidget * widget, gpointer ctree);
void cleanup_tmpfiles(void);
GtkCTreeNode *find_root(GtkCTree * ctree);
#endif
