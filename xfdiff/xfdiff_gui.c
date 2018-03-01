/*   xfdiff_gui.c */

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

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <gtk/gtk.h>
#include "constant.h"
#include "xfce-common.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "xfdiff.h"
#include "xfdiff_colorsel.h"
#include "xfdiff_diag.h"
#include "xfdiff_misc.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define COLOR_GDK	65535.0

#include "icons/xfdiff.xpm"

static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URI_LIST},
  {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

static GtkWidget *viewW;

#ifdef __GTK_2_0
	/*  */
static GtkTextBuffer *text;
#else
static GtkWidget *text;
#endif

/* static prototypes: */
/* GUI prototypes: */
static gint expose_event (GtkWidget * widget, GdkEventExpose * event);

/* CB prototypes: */
static void cb_do_patch (GtkWidget * widget, gpointer data);
static void cb_do_diff (GtkWidget * widget, gpointer data);

/******************** CB section ************************/
#ifdef __GTK_2_0
void
clear_text_buffer (GtkTextBuffer * buffer)
{
  /*GtkTextIter *start=NULL,*end=NULL; */
  if (!buffer)
    return;
  gtk_text_buffer_set_text (buffer, "", -1);
/*	gtk_text_buffer_get_bounds (buffer,start,end);  */
/*	gtk_text_buffer_delete(buffer,start,end);*/
}
#endif

#ifndef __GTK_2_0
/* this hack doesnt work with gtk+1.3 */
static void
cb_wheel (GtkWidget * widget, GdkEventButton * event, void *data)
{
  int new_adj;

  new_adj = (GTK_ADJUSTMENT (adj)->value);

  if (event->button == 4)
    new_adj -= (3 * lineH);	/*printf("button clicked4\n"); */
  if (event->button == 5)
    new_adj += (3 * lineH);	/*printf("button clicked5\n"); */
  if (new_adj < 0)
    new_adj = 0;
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), new_adj);
}
#endif
static void
on_drag_data_received (GtkWidget * entry, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
  char *text, *file;

  if ((data->length == 0) || (data->format != 8) || (info != TARGET_STRING))
  {
    gtk_drag_finish (context, FALSE, TRUE, time);
  }

  text = (char *) malloc (data->length + 1);
  strncpy (text, (char *) data->data, data->length);
  text[data->length] = '\0';

  if (strstr (text, "\n"))
    text = strtok (text, "\n");
  if (strncmp (text, "file:", strlen ("file:")) == 0)
  {
    file = text + strlen ("file:");
    {
      int i;
      for (i = 0; i < strlen (file); i++)
	if (file[i] == 13)
	  file[i] = 0;
    }
    if (patching)
    {
      if (checkdir (file))
	fileD = assign (fileD, file);
      else
	fileP = assign (fileP, file);
    }
    else
    {				/* spot 1/3 where fileR, fileL come in */
#ifdef __GTK_2_0
      if (entry == viewR)
      {
#else
      if (entry == text_right)
      {
#endif
	fileR = assign (fileR, file);
	fileRD = assign (fileRD, checkdir (file) ? file : NULL);
      }
      else
      {
	fileL = assign (fileL, file);
	fileLD = assign (fileLD, checkdir (file) ? file : NULL);
      }
    }
    update_titlesP ();
    cleanF ();
    cleanP ();
    cleanA ();
    cleanT ();
  }
  gtk_drag_finish (context, TRUE, TRUE, time);
  free (text);
}

static void
cb_patchapply (GtkWidget * widget, gpointer data)
{
  int undoing;
  undoing = (int) data;
  if ((!fileD) || (!fileP))
  {
    my_show_message (_("A patch file and directory must be selected first!"));
    return;
  }

  if (undoing)
    reversed = !reversed;
  applying_patch = 1;
  do_patch ();
  applying_patch = 0;
  if (undoing)
    reversed = !reversed;
  if (!check_reject ())
    my_show_message ((undoing) ? _("Patch has been undone.") : _("Patch has been applied."));
}


/* this function should only be called from diff mode.*/
static void
cb_patchmake (GtkWidget * widget, gpointer data)
{
  char *patchDL = NULL, *patchDR = NULL;
  int cancelled = 0;
  /* select file name for output: */

  patchDR = assign (patchDR, fileR);
  patchDL = assign (patchDL, fileL);

  patchO = prompt_outfile (_("Please select a name for the patch file in the next dialog."), patchO);
  if (!patchO)
  {
    cancelled = 1;
    goto exit_patchmake;
  }

  if (fileLD)
    fileL = assign (fileL, fileLD);
  if (fileRD)
    fileR = assign (fileR, fileRD);

  if (!fileL)
    fileL = prompt_path (_(no_left_path), fileL);
  if ((fileL) && (!fileR))
    fileR = prompt_path (_(no_right_path), fileR);
  if ((!fileL) || (!fileR))
    goto exit_patchmake;

  if (!my_yesno_dialog (_("XFdiff will now start diff.\nPlease be patient (xfdiff might seem to hang).\nDiff will be working")))
  {
    cancelled = 1;
    goto exit_patchmake;
  }

  if (!do_diff ())
    my_show_message (_("Patch file could not be created"));
  else
  {
    char *number;
    int numL, numR;
    number = (char *) malloc (strlen (_("Strip level for created patch file=")) + 16);
    if (!number)
      xfdiff_abort (E_MALLOC);
    numL = max_strip (fileL);
    numR = max_strip (fileR);
    sprintf (number, "%s%d\n", _("Strip level for created patch file="), (numL < numR) ? numL : numR);
    show_diag (patchO);
    show_diag (_(" has been created\n"));
    show_diag (number);
    my_show_message (number);

    show_diag ("(");
    show_diag (fileL);
    show_diag ("-->");
    show_diag (fileR);
    show_diag (")\n");
    free (number);
  }

exit_patchmake:
  if (cancelled)
    my_show_message (_("Patch file creation cancelled"));
  fileR = assign (fileR, patchDR);
  fileL = assign (fileL, patchDL);
  patchDR = assign (patchDR, NULL);
  patchDL = assign (patchDL, NULL);
  patchO = assign (patchO, NULL);
  return;
}

static void
cb_radio_strip (GtkWidget * widget, gpointer data)
{
  char texto[16];
  static int laststrip = 0;	/* default strip level */
  strip = ((int) ((long) data));
  if (strip != laststrip)
  {
    show_diag (_(strip_set_to));
    sprintf (texto, "%d (-p%d)\n", strip, strip);
    show_diag (texto);
    laststrip = strip;
    /* reparse to check if null patchfile with that strip: */
    cleanF ();
    update_titlesP ();
    cleanT ();
    cleanP ();
    cleanA ();
  }
}

/*
static void cb_toggle_diagnostics(GtkWidget *widget,gpointer data){
	silent=!silent;
	if (silent) {
		show_diag (_("Diff diagnostics disabled.\n"));
		show_diag (_("(Patch diagnostics remain enabled)\n"));
	} else {
		show_diag (_("Diff diagnostics enabled.\n"));
	}
#ifdef AUTOSAVE
	cb_save_defaults(NULL,NULL); 
#endif	
}*/

/* 1.2.3 uses 8: */
#define DIFF_MENUS 8
/* 1.2.3 uses 10: */
#define PATCH_MENUS 10
/* 1.2.3 uses 10 */
#define STRIP_LEVELS 10


static GtkWidget *patchM[PATCH_MENUS];
static GtkWidget *diffM[DIFF_MENUS];
static GtkWidget *stripM[STRIP_LEVELS];

static void
hideshow_menus (void)
{
  gint i;
  if (patching)
  {
    for (i = 0; i < STRIP_LEVELS; i++)
    {
      if (i == strip)
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (stripM[i]), TRUE);
      if (!autostrip)
      {
	if (!GTK_WIDGET_VISIBLE (stripM[i]))
	  gtk_widget_show (stripM[i]);
      }
      else
      {
	if (GTK_WIDGET_VISIBLE (stripM[i]))
	  gtk_widget_hide (stripM[i]);
      }
    }

    for (i = 0; i < PATCH_MENUS; i++)
      if ((patchM[i]) && (!GTK_WIDGET_VISIBLE (patchM[i])))
	gtk_widget_show (patchM[i]);
    for (i = 0; i < DIFF_MENUS; i++)
      if ((diffM[i]) && (GTK_WIDGET_VISIBLE (diffM[i])))
	gtk_widget_hide (diffM[i]);
  }
  else
  {
    for (i = 0; i < STRIP_LEVELS; i++)
      if (GTK_WIDGET_VISIBLE (stripM[i]))
	gtk_widget_hide (stripM[i]);
    for (i = 0; i < PATCH_MENUS; i++)
      if ((patchM[i]) && (GTK_WIDGET_VISIBLE (patchM[i])))
	gtk_widget_hide (patchM[i]);
    for (i = 0; i < DIFF_MENUS; i++)
      if ((diffM[i]) && (!GTK_WIDGET_VISIBLE (diffM[i])))
	gtk_widget_show (diffM[i]);
  }
}

