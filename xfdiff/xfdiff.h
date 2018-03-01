 /* xfdiff.h */

/*  xfdiff (a gtk frontend for diff)
 *  Copyright (C)  Edscott Wilson Garcia under GNU GPL
 *
 *  xfdiff uses several modules from XFCE which is 
 *  Copyright (c) by Olivier Fourdan under GNU GPL
 *  http://www.xfce.org/
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

/* includes: */

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
/* for _( definition, it also includes config.h: */
#include "my_intl.h"
#include "constant.h"

#ifdef __GTK_2_0
	/*  */
#define my_show_message(a)   	xf_message (a)
#define my_yesno_dialog(a) 	xf_yesno (a)
	/*  */
#else
	/*  */
#include "xfce-common.h"
#include "fileutil.h"
#include "fileselect.h"
#include "colorselect.h"
#include "fontselection.h"
	/*  */
#endif


#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif


#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* defines: */

#define DIFF "diff"		/* diff with -D option  (GNU, solaris, etc...) */
#define PATCH "patch"		/* GNU patch, traditional patch, or a patch that  conforms  to POSIX */

/* if GNU patch is used, you can have --verbose diagnostics is GNU_PATCH is defined */
/*#define GNU_PATCH*/

#define TMP_DIR "/tmp"		/* directory for placement of temporary patch files (used only if environment variable TMPDIR TMP or TEMP are not set ;) */

#define PATCH_f TRUE		/* patch has -f option. This is mandatory for GNU patch, as it queries on reversed patches */

#define XFDIFF_CONFIG_FILE "xfdiffrc"
#define XFDIFF_VERSION "1.2.8"

#define AUTOSAVE		/* automatic configuration save, a la xfclock */
/* draw window width (should be dynamic))*/

#define DRAW_W  (gdk_string_width(style->font,drawA_width()))
#define BLOK_SIZE 16384		/* block size for piping files and other similar stuff */
#define PATCH_M "patching file"	/* this is a message from patch used for parsing output. Translation would be necesary if and only if patch gives a different message and locale is set. */

/* defines used for exiting xfdiff: */
#define E_CLEANUP  0
#define E_MALLOC   1
#define E_FILE     2
#define E_SEGV     3


#define REVERSE_TOGGLE patchM[3]


/* typedefs: */
enum
{
  TARGET_URI_LIST,
  TARGET_PLAIN,
  TARGET_STRING,
  TARGET_XTREE_WIDGET,
  TARGET_XTREE_WINDOW,
  TARGET_ROOTWIN,
  TARGETS
};

typedef struct polygon
{
  int topR;
  int botR;
  int topL;
  int botL;
  int topLR;
  int botLR;
  int topLL;
  int botLL;
  struct polygon *next;
  struct polygon *previous;
}
polygon;


typedef struct patched_file
{
  char *file;
  char *newfile;
  int offset;
  int length;
  int diffC;
  struct patched_file *next;
  struct patched_file *previous;
}
patched_file;



/* function prototypes: */

GtkWidget *create_diff_window (void);
gint configure_event (GtkWidget * widget, GdkEventConfigure * event);
void cb_next_diff (GtkWidget * widget, gpointer data);
void cb_save_defaults (GtkWidget * widget, gpointer data);
void cb_adjust (GtkAdjustment * adj, GtkWidget * widget);
void cb_toggle_reversed (GtkWidget * widget, gpointer data);


int do_diff (void);
int do_patch (void);
int process_patch (patched_file * thisF);

#ifdef __GTK_2_0
/* enhancements due to gtk+2.0*/
void utf8_insert (GtkTextBuffer * buffer, char *line, int linecount);
void utf8_file_insert (GtkTextBuffer * buffer, char *line, int linecount);
void clear_text_buffer (GtkTextBuffer * buffer);
gboolean xf_message (char *message);
gboolean xf_confirm (char *message);
gboolean xf_yesno (char *message);
gdouble *xf_colorselect (char *title, gdouble * colors);
char *xf_fontselect (char *title, char *aux);
char *xf_fileselect (char *title);
#else
#endif


/* global variables */
#ifdef __XFDIFF__
#define EXTERN
#else
#define EXTERN extern
#endif
EXTERN GdkCursor *cursor;
EXTERN int sizeR, sizeL, sizeH;
EXTERN polygon *head, *current;
EXTERN patched_file *headF, *currentF;
EXTERN int strip;
EXTERN GtkStyle *style;
EXTERN GtkStyle *xfce_style;
EXTERN GdkFont *the_font;
EXTERN GtkWidget *diff, *OKbutton;

#ifdef __GTK_2_0
/* GTK_TEXT is now obsolete */
EXTERN GtkTextBuffer *text_right, *text_left, *titleR, *titleL, *titleP, *what_dir;
#else /* GTK_1_2 */
EXTERN GtkWidget *text_right, *text_left, *titleR, *titleL, *titleP, *what_dir;
#endif

EXTERN GtkWidget *drawA, *viewR, *titleD;
EXTERN GtkWidget *diffbox, *patchbox, *patch_label_box;
EXTERN char *done_str;
EXTERN GdkPixmap *drawP;
EXTERN GtkObject *adj, *adjR;
EXTERN int lineW, lineH;
EXTERN GdkGC *drawGC, *fileGC;
EXTERN char *fileO, *fileRR, *fileI, *patchO;
EXTERN char *fileR, *fileL, *fileP, *fileD;
EXTERN char *fileRD, *fileLD;
EXTERN GdkColor colorfg, colorbg, filefg, filebg;
EXTERN char *fuente;
EXTERN int silent;
EXTERN int patching;

EXTERN char *strip_set_to, *no_patch, *no_patch_dir;

EXTERN char *no_left_path, *no_right_path;
EXTERN int reversed, filledP, show_lineN;
EXTERN int rightC, leftC, good_dir, addedR, addedL;

EXTERN int applying_patch, synchronize, autostrip;
#ifdef GNU_PATCH
EXTERN int verbose;
#endif
