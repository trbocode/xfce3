
/* poner select color y select font dentro de este archivo. */


/*   xfdiff_misc.c */

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



/******************** CB section ************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdarg.h>

#include "xfdiff.h"
#include "xfdiff_diag.h"
#include "xfdiff_misc.h"
#include "glob.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef __GTK_2_0
/*****************************************************/
/* private dialog functions: */

#define NOEXPAND FALSE
#define EXPAND TRUE
#define NOFILL FALSE
#define FILL TRUE

#define FILE_SELECTION 0
#define FONT_SELECTION 1
#define COLOR_SELECTION 2

static gboolean xf_result;
static void
xf_button (GtkWidget * widget, gpointer data)
{
  int caso;
  caso = (int) ((long) data);
  if (caso)
    xf_result = TRUE;
  gtk_main_quit ();
  return;
}

static GtkWidget *
xf_selection (char *title, int caso, char *aux)
{
  static GtkWidget *dialogo;
  GtkObject *ok_object = NULL, *ko_object = NULL;
  switch (caso)
  {
  case FONT_SELECTION:
    dialogo = gtk_font_selection_dialog_new (title);
    ok_object = GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (dialogo)->ok_button);
    ko_object = GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (dialogo)->cancel_button);

    gtk_font_selection_dialog_set_preview_text (GTK_FONT_SELECTION_DIALOG (dialogo), "Xfce is at http://www.xfce.org/");
    if (!gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (dialogo), aux))
    {
      xf_message (_("Could not set selected font"));
    }
    break;
  case COLOR_SELECTION:
    dialogo = gtk_color_selection_dialog_new (title);
    ok_object = GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (dialogo)->ok_button);
    ko_object = GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (dialogo)->cancel_button);
    gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialogo)->colorsel), (gdouble *) aux);
    break;
  default:			/* FILE_SELECTION */
    dialogo = gtk_file_selection_new (title);
    ok_object = GTK_OBJECT (GTK_FILE_SELECTION (dialogo)->ok_button);
    ko_object = GTK_OBJECT (GTK_FILE_SELECTION (dialogo)->cancel_button);
    break;
  }
  gtk_signal_connect (ok_object, "clicked", GTK_SIGNAL_FUNC (xf_button), (gpointer) ((long) 1));
  gtk_signal_connect (ko_object, "clicked", GTK_SIGNAL_FUNC (xf_button), (gpointer) ((long) 0));

  gtk_window_position (GTK_WINDOW (dialogo), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialogo), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialogo), 5);
  gtk_window_set_modal (GTK_WINDOW (dialogo), TRUE);
  gtk_widget_realize (dialogo);


  gtk_widget_show (dialogo);
  return dialogo;
}


static GtkWidget *
xf_dialog (char *message, int caso)
{
  static GtkWidget *dialogo;
  GtkWidget *hbox, *label;

  switch (caso)
  {
  case 1:
    dialogo = gtk_dialog_new_with_buttons (_("Confirm"), GTK_WINDOW (diff), GTK_DIALOG_MODAL, _("OK"), GTK_RESPONSE_ACCEPT, _("Abort"), GTK_RESPONSE_REJECT, NULL);
    break;
  case 2:
    dialogo = gtk_dialog_new_with_buttons (_("Question"), GTK_WINDOW (diff), GTK_DIALOG_MODAL, _("Yes"), GTK_RESPONSE_ACCEPT, _("No"), GTK_RESPONSE_REJECT, NULL);
    break;
  default:
    dialogo = gtk_dialog_new_with_buttons (_("Alert"), GTK_WINDOW (diff), GTK_DIALOG_MODAL, _("OK"), GTK_RESPONSE_ACCEPT, NULL);
    break;
  }
  gtk_window_position (GTK_WINDOW (dialogo), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (dialogo), TRUE, TRUE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (dialogo), 5);
  gtk_window_set_modal (GTK_WINDOW (dialogo), TRUE);
  gtk_widget_realize (dialogo);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialogo)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hbox), label, NOEXPAND, NOFILL, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  gtk_widget_show (dialogo);
  return dialogo;
}