static void
cb_toggle_mode (GtkWidget * widget, gpointer data)
{
  static char *LfileR = NULL, *LfileL = NULL;
  if ((int) ((long) data))
  {
    patching = !patching;
    cleanA ();
    cleanP ();
    cleanT ();
    cleanF ();
    if (patching)
    {
      LfileR = assign (LfileR, fileR);
      LfileL = assign (LfileL, fileL);
    }
    else
    {
      if (fileRR)
	remove (fileRR);
      fileRR = assign (fileRR, NULL);
      if (fileO)
	remove (fileO);
      fileO = assign (fileO, NULL);
      if (fileI)
	remove (fileI);
      fileI = assign (fileI, NULL);
      fileR = assign (fileR, LfileR);
      fileL = assign (fileL, LfileL);
    }

    if (patching)
    {
      if (!GTK_WIDGET_VISIBLE (patch_label_box))
	gtk_widget_show (patch_label_box);
    }
    else
    {
      if (GTK_WIDGET_VISIBLE (patch_label_box))
	gtk_widget_hide (patch_label_box);
    }
    update_titlesP ();
  }
  hideshow_menus ();
}

static void
cb_toggle_synchronize (GtkWidget * widget, gpointer data)
{
  synchronize = !synchronize;
  if (silent)
    show_diag (_("Line synchronization enabled.\n"));
  else
    show_diag (_("Line synchronization disabled.\n"));
#ifdef AUTOSAVE
  cb_save_defaults (NULL, NULL);
#endif
  {
    int old_adj = 0;
    polygon *thisP;
    if (current)
    {
      old_adj = current->topLR;
    }
    if (patching)
      cb_do_patch (NULL, (gpointer) ((int) 0));
    else
      cb_do_diff (NULL, (gpointer) ((int) 0));
    thisP = head;
    while (thisP)
    {
      if (thisP->topLR == old_adj)
      {
	old_adj = thisP->topR;
	break;
      }
      thisP = thisP->next;
    }
    current = thisP;
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), old_adj);
  }

}

#ifdef GNU_PATCH
static void
cb_toggle_verbose (GtkWidget * widget, gpointer data)
{
  verbose = !verbose;
  if (verbose)
    show_diag (_("Patch verbose diagnostics enabled.\n"));
  else
    show_diag (_("Patch verbose diagnostics disabled.\n"));
#ifdef AUTOSAVE
  cb_save_defaults (NULL, NULL);
#endif
}
#endif

static void
cb_toggle_autostrip (GtkWidget * widget, gpointer data)
{
  autostrip = !autostrip;
  hideshow_menus ();
}

static void
cb_toggle_show_lineN (GtkWidget * widget, gpointer data)
{
  show_lineN = !show_lineN;
  if (show_lineN)
    show_diag (_("Line numbers enabled.\n"));
  else
    show_diag (_("Line numbers disabled.\n"));
#ifdef AUTOSAVE
  cb_save_defaults (NULL, NULL);
#endif
  configure_event (drawA, NULL);
}

static void
cb_toggle_filledP (GtkWidget * widget, gpointer data)
{
  filledP = !filledP;
  if (filledP)
    show_diag (_("Polygons will now be filled.\n"));
  else
    show_diag (_("Polygons will now *not* be filled.\n"));
#ifdef AUTOSAVE
  cb_save_defaults (NULL, NULL);
#endif
  configure_event (drawA, NULL);
}

