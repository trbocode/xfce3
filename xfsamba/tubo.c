
/*   tubo.c (0.21) */

/*  A program independent forking object module for gtk based programs.
 *  
 *  Copyright 2000-2002(C)  Edscott Wilson Garcia under GNU GPL
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

#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif
/* GTK */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* public stuff */
#include "tubo.h"


/* private stuff */

/* memcpy is necesary  */
#ifdef __GNUC__
/* memcpy is a GNU extension.*/
#define MEMCPY memcpy
#else
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


typedef struct fork_structure
{
  pid_t childPID;
  pid_t PID;
  int tubo[3][3];
  /* user fork function: */
  void (*fork_function) (void);
  void (*fork_finished_function) (pid_t);
  /* user parse functions: */
  int operate_stdin;
  int (*operate_stdout) (int, void *);
  int (*operate_stderr) (int, void *);
}
fork_struct;

#ifdef DEBUG_RUN

#endif

static fork_struct *
TuboChupoFaros (fork_struct * forkO)
{
  int i;
#ifdef DEBUG_RUN
  fprintf (stderr, "Chupando faros\n");
#endif
  if (!forkO)
  {
    return NULL;
  }
  for (i = 0; i < 3; i++)
  {
    if (forkO->tubo[i][0] > 0)
    {
      close (forkO->tubo[i][0]);
    }
    if (forkO->tubo[i][1] > 0)
    {
      close (forkO->tubo[i][1]);
    }
    if (forkO->tubo[i][2] > 0)
    {
#ifdef DEBUG_RUN
      fprintf (stderr, "removing input signal %d\n", i);
#endif
      gtk_input_remove (forkO->tubo[i][2]);
    }
  }
  free (forkO);
  return NULL;
}

#define TUBO_BLK_SIZE 256	/* 256 is the size of my cpu internal cache */
static gboolean
TuboInput (gpointer data, gint src, GdkInputCondition condition)
{
  static char *buffer, line[TUBO_BLK_SIZE];
  int i = 0;
  int (*user_parse_function) (int, char *);


  if (!data)
    return FALSE;
  user_parse_function = (int (*)(int, char *)) ((long) data);

#ifdef DEBUG_RUN
  printf ("input found\n");
  fflush (NULL);
#endif
  buffer = line;
  for (i = 0; i < TUBO_BLK_SIZE - 1; i++)
  {
    if (!read (src, line + i, 1))
      break;
    if (*(line + i) == '\n')
    {				/* only un*x ascii files */
      *(line + i + 1) = (char) 0;
      /*first parameter, gboolean no newline-char | binary data length */
      (*user_parse_function) (0, (void *) line);
      return TRUE;
    }
  }
  if (i)
  {				/* something has been read */
    (*user_parse_function) (0, (void *) line);
    return TRUE;
  }
  return FALSE;
}

static gint
TuboWaitDone (gpointer fork_object)
{
  pid_t PID;
  void (*user_end_function) (pid_t);
  fork_struct *forkO;
  forkO = (fork_struct *) ((long) fork_object);
  PID=forkO->PID;
  user_end_function = forkO->fork_finished_function;
  /*      printf("wait timeoutdone childpid=%d\n",forkO->childPID); */
  if (forkO->childPID) return TRUE;
#ifdef DEBUG_RUN
/* printf("wait timeoutdone childpid=%d\n",forkO->childPID); */
  {
    void print_diagnostics (char *message);
    print_diagnostics ("checking pipes before zap.\n");
  }
#endif
  /* what if the pipe has data? */
  if (TuboInput ((gpointer) (forkO->operate_stdout), forkO->tubo[1][0], GDK_INPUT_READ))
    return TRUE;
  if (TuboInput ((gpointer) (forkO->operate_stderr), forkO->tubo[1][0], GDK_INPUT_READ))
    return TRUE;
#ifdef DEBUG_RUN
  {
    void print_diagnostics (char *message);
    print_diagnostics ("zapping pipe!\n");
  }
#endif
  TuboChupoFaros (forkO);
  if (user_end_function) (*user_end_function) (PID);
  return FALSE;
}


static gint
TuboWait (gpointer fork_object)
{
  fork_struct *forkO;
  int status;
  forkO = (fork_struct *) ((long) fork_object);
  waitpid (forkO->childPID, &status, WNOHANG);
  if (WIFEXITED (status))
  {
    forkO->childPID = 0;
    return FALSE;
  }
  return TRUE;
}



static void
TuboSemaforo (int sig)
{
  switch (sig)
  {
  case SIGUSR1:
    break;			/* green light */
  case SIGTERM:
    _exit (1);
    break;
  default:
    break;
  }
  return;
}



