/*    xfdiff.c   */

/*  xfdiff (a gtk frontend for diff+patch)
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

/* Description:
 * 
 * XFdiff is a module for XFtree (XFCE project by Olivier Fourdan, http://www.xfce.org)
 * This module may be run from XFtree or independently, with command line
 * options (use: xfdiff -h to get a summary).
 *
 * XFdiff does no heavy processing. All it does is allow a fast and easy access
 * to the standard utilities diff and patch. Quoting Olivier: "the goal [for XFCE] 
 * is keep most system resources for the applications, and not to consume all memory 
 * and CPU usage with the desktop environment."
 * 
 * This program will open a window with two text widgets and display two files, 
 * highlighting the differences. Buttons permit rapid navigation through the 
 * diffences of the two files. Among the options, you may toggle line numbers, 
 * change highlight color, and font, toggle verbose output and other
 * stuff.
 * 
 * XFdiff can also open patch files (created in unified format) and display in
 * the same manner the differences. Buttons and a drop-down list permit rapid 
 * navigation through every file that is modified by the patch.
 * Among the options, you can specify strip level, whether or not
 * a patch file is reversed, apply or undo a patch (and for GNUC only, create a 
 * patch file of your own).
 *
 * XFdiff uses the standard libs from XFCE, GTK+, and modules from XFtree
 * and XFclock.
 * 

 
*/
#define __XFDIFF__
#include "xfdiff.h"
#include "xfdiff_diag.h"
#include "xfdiff_misc.h"
#undef EXTERN
#include "../xfglob/globber.h"


static struct stat patch_st;

static char *cannot_read_patch_file, *identical_files, *patch_file_error, *empty_file;

#ifdef __GTK_2_0
void
utf8_insert_blue (GtkTextBuffer * buffer, char *line, int linecount)
{
/* this puts everything at the end, no blue color  GtkTextIter iter;
  gtk_text_buffer_get_end_iter (buffer,&iter);
  gtk_text_buffer_insert_with_tags_by_name 
	  (buffer,&iter,line,linecount,"bluetext",NULL);*/
  gtk_text_buffer_insert_at_cursor (buffer, line, linecount);
}

void
utf8_insert (GtkTextBuffer * buffer, char *line, int linecount)
{
  if (!g_utf8_validate (line, linecount, NULL))
  {
    gchar *s = NULL;
    gint s_len = 0;
    /*printf("utf8: linecount=%d\n",linecount); */
    s = g_locale_to_utf8 (line, linecount, NULL, &s_len, NULL);
    gtk_text_buffer_insert_at_cursor (buffer, s, s_len);
    g_free (s);			/* is this correct ? */
  }
  else
    gtk_text_buffer_insert_at_cursor (buffer, line, linecount);
}

void
utf8_file_insert (GtkTextBuffer * buffer, char *line, int linecount)
{
  if (!g_utf8_validate (line, linecount, NULL))
  {
    gchar *s = NULL;
    gint s_len = 0;
    /*printf("utf8: linecount=%d\n",linecount); */
    s = g_filename_to_utf8 (line, linecount, NULL, &s_len, NULL);
    gtk_text_buffer_insert_at_cursor (buffer, s, s_len);
    g_free (s);			/* is this correct ? */
  }
  else
    gtk_text_buffer_insert_at_cursor (buffer, line, linecount);
}
#endif

static void
xfdiff_init (void)
{
/* convenience location of duplicate dialog texts  */
  /* non-static */
  strip_set_to = N_("Strip level now set to ");
  no_patch = N_("No patch file has been selected.");
  no_patch_dir = N_("No directory to apply patch has been selected.");
  no_left_path = N_("No left path has been selected.");
  no_right_path = N_("No right path has been selected.");
  /* static */
  cannot_read_patch_file = N_("Patch file can not be opened for read!");
  identical_files = N_("Files are identical\n(or binary files that may or may not differ)");
  patch_file_error = N_("Patch file is not in unified format");
  empty_file = N_("****Empty file****\n");
/* initial sizes for text files, sizes in line numbers and average char length */
  sizeR = 30, sizeL = 30, sizeH = 30;
  head = NULL, current = NULL;
  headF = NULL, currentF = NULL;
  reversed = strip = 0;
  style = NULL;
  xfce_style = NULL;
  the_font = NULL;
  diff = OKbutton = NULL;
  drawA = NULL;
  titleD = NULL;
  what_dir = text_right = text_left = NULL;
  titleR = titleL = NULL;
  titleP = NULL;
  diffbox = patchbox = NULL;
  patch_label_box = NULL;
  done_str = (char *) malloc (128);
  if (!done_str)
    xfdiff_abort (E_MALLOC);
  drawP = NULL;
  adj = NULL;
  lineW = lineH = rightC = leftC = 0;
  drawGC = NULL;
  fileO = fileRR = fileI = patchO = NULL;
  fileRD = fileLD = fileR = fileL = fileP = fileD = NULL;
  /* not initialized: colorfg,colorbg; */
  fuente = DEFAULTFONT;
  silent = 0;
  patching = 0;
  applying_patch = 0;
  autostrip = 1;
}