void
cb_toggle_reversed (GtkWidget * widget, gpointer data)
{
  reversed = !reversed;

  GTK_CHECK_MENU_ITEM (REVERSE_TOGGLE)->active = reversed;
  if (reversed)
    show_diag (_("Patch will now be assumed reversed.\n"));
  else
    show_diag (_("Patch will now be assumed *not* reversed.\n"));
  if (headF)
  {
    /* cleanF(); no need to parse patch file again */
    cleanP ();
    cleanA ();
    cleanT ();
  }
}

static void
cb_select_font (GtkWidget * widget, gpointer data)
{
  static char *fnt;
  int old_lineH;

  /*printf("font=%s\n",fuente); */
  /*  */
  old_lineH = viewR->style->font->ascent + viewR->style->font->descent;
#ifdef __GTK_2_0
  /*  */
  fnt = xf_fontselect (_("Font selection"), fuente);
#else
  fnt = open_fontselection (fuente);
#endif
  if (fnt)
  {
    the_font = gdk_font_load (fnt);
    if (the_font == NULL)
    {
      my_show_message (_("Could not load specified font\n"));
      return;
    }
    else
    {
      int old_silent, old_adj;
      style->font = the_font;
      fuente = fnt;
#ifdef AUTOSAVE
      cb_save_defaults (NULL, NULL);
#endif
      old_silent = silent;
      silent = 1;
      old_adj = (GTK_ADJUSTMENT (adj)->value) / old_lineH;
      cb_do_diff (widget, (gpointer) ((int) 0));
      silent = old_silent;
      old_adj *= (viewR->style->font->ascent + viewR->style->font->descent);


      gtk_widget_set_usize (OKbutton, DRAW_W, lineH + 5);
      gtk_widget_set_usize (drawA, DRAW_W, lineH + 5);

      configure_event (drawA, NULL);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), old_adj);
    }
  }

}

static void
cb_select_colors (GtkWidget * widget, gpointer data)
{
  gdouble colors[4];
  gdouble *newcolor;
  GtkStyle *style;
  int caso;

  caso = (long) data;

  style = gtk_widget_get_style (viewR);
  /* initial colors defined in getdefaults() */
  switch (caso)
  {
  case 0:
    colors[0] = ((gdouble) colorfg.red) / COLOR_GDK;
    colors[1] = ((gdouble) colorfg.green) / COLOR_GDK;
    colors[2] = ((gdouble) colorfg.blue) / COLOR_GDK;
    break;
  case 1:
    colors[0] = ((gdouble) colorbg.red) / COLOR_GDK;
    colors[1] = ((gdouble) colorbg.green) / COLOR_GDK;
    colors[2] = ((gdouble) colorbg.blue) / COLOR_GDK;
    break;
  }
#ifdef __GTK_2_0
  /*  */
  switch (caso)
  {
  case 1:
    newcolor = xf_colorselect (_("Select highlight background color"), colors);
    break;
  default:
    newcolor = xf_colorselect (_("Select highlight text color"), colors);
    break;
  }
#else
  newcolor = xfdiff_colorselect (colors);
#endif
  if (newcolor)
  {
    int old_silent, old_adj;
    switch (caso)
    {
    case 0:
      colorfg.red = ((guint) (newcolor[0] * COLOR_GDK));
      colorfg.green = ((guint) (newcolor[1] * COLOR_GDK));
      colorfg.blue = ((guint) (newcolor[2] * COLOR_GDK));
      break;
    case 1:
      colorbg.red = ((guint) (newcolor[0] * COLOR_GDK));
      colorbg.green = ((guint) (newcolor[1] * COLOR_GDK));
      colorbg.blue = ((guint) (newcolor[2] * COLOR_GDK));
      break;
    }
    /* printf("color=0x%x\n",caso);
       printf("r=0x%x, g=0x%x b=0x%x\n",colorfg.red,colorfg.green ,colorbg.blue); */
    set_highlight ();

    old_silent = silent;
    silent = 1;
    old_adj = (GTK_ADJUSTMENT (adj)->value);
    if ((fileR && fileL) || (fileP && fileD))
      cb_do_diff (widget, (gpointer) ((int) 0));
    silent = old_silent;
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), old_adj);
#ifdef AUTOSAVE
    cb_save_defaults (NULL, NULL);
#endif
  }
}


static void
cb_do_diff (GtkWidget * widget, gpointer data)
{
  cleanP ();
  cleanA ();
  cleanT ();
  do_diff ();
}


static void
cb_do_patch (GtkWidget * widget, gpointer data)
{
  gchar *s = NULL;
  patched_file *thisF;
  int from_menu;
  from_menu = (int) data;

  cleanP ();
  cleanA ();
  cleanT ();
  if (from_menu)
    cleanF ();			/* force a new patch run if selected from menu */

  if (!headF)
  {
    if (!patching)
      cb_do_diff (widget, data);
    else
    {
      do_patch ();
      check_reject ();
    }
    return;
  }

#ifdef __GTK_2_0
  /* s must be g_freed now. */
  s = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (titleD)->entry), 0, -1);
#else
/* all bug workarounds for this function can be eliminated. 
*  the function is deprecated in 2.0 */
  s = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (titleD)->entry));
#endif
  thisF = headF;
  while (thisF)
  {
    if (strcmp (s, thisF->file) == 0)
      break;
    thisF = thisF->next;
  }
  if (thisF)
    currentF = thisF;
  else
  {
    char *texto;
    if (!patching)
    {
      texto = (char *) malloc (strlen (s) + strlen (_("Warning: cannot read from ")) + 2);
      sprintf (texto, "%s %s", _("Warning: cannot read from "), s);
    }
    else
    {
      texto = (char *) malloc (strlen (s) + strlen (_("is not in patch file.")) + 2);
      sprintf (texto, "%s %s", s, _("is not in patch file."));
    }
    my_show_message (texto);
    free (texto);
#ifdef __GTK_2_0
    /*  */
    g_free (s);
#endif
    return;
  }
  if (patching)
    process_patch (currentF);
  else
  {
    fileL = assign (fileL, currentF->file);
    fileR = assign (fileR, currentF->newfile);
    do_diff ();
  }
  first_diff ();
  update_titlesP ();
#ifdef __GTK_2_0
  /*  */
  g_free (s);
#endif
  return;
}