void *
Tubo (void (*fork_function) (void), void (*fork_finished_function) (pid_t), int operate_stdin, int (*operate_stdout) (int, void *), int (*operate_stderr) (int, void *))
{
  int i;
  fork_struct tmpfork, *newfork = NULL;

  if ((!operate_stdout) && (!operate_stderr))
  {
    TuboChupoFaros (newfork);	/* no mames condition */
    return NULL;
  }

  for (i = 0; i < 3; i++)
  {
    tmpfork.tubo[i][0] = tmpfork.tubo[i][1] = -1;
    tmpfork.tubo[i][2] = 0;
    if (pipe (tmpfork.tubo[i]) == -1)
    {
      TuboChupoFaros (NULL);
      return NULL;
    }
  }

  tmpfork.operate_stdin = operate_stdin;
  tmpfork.operate_stdout = operate_stdout;
  tmpfork.operate_stderr = operate_stderr;
  tmpfork.fork_function = fork_function;
  tmpfork.fork_finished_function = fork_finished_function;

  /* signal(SIGUSR1) has to be done before fork, to avoid race */

  signal (SIGUSR1, TuboSemaforo);
  tmpfork.PID = tmpfork.childPID =fork ();
  if (tmpfork.childPID)
  {				/* the parent */
    /* INPUT PIPES *************** */
#ifdef DEBUG_RUN
/* printf("The parent process has forked\n"); */
#endif
    newfork = (fork_struct *) malloc (sizeof (fork_struct));
    if (!newfork)
    {
      /* red light to child */
      perror ("malloc");
#ifdef DEBUG_RUN
/* printf("The parent process is sending a red light\n"); */
#endif
      kill (tmpfork.childPID, SIGTERM);
      TuboChupoFaros (NULL);
      return NULL;
    }
    MEMCPY ((void *) newfork, (void *) (&tmpfork), sizeof (fork_struct));

    close (newfork->tubo[0][0]);	/* not used */
    if (operate_stdout)
    {
      newfork->tubo[1][2] = gdk_input_add (newfork->tubo[1][0], GDK_INPUT_READ, (GdkInputFunction) TuboInput, (gpointer) operate_stdout);
    }
    if (operate_stderr)
    {
      newfork->tubo[2][2] = gdk_input_add (newfork->tubo[2][0], GDK_INPUT_READ, (GdkInputFunction) TuboInput, (gpointer) operate_stderr);
    }
    /* OUTPUT PIPES ************** */
    if (!operate_stdin)
      close (newfork->tubo[0][1]);	/* not used */
    close (newfork->tubo[1][1]);	/* not used */
    close (newfork->tubo[2][1]);	/* not used */

    /* the wait */
    gtk_timeout_add (260, (GtkFunction) TuboWait, (gpointer) (newfork));
    /* what to do when child is kaput. */
    gtk_timeout_add (520, (GtkFunction) TuboWaitDone, (gpointer) (newfork));
    /*send greenlight to child: ok to continue */
    usleep (500000);			/* race: child must be at pause before sending signal */
#ifdef DEBUG_RUN
/* printf("The parent process is sending a green light\n"); */
#endif
    kill (newfork->childPID, SIGUSR1);
    return newfork;		/* back to user's program flow */
  }
  else
  {				/* the child */
    /* waitfor green light. */
    signal (SIGTERM, TuboSemaforo);
#ifdef DEBUG_RUN
/* printf("The child process is waiting for the green light\n"); */
#endif
    pause ();
#ifdef DEBUG_RUN
/* printf("The child has received the green light\n"); */
#endif
    newfork = (fork_struct *) malloc (sizeof (fork_struct));
    if (!newfork)
    {

      _exit (1);
    }
    MEMCPY ((void *) newfork, (void *) (&tmpfork), sizeof (fork_struct));

    /* INPUT PIPES */
    if (operate_stdin)
    {
      dup2 (newfork->tubo[0][0], 0);	/* stdin */
    }
    else
      close (newfork->tubo[0][0]);	/* not used */
    close (newfork->tubo[1][0]);	/* not used */
    close (newfork->tubo[2][0]);	/* not used */
    /* OUTPUT PIPES */
    close (newfork->tubo[0][1]);	/* not used */
    if (operate_stdout)
      dup2 (newfork->tubo[1][1], 1);	/* stdout */
    else
      close (newfork->tubo[1][1]);	/* not used */
    if (operate_stdout)
      dup2 (newfork->tubo[2][1], 2);	/* stderr */
    else
      close (newfork->tubo[2][1]);	/* not used */
    /*  user fork function */
    if (newfork->fork_function)
      (*(newfork->fork_function)) ();
    /* if it returns, clean up before sinking */
    TuboChupoFaros (newfork);
    _exit (1);
  }
}

int
TuboWrite (void *forkObject, void *data, int n)
{
  /* if n!=0 --> binary data */
  int size;
  fork_struct *forkO;
  forkO = (fork_struct *) forkObject;
  if (!forkO)
    return 0;
  if (!data)
    return 0;
  if (!n)
    size = n;
  else
    size = strlen ((char *) data);
  write (forkO->tubo[0][1], data, size);	/* watchout for closed pipes */
  return 1;
}


void *
TuboCancel (void *forkObject, void (*cleanup) (void))
{
  int i;
  fork_struct *forkO;
  forkO = (fork_struct *) forkObject;
  if (!forkO)
  {
    return NULL;
  }
#ifdef DEBUG_RUN
  fprintf (stderr, "cancelling fork object\n");
#endif
  for (i = 0; i < 3; i++)
  {
    if (forkO->tubo[i][2] > 0)
    {				/* remove input callbacks */
#ifdef DEBUG_RUN
      fprintf (stderr, "removing input signal %d\n", i);
#endif
      gtk_input_remove (forkO->tubo[i][2]);
    }
    if (forkO->tubo[i][0] > 0)
    {				/* close input pipes */
      close (forkO->tubo[i][0]);
    }
    if (forkO->tubo[i][1] > 0)
    {				/* close output pipes */
      close (forkO->tubo[i][1]);
    }
  }
  forkO->fork_finished_function = NULL;
  forkO->operate_stdin = FALSE;
  forkO->operate_stdout = NULL;
  forkO->operate_stderr = NULL;

  if (forkO->childPID)
    kill (forkO->childPID, SIGTERM);
  if (cleanup)
    (*cleanup) ();

  /*note: fork object freed by TuboWaitDone() function */
  return NULL;
}

pid_t
TuboPID (gpointer fork_object)
{
  fork_struct *forkO;
  forkO = (fork_struct *) ((long) fork_object);
  return (forkO->PID);
}