static gboolean
xf_hey (char *message, int caso)
{
  GtkWidget *widget;
  gint result;
  widget = xf_dialog (message, caso);
  gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (diff));
  result = gtk_dialog_run (GTK_DIALOG (widget));
  gtk_widget_destroy (widget);
  switch (result)
  {
  case GTK_RESPONSE_ACCEPT:
    return TRUE;
    break;
  default:
    break;
  }
  return FALSE;

}

char *
xf_selector (char *title, int caso, char *aux)
{
  static char *name = NULL;
  static gdouble selcolor;
  GtkWidget *widget;

  xf_result = FALSE;
  if (name)
    free (name);
  widget = xf_selection (title, caso, aux);
  gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (diff));
  gtk_main ();
  name = NULL;
  if (xf_result)
  {
    switch (caso)
    {
    case FONT_SELECTION:
      name = (char *) malloc (strlen (gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (widget))) + 1);
      strcpy (name, gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG (widget)));
      printf ("out=%s\n", name);
      break;
    case COLOR_SELECTION:
      gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (widget)->colorsel), &selcolor);
      gtk_widget_destroy (widget);
      return (char *) (&selcolor);
      break;
    default:			/* FILE_SELECTION */
      name = (char *) malloc (strlen (gtk_file_selection_get_filename (GTK_FILE_SELECTION (widget))) + 1);
      strcpy (name, gtk_file_selection_get_filename (GTK_FILE_SELECTION (widget)));
    }
  }
  gtk_widget_destroy (widget);
  return name;
}

/* public dialog functions: */
char *
xf_fontselect (char *title, char *aux)
{
  xf_message ("font selection bugged in gtk1.3");
  return NULL;
/*  char *fuente;
  fuente=aux;
  while ((fuente[0]==' ') && (fuente[0]!=0)) fuente++;
printf("in=%s\n",fuente);
  return xf_selector(title,FONT_SELECTION,fuente);*/
}

gdouble *
xf_colorselect (char *title, gdouble * colors)
{
  return (gdouble *) (xf_selector (title, COLOR_SELECTION, (char *) colors));
}

char *
xf_fileselect (char *title)
{
  return xf_selector (title, FILE_SELECTION, NULL);
}

gboolean
xf_message (char *message)
{
  return xf_hey (message, 0);
}

gboolean
xf_confirm (char *message)
{
  return xf_hey (message, 1);
}

gboolean
xf_yesno (char *message)
{
  return xf_hey (message, 2);
}

/*****************************************************/
#endif



char *
drawA_width (void)
{
  if ((rightC < 1000) && (leftC < 1000))
    return "88800888";
  if ((rightC < 10000) && (leftC < 10000))
    return "8888008888";
  if ((rightC < 100000) && (leftC < 100000))
    return "888880088888";
  return "88888800888888";
}

/* in gnu-c, GLOB_NOMATCH is 3 */
#ifndef GLOB_NOMATCH
#define GLOB_NOMATCH 0
#endif

/*static gboolean inverted=FALSE;*/