static void
cb_do_diff_new (GtkWidget * widget, gpointer data)
{
  char Pstrip[4];
  char *Pright, *Pleft;
  char *arguments[10];
  int argc = 0;


  arguments[argc++] = "xfdiff";
  if (patching)
    arguments[argc++] = "-p";
  strcpy (Pstrip, "-P");
  Pstrip[2] = strip + '0';
  Pstrip[3] = 0;
  arguments[argc++] = Pstrip;
  if (reversed)
    arguments[argc++] = "-R";
  else
    arguments[argc++] = "-N";
  if (patching)
  {
    arguments[argc++] = "-p";
    Pleft = fileD;
    Pright = fileP;
  }
  else
  {
    if (fileRD)
      Pright = fileRD;
    else
      Pright = fileR;
    if (fileLD)
      Pleft = fileLD;
    else
      Pleft = fileL;
  }
  if (Pleft)
    arguments[argc++] = Pleft;
  if ((Pright) && (Pleft))
    arguments[argc++] = Pright;
  arguments[argc++] = (char *) 0;

  if (fork ())
  {				/*  parent will fork to new  */
    execvp ("xfdiff", arguments);
    perror ("exec");
    _exit (127);		/* parent never gets here (hopefully) */
  }
  return;
}

static void
cb_do_patch_new (GtkWidget * widget, gpointer data)
{
  cb_do_diff_new (NULL, NULL);
  return;
}



void
cb_save_defaults (GtkWidget * widget, gpointer data)
{
  FILE *defaults;
  char *homedir;
  int len;

  len = strlen ((char *) getenv ("HOME")) + strlen ("/.xfce/") + strlen (XFDIFF_CONFIG_FILE) + 1;
  homedir = (char *) malloc ((len) * sizeof (char));
  if (!homedir)
    xfdiff_abort (E_MALLOC);
  snprintf (homedir, len, "%s/.xfce/%s", (char *) getenv ("HOME"), XFDIFF_CONFIG_FILE);
  defaults = fopen (homedir, "w");
  free (homedir);

  if (!defaults)
  {
    my_show_message (_("Default .xfdiffrc file cannot be created\n"));
    return;
  }
  fprintf (defaults, "# file created by xfdiff, if removed xfdiff returns to XFCE defaults.\n");
  fprintf (defaults, "colorfg.red : %d\n", colorfg.red);
  fprintf (defaults, "colorfg.green : %d\n", colorfg.green);
  fprintf (defaults, "colorfg.blue : %d\n", colorfg.blue);
  fprintf (defaults, "colorbg.red : %d\n", colorbg.red);
  fprintf (defaults, "colorbg.green : %d\n", colorbg.green);
  fprintf (defaults, "colorbg.blue : %d\n", colorbg.blue);
/*	fprintf(defaults,"silent : %d\n",silent);*/
  fprintf (defaults, "font : %s\n", fuente);
  fprintf (defaults, "filledP : %d\n", filledP);
  fprintf (defaults, "show_lineN : %d\n", show_lineN);
  fprintf (defaults, "synchronize : %d\n", synchronize);
#ifdef GNU_PATCH
  fprintf (defaults, "verbose : %d\n", verbose);
#endif
  fclose (defaults);
#ifndef AUTOSAVE
  my_show_message (_("Default configuration has been saved\n"));
#endif
  return;
}

static void
cb_about (GtkWidget * item, GtkWidget * ctree)
{
  my_show_message (_("This is XFDiff " XFDIFF_VERSION "\n(c) by Edscott Wilson Garcia under GNU GPL" "\nXFCE modules (c) Olivier Fourdan http://www.xfce.org/"));
}

void
cb_next_diff (GtkWidget * widget, gpointer data)
{
  int value;
  long next;
  next = (long) data;
  if (!current)
    value = 0;
  else
  {
    if (next)
    {
      if (current->next)
	current = current->next;
      else
      {
	my_show_message (_("There is no next difference"));
      }
    }
    else
    {
      if (current->previous)
	current = current->previous;
      else
      {
	my_show_message (_("There is no previous difference"));
      }
    }

    value = (current->topR < current->topL) ? current->topR : current->topL;
    if (value >= lineH)
      value -= lineH;
    else
      value = 0;
  }
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), value);
  configure_event (drawA, NULL);
  cb_adjust ((GtkAdjustment *) adj, NULL);
}

static void
cb_next_file (GtkWidget * widget, gpointer data)
{
  long next;
  next = (long) data;
  if (currentF == NULL)
  {
    my_show_message (_("There is no such file"));
    return;
  }

  if (next)
  {
    if (currentF->next)
      currentF = currentF->next;
    else
    {
      my_show_message (_("There is no next file"));
      return;
    }

  }
  else
  {
    if (currentF->previous)
      currentF = currentF->previous;
    else
    {
      my_show_message (_("There is no previous file"));
      return;
    }
  }

  cleanT ();
  cleanP ();
  if (patching)
  {
    process_patch (currentF);
  }
  else
  {
    fileL = assign (fileL, currentF->file);
    fileR = assign (fileR, currentF->newfile);
    do_diff ();
  }
  update_titlesP ();
  first_diff ();
}

void
cb_adjust (GtkAdjustment * adj, GtkWidget * widget)
{
  GdkEventExpose event;

#ifdef __GTK_2_0
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adjR), GTK_ADJUSTMENT (adj)->value);
#else
  gtk_adjustment_set_value (GTK_TEXT (viewR)->vadj, GTK_ADJUSTMENT (adj)->value);
#endif

  event.area.x = 0;
  event.area.y = 0;
  event.area.width = drawA->allocation.width;
  event.area.height = drawA->allocation.height;
  configure_event (drawA, NULL);
}

/* spot 2/3 where fileR and fileL come in */
static void
cb_get_fileR (GtkWidget * widget, gpointer data)
{
  fileR = get_the_file (fileR);
  fileRD = assign (fileRD, checkdir (fileR) ? fileR : NULL);
  update_titlesP ();
  cleanP ();
  cleanA ();
#ifdef __GTK_2_0
  /*  */
#else
  gtk_text_backward_delete (GTK_TEXT (text_right), gtk_text_get_length (GTK_TEXT (text_right)));
#endif
}

static void
cb_get_fileL (GtkWidget * widget, gpointer data)
{
  fileL = get_the_file (fileL);
  fileLD = assign (fileLD, checkdir (fileL) ? fileL : NULL);
  update_titlesP ();
  cleanP ();
  cleanA ();
#ifdef __GTK_2_0
  /*  */
#else
  gtk_text_backward_delete (GTK_TEXT (text_left), gtk_text_get_length (GTK_TEXT (text_left)));
#endif
}
static void
cb_get_fileD (GtkWidget * widget, gpointer data)
{
  fileD = get_the_file (fileD);
  /* checknotdir() includes my_show_message */
  if (checknotdir (fileD))
    fileD = assign (fileD, NULL);
  update_titlesP ();

}