/* memcpy is necesary because patch file may contain binary data */
#ifdef __GNUC__
/* memcpy is a GNU extension.*/
#define MEMCPY memcpy
#else
/* memcpy is a GNU extension.*/
static void *
MEMCPY (void *dest, const void *src, size_t n)
{
  char *destC, *srcC;
  size_t i;

  destC = (char *) dest;
  srcC = (char *) src;
  for (i = 0; i < n; i++)
    destC[i] = srcC[i];
  return dest;
}
#endif
static void
assign_tmp_files (void)
{
  char pidT[32];
  char *tmp_dir = NULL;

  /* TMPDIR processing */

  if (!tmp_dir)
    tmp_dir = getenv ("TMPDIR");
  if (!tmp_dir)
    tmp_dir = getenv ("TMP");
  if (!tmp_dir)
    tmp_dir = getenv ("TEMP");
  if (!tmp_dir)
    tmp_dir = TMP_DIR;

  sprintf (pidT, "%x", (int) getpid ());
  if (fileO)
  {
    remove (fileO);
    free (fileO);
  }
  fileO = (char *) malloc (strlen (tmp_dir) + 2 + strlen ("xfdiffR.") + strlen (pidT));
  if (!fileO)
    xfdiff_abort (E_MALLOC);
  sprintf (fileO, "%s/%s%s", tmp_dir, "xfdiffR.", pidT);

  if (fileI)
  {
    remove (fileI);
    free (fileI);
  }
  fileI = (char *) malloc (strlen (tmp_dir) + 2 + strlen ("xfdiffL.") + strlen (pidT));
  if (!fileI)
    xfdiff_abort (E_MALLOC);
  sprintf (fileI, "%s/%s%s", tmp_dir, "xfdiffL.", pidT);

  if (fileRR)
  {
    remove (fileRR);
    free (fileRR);
  }
  fileRR = (char *) malloc (strlen (tmp_dir) + 2 + strlen ("xfdiff.rej.") + strlen (pidT));
  if (!fileRR)
    xfdiff_abort (E_MALLOC);
  sprintf (fileRR, "%s/%s%s", tmp_dir, "xfdiff.rej.", pidT);
}

static patched_file *
pushF (char *file, int offset, int length)
{
  if (!currentF)
  {
    headF = (patched_file *) malloc (sizeof (patched_file));
    if (!headF)
      xfdiff_abort (E_MALLOC);
    currentF = headF;
    currentF->previous = NULL;
  }
  else
  {
    currentF->next = (patched_file *) malloc (sizeof (patched_file));
    if (!currentF->next)
      xfdiff_abort (E_MALLOC);
    (currentF->next)->previous = currentF;
    currentF = currentF->next;

  }
  currentF->next = NULL;
  currentF->offset = offset;
  currentF->length = length;
  currentF->diffC = 0;
  currentF->file = currentF->newfile = NULL;

  currentF->file = assign (currentF->file, file);
  return currentF;
}

/* functions for creating the file list on dir-dir diff: */
int
pushemL (char *file)
{
  char *path;
  pushF (file, 0, 0);

  path = file + strlen (fileL);
  if (path[0] == '/')
    path++;
  currentF->newfile = (char *) malloc (strlen (fileR) + strlen (path) + 2);
  if (!currentF->newfile)
    xfdiff_abort (E_MALLOC);
  sprintf (currentF->newfile, "%s/%s", fileR, path);


  /*printf("%s -- %s\n",currentF->file,currentF->newfile); */
  return 0;
}

/*
int pushemR(char *file){
	char *tmp_d,*tmp_e;
	patched_file *thisF;
	tmp_d=strip_it(file,max_strip(file));
	thisF=headF;
	while (thisF){
		tmp_e=strip_it(thisF->file,max_strip(thisF->file));
		if (strcmp(tmp_d,tmp_e)==0) return 0; 
		thisF = thisF->next;
	}
	pushF(file,0,0);
	currentF->newfile=currentF->file;
	currentF->file=(char *)malloc(strlen(fileL)+strlen(tmp_d)+2);
	if (!currentF->file) xfdiff_abort(E_MALLOC);			
	sprintf(currentF->file,"%s/%s",fileL,tmp_d);
//	printf("%s\n",file);
	return 0;
}*/

static int
checkfile (char *file)
{
  FILE *check;
  if (!file)
    return FALSE;
  if ((check = fopen (file, "r")) == NULL)
    return FALSE;
  fclose (check);
  return TRUE;
}

#ifdef __GTK_2_0
static void
writebinary (GtkTextBuffer * text_location)
{
  // missing color attribute
  utf8_insert (text_location, _("****Binary data****\n"), -1);
}
#else
static void
writebinary (GtkWidget * text_location)
{
  gtk_text_insert (GTK_TEXT (text_location), style->font, &filefg, &filebg, _("****Binary data****\n"), strlen (_("****Binary data****\n")));
}
#endif

