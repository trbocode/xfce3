/*
 * xtree.c
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef linux
#include <getopt.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <X11/Xlib.h>		/* XParseGeometry */
#include <glib.h>
#include <gdk/gdkx.h>
#include "constant.h"
#include "my_intl.h"
#include "xtree_gui.h"
#include "xtree_mess.h"
#include "xtree_cfg.h"
#include "gtk_dlg.h"
#include "uri.h"
#include "xfce-common.h"

#ifndef VERSION
#define VERSION "(not defined)"
#endif

#define TRASH_DIR "trash"
#define BASE_DIR ".xfce"

#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif
GtkWidget *io_parent=NULL;
char *arg_hostname=NULL;
char *arg_display=NULL;
extern int pixmap_level;

static gint open_warning(gpointer data){
	  FILE *mess;
	  char line[256];
	  mess=fopen("/tmp/xftree.USR1","r");
	  if (mess) {
		  fgets(line,255,mess);
		  line[255]=0;
		  fclose(mess);
		  unlink("/tmp/xftree.USR1");
		  /*printf("dgb:parent got %s",line);*/
		  if (io_parent) xf_dlg_error(io_parent,line,NULL);
	  }
	  return FALSE;
}

static void
finishit (int sig)
{
  if (sig == SIGUSR1) {
          /*while (gtk_events_pending()) gtk_main_iteration();*/
	  /* must do it this way to avoid threads fighting for gtk_main loop */
          gtk_timeout_add (260, (GtkFunction) open_warning, NULL);
	  return;
  } else
  {
    fprintf(stderr,"xftree: signal %d received. Cleaning up before exiting\n",sig);
    cleanup_tmpfiles();
    /*on_signal(sig);*/
    exit(1);
  }
}

extern gboolean check_hostname(char *host);
extern char *our_host_name(void);