static void
cb_get_fileP (GtkWidget * widget, gpointer data)
{
  fileP = get_the_file (fileP);
  if (checkdir (fileP))
  {
    /* only spot where directories for input is not OK: */
    my_show_message (_("That is a directory (or symlink)"));
    fileP = assign (fileP, NULL);
  }
  update_titlesP ();
}



/********************  GUI section **********************/

gint
configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
  int height;
  if (widget->window == NULL)
    return FALSE;

  /* if height definition is changed, watchout at function cleanA() */
  height = 3 * widget->allocation.height;
  if (drawP)
    gdk_pixmap_unref (drawP);
  drawP = gdk_pixmap_new (widget->window, widget->allocation.width, height, -1);
  gdk_draw_rectangle (drawP, widget->style->bg_gc[GTK_WIDGET_STATE (widget)], TRUE, 0, 0, widget->allocation.width, height);
#define Y1 (thisP->topL - GTK_ADJUSTMENT (adj)->value + widget->allocation.height)
#define Y2 (thisP->topR - GTK_ADJUSTMENT (adj)->value + widget->allocation.height)
#define Y3 (thisP->botR - GTK_ADJUSTMENT (adj)->value + widget->allocation.height)
#define Y4 (thisP->botL - GTK_ADJUSTMENT (adj)->value + widget->allocation.height)
#define H  (widget->allocation.height)
#define W  (widget->allocation.width)
  {
    polygon *thisP;
    GdkRectangle update_rect;
    GdkPoint pt[4];
    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = W;
    update_rect.height = H;
    thisP = head;
    while (thisP)
    {
      /* is it in the update region? */
      if (((Y1 <= 3 * H) || (Y2 <= 3 * H)) && ((Y3 >= 0) || (Y4 >= 0)))
      {
	pt[0].x = 0;
	pt[0].y = (Y1 > 3 * H) ? 3 * H : (Y1 < 0) ? 0 : Y1;
	pt[1].x = W;
	pt[1].y = (Y2 > 3 * H) ? 3 * H : (Y2 < 0) ? 0 : Y2;
	pt[2].x = W;
	pt[2].y = (Y3 > 3 * H) ? 3 * H : (Y3 < 0) ? 0 : Y3;
	pt[3].x = 0;
	pt[3].y = (Y4 > 3 * H) ? 3 * H : (Y4 < 0) ? 0 : Y4;
	gdk_draw_polygon (drawP, drawGC, filledP, pt, 4);
	if (show_lineN)
	{
	  char number[32];
	  int x;
	  sprintf (number, "%d", thisP->topLL);
	  gdk_draw_text (drawP, style->font, fileGC, 0, pt[0].y + lineH, number, strlen (number));
	  if (pt[3].y >= pt[0].y + lineH * 2)
	  {
	    sprintf (number, "%d", thisP->botLL - 1);
	    gdk_draw_text (drawP, style->font, fileGC, 0, pt[3].y, number, strlen (number));
	  }

	  sprintf (number, "%d", thisP->topLR);
	  x = W - gdk_string_width (style->font, number);
	  if (x < 0)
	    x = 0;
	  gdk_draw_text (drawP, style->font, fileGC, x, pt[1].y + lineH, number, strlen (number));
	  if (pt[2].y >= pt[1].y + lineH * 2)
	  {
	    sprintf (number, "%d", thisP->botLR - 1);
	    x = W - gdk_string_width (style->font, number);
	    if (x < 0)
	      x = 0;
	    gdk_draw_text (drawP, style->font, fileGC, x, pt[2].y, number, strlen (number));
	  }
	}			/* end if in update region */
      }
      thisP = thisP->next;
    }
    gtk_widget_draw (drawA, &update_rect);
  }
  return TRUE;
}



static gint
expose_event (GtkWidget * widget, GdkEventExpose * event)
{
  gdk_draw_pixmap (widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)], drawP, event->area.x, event->area.y + H, event->area.x, event->area.y, event->area.width, event->area.height);

  return FALSE;
}



static void
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  xfdiff_abort (E_CLEANUP);
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
#ifdef __GTK_2_0
    /*  */
    gtk_menu_append (GTK_MENU_SHELL (parent), menuitem);
#else
    gtk_menu_append (GTK_MENU (parent), menuitem);
#endif
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
create_menu (GtkWidget * top)
{
  GtkWidget *menu, *menubar;
  int i;

  menubar = gtk_menu_bar_new ();
#ifdef __GTK_2_0
  /*  */
#else
  gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menubar), GTK_SHADOW_NONE);
#endif
  gtk_widget_show (menubar);
  for (i = 0; i < PATCH_MENUS; i++)
    patchM[i] = NULL;
  for (i = 0; i < DIFF_MENUS; i++)
    diffM[i] = NULL;
  /* Create "File" menu */
  menu = shortcut_menu (MENUBAR, menubar, _("File"), NULL, NULL);
  diffM[0] = shortcut_menu (SUBMENU, menu, _("Select left file"), (gpointer) cb_get_fileL, NULL);
  diffM[1] = shortcut_menu (SUBMENU, menu, _("Select right file"), (gpointer) cb_get_fileR, NULL);
  patchM[0] = shortcut_menu (SUBMENU, menu, _("Select patch file"), (gpointer) cb_get_fileP, NULL);
  patchM[1] = shortcut_menu (SUBMENU, menu, _("Select directory where to apply patch"), (gpointer) cb_get_fileD, NULL);

  shortcut_menu (EMPTY_SUBMENU, menu, NULL, NULL, NULL);
  diffM[2] = shortcut_menu (SUBMENU, menu, _("Switch to patch mode"),
			    /* new text 1.2.3 */
			    (gpointer) cb_toggle_mode, (gpointer) ((long) 1));
  patchM[2] = shortcut_menu (SUBMENU, menu, _("Switch to diff mode"),
			     /* new text 1.2.3 */
			     (gpointer) cb_toggle_mode, (gpointer) ((long) 1));

  shortcut_menu (EMPTY_SUBMENU, menu, NULL, NULL, NULL);
  shortcut_menu (SUBMENU, menu, _("Exit"), (gpointer) delete_event, NULL);

  /* Create "Settings" menu */
  menu = shortcut_menu (MENUBAR, menubar, _("Diff Settings"), NULL, NULL);