static int
go_figure_strip (void)
{
  patched_file *thisF;
  int i, j, *hits, Mstrip = 0, Mlength = 0, ch = 1;
  char *filename;
  glob_t pglob;
chance2:
  thisF = headF;
  while (thisF)
  {
    int j;
    if (thisF->file && thisF->newfile)
    {
      j = max_strip ((ch) ? thisF->file : thisF->newfile);
      if (strlen ((ch) ? thisF->file : thisF->newfile) > Mlength)
	Mlength = strlen ((ch) ? thisF->file : thisF->newfile);
      if (j > Mstrip)
	Mstrip = j;
    }
    else
    {
      /*printf("oops.\n"); */
    }
    thisF = thisF->next;
  }
  hits = (int *) malloc ((Mstrip + 1) * sizeof (int));
  if (!hits)
    xfdiff_abort (E_MALLOC);
  filename = (char *) malloc (Mlength + strlen (fileD) + 2);
  if (!filename)
    xfdiff_abort (E_MALLOC);
  for (i = 0; i <= Mstrip; i++)
  {
    *(hits + i) = 0;
    thisF = headF;
    while (thisF)
    {
      if (thisF->file && thisF->newfile)
      {
	sprintf (filename, "%s/%s", fileD, strip_it ((ch) ? thisF->file : thisF->newfile, i));
	if (glob (filename, GLOB_ERR, NULL, &pglob) != GLOB_NOMATCH)
	  (*(hits + i))++;
	globfree (&pglob);
      }
      thisF = thisF->next;
    }
  }
  Mlength = j = 0;
  for (i = 0; i <= Mstrip; i++)
  {
    /*printf("strip=%d, hits=%d\n",i, *(hits+i)); */
    if (*(hits + i) <= Mlength)
      continue;
    Mlength = *(hits + i);
    j = i;
  }

  if (Mlength == 0)
    free (filename);
  free (hits);
  if ((ch) && (Mlength == 0))
  {				/* second chance */
    ch = 0;
    goto chance2;
  }
  return j;
}


void
xfdiff_abort (int why)
{
  switch (why)
  {
  case E_SEGV:
    my_show_message (_("A bug?\nPlease send full traceback to xfce-dev@moongroup.com"));
    break;
  case E_CLEANUP:
    break;
  case E_MALLOC:
    my_show_message (_("Low memory (malloc) error: xfdiff_aborting run"));
    break;
  case E_FILE:
    my_show_message (_("file open error: xfdiff_aborting run"));
    break;
  default:
    my_show_message (_("unspecified error: xfdiff_aborting run"));
    break;
  }
  /* clean up: */
#ifdef AUTOSAVE
  cb_save_defaults (NULL, NULL);
#endif

  if (fileRR)
    remove (fileRR);
  if (fileO)
    remove (fileO);
  if (fileI)
    remove (fileI);
  gtk_main_quit ();
  exit (1);			/* should not get here */
}

void
finish (int sig)
{
  if (sig == SIGSEGV)
    xfdiff_abort (E_SEGV);
  xfdiff_abort (E_CLEANUP);
}

void
cleanF (void)
{
  patched_file *thisF;
  if (!headF)
    return;
  while (headF != NULL)
  {
    thisF = headF;
    headF = thisF->next;
    if (thisF->file)
      free (thisF->file);
    if (thisF->newfile)
      free (thisF->newfile);
    free (thisF);
  }
  headF = currentF = NULL;
  return;
}

polygon *
pushP (int topR, int botR, int topL, int botL)
{
  int O = 3;
  if (!current)
  {
    head = (polygon *) malloc (sizeof (polygon));
    if (!head)
      xfdiff_abort (E_MALLOC);
    current = head;
    current->previous = NULL;
  }
  else
  {
    current->next = (polygon *) malloc (sizeof (polygon));
    if (!current->next)
      xfdiff_abort (E_MALLOC);
    (current->next)->previous = current;
    current = current->next;
  }
  current->next = NULL;
  current->topR = topR * lineH + O;
  current->botR = botR * lineH + O;
  current->topL = topL * lineH + O;
  current->botL = botL * lineH + O;

  current->topLR = topR + 1 - addedR;
  current->botLR = botR + 1 - addedR;
  current->topLL = topL + 1 - addedL;
  current->botLL = botL + 1 - addedL;
  return current;
}

void
cleanP (void)
{
  polygon *thisP;
  if (!head)
    return;
  while (head != NULL)
  {
    thisP = head;
    head = thisP->next;
    free (thisP);
  }
  head = current = NULL;
  return;
}

