/*   tubo.h */

/*  A program independent forking object module for gtk based programs.
 *  
 *  Copyright (C)  Edscott Wilson Garcia under GNU GPL
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
/* public stuff */

/* Tubo() returns void pointer to tubo object 
*  usage: see example below */
#include <sys/types.h>
#include <unistd.h>

void *Tubo (void (*fork_function) (void), void (*fork_finished_function) (pid_t), int operate_stdin, int (*operate_stdout) (int, void *), int (*operate_stderr) (int, void *));

/* TuboCancel() terminates a running fork. Called
*  with the pointer returned from Tubo()  */
void *TuboCancel (void *forkObject, void (*cleanup) (void));

/* TuboWrite() will sent n bytes of the data pointed
*  at by the void pointer "data". forkObject is the
*  pointer returned by Tubo on fork creation */

int TuboWrite (void *forkObject, void *data, int n);

/* this returns the pid of child */
pid_t TuboPID (void *forkObject);

/* EXAMPLES for calling Tubo()****************************/
/* ref. xfsamba.c */
/* example fork_function: ********************************
 
void a_fork_function(void){
	execlp("nmblookup","nmblookup","-M","-",(char *)0);
}

   example fork over function: **************************

static void a_fork_finished_function(void){
	printf("fork is over\n");
}

   example operate_stdin: *******************************
operate_stdin may be either True or False. If true, the child
will connect stdin to the proper pipe to be written by 
TuboWrite()

   example operate_stdout or operate_stderr function: ***
  
static int a_operate_stderr(int n, void *data){
  char *line;
  if (n) {
  	printf("%d bytes of binary data read from pipe\n",n);
  } else {
  	line = (char *) data;
  	printf("pipe: %s",line;
  }
  return TRUE;
}
   

***********************************************************/