/*  shortcut_menu(TOGGLENOT,menu,_("Show diagnostics"),
		  (gpointer)cb_toggle_diagnostics, (gpointer) silent);*/
  shortcut_menu (SUBMENU, menu, _("Font"), (gpointer) cb_select_font, NULL);
  shortcut_menu (SUBMENU, menu, _("Highlight text color"), (gpointer) cb_select_colors, (gpointer) ((long) 0));
  shortcut_menu (SUBMENU, menu, _("Highlight background color"), (gpointer) cb_select_colors, (gpointer) ((long) 1));
  shortcut_menu (TOGGLE, menu, _("Fill polygons"), (gpointer) cb_toggle_filledP, (gpointer) filledP);
  shortcut_menu (TOGGLE, menu, _("Show line numbers"), (gpointer) cb_toggle_show_lineN, (gpointer) show_lineN);

  shortcut_menu (TOGGLE, menu, _("Use line synchronization voids"),
		 /* new text 1.2.3 */
		 (gpointer) cb_toggle_synchronize, (gpointer) synchronize);

#ifndef AUTOSAVE
  shortcut_menu (EMPTY_SUBMENU, menu, NULL, NULL, NULL);
  shortcut_menu (SUBMENU, menu, _("Save settings as default"), (gpointer) cb_save_defaults, NULL);
#endif
  menu = shortcut_menu (MENUBAR, menubar, _("Patch Settings"), NULL, NULL);
#ifdef GNU_PATCH
  shortcut_menu (TOGGLE, menu, _("Verbose patch diagnostics"), (gpointer) cb_toggle_verbose, (gpointer) verbose);
#endif
  diffM[3] = shortcut_menu (SUBMENU, menu, _("Patch options only active in patch-mode"),
			    /* new text 1.2.3 */
			    NULL, NULL);

  /* watchout, REVERSE_TOGGLE is #define patchM[whatever] at xfdiff.h */
  patchM[3] = shortcut_menu (TOGGLE, menu, _("Assume patch is reversed"), (gpointer) cb_toggle_reversed, (gpointer) reversed);

  patchM[4] = shortcut_menu (TOGGLE, menu, _("Auto strip level"),
			     /* new text 1.2.3 */
			     (gpointer) cb_toggle_autostrip, (gpointer) autostrip);

  {
    GSList *group = NULL;
    gint i;
    for (i = 0; i < 10; i++)
    {
      char s[48];
      sprintf (s, "Strip level %d (-p%d)", i, i);
      stripM[i] = gtk_radio_menu_item_new_with_label (group, s);
      group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (stripM[i]));
      if (i == strip)
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (stripM[i]), TRUE);
#ifdef __GTK_2_0
      /*  */
      gtk_menu_append (GTK_MENU_SHELL (menu), stripM[i]);
#else
      gtk_menu_append (GTK_MENU (menu), stripM[i]);
#endif
      if (!autostrip)
	gtk_widget_show (stripM[i]);
      gtk_signal_connect (GTK_OBJECT (stripM[i]), "activate", GTK_SIGNAL_FUNC (cb_radio_strip), (gpointer) ((long) i));

    }
  }

  /* Create "Actions" menu */
  menu = shortcut_menu (MENUBAR, menubar, _("Actions"), NULL, NULL);
  diffM[4] = shortcut_menu (SUBMENU, menu, _("View differences"), (gpointer) cb_do_diff, (gpointer) ((int) 1));
  diffM[5] = shortcut_menu (SUBMENU, menu, _("View differences in new"), (gpointer) cb_do_diff_new, (gpointer) ((int) 1));

  patchM[5] = shortcut_menu (SUBMENU, menu, _("View patch created differences"),
			     /* 1.2.3 correction, was: "Do patch" */
			     (gpointer) cb_do_patch, (gpointer) ((int) 1));
  patchM[6] = shortcut_menu (SUBMENU, menu, _("View patch created differences in new"),
			     /* 1.2.3 correction, was: "Do patch in new" */
			     (gpointer) cb_do_patch_new, (gpointer) ((int) 1));
#ifdef __GNUC__
  diffM[6] = shortcut_menu (EMPTY_SUBMENU, menu, NULL, NULL, NULL);
  diffM[7] = shortcut_menu (SUBMENU, menu, _("Create patch file"), (gpointer) cb_patchmake, (gpointer) ((int) 0));
/* obsolete:  
  patchM[7]=shortcut_menu(SUBMENU,menu,_("Create patch file for full directory"),
		  (gpointer)cb_patchmake,(gpointer)((int) 1));*/
#endif

  patchM[7] = shortcut_menu (EMPTY_SUBMENU, menu, NULL, NULL, NULL);
  patchM[8] = shortcut_menu (SUBMENU, menu, _("Apply patch to Hard Drive"),
			     /* 1.2.3 correction, was: "Apply patch" */
			     (gpointer) cb_patchapply, (gpointer) ((int) 0));
  patchM[9] = shortcut_menu (SUBMENU, menu, _("Undo patch to Hard Drive"),
			     /* 1.2.3 correction, was: "Undo patch" */
			     (gpointer) cb_patchapply, (gpointer) ((int) 1));

  /* Create "Help" menu */
  menu = shortcut_menu (RIGHT_MENU, menubar, _("Help"), NULL, NULL);
  shortcut_menu (SUBMENU, menu, _("About"), (gpointer) cb_about, NULL);

  /* hide and show whatever */
  hideshow_menus ();
  return menubar;
}

static void
on_clear_show_diag (GtkWidget * widget, gpointer data)
{
#ifdef __GTK_2_0
  void clear_text_buffer (GtkTextBuffer * buffer);
  clear_text_buffer ((GtkTextBuffer *) data);
#else
  guint lg;
  lg = gtk_text_get_length (GTK_TEXT (text));
  gtk_text_backward_delete (GTK_TEXT (text), lg);
#endif
}

void
show_diag (gchar * message)
{
  if ((!message) || (!strlen (message)))
    return;

#ifdef __GTK_2_0
  utf8_insert (text, message, -1);
#else
  gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, message, strlen (message));