void
cleanA (void)
{
  GdkRectangle update_rect;
  if (!drawA)
    return;
  if (!drawP)
    return;

  update_rect.x = 0;
  update_rect.y = GTK_ADJUSTMENT (adj)->value;
  update_rect.width = drawA->allocation.width;
  update_rect.height = drawA->allocation.height;

  gdk_draw_rectangle (drawP, drawA->style->bg_gc[GTK_WIDGET_STATE (drawA)], TRUE, 0, 0, drawA->allocation.width, 3 * drawA->allocation.height);
  gtk_widget_draw (drawA, &update_rect);
  gdk_draw_pixmap (drawA->window, drawA->style->bg_gc[GTK_WIDGET_STATE (drawA)], drawP, 0, 0, 0, 0, drawA->allocation.width, drawA->allocation.height);
}

void
cleanT (void)
{
#ifdef __GTK_2_0
  clear_text_buffer (text_left);
  clear_text_buffer (text_right);
#else
  if (text_left)
    gtk_text_backward_delete (GTK_TEXT (text_left), gtk_text_get_length (GTK_TEXT (text_left)));
  if (text_right)
    gtk_text_backward_delete (GTK_TEXT (text_right), gtk_text_get_length (GTK_TEXT (text_right)));
#endif
}

int
max_strip (char *file)
{
  int max_s = 0;
  char *place = NULL;
  if (!file)
    return max_s;
  /*printf("figuring striplevel:%s\n",file); */
  place = strstr (file, "/");
  if (place)
  {
    max_s++;
    place++;
    while (place)
    {
      /*printf("figuring striplevel %d:%s\n",max_s,place); */
      place = strstr (place, "/");
      if (place)
      {
	max_s++;
	place++;
      }
    }
  }
  return max_s;
}

char *
strip_it (char *file, int strip_level)
{
  int i;
  char *strip_file, *strip_try;
  strip_file = file;
  if (!strip_level) { goto strip_error;}
  strip_try = strstr (file, "/");
  if (!strip_try)
  {
  strip_error:			/* on strip exceeding maximum strip, strip all slashes */
  /*  my_show_message(_("Patch file names cannot be processed.\nPatch file might not be in unified format\n or strip level may be set incorrectly.")); */
    return strip_file;
  }
  strip_file = ++strip_try;
  for (i = 1; i < strip_level; i++)
  {
    strip_try = strstr (strip_file, "/");
    if (!strip_try)
      goto strip_error;
    strip_file = ++strip_try;
  }
  return strip_file;
}

/* find at least one file to patch */
int
check_patch_dir (void)
{
  patched_file *thisF;
  int i = 0, fileH;
  static char *file = NULL;
  char *strip_file;

  if (autostrip)
  {
    char texto[16];
    strip = go_figure_strip ();
    /*printf("using strip level=%d\n",strip); */
    show_diag (_(strip_set_to));
    sprintf (texto, "%d (-p%d)\n", strip, strip);
    show_diag (texto);
  }


  good_dir = 0;
  thisF = headF;
  while (thisF)
  {
    strip_file = strip_it (thisF->file, strip);
  other:i++;
    if (!strip_file)
      break;
    if (file)
      free (file);
    file = (char *) malloc (strlen (fileD) + strlen (strip_file) + 2);
    if (!file)
      xfdiff_abort (E_MALLOC);
    sprintf (file, "%s/%s", fileD, strip_file);
    fileH = open (file, O_RDONLY);
    if (fileH != -1)
    {
      close (fileH);
      good_dir = 1;
      break;
    }
    if (i % 2)
    {
      strip_file = strip_it (thisF->newfile, strip);
      goto other;
    }
    thisF = thisF->next;
  }
  if (good_dir)
    return TRUE;
  {
    char *no_good_dir;
    no_good_dir = _("No file was found for patching\nEither the directory to apply patch is not correctly selected\nor an incorrect strip level was used.\n");
    my_show_message (no_good_dir);
    show_diag (no_good_dir);
  }
  return FALSE;

}


