
/*
 * ** Module.c: code for modules to communicate with xfwm
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include "utils.h"
#include "module.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/************************************************************************
 * 
 * Reads a single packet of info from xfwm. Prototype is:
 * unsigned long header[HEADER_SIZE];
 * unsigned long *body;
 * int fd[2];
 *
 * ReadXfwmPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *   body is a malloc'ed space which needs to be freed 
 *
 **************************************************************************/
int
ReadXfwmPacket (int fd, unsigned long *header, unsigned long **body)
{
  int count, total, count2, body_length;
  char *cbody;

  if ((count = read (fd, header, HEADER_SIZE * sizeof (unsigned long))) > 0)
  {
    if (header[0] == START_FLAG)
    {
      body_length = header[2] - HEADER_SIZE;
      *body = (unsigned long *) safemalloc (body_length * sizeof (unsigned long));
      cbody = (char *) (*body);
      total = 0;
      while (total < body_length * sizeof (unsigned long))
      {
	if ((count2 = read (fd, &cbody[total], body_length * sizeof (unsigned long) - total)) > 0)
	{
	  total += count2;
	}
	else if (count2 < 0)
	{
	  kill (getpid (), SIGPIPE);
	}
      }
    }
    else
      count = 0;
  }
  if (count <= 0)
    kill (getpid (), SIGPIPE);
  return count;
}

/************************************************************************
 *
 * SendText - Sends arbitrary text/command back to xfwm
 *
 ***********************************************************************/
void
SendText (int *fd, char *message, unsigned long window)
{
  static int w;
  static unsigned long win;

  win = window;
  if (message != NULL)
  {
    write (fd[0], &win, sizeof (unsigned long));
    w = strlen (message);
    write (fd[0], &w, sizeof (int));
    write (fd[0], message, w);
    w = 1;
    write (fd[0], &w, sizeof (int));
  }
}

/***************************************************************************
 * 
 * Sets the which-message-types-do-I-want mask for modules 
 *
 **************************************************************************/
void
SetMessageMask (int *fd, unsigned long mask)
{
  char set_mask_mesg[50];

  sprintf (set_mask_mesg, "SET_MASK %lu\n", mask);
  SendText (fd, set_mask_mesg, 0);
}

/***************************************************************************
 *
 * Gets a module configuration line from xfwm. Returns NULL if there are 
 * no more lines to be had. "line" is a pointer to a char *.
 *
 **************************************************************************/
void *
GetConfigLine (int *fd, char **tline)
{
  static int first_pass = 1;
  int count, done = 0;
  static char *line = NULL;
  unsigned long header[HEADER_SIZE];

  if (line != NULL)
    free (line);

  if (first_pass)
  {
    SendInfo (fd, "Send_ConfigInfo", 0);
    first_pass = 0;
  }

  while (!done)
  {
    count = ReadXfwmPacket (fd[1], header, (unsigned long **) &line);
    if (count > 0)
      *tline = &line[3 * sizeof (long)];
    else
      *tline = NULL;
    if (*tline != NULL)
    {
      while (isspace (**tline))
	(*tline)++;
    }

/*   fprintf(stderr,"%x %x\n",header[1],XFCE_M_END_CONFIG_INFO); */
    if (header[1] == XFCE_M_CONFIG_INFO)
      done = 1;
    else if (header[1] == XFCE_M_END_CONFIG_INFO)
    {
      done = 1;
      if (line != NULL)
	free (line);
      line = NULL;
      *tline = NULL;
    }
  }
  return NULL;
}