#endif
  return;
}


static GtkWidget *
shortcut_button (GtkWidget * box, char *txt, gpointer func, gpointer data)
{
  static GtkWidget *button = NULL;
  button = gtk_button_new_with_label (txt);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (func), (gpointer) data);
  if (box != NULL)
  {
/*	  gtk_container_add (GTK_CONTAINER (box), button);*/
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);
  }
  return button;
}

GtkWidget *
create_diff_window (void)
{
  GtkWidget *vbox, *vbox1, *vbox1a, *vbox1b, *vbox2;
  GtkWidget *hbox, *menutop, *handlebox;
  GtkWidget *bbox, *vscrollbar, *button;

  GtkWidget *view, *viewL, *vpaned, *scrolled_window;
#ifdef __GTK_2_0
  GtkWidget *scrolled_windowR, *scrolled_windowL;
#endif


  get_defaults ();
  diff = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (diff), "destroy", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (diff));
  gtk_signal_connect (GTK_OBJECT (diff), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (diff));
/* menu */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (diff), vbox);
  gtk_widget_show (vbox);

  handlebox = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handlebox, FALSE, FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (handlebox), 2);
  gtk_widget_show (handlebox);

  menutop = create_menu (diff);
  gtk_container_add (GTK_CONTAINER (handlebox), menutop);

  gtk_widget_show (menutop);

  gtk_window_position (GTK_WINDOW (diff), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (diff), "Xfdiff");
  gtk_widget_realize (diff);

/* box arrangement */
  vpaned = gtk_vpaned_new ();
  gtk_widget_ref (vpaned);
  gtk_object_set_data (GTK_OBJECT (diff), "vpaned", vpaned);
  gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
  gtk_widget_show (vpaned);

  bbox = gtk_hbox_new (FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (vpaned), bbox, TRUE, TRUE);
/*  gtk_box_pack_start (GTK_BOX (vbox),bbox ,TRUE, TRUE, 0);*/
  gtk_widget_show (bbox);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bbox), vbox1, TRUE, TRUE, 0);
  gtk_widget_show (vbox1);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bbox), vbox2, FALSE, TRUE, 0);
  gtk_widget_show (vbox2);

  vbox1a = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox1a, FALSE, TRUE, 0);
  gtk_widget_show (vbox1a);

  vbox1b = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox1b, TRUE, TRUE, 0);
  gtk_widget_show (vbox1b);

  adj = gtk_adjustment_new (0, 0, 0, 0, 0, 0);
  adjR = gtk_adjustment_new (0, 0, 0, 0, 0, 0);
  vscrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));

  gtk_box_pack_start (GTK_BOX (vbox2), vscrollbar, TRUE, TRUE, 0);
  gtk_widget_show (vscrollbar);

  /* invisible box : */

  patchbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1a), patchbox, TRUE, TRUE, 0);
  gtk_widget_show (patchbox);

  titleD = gtk_combo_new ();
  OKbutton = button = shortcut_button (NULL, _("OK"), (gpointer) cb_do_patch, (gpointer) ((int) 0));
#ifdef __GTK_2_0

  view = gtk_text_view_new ();

  /* this is the good place for lineH */
  set_the_style (titleD);
  lineH = titleD->style->font->ascent + titleD->style->font->descent;
  lineW = gdk_string_width (titleD->style->font, "W");

  titleP = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_view_set_cursor_visible ((GtkTextView *) view, FALSE);
  gtk_text_view_set_editable ((GtkTextView *) view, FALSE);
  gtk_text_view_set_wrap_mode ((GtkTextView *) view, GTK_WRAP_NONE);
  gtk_text_view_set_text_window_size ((GtkTextView *) view, sizeL * lineW, lineH + 5);

#else
  titleP = gtk_text_new (NULL, NULL);

  /* this is the good place for lineH */
  set_the_style (titleP);
  lineH = titleP->style->font->ascent + titleP->style->font->descent;
  lineW = gdk_string_width (titleP->style->font, "W");

  gtk_text_set_editable (GTK_TEXT (titleP), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (titleP), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (titleP), FALSE);
  gtk_widget_set_usize (titleP, sizeL * lineW, lineH + 5);
  view = titleP;
#endif

  gtk_widget_set_usize (titleD, sizeL * lineW, lineH + 5);
  gtk_widget_set_usize (OKbutton, DRAW_W, lineH + 5);
  gtk_box_pack_start (GTK_BOX (patchbox), titleD, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (patchbox), button, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (patchbox), view, TRUE, TRUE, 0);

  gtk_widget_show (view);
  gtk_widget_show (titleD);
  gtk_widget_show (button);

  /* text boxes : */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1b), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

#ifdef __GTK_2_0
  /* viewL should be using horiz-adjust adj!!!! */
  scrolled_windowL = gtk_scrolled_window_new (NULL, GTK_ADJUSTMENT (adj));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_windowL), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_widget_show (scrolled_windowL);

  viewL = gtk_text_view_new ();
  text_left = gtk_text_view_get_buffer (GTK_TEXT_VIEW (viewL));
  gtk_text_view_set_cursor_visible ((GtkTextView *) viewL, FALSE);
  gtk_text_view_set_editable ((GtkTextView *) viewL, FALSE);
  gtk_text_view_set_wrap_mode ((GtkTextView *) viewL, GTK_WRAP_NONE);
  gtk_text_view_set_text_window_size ((GtkTextView *) viewL, sizeL * lineW, sizeH * lineH);
  gtk_container_add (GTK_CONTAINER (scrolled_windowL), viewL);



  scrolled_windowR = gtk_scrolled_window_new (NULL, GTK_ADJUSTMENT (adjR));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_windowR), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_widget_show (scrolled_windowR);
  viewR = gtk_text_view_new ();
  text_right = gtk_text_view_get_buffer (GTK_TEXT_VIEW (viewR));
  gtk_text_view_set_cursor_visible ((GtkTextView *) viewR, FALSE);
  gtk_text_view_set_editable ((GtkTextView *) viewR, FALSE);
  gtk_text_view_set_wrap_mode ((GtkTextView *) viewR, GTK_WRAP_NONE);
  gtk_text_view_set_text_window_size ((GtkTextView *) viewR, sizeL * lineW, sizeH * lineH);
  gtk_container_add (GTK_CONTAINER (scrolled_windowR), viewR);

  drawA = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawA, DRAW_W, sizeH * lineH);

  gtk_box_pack_start (GTK_BOX (hbox), scrolled_windowL, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), drawA, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_windowR, TRUE, TRUE, 0);

  gtk_widget_show (scrolled_windowL);
  gtk_widget_show (viewL);
  gtk_widget_show (drawA);
  gtk_widget_show (viewR);
  gtk_widget_show (scrolled_windowR);