#ifdef __GTK_2_0
static void
writeout (GtkTextBuffer * text_location, char *file)
{
  int fileH;
  fileH = open (file, O_RDONLY);
  if (fileH == -1)
  {
    utf8_insert (text_location, _(empty_file), -1);
#else
static void
writeout (GtkWidget * text_location, char *file)
{
  int fileH;
  fileH = open (file, O_RDONLY);
  if (fileH == -1)
  {
    gtk_text_insert (GTK_TEXT (text_location), style->font, &filefg, &filebg, _(empty_file), strlen (_(empty_file)));
#endif
  }
  else
  {
    char *blok;
    ssize_t blok_s;
    int i;
    blok = (char *) malloc (BLOK_SIZE);
    if (!blok)
      xfdiff_abort (E_MALLOC);
    while ((blok_s = read (fileH, (void *) blok, BLOK_SIZE)) > 0)
    {
      for (i = 0; i < blok_s; i++)
      {
	if (((blok[i] < 9) && (blok[i] >= 0)) || ((blok[i] < 32) && (blok[i] > 13)))
	{
	  writebinary (text_location);
	  free (blok);
	  close (fileH);
	  return;
	}
      }
#ifdef __GTK_2_0
      utf8_insert (text_location, blok, blok_s);
#else
      gtk_text_insert (GTK_TEXT (text_location), style->font, NULL, NULL, blok, blok_s);
#endif

    }
    free (blok);
    close (fileH);
  }
}

/* spot 3/3 where fileR,fileL come in */
static int
pre_diff ()
{
  if (!fileL)
  {
    fileL = prompt_path (_(no_left_path), fileL);
    if (!fileL)
      return FALSE;
    fileLD = assign (fileLD, checkdir (fileL) ? fileL : NULL);
    update_titlesP ();
  }
  if (!fileR)
  {
    fileR = prompt_path (_(no_right_path), fileR);
    if (!fileR)
      return FALSE;
    fileRD = assign (fileRD, checkdir (fileR) ? fileR : NULL);
    update_titlesP ();
  }

  if (!patchO)
  {				/* not creating a patchfile */
    if (checkdir (fileL) && checkdir (fileR))
    {
      /* initialization for globber */
      void *glob_object;
      glob_object = globber_create ();
      if (my_yesno_dialog (_("You have selected a directory.\nShould subdirectories be included?")))
	glob_set_options (glob_object, GLOBBER_RECURSIVE);
      glob_set_options (glob_object, GLOBBER_TYPE);
      glob_set_type (glob_object, S_IFREG);
      cleanF ();
      globber (glob_object, fileL, pushemL, "*");
      /* now we no longer need the original fileL and fileR */
      currentF = headF;
      if (!currentF)
	return FALSE;
      fileL = assign (fileL, currentF->file);
      fileR = assign (fileR, currentF->newfile);
    }

    if (checkdir (fileL) || checkdir (fileR))
    {
      static char *tmp_c = NULL;
      char *tmp_d;
      /* one of the selections is a directory. Prepare any-diff for the way gnu-diff would react. */
      if (checkdir (fileL))
      {
	tmp_d = strip_it (fileR, max_strip (fileR));
	tmp_c = (char *) malloc (strlen (fileL) + strlen (tmp_d) + 2);
	if (!tmp_c)
	  xfdiff_abort (E_MALLOC);
	sprintf (tmp_c, "%s/%s", fileL, tmp_d);
	if (fileL)
	  free (fileL);
	fileL = tmp_c;
	tmp_c = NULL;
      }
      else
      {
	tmp_d = strip_it (fileL, max_strip (fileL));
	tmp_c = (char *) malloc (strlen (fileR) + strlen (tmp_d) + 2);
	if (!tmp_c)
	  xfdiff_abort (E_MALLOC);
	sprintf (tmp_c, "%s/%s", fileR, tmp_d);
	if (fileR)
	  free (fileR);
	fileR = tmp_c;
	tmp_c = NULL;
      }
      /* now continue using fileR and fileL gnu-diff interpreted */

    }
    cleanP ();			/* clean difference polygons */
    cleanA ();
    cleanT ();			/* clean display area. */
    update_titlesP ();
    /* Case of void or unreadable files will be dealt with here. */
    if ((!checkfile (fileL)) || (!checkfile (fileR)))
    {
      writeout (text_left, fileL);
      writeout (text_right, fileR);
      return FALSE;
    }
  }				/* end if !creating patchfile */

  if (!silent)
  {
    if (currentF)
      show_diag (currentF->file);
    else
      show_diag (fileL);
    show_diag (_(" versus "));
    if (currentF)
      show_diag (currentF->newfile);
    else
      show_diag (fileR);
    show_diag (":\n");
  }
  return TRUE;
}

static void
post_diff (int diffC)
{
  if (patchO)
    return;
  if (!silent)
  {
    if (diffC)
    {
      char *texto;		/* 10 is size of MAXINT of 32 bits: */
      texto = (char *) malloc (strlen (_("different sections found")) + 12);
      if (!texto)
	xfdiff_abort (E_MALLOC);
      /* 10 is size of MAXINT of 32 bits */
      sprintf (texto, "%d %s", diffC, _("different sections found"));
      show_diag (texto);
      free (texto);
    }
    else
      show_diag (_(identical_files));
    show_diag ("\n******************************\n");
  }
}

static void
pad_txt (int local_leftC, int local_rightC)
{
  int i;
  if (local_rightC > local_leftC)
    for (i = local_leftC; i < local_rightC; i++)
    {
      leftC++;
#ifdef __GTK_2_0
      utf8_insert (text_left, "\n", -1);
#else
      gtk_text_insert (GTK_TEXT (text_left), style->font, NULL, NULL, "\n", strlen ("\n"));
#endif
    }
  else
    for (i = local_rightC; i < local_leftC; i++)
    {
      rightC++;
#ifdef __GTK_2_0
      utf8_insert (text_right, "\n", -1);
#else
      gtk_text_insert (GTK_TEXT (text_right), style->font, NULL, NULL, "\n", strlen ("\n"));
#endif
    }
}


static void
synchronize_txt (void)
{
  char *texto = "***Synchronization void***\n";
  if (rightC == leftC)
    return;
#ifdef __GTK_2_0
  utf8_insert_blue ((rightC < leftC) ? text_right : text_left, texto, -1);
#else
  gtk_text_insert (GTK_TEXT ((rightC < leftC) ? text_right : text_left), style->font, &filefg, &filebg, texto, strlen (texto));
#endif
  (rightC < leftC) ? rightC++, addedR++ : leftC++, addedL++;
  if (rightC == leftC)
    return;
  while (rightC != leftC)
  {
#ifdef __GTK_2_0
    utf8_insert_blue ((rightC < leftC) ? text_right : text_left, "*\n", -1);
#else
    gtk_text_insert (GTK_TEXT ((rightC < leftC) ? text_right : text_left), style->font, &filefg, &filebg, "*\n", strlen ("*\n"));
#endif
    (rightC < leftC) ? rightC++, addedR++ : leftC++, addedL++;
  }
}

int
do_diff (void)
{
  static int pfd[2];
  int diffC = 0;
  char pidT[32];
  int recursive_patchmake = FALSE, all_files = FALSE;
  static char *directive;

  sprintf (pidT, "%d", (int) getpid ());
  if (directive)
    free (directive);
  directive = (char *) malloc (strlen ("__xfdiff_directive__") + strlen (pidT) + 1);
  if (!directive)
    xfdiff_abort (E_MALLOC);
  sprintf (directive, "__xfdiff_directive__%s", pidT);

  if (!pre_diff ())
    return FALSE;

  if (checkdir (fileR) && checkdir (fileL))
  {
    if (my_yesno_dialog (_("Include binary files?")))
      all_files = TRUE;
    if (my_yesno_dialog (_("Include subdirectories?")))
      recursive_patchmake = TRUE;
  }
  addedR = addedL = 0;
  show_diag (_("Working diff...\n"));
  /* open the pipe */
  if (pipe (pfd) < 0)
  {
    perror ("pipe");
    return FALSE;
  }

  sprintf (done_str, "XFDIFF DONE (pid=%d)\n", (int) getpid ());
  if (fork ())
  {				/*  parent will fork to diff  */
    int status;
    dup2 (pfd[1], 1);		/* assign parent stdout to pipe */
    close (pfd[0]);		/* not used by parent */

    if (fork () == 0)
    {				/* forks again, to wait for completion */
      char *arguments[10];
      int argc = 0;
      arguments[argc++] = DIFF;
      if (patchO)
      {
	arguments[argc++] = "-u";
	arguments[argc++] = "-N";
	if (all_files)
	  arguments[argc++] = "-a";
	if (recursive_patchmake)
	  arguments[argc++] = "-r";
      }
      else
      {
	arguments[argc++] = "-D";
	arguments[argc++] = directive;
      }
      arguments[argc++] = fileL;
      arguments[argc++] = fileR;
      arguments[argc++] = (char *) 0;
      execvp (DIFF, arguments);
      perror ("exec");
      _exit (127);		/* parent never gets here (hopefully) */
    }

    /* the parent waits for diff to finish */
    /*fprintf(stderr,"waiting\n"); */
    wait (&status);
    fflush (NULL);
    fprintf (stdout, "%s", done_str);

    sleep (1);			/* this sleep is to avoid a "race condition" */
    fflush (NULL);		/* must flush so first child can continue */
    /*fprintf(stderr,"exiting subprocesses\n"); */
    _exit (1);
  }

  /* meanwhile, the first child is busy reading the pipe */
  gdk_window_set_cursor (diff->window, cursor);
  gdk_flush ();
#ifdef __GTK_2_0
  /* freezing improves speed of application */
#else
  gtk_text_freeze (GTK_TEXT (text_right));
  gtk_text_freeze (GTK_TEXT (text_left));
#endif
  {
    char *line, *lineE, *input;
    int lineL = 256, linecount = 0, byrice, binary_line = 0, binary_section = 0;
    int right = 1, left = 1, topR = 0, topL = 0, botR = 0, botL = 0;
    FILE *Opatch = NULL;	/* buffered, for better performance */

    if (patchO)
      Opatch = fopen (patchO, "w");
    /* file write has been tested previously! */
    rightC = leftC = 0;
    lineH = viewR->style->font->ascent + viewR->style->font->descent;
    input = line = (char *) malloc (lineL);
    if (!input)
      xfdiff_abort (E_MALLOC);
    while (1)
    {
      if (!read (pfd[0], input, 1))
	break;
      input[1] = 0;
      linecount++;
      if (input[0] == '\n')
      {
	/*printf("?:%s:%s",done_str,line); */
	byrice = 0;
	if (binary_line)
	{			/* ignore binary lines */
	  if (!binary_section)
	  {
	    binary_section = 1;
	    if (right)
	    {
	      rightC++;
	      writebinary (text_right);
	    }
	    if (left)
	    {
	      leftC++;
	      writebinary (text_left);
	    }
	  }
	  binary_line = 0;
	  input = line;
	  linecount = 0;
	  continue;
	}
	binary_section = 0;	/* by now it is not binary */
	if (strncmp (line, done_str, strlen (done_str)) == 0)
	  break;
	if (patchO)
	{
	patchproceed:
	  /*printf("line:%s",line); */
	  fwrite (line, 1, linecount, Opatch);
	  input = line;
	  linecount = 0;
	  continue;
	}
	/* process precompiler directives */
	if ((strncmp (line, "#ifdef", strlen ("#ifdef")) == 0) && (strstr (line, directive)))
	{			/* short circuit: it's a string */
	  linecount = 0;	/* bug caught using gtk+1.3 */
	  topR = rightC;
	  topL = leftC;
	  right = 1;
	  left = 0;
	  input = line;
	  diffC++;
	  /*printf("%s ifcount=%d iflevel=%d\n",lineW,ifcount,iflevel); */
	  continue;
	}
	if ((strncmp (line, "#ifndef", strlen ("#ifndef")) == 0) && (strstr (line, directive)))
	{
	  topR = rightC;
	  topL = leftC;
	  right = 0;
	  left = 1;
	  input = line;
	  diffC++;
	  linecount = 0;	/* bug caught using gtk+1.3 */
	  continue;
	}
	if ((strncmp (line, "#else", strlen ("#else")) == 0) && (strstr (line, directive)))
	{
	  linecount = 0;	/* bug caught using gtk+1.3 */
	  if (right)
	  {
	    topL = leftC;
	    right = 0;
	    left = 1;
	    input = line;
	    continue;
	  }
	  else
	  {
	    topR = rightC;
	    right = 1;
	    left = 0;
	    input = line;
	    continue;
	  }
	}
	if ((strncmp (line, "#endif", strlen ("#endif")) == 0) && (strstr (line, directive)))
	{
	  linecount = 0;	/* bug caught using gtk+1.3 */
	  botR = rightC;
	  botL = leftC;
	  right = left = 1;
	  input = line;
	  current = pushP (topR, botR, topL, botL);
	  if (synchronize)
	    synchronize_txt ();

	  /*printf("left=%d,%d  right=%d,%d\n",topL,botL,topR,botR); */
	  continue;
	}
      diff_proceed:
	/* write to apropriate text widget */
	if (right)
	{
	  if (!byrice)
	    rightC++;
#ifdef __GTK_2_0
	  utf8_insert (text_right, line, linecount);
#else
	  gtk_text_insert (GTK_TEXT (text_right), style->font, (left) ? NULL : &colorfg, (left) ? NULL : &colorbg, line, linecount);
#endif
	}
	if (left)
	{
	  if (!byrice)
	    leftC++;
#ifdef __GTK_2_0
	  utf8_insert (text_left, line, linecount);
#else
	  gtk_text_insert (GTK_TEXT (text_left), style->font, (right) ? NULL : &colorfg, (right) ? NULL : &colorbg, line, linecount);
#endif
	}
	input = line;
	linecount = 0;
	continue;
      }
      else if (!binary_line)
      {				/* some files have binary data imbedded: */
	if (((input[0] < 9) && (input[0] >= 0)) || ((input[0] < 32) && (input[0] > 13)))
	{
	  binary_line = 1;
	}
      }
      input++;
      if (linecount >= lineL - 2)
      {
	/*printf("reconfiguring because line size %d exceeded\n",lineL); */
	if (binary_line)
	{			/* don't bother with binary lines */
	  if (!binary_section)
	  {
	    binary_section = 1;
	    if (right)
	      writebinary (text_right);
	    if (left)
	      writebinary (text_left);
	  }
	  input = line;
	  linecount = 0;
	  continue;
	}
	if (patchO)
	  goto patchproceed;
	if (linecount >= 32768 - 2)
	{
	  byrice = 1;
	  goto diff_proceed;
	}

	lineE = line;
	line = (char *) malloc (lineL * 2);
	if (!line)
	  xfdiff_abort (E_MALLOC);

	MEMCPY (line, lineE, linecount);
	free (lineE);

	input = line + linecount;
	lineL *= 2;
      }
    }
    if (patchO)
      fclose (Opatch);
    else
    {
      if (leftC != rightC)
	pad_txt (leftC, rightC);
    }
    free (line);
  }
  close (pfd[0]);
  close (pfd[1]);		/* close pipes */
  /*printf("pipes closed\n"); */
  post_diff (diffC);

  /* reset to first difference and draw polygons */
#ifdef __GTK_2_0
  /* freezing improves speed of application */
#else
  gtk_text_thaw (GTK_TEXT (text_right));
  gtk_text_thaw (GTK_TEXT (text_left));
#endif
  gdk_window_set_cursor (diff->window, NULL);
  /* dynamic reconfiguration of polygon area */
  gtk_widget_set_usize (OKbutton, DRAW_W, lineH + 5);
  gtk_widget_set_usize (drawA, DRAW_W, lineH + 5);

  if (!patchO)
  {
    if (!first_diff ())
      my_show_message (_(identical_files));
    if (!patching)
      update_titlesP ();
  }

  return TRUE;

}



static int
build_tmpL (char *infile)
{
  int fileH, fileHI;
  static char *file;

  if (!infile)
    return FALSE;

  if (file)
    free (file);
  file = (char *) malloc (strlen (fileD) + strlen (infile) + 2);
  if (!file)
    xfdiff_abort (E_MALLOC);
  sprintf (file, "%s/%s", fileD, infile);

  fileHI = creat (fileI, S_IRUSR | S_IWUSR);
  if (fileHI == -1)
    xfdiff_abort (E_FILE);

  fileH = open (file, O_RDONLY);

/*     printf("preparing %s as %s\n",file,fileI);*/

  if (fileH == -1)
  {				/* a new file! */
    write (fileHI, (void *) _(empty_file), strlen (_(empty_file)));
  }
  else
  {
    ssize_t blok_s;
    void *blok;
    blok = malloc (BLOK_SIZE);
    if (!blok)
      xfdiff_abort (E_MALLOC);
    while ((blok_s = read (fileH, blok, BLOK_SIZE)) > 0)
    {
      write (fileHI, blok, blok_s);
    }
    free (blok);
  }
  close (fileH);
  close (fileHI);
  return TRUE;
}



/* max arguments to patch: !
 arg0 -d dir -o outfile -D directive -pNUM [-N | -R] (0)*/
#define MAX_PATCH_ARG 15

int
process_patch (patched_file * thisF)
{
  static int pfd[2];
  static int pfd2[2];
  char *argument[MAX_PATCH_ARG];
  int fileH, arglist = 0;
  char strip_level[16];
  char *file;
  int reversed_detected;

  if (!thisF)
    return FALSE;
  if (!thisF->file)
    return FALSE;
  if (!fileD)
    return FALSE;		/* insurance */
  reversed_detected = FALSE;
  /* check for changes in patchfile */
  {
    struct stat st;
    lstat (fileP, &st);
    if ((st.st_mtime != patch_st.st_mtime) || (st.st_ctime != patch_st.st_ctime))
    {
      my_show_message (_("Patch file has been modified by another process\nPlease reload\n"));
      cleanF ();
      cleanT ();
      cleanP ();
      cleanA ();
      free (fileP);
      fileP = NULL;
      return FALSE;
    }
  }
  if (strip)
    sprintf (strip_level, "-p%d", strip);
  else
    sprintf (strip_level, "-p0");

/*	printf("%s: offset=%d length=%d\n",thisF->file,thisF->offset,thisF->length);*/



  sprintf (done_str, "XFDIFF_PATCH DONE (pid=%d)\n", (int) getpid ());
  assign_tmp_files ();


  argument[arglist++] = PATCH;	/*0 */


  argument[arglist++] = "-d";	/*2 */
  argument[arglist++] = fileD;	/*3 */
  if (!applying_patch)
  {
    argument[arglist++] = "-o";	/*4 */
    argument[arglist++] = fileO;	/*5 */
  }
  argument[arglist++] = "-r";	/*6 */
  argument[arglist++] = fileRR;	/*7 */

  /* moving from -u to -D format is buggy in GNU patch! */

#ifdef GNU_PATCH
  if (verbose)
    argument[arglist++] = "--verbose";	/*8 */
#endif
  /* this should be user modify-able */
  argument[arglist++] = strip_level;	/*9 */
  if (reversed)
    argument[arglist++] = "-R";
  else
    argument[arglist++] = "-N";			/*10 *//*-N or -R */

  /* end of arguments */
  argument[arglist++] = (char *) 0;	/*11 */

  /* thisF->file address is no longer valid after forking, why? I dunno */
  file = (char *) malloc (strlen (fileD) + strlen (thisF->file) + 1);
  if (!file)
    xfdiff_abort (E_MALLOC);
  sprintf (file, "%s", thisF->file);

  /* open the pipe */
  if (pipe (pfd) < 0)
  {
    perror ("pipe");
    return FALSE;
  }

  /* for the sake of clarity, dup2 commands will be placed 
   * without optimization: let -O2 take care of that */

  if (fork ())
  {				/*  parent will fork to patch  */
    int status;
    if (pipe (pfd2) < 0)
    {
      perror ("pipe2");
      return FALSE;
    }
    if (fork () == 0)
    {				/* forks again, to wait for completion */
      dup2 (pfd[1], 1);		/* assign parent stdout to pipe1 */
      dup2 (pfd[1], 2);		/* assign parent stderr to pipe1 */
      close (pfd[0]);		/* not used by parent */
      close (pfd2[1]);
      dup2 (pfd2[0], 0);	/* assign stdin to pipe2 */
      execvp (PATCH, argument);
      perror ("exec");
      _exit (127);		/* parent never gets here (hopefully) */
    }
    /* send patchfile lines through pipe to patch. */
    dup2 (pfd[1], 1);		/* assign parent stdout to pipe1 */
    close (pfd[0]);		/* not used by parent */
    close (pfd2[0]);
    fileH = open (fileP, O_RDONLY);
    if (fileH == -1)
      xfdiff_abort (E_FILE);	/* this should almost never be true */
  applying_loop:
    lseek (fileH, thisF->offset, SEEK_SET);
    {
      void *blok;
      ssize_t blok_s;
      int readbytes;
      blok = malloc (BLOK_SIZE);
      if (!blok)
	xfdiff_abort (E_MALLOC);
      readbytes = thisF->length;
      while (readbytes > 0)
      {
	if (readbytes >= BLOK_SIZE)
	{
	  blok_s = read (fileH, blok, BLOK_SIZE);

	}
	else
	{
	  blok_s = read (fileH, blok, readbytes);
	}
	if (blok_s == 0)
	  break;		/* hmmm */
	readbytes -= blok_s;

	write (pfd2[1], blok, blok_s);
	/*fwrite(blok,1,blok_s,stderr); */
      }
      free (blok);
    }
    fflush (NULL);
    if (applying_patch)
    {
      thisF = thisF->next;
      if (thisF)
	goto applying_loop;
    }
    close (pfd2[1]);
    close (fileH);
    fflush (NULL);
    /* the parent waits for patch to finish */
    wait (&status);
    fflush (NULL);
    fprintf (stdout, "%s", done_str);
    sleep (1);			/* this sleep is to avoid a "race condition" */
    fflush (NULL);		/* must flush so first child can continue */
    _exit (1);
  }
  /* meanwhile, the first child is busy reading the pipe for diagnostic box
   * after pipe has closed, the child will open create the left temporary
   * file by means of the filelist and process it versus the right temporary file 
   *    
   *    Here it should be perfectly safe to use str functions in lieu of mem functions
   *
   *    */
  {
    char *line, *input;
    int lineL = 256;
    reversed_detected = FALSE;
    input = line = (char *) malloc (lineL);
    if (!input)
      xfdiff_abort (E_MALLOC);
    while (1)
    {				/* patch stderr output: no binary stuff */
      if (!read (pfd[0], input, 1))
	break;
      input[1] = 0;
      if ((input[0] == '\n') || (strlen (line) >= lineL - 2))
      {
	/*fprintf(stderr,"line:%s",line); */
	if (strcmp (line, done_str) == 0)
	  break;
	/* write to apropriate text widget */
	show_diag (line);
	input = line;
	if ((strncmp (line, "Reversed", strlen ("Reversed")) == 0) && (strstr (line, "detected")))
	{
	  reversed_detected = TRUE;
	}
	continue;
      }
      input++;
    }
    free (line);
  }
  close (pfd[0]);
  close (pfd[1]);		/* close the pipe */



  /* ok, now we have to build the left temporary file */
  /*printf("before build, file:%s\n",file); */

  if (applying_patch)
    return TRUE;

  {
    char *strip_file;

    strip_file = (strip) ? strip_it (file, strip) : file;
    if (!strip_file)
      return FALSE;
    if (!build_tmpL (strip_file))
    {
      /*printf("buildtmpL failed\n"); */
      free (file);
      return FALSE;
    }
  }
  free (file);
  /* now we assign fileL and fileR and then go dodiff */
  fileL = assign (fileL, fileI);
  fileR = assign (fileR, fileO);

  if (reversed_detected)
  {
    if (my_yesno_dialog (_("Reversed patch detected. Should I try reversing it?")))
    {				/* new text 1.2.3 */
      cb_toggle_reversed (NULL, NULL);
      return 2;
    }
  }

  if (good_dir)
  {
    show_diag (_("Now forking diff. Please be patient.\n"));
    do_diff ();
    show_diag (_("Finished forking.\n"));
  }
  return 1;
}

static int
parse_patchfile (void)
{
  char *line, *input, *file;
  int lineL = 256, linecount = 0, linenumber = 0;
  int offset = 0, where = 0, ricefound = 0, last_is_pushed = 0;
  FILE *Pfile;
  
  /* use buffered i/o so single byte reads doesn't hit performance */
  Pfile = fopen (fileP, "r");
  if (!Pfile)
  {
    my_show_message (_(cannot_read_patch_file));
    return FALSE;
  }

  cleanF ();			/* clean file list */
  lstat (fileP, &patch_st);

  /*printf("parsing %s\n",fileP); */

  input = line = (char *) malloc (lineL);
  if (!input)
    xfdiff_abort (E_MALLOC);
  while (!feof (Pfile))
  {
    fread (input, 1, 1, Pfile);
    if (feof (Pfile))
      break;
    where++;
    linecount++;
    input[1] = 0;
    if (input[0] == '\n')
    {
      linenumber++;
      if (ricefound)
      {
	ricefound = 0;
	input = line;
	linecount = 0;
	continue;		/* avoid a freak bug (which would that be?) */
      }
      /* this may or may not be in patch file! */
      /* if (strncmp(line,"diff",4)==0) { } */
      if (
       (strncmp (line, "---", 3) == 0)
       && 
       (!strstr (line, "----"))/* not the line we're looking 4 */
      )
      {
/*	printf("line=%s",line);*/
	if ((currentF) && (!last_is_pushed))
	{			/* not the first file */
	  /*      currentF->length = (where - strlen(line)) - currentF->offset ; */
	  currentF->length = (where - linecount) - currentF->offset;
	}
	/* currentF will be pushed with filename */
	if (!last_is_pushed) offset = where - linecount;	/* before we screw up line with strtok */
	
	/* new way, do strtok by either \t or space 
	* (does \w for whitespace exist? must RTFM) */
	{
          char *tmpC;
	  tmpC = strtok (line, " ");
/*	printf("-1:tmpC=%s\n",tmpC);*/
	  if (tmpC) tmpC += (strlen(tmpC) + 1); else continue;
/*	printf("-2:tmpC=%s\n",tmpC);*/
	  if (strchr(tmpC,'\t')) file = strtok (tmpC, "\t");
	  else file = strtok (tmpC, " "); 
/*	printf("-3:file=%s\n",file);*/
	}
	/* file = strtok (NULL, "\t"); old way after a strtok for space */
	  
	if (file)
	{
	  if (last_is_pushed) {
	    currentF->newfile = (char *) malloc (strlen (file) + 1);
	    if (!currentF->newfile)
	      xfdiff_abort (E_MALLOC);
	    strcpy (currentF->newfile, file);
	    last_is_pushed=0;
 /*   printf("added\n");	    */
	  } else {
  	    currentF = pushF (file, offset, 0);
	    last_is_pushed=1;
  /*  printf("pushed\n");	    */
	  }
	}
	/*printf("%s: offset=%d, length=%d\n",
	   currentF->file,offset,length); */

      }
      if (
       (strncmp (line, "+++", 3) == 0) 
       || 
       (
	(strncmp (line, "***", 3) == 0) 
	&& 
	(!strstr (line, "****"))/* not the line we're looking 4 */
       ) 
      )
      {
/*	printf("line=%s",line);*/
       if (!currentF)
	{
	  my_show_message (_(patch_file_error));/* not unified format (--- comes first) */
	}
       
	if ((currentF)&& (!last_is_pushed))
	{			/* not the first file */
	  /*      currentF->length = (where - strlen(line)) - currentF->offset ; */
	  currentF->length = (where - linecount) - currentF->offset;
	}
        if (!last_is_pushed) offset = where - linecount;	/* before we screw up line with strtok */

/*	else !currentF: let's process any way, fair warning has been given.*/ 
	{
		/* new way, do strtok by either \t or space 
		* (does \w for whitespace exist? must RTFM) */
	  char *tmpC;

	  tmpC = strtok (line, " ");
/*	printf("1:tmpC=%s\n",tmpC);*/
	  if (tmpC) tmpC += (strlen(tmpC) + 1); else continue;
/*	printf("2:tmpC=%s\n",tmpC);*/
	  if (strchr(tmpC,'\t')) file = strtok (tmpC, "\t");
	  else file = strtok (tmpC, " "); 
/*	printf("*3:file=%s\n",file);*/
/*	  file = strtok (NULL, "\t");old way*/
	  if (file)
	  {
	   if (last_is_pushed) {
	    currentF->newfile = (char *) malloc (strlen (file) + 1);
	    if (!currentF->newfile)
	      xfdiff_abort (E_MALLOC);
	    strcpy (currentF->newfile, file);
	    last_is_pushed=0;	   
 /*   printf("added\n");*/	    
	   } else {
	    currentF = pushF (file, offset, 0);
	    last_is_pushed=1;	   
 /*   printf("pushed\n");	    */
	   }
	  }
	}
      }
      input = line;
      linecount = 0;
      continue;
    }
    input++;
    if (linecount >= lineL - 2)
    {				/* by now line is of no interest */
      ricefound = 1;
      input = line;
      linecount = 0;
      continue;
    }
  }
  /* cleanup: */
  if (currentF)
  {
    currentF->length = where - currentF->offset;
    free (line);
    fclose (Pfile);
  }
  else
  {
    my_show_message (_(patch_file_error));
    free (line);
    fclose (Pfile);
    return FALSE;
  }
  if (!check_patch_dir ())
  {
    return FALSE;
    /*cleanF(); process patch anyway??? */
  }
  return TRUE;
}


int
do_patch (void)
{
  int fileH;

  cleanP ();
  cleanA ();
  cleanT ();			/* clean display area. */
  if (!fileP)
  {
    fileP = prompt_path (_(no_patch), fileP);
    if (!fileP)
      return FALSE;
  }
  if (!fileD)
  {
    fileD = prompt_path (_(no_patch_dir), fileD);
    if (checknotdir (fileD))
      fileD = assign (fileD, NULL);
    if (!fileD)
      return FALSE;
  }

  fileH = open (fileP, O_RDONLY);
  if (fileH == -1)
  {
    my_show_message (_(cannot_read_patch_file));
    return FALSE;
  }
  else
    close (fileH);

  show_diag (_("Running patch...\n"));
/* first, let's parse the patch file. */
  gdk_window_set_cursor (diff->window, cursor);
  gdk_flush ();
  if (!parse_patchfile ())
  {
    gdk_window_set_cursor (diff->window, NULL);
    cleanF ();
    return FALSE;
  }
  currentF = headF;
  while (1)
  {
    currentF = headF;
    if ((fileH = process_patch (currentF)) != 2)
      break;
  }
  update_titlesP ();
  gdk_window_set_cursor (diff->window, NULL);
  return fileH;
}


int
main (int argc, char *argv[])
{
  int i, leftA = 1, rightA = 2;
  xfdiff_init ();

  if (argc >= 2)
    for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-p") == 0)
      {
	leftA++;
	rightA++;
	patching = 1;
      }
      if (strcmp (argv[i], "-R") == 0)
      {
	leftA++;
	rightA++;
	reversed = 1;
      }
      if (strcmp (argv[i], "-N") == 0)
      {
	leftA++;
	rightA++;
	reversed = 0;
      }
      if (strncmp (argv[i], "-P", 2) == 0)
      {
	leftA++;
	rightA++;
	if ((strlen (argv[i]) == 3))
	{
	  if ((argv[i][2] > '0') && (argv[i][2] < '9'))
	    strip = atoi (argv[i] + 2);
	}
      }
      /* obsolete option: (keep for backwards compatibility) */
      if ((strcmp (argv[i], "-n") == 0) || (strcmp (argv[i], "--no-prompt") == 0))
      {
	leftA++;
	rightA++;
      }
      if ((strcmp (argv[i], "-h") == 0) || (strcmp (argv[i], "--help") == 0))
      {
	fprintf (stdout, "%s %s\n\n", _("xfdiff version"), XFDIFF_VERSION);
	fprintf (stdout, _("use xfdiff [ -h | --help | -p] [leftfile] [rightfile]\n"));
	fprintf (stdout, _("-p : leftfile and rightfile are a patchdir and a patchfile\n"));
	fprintf (stdout, _("-PN: N is strip level (0-9) for patch (quick toggle)\n"));
	fprintf (stdout, _("-R: patch file is reversed (quick toggle)\n"));
	fprintf (stdout, _("-N: patch file is not reversed (quick toggle)\n"));
	fprintf (stdout, _("-h, --help : print this message\n\n"));
	fprintf (stdout, _("xfdiff (c) Edscott Wilson Garcia 2001, under GNU-GPL\n"));
	fprintf (stdout, _("XFCE modules copyright Olivier Fourdan 1999-2001, under GNU-GPL\n\n"));
	exit (1);
      }
    }
  if (argc >= leftA + 1)
  {
    if (patching)
      fileD = assign (fileD, argv[leftA]);
    else
      fileL = assign (fileL, argv[leftA]);
  }
  if (argc >= rightA + 1)
  {
    if (patching)
      fileP = assign (fileP, argv[rightA]);
    else
      fileR = assign (fileR, argv[rightA]);
  }


#ifdef __GTK_2_0
  gtk_init (&argc, &argv);
#else
  xfce_init (&argc, &argv);
  signal (SIGHUP, finish);
  signal (SIGSEGV, finish);
  signal (SIGKILL, finish);
  signal (SIGTERM, finish);
#endif
/*printf("1...\n");*/
  diff = create_diff_window ();
  cursor = gdk_cursor_new (GDK_WATCH);
  /* process command line arguments, if any first dumping invalid values */

  if (patching)
  {
    if ((fileD) && (!checkdir (fileD)))
      fileD = assign (fileD, NULL);
    if ((fileP) && (checkdir (fileP)))
      fileP = assign (fileP, NULL);
    if ((fileD) && (fileP))
      do_patch ();
  }
  else
  {
    if ((fileR) && (fileL))
      do_diff ();
  }
  update_titlesP ();
  gtk_main ();
  return (0);
}