int
check_reject (void)
{
  FILE *failed = NULL;
  failed = fopen (fileRR, "r");
  if (failed)
  {
    fclose (failed);
    my_show_message (_("Patch command has produced reject output.\nSee diagnostics window."));
    return TRUE;
  }
  return FALSE;
}

int
checkdir (char *file)
{
  struct stat st;
  stat (file, &st);
  if (st.st_mode & S_IFDIR) 
  {
    /*my_show_message(_("That is a directory (or symlink)")); */
    return TRUE;
  }
  return FALSE;
}

int
checknotdir (char *file)
{
  struct stat st;
  stat (file, &st);
  if (st.st_mode & S_IFDIR);
  else
  {
    my_show_message (_("No directory was selected!"));
    return TRUE;
  }
  return FALSE;
}


char *
get_the_file (char *file)
{
  char *fileS;
  /*  */
#ifdef __GTK_2_0
  /*  */
  fileS = xf_fileselect (_("Directory or file selection"));
#else
  fileS = open_fileselect ("*");
#endif
  if (fileS)
  {
    if ((file) && (strcmp (file, fileS) == 0))
      return file;
    /* ok, the file selection has changed */
    file = assign (file, fileS);
  }
  return file;
}

char *
assign (char *dst, char *src)
{
  if (dst)
    free (dst);
  if (!src)
  {
    dst = NULL;
    return NULL;
  }
  dst = (char *) malloc (strlen (src) + 1);
  if (!dst)
    xfdiff_abort (E_MALLOC);
  strcpy (dst, src);
  return dst;
}

char *
prompt_path (char *message, char *file)
{
  char *texto;
  texto = (char *) malloc (strlen (message) + strlen (_("Please select it in the next dialog.")) + 3);
  if (!texto)
    xfdiff_abort (E_MALLOC);
  sprintf (texto, "%s\n%s", message, _("Please select it in the next dialog."));
#ifdef __GTK_2_0
  /*  */
  if (!xf_confirm (texto))
    return NULL;
#else
  my_show_message (texto);
#endif

  file = get_the_file (file);
  if (!file)
  {
    sprintf (texto, "%s", message);
    my_show_message (texto);
    free (texto);
    return NULL;
  }
  free (texto);
  return file;
}

char *
prompt_outfile (char *message, char *file)
{
#ifdef __GTK_2_0
  if (!xf_confirm (message))
    return NULL;
#else
  my_show_message (message);
#endif
  file = get_the_file (file);

  if (file)
  {
    FILE *fileX = NULL;
    if ((fileX = fopen (file, "r")) != NULL)
    {
      fclose (fileX);
      if (my_yesno_dialog (_("File exists, overwrite?")))
      {
	if ((fileX = fopen (file, "w")) == NULL)
	{
	  my_show_message (_("Cannot open file for write\n"));
	  goto outfile_error;
	}
	else
	  fclose (fileX);
      }
      else
	goto outfile_error;
    }
  }
  else
    goto outfile_error;
  return file;
outfile_error:
  if (file)
    free (file);
  return NULL;
}


int
first_diff (void)
{
  int value;
  if (head == NULL)
    value = 0;
  else
  {
    current = head;
    value = (current->topR < current->topL) ? current->topR : current->topL;
    if (value >= lineH)
      value -= lineH;
    else
      value = 0;
  }
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), value);
  configure_event (drawA, NULL);
  cb_adjust ((GtkAdjustment *) adj, NULL);
  if (head)
    return TRUE;
  else
    return FALSE;
}

void
set_the_style (GtkWidget * widget)
{

  xfce_style = gtk_rc_get_style (widget);
  if (xfce_style)
    style = gtk_style_copy (xfce_style);
  else
    style = gtk_style_new ();

  if (fuente)
  {
    if ((style->font = gdk_fontset_load (fuente)) == NULL)
    {
      /* some fonts are buggy on startup with iso-8859-1 systems (like mine) */
      my_show_message (_("Could not load specified font\n"));
      fuente = DEFAULTFONT;
#ifdef AUTOSAVE
      cb_save_defaults (NULL, NULL);
#endif
    }
  }

}

