/*
 * io.c
 *
 * Copyright (C) 1998 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
 *
 * Edscott Wilson Garcia Copywrite 2001-2002 for Xfce project.
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include "io.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

extern void cleanup_tmpfiles(void);
extern GtkWidget *io_parent;

extern char **environ;

/*
 * check is named file is an executable
 * return the mode field, so we don't have to stat() again if
 * we need some further informations
 */
int
io_is_exec (char *file)
{
  struct stat file_stat;
  if (stat (file, &file_stat) != -1)
  {
    if ((!S_ISDIR (file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR || file_stat.st_mode & S_IXGRP || file_stat.st_mode & S_IXOTH))
      return (file_stat.st_mode);
  }
  return (0);
}

/*
 * is named file a directory
 */
int
io_is_directory (char *path)
{
  struct stat file_stat;
  if (stat (path, &file_stat) != -1)
  {
    if (S_ISDIR (file_stat.st_mode))
      return (1);
  }
  return (0);
}

/*
 * is named file a directory
 */
int
io_is_file (char *path)
{
  struct stat file_stat;
  if (stat (path, &file_stat) != -1)
  {
    if (S_ISREG (file_stat.st_mode))
      return (file_stat.st_mode);
  }
  return (0);
}

/*
 */
int
io_can_exec (char *file)
{
  if (access (file, X_OK) != -1)
    return (1);
  return (0);
}

/*
 */
int
io_can_write_to_parent (char *file)
{
  char *p;
  char path[PATH_MAX + 1];
  p = strrchr (file, '/');
  if (p)
  {
    strncpy (path, file, p - file + 1);
    path[p - file + 1] = '\0';
  }
  else
    strcpy (path, ".");
  if (access (path, W_OK | X_OK) == -1)
    return (0);
  return (1);
}

/*
 */

pid_t io_pid;
int
io_system (char **argv,GtkWidget *parent)
{
  int pid, status;
  int nulldev;

  if (!argv[0]) return (1);
  io_pid=getpid();
  io_parent=parent;
  pid = fork ();
  if (pid == -1) return (-1);
  if (pid==0) { /* child process */
    /*printf("dbg:[1]%s [2]%s [3]%s [4]%s\n",argv[0],argv[1],argv[2],argv[3]);*/
    /* The following is to avoid X locking when executing 
       terminal based application that requires user input */
    if ((nulldev = open ("/dev/null", O_RDWR)))
    {
      close (0);
      dup (nulldev);
#if 0
      /* keep stdout and stderr open for diagnostics */
      close (1);
      dup (nulldev);
      close (2);
      dup (nulldev);
#endif
    } else _exit (127);

    /* second fork instead of using /bin/sh,
     * this enables error processing 
     * (sometimes, at least :-0*/
    if (fork ()==0) { 
       /*printf("dbg:by direct...\n");*/
       if (execve (argv[0], argv, environ) == -1) {
           FILE *mess;
	   if (errno != ENOEXEC) execvp (argv[0], argv); 
	   mess=fopen("/tmp/xftree.USR1","w");
	   if (mess){
	       fprintf(mess,"%s: %s\n",argv[0],strerror(errno));
	       fclose(mess);
	       kill(io_pid,SIGUSR1);
	   }
	   usleep(50000);
	   /*perror (argv[0]);*/
       }
    }
    _exit (127);
  }
  while (1){
    if (waitpid (pid, &status, 0) == -1)
    {
      if (errno != EINTR)
	return (-1);
    }
    else
      return status;
  }
}

/*
 */
static void
sig_seg (int signum)
{
  fprintf (stderr, "segmention fault (pid=%d)\n", (int) getpid ());
  cleanup_tmpfiles();
  exit (127);
}

/*
 */
int
io_system_var (char **arg, int len)
{
  pid_t pid;
  int status, i;
  void *old;

  if ((arg == NULL) || (len == 0))
    return -1;
  if (access (*arg, X_OK) != 0)
    return -1;
  old = signal (SIGSEGV, sig_seg);

  pid = fork ();
  if (pid == -1)
  {
    perror ("fork()");
    return (-1);
  }

  if (pid == 0)
  {
    /* child process */
    char **argv = (char **) malloc (sizeof (char *) * (len + 2));
    if (!argv)
      _exit (127);
    for (i = 0; i < len; i++)
    {
      argv[i] = arg[i];
    }
    argv[len] = NULL;
    if (execve (argv[0], argv, environ) == -1)
    {
      perror (argv[0]);
      _exit (127);
    }
  }
  /* parent process */
  sleep (1);
  do
  {
    if (waitpid (pid, &status, WNOHANG) == -1)
    {
      if (errno != EINTR)
	return (-1);
    }
    else
    {
      if (WIFEXITED (status))
      {
	if (WEXITSTATUS (status) == 127)
	  return -1;
	else
	  return WEXITSTATUS (status);
      }
      return 0;
    }
  }
  while (1);
}

/*
 */
int
io_item_exists (char *path)
{
  struct stat s;
  if (stat (path, &s) != -1)
  {
    return (s.st_mode);
  }
  return (0);
}