#else
  viewL = text_left = gtk_text_new (NULL, GTK_ADJUSTMENT (adj));
  if (style->font != NULL)
    gtk_widget_set_style (text_left, style);
  gtk_widget_set_usize (text_left, sizeL * lineW, sizeH * lineH);
  gtk_text_set_editable (GTK_TEXT (text_left), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text_left), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text_left), FALSE);

  viewR = text_right = gtk_text_new (NULL, NULL);
  if (style->font != NULL)
    gtk_widget_set_style (text_right, style);
  gtk_widget_set_usize (text_right, sizeR * lineW, sizeH * lineH);
  gtk_text_set_editable (GTK_TEXT (text_right), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text_right), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text_right), FALSE);

  drawA = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawA, DRAW_W, sizeH * lineH);

  gtk_box_pack_start (GTK_BOX (hbox), viewL, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), drawA, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), viewR, TRUE, TRUE, 0);

  gtk_widget_show (viewL);
  gtk_widget_show (drawA);
  gtk_widget_show (viewR);
#endif



/* diagnostics */
  bbox = gtk_vbox_new (TRUE, 0);
  gtk_paned_pack2 (GTK_PANED (vpaned), bbox, TRUE, TRUE);
  gtk_widget_show (bbox);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (bbox), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
#ifdef __GTK_2_0
				  GTK_POLICY_AUTOMATIC,
#else
				  GTK_POLICY_NEVER,
#endif
				  GTK_POLICY_ALWAYS);
  gtk_widget_show (scrolled_window);
#ifdef __GTK_2_0
  {
    GtkWidget *view;
    view = gtk_text_view_new ();
    text = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_view_set_cursor_visible ((GtkTextView *) view, FALSE);
    gtk_text_view_set_editable ((GtkTextView *) view, FALSE);
    gtk_text_view_set_wrap_mode ((GtkTextView *) view, GTK_WRAP_NONE);
    gtk_widget_show (view);
    gtk_container_add (GTK_CONTAINER (scrolled_window), view);
  }
#else
  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (text), TRUE);
  gtk_widget_show (text);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text);
#endif


/* another visible/invisible box */
  patch_label_box = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_usize (patch_label_box, 2 * sizeL * lineW + DRAW_W, lineH + 5);
  gtk_box_pack_start (GTK_BOX (vbox), patch_label_box, FALSE, TRUE, 0);
  gtk_widget_show (patch_label_box);

#ifdef __GTK_2_0
  viewW = gtk_text_view_new ();
  what_dir = gtk_text_view_get_buffer (GTK_TEXT_VIEW (viewW));
  gtk_text_view_set_cursor_visible ((GtkTextView *) viewW, FALSE);
  gtk_text_view_set_editable ((GtkTextView *) viewW, FALSE);
  gtk_text_view_set_wrap_mode ((GtkTextView *) viewW, GTK_WRAP_NONE);
#else
  viewW = what_dir = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (what_dir), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (what_dir), FALSE);
  gtk_text_set_line_wrap (GTK_TEXT (what_dir), FALSE);
#endif

  gtk_box_pack_start (GTK_BOX (patch_label_box), viewW, TRUE, TRUE, 0);
  gtk_widget_show (viewW);
  if (!patching)
    gtk_widget_hide (patch_label_box);



/* last box on bottom */
  bbox = gtk_hbox_new (TRUE, 0);
/*  bbox = gtk_hbutton_box_new ();*/
  /* gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END); */
#ifdef __GTK_2_0
  /*  */
#else
  /* gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5); */
#endif
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, TRUE, 0);
  gtk_widget_show (bbox);

  button = shortcut_button (bbox, _("Clear"), (gpointer) on_clear_show_diag, (gpointer) text);

/*  gtk_button_new_with_label (_("Clear"));
  gtk_box_pack_end (GTK_BOX (bbox),button,TRUE,TRUE , 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_clear_show_diag),
		      (gpointer) text);*/

  button = shortcut_button (bbox, _("Next difference"), (gpointer) cb_next_diff, (gpointer) ((long) 1));
  gtk_widget_grab_default (button);

  button = shortcut_button (bbox, _("Prev difference"), (gpointer) cb_next_diff, (gpointer) ((long) 0));
  button = shortcut_button (bbox, _("Next file"), (gpointer) cb_next_file, (gpointer) ((long) 1));
  button = shortcut_button (bbox, _("Previous file"), (gpointer) cb_next_file, (gpointer) ((long) 0));

  button = shortcut_button (bbox, _("Exit"), (gpointer) delete_event, NULL);

  /* gtk_widget_set_events (drawA,GDK_EXPOSURE_MASK); */

  gtk_drag_dest_set (viewL, GTK_DEST_DEFAULT_ALL, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);
  gtk_drag_dest_set (viewR, GTK_DEST_DEFAULT_ALL, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);

  gtk_signal_connect (GTK_OBJECT (viewL), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (viewR), "drag_data_received", GTK_SIGNAL_FUNC (on_drag_data_received), (gpointer) NULL);

#ifdef __GTK_2_0
  /* this hack doesnt work with gtk+1.3 */
#else
  gtk_signal_connect (GTK_OBJECT (viewR), "button_press_event", GTK_SIGNAL_FUNC (cb_wheel), (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (viewL), "button_press_event", GTK_SIGNAL_FUNC (cb_wheel), (gpointer) NULL);
#endif


  gtk_signal_connect (GTK_OBJECT (diff), "delete_event", GTK_SIGNAL_FUNC (delete_event), (gpointer) GTK_WIDGET (diff));
  gtk_signal_connect (GTK_OBJECT (drawA), "configure_event", (GtkSignalFunc) configure_event, (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (drawA), "expose_event", (GtkSignalFunc) expose_event, (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed", GTK_SIGNAL_FUNC (cb_adjust), (gpointer) adj);
  set_icon (diff, "Xfdiff", xfdiff_xpm);
  gtk_widget_show (diff);
  set_highlight ();


  return diff;

}