void
get_defaults (void)
{
  FILE *defaults;
  char *homedir;
  int len;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFDIFF_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir)
    xfdiff_abort (E_MALLOC);
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"), XFDIFF_CONFIG_FILE);
  defaults = fopen (homedir, "r");
  free (homedir);

  filefg.red = 0xffff;
  filefg.green = 0xffff;
  filefg.blue = 0;
  filebg.red = 0;
  filebg.green = 0;
  filebg.blue = 0xffff;
  filledP = TRUE;
  show_lineN = TRUE;
  synchronize = FALSE;

#ifdef GNU_PATCH
  verbose = 0;
#endif

  if (!defaults)
  {
    colorfg.red = 0xffff;
    colorfg.green = 0xffff;
    colorfg.blue = 0;
    colorbg.red = 0xffff;
    colorbg.green = 0;
    colorbg.blue = 0;
    fuente = DEFAULTFONT;
    silent = 0;
  }
  else
  {
    char line[256], *word, *value;
    while ((!feof (defaults)) && (fgets (line, 255, defaults)))
    {
      line[255] = 0;
      if (line[0] == '#')
	continue;
      word = strtok (line, ":");
      value = word + strlen (word) + 1;
      /*printf("word=%s value=%s\n",word,value); */

      if (!word)
	continue;
      if (strstr (word, "colorfg.red"))
	colorfg.red = atoi (value);
      if (strstr (word, "colorfg.green"))
	colorfg.green = atoi (value);
      if (strstr (word, "colorfg.blue"))
	colorfg.blue = atoi (value);
      if (strstr (word, "colorbg.red"))
	colorbg.red = atoi (value);
      if (strstr (word, "colorbg.green"))
	colorbg.green = atoi (value);
      if (strstr (word, "colorbg.blue"))
	colorbg.blue = atoi (value);
      /*if (strstr(word,"silent"))  silent  = atoi(value); */
      if (strstr (word, "filledP"))
	filledP = atoi (value);
      if (strstr (word, "show_lineN"))
	show_lineN = atoi (value);
      if (strstr (word, "synchronize"))
	synchronize = atoi (value);
      if (strstr (word, "font"))
      {
	if (strstr (value, "\n"))
	  value = strtok (value, "\n");
	fuente = (char *) malloc (strlen (value) + 1);
	if (!fuente)
	  xfdiff_abort (E_MALLOC);
	strcpy (fuente, value);
      }
#ifdef GNU_PATCH
      if (strstr (word, "verbose"))
	verbose = atoi (value);
#endif
    }
  }

}


#ifdef __GTK_2_0
/*GtkTextTagTable *tagT;*/
#endif
/* set colors in GC to be used for drawing polygons */
void
set_highlight (void)
{
  GdkColormap *cmap;

#ifdef __GTK_2_0
/*	tagT=gtk_text_tag_table_new();
	gtk_text_tag_table_add(tagT,gtk_text_tag_new ("bluetext"));*/

  gtk_text_buffer_create_tag (text_right, "bluetext", "background-gdk", filebg, "foreground-gdk", filefg, NULL);
  gtk_text_buffer_create_tag (text_left, "bluetext", "background-gdk", filebg, "foreground-gdk", filefg, NULL);
#endif


  cmap = gdk_colormap_get_system ();
  if (!gdk_color_alloc (cmap, &colorfg))
  {
    g_error (_("couldn't allocate color"));
  }

  if (!gdk_color_alloc (cmap, &colorbg))
  {
    g_error (_("couldn't allocate color"));
  }

  drawGC = gdk_gc_new (drawA->window);
  gdk_gc_set_foreground (drawGC, &colorbg);	/* bg is actually fg for draw */
  fileGC = gdk_gc_new (drawA->window);
  /* Hmm... I wonder why this didn't work for drawtext function: */
  /*gdk_gc_set_foreground (fileGC,&filefg); */
  /*gdk_gc_set_background (fileGC,&filebg); */
}