int
main (int argc, char *argv[])
{
  int c;
  int verbose = 0;
  int flags = IGNORE_HIDDEN;
  char *geometry = NULL;
  static char path[PATH_MAX + NAME_MAX + 1];
  static char rc[PATH_MAX + NAME_MAX + 1];
  static char base[PATH_MAX + 1];
  static char trash[PATH_MAX + 1];
  static char reg[PATH_MAX + 1];
  char tmp[PATH_MAX + 1];
  struct stat st;
  wgeo_t geo = { -1, -1, 380, 480 };

  /* xfce_init must appear after argument processing 
   * so that alternate display can be specified */
  
  sprintf (rc, "%s/%s/%s", getenv("HOME"), BASE_DIR, "xtree.rc");
  read_defaults();
  strcpy (path,(custom_home_dir)?custom_home_dir: getenv ("HOME"));

  while ((c = getopt (argc, argv, "hp:vg:i:H:d:")) != EOF)
  {
    switch (c)
    {
    case 'h':
	    printf("xftree -p (0|1|2|3) sets the icon level\n\
0 = no icons at all\n\
1 = basic icon set, no scaling done\n\
2 = extended icon set, no scaling done\n\
3 = extended icon set with scaling to font size\n");
	    exit(1);
			   
    case 'p':
      pixmap_level=atoi(optarg);
      break;
    case 'v':
      verbose++;
      break;
    case 'g':
      geometry = optarg;
      break;
    case 'H':
      arg_hostname=optarg;
      break;  
    case 'd':
      arg_display=optarg;
      break;  
    case 'i':
      if (atoi (optarg))
	flags |= IGNORE_HIDDEN;
      else
	flags &= ~IGNORE_HIDDEN;
      break;
    default:
      break;
    }
  }

  if (arg_display) {
	  char e[256];
	  sprintf(e,"DISPLAY=%s",arg_display);
	  putenv(e);
	  /*setenv("DISPLAY",arg_display,TRUE);*/
	  /*printf("display is %s\n",getenv("DISPLAY"));*/
  }

  /* remote xftree instance
   * (no enabled yet, a little buggy because of the "sane()"
   *  routine eliminating register entries on remote host) */
  if ((arg_hostname) && (!check_hostname(arg_hostname))){
#if 0
	  char *Argv[12];
	  char *dpy;
	  int i=0,status;
	  pid_t pid;
	  dpy=(char *)malloc(strlen(our_host_name())+strlen(":0.0")+1);
	  if (dpy) {
	    sprintf(dpy,"%s:0.0",our_host_name());	  
	    Argv[i++]="ssh";
	    Argv[i++]=arg_hostname;
	    Argv[i++]="xftree";
	    if (argc != optind) Argv[i++]=argv[argc - 1];
	    Argv[i++]="-d";
	    Argv[i++]=dpy;
	    Argv[i++]=0;
	    
	    pid=fork();
	    if (pid<0){
	      printf("Xftree: Cannot fork\n");
	    } else if (fork()!=0) wait(&status);
	    else {
	      execvp(Argv[0],Argv);
	      printf("Xftree: Cannot open remote xftree at %s\n",arg_hostname);
	    }
	  } 
#endif
	  exit(1);  	  
  }
  xfce_init (&argc, &argv);
 /* for temporary file cleanup (after xfce_init) */
  signal (SIGHUP, finishit);
  signal (SIGINT, finishit);
  signal (SIGQUIT, finishit);
  signal (SIGABRT, finishit);
  signal (SIGBUS, finishit);
  signal (SIGSEGV, finishit);
  signal (SIGTERM, finishit);
  signal (SIGFPE, finishit);

  signal (SIGKILL, finishit);
  signal (SIGUSR1, finishit);
  signal (SIGUSR2, finishit);
  
  if (argc != optind)
  {
    strcpy (path, argv[argc - 1]);
  }
  if (strcmp (path, ".") == 0)
  {
    getcwd (path, PATH_MAX);
  }

  if (verbose)
  {
    printf (_("XFTree, based on XTree Version %s\n"), VERSION);
    printf (_("directory: %s\n"), path);
  }
  sprintf (base, "%s/%s", getenv ("HOME"), BASE_DIR);
  if (stat (base, &st) == -1)
  {
    if (verbose)
    {
      printf (_("creating directory: %s\n"), base);
    }
    mkdir (base, 0700);
  }
  sprintf (reg, "%s/xtree.reg", base);
  if (stat (reg, &st) == -1)
  {
    char buffer[MAXSTRLEN + 1];
    char *src, *dst;
    FILE *copyfile;
    FILE *backfile;
    int nb_read;

    src = (char *) malloc (sizeof (char) * (sizeof (XFCE_CONFDIR) + 15));
    dst = (char *) malloc (sizeof (char) * (sizeof (reg) + 1));
    sprintf (src, "%s/xtree.reg", XFCE_CONFDIR);
    sprintf (dst, "%s", reg);

    copyfile = fopen (src, "r");
    backfile = fopen (dst, "w");

    if ((backfile) && (copyfile))
    {
      while ((nb_read = fread (buffer, 1, MAXSTRLEN, copyfile)) > 0)
      {
	fwrite (buffer, 1, nb_read, backfile);
      }
      fflush (backfile);
      fclose (backfile);
      fclose (copyfile);
    }
    free (src);
    free (dst);
  }
  sprintf (trash, "%s/%s", base, TRASH_DIR);
  if (stat (trash, &st) == -1)
  {
    if (verbose)
    {
      printf (_("creating directory: %s\n"), trash);
    }
    mkdir (trash, 0700);
  }
  if (strncmp (path, "..", 2) == 0)
  {
    sprintf (tmp, "%s/", getcwd (NULL, PATH_MAX));
    strcat (tmp, path);
    strcpy (path, tmp);
  }
  strcpy (tmp, uri_clear_path (path));
  strcpy (path, tmp);
  if (geometry)
  {
    XParseGeometry (geometry, &geo.x, &geo.y, (unsigned int *) &geo.width, (unsigned int *) &geo.height);
    if (verbose)
    {
      printf (_("geometry: %dx%d+%d+%d\n"), geo.width, geo.height, geo.x, geo.y);
    }
  }
  fcntl (ConnectionNumber (GDK_DISPLAY ()), F_SETFD, 1);
  gui_main (path, base, trash, reg, &geo, flags);
  cleanup_tmpfiles();
  xfce_end ((gpointer) NULL, 0);
  return (0);
}