static GList *items = NULL;
void
update_titlesP (void)
{
  char *texto;
  patched_file *thisF;
  thisF = currentF;
  if (patching)
  {
#ifdef __GTK_2_0
    if (what_dir)
    {
      clear_text_buffer (what_dir);
      if (!fileP)
      {
	utf8_insert (what_dir, _(no_patch), -1);
      }
      else
      {
// font color!
	utf8_insert (what_dir, _("Patch: "), -1);
	utf8_insert (what_dir, fileP, -1);
      }
      utf8_insert (what_dir, _(" applied at: "), -1);
      if (!fileD)
      {
	utf8_insert (what_dir, _(no_patch_dir), -1);
      }
      else
      {
	utf8_file_insert (what_dir, fileD, -1);
      }
    }
#else
    if (what_dir)
      gtk_text_backward_delete (GTK_TEXT (what_dir), gtk_text_get_length (GTK_TEXT (what_dir)));
    if (!fileP)
    {
      if (what_dir)
	gtk_text_insert (GTK_TEXT (what_dir), NULL, &filefg, &filebg, _(no_patch), strlen (_(no_patch)));
    }
    else
    {
      if (what_dir)
	gtk_text_insert (GTK_TEXT (what_dir), NULL, NULL, NULL, _("Patch: "), strlen (_("Patch: ")));
      if (what_dir)
	gtk_text_insert (GTK_TEXT (what_dir), NULL, &filefg, &filebg, fileP, strlen (fileP));
    }
    if (what_dir)
      gtk_text_insert (GTK_TEXT (what_dir), NULL, NULL, NULL, _(" applied at: "), strlen (_(" applied at: ")));
    if (!fileD)
    {
      if (what_dir)
	gtk_text_insert (GTK_TEXT (what_dir), NULL, &filefg, &filebg, _(no_patch_dir), strlen (_(no_patch_dir)));
    }
    else
    {
      if (what_dir)
	gtk_text_insert (GTK_TEXT (what_dir), NULL, &filefg, &filebg, fileD, strlen (fileD));
    }
#endif

    if (!fileP)
      texto = no_patch;
    else
    {
      if (!fileD)
	texto = no_patch_dir;
      else
	texto = (N_("Press OK to see patch diffs"));
    }
  }
  else
  {				/* !patching */
    if (!fileR)
      texto = no_right_path;
    else
      texto = fileR;
  }
  if (thisF)
    texto = thisF->newfile;

  if (titleP)
  {
#ifdef __GTK_2_0
    clear_text_buffer (titleP);
    utf8_file_insert (titleP, _(texto), -1);
#else
    gtk_text_backward_delete (GTK_TEXT (titleP), gtk_text_get_length (GTK_TEXT (titleP)));
    gtk_text_insert (GTK_TEXT (titleP), NULL, NULL, NULL, _(texto), strlen (_(texto)));
#endif
  }

  if (items != NULL)
  {
    g_list_free (items);
    items = NULL;
  }
  if (thisF)
  {
    while (thisF)
    {
      items = g_list_append (items, thisF->file);
      thisF = thisF->next;
    }
    thisF = headF;
    while (thisF != currentF)
    {
      items = g_list_append (items, thisF->file);
      thisF = thisF->next;
    }
    if (titleD)
      gtk_combo_set_popdown_strings (GTK_COMBO (titleD), items);
  }
  else
  {
    if (!patching)
    {
      if (fileL)
	texto = fileL;
      else
	texto = no_left_path;
    }
    items = g_list_append (items, _(texto));
    if (titleP)
      gtk_combo_set_popdown_strings (GTK_COMBO (titleD), items);
  }
}
