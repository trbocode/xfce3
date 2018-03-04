/* xfsamba_mem.c : memory operations for xfsamba 
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfsamba.h"

static nmb_history *headHS;

static smb_cache *headS;
void
clean_smb_cache (void)
{
  smb_cache *last;
  while (headS)
  {
    if (headS->directory)
      free (headS->directory);
    last = headS;
    headS = headS->next;
    free (last);
  }
  return;
}

void
push_smb_cache (GtkCTreeNode * node, char *directory)
{
  smb_cache *currentS;
  currentS = headS;
  if (!currentS)
  {
    headS = currentS = (smb_cache *) malloc (sizeof (smb_cache));
  }
  else
  {
    while (currentS->next)
      currentS = currentS->next;
    currentS->next = (smb_cache *) malloc (sizeof (smb_cache));
    currentS = currentS->next;
  }
  if (directory)
  {
    if (strstr (directory, "\n"))
      strtok (directory, "\n");
    currentS->directory = (char *) malloc (strlen (directory) + 1);
    strcpy (currentS->directory, directory);
  }
  currentS->node = node;
  currentS->next = NULL;

  return;
}

GtkCTreeNode *
find_smb_cache (char *directory)
{
  smb_cache *currentS;
  currentS = headS;
  while (currentS)
  {
    /*
       fprintf(stderr,"DBG:%s<->%s#\n",directory,currentS->directory);
     */
    if (!strcmp (directory, currentS->directory))
    {
      return currentS->node;
    }
    currentS = currentS->next;
  }

  return NULL;
}

void
pop_cache (nmb_cache * cache)
{
  nmb_cache *currentC, *lastC = NULL;
  if (!cache)
    return;
  currentC = cache;
  while (currentC->next)
  {
    lastC = currentC;
    currentC = currentC->next;
  }
  free (currentC);
  if (lastC)
    lastC->next = NULL;
}

void
eliminate2_cache (nmb_cache * the_cache, char *entry)
{
  /* eliminate from level 2 cache */
  nmb_cache *currentC;
  currentC = the_cache;
  while (currentC)
  {
    /*printf("%s<->%s\n",currentC->textos[2], entry); */
    if (strcmp (currentC->textos[SERVER_COMMENT_COLUMN], entry) == 0)
    {
      strcpy (currentC->textos[SERVER_COMMENT_COLUMN], ".");
      break;
    }
    currentC = currentC->next;
  }
}




void
smoke_nmb_cache (nmb_cache * fromC)
{
  nmb_cache *currentC;
  if (!fromC)
    return;
  currentC = fromC->next;
  while (currentC)
  {
    int i;
    nmb_cache *nextC;
    nextC = currentC->next;
    for (i = 0; i < SERVER_COLUMNS; i++)
    {
      if (currentC->textos[i])
	free (currentC->textos[i]);
    }
    free (currentC);
    currentC = nextC;
  }
  fromC->next = NULL;
}

nmb_cache *
clean_cache (nmb_cache * cache)
{
  nmb_cache *last;

  while (cache)
  {
    {
      int i;
      for (i = 0; i < SERVER_COLUMNS; i++)
      {
	if (cache->textos[i])
	  free (cache->textos[i]);
      }
    }
    last = cache;
    cache = cache->next;
    free (last);
  }
  return NULL;
}

nmb_cache *
push_nmb_cache (nmb_cache * headC, char **textos)
{
  nmb_cache *currentC;
  currentC = headC;
  if (!currentC)
  {
    currentC = (nmb_cache *) malloc (sizeof (nmb_cache));
  }
  else
  {
    while (currentC->next)
      currentC = currentC->next;
    currentC->next = (nmb_cache *) malloc (sizeof (nmb_cache));
    currentC = currentC->next;
  }
  {
    int i;
    for (i = 0; i < SHARE_COLUMNS; i++)
    {
      if (textos[i])
      {
	/*
	   int j;       j=strlen(textos[i]-1);
	 */
	/*printf("dbg:textos[%d]=%s\n",i,textos[i]);*/
	while (textos[i][strlen (textos[i]) - 1] == ' ')
	  textos[i][strlen (textos[i]) - 1] = 0;
	currentC->textos[i] = (char *) malloc (strlen (textos[i]) + 1);
	strcpy (currentC->textos[i], textos[i]);
      }
      else
	currentC->textos[i] = NULL;
    }
  }
  currentC->visited = 0;
  currentC->next = NULL;

  if (headC)
    return headC;
  else
    return currentC;
}

/* FIXME: uniformize with above function:
 * this one uses a char ** of length 3 while
 * the above uses a char ** of length SHARE_COLUMNS.
 * Both functions are called from different parts
 * of xfsamba. It does not look nice and should
 * be fixed. */
nmb_cache *
push_nmb_cacheF (nmb_cache * headC, char **textos)
{
  nmb_cache *currentC;
  currentC = headC;
  if (!currentC)
  {
    currentC = (nmb_cache *) malloc (sizeof (nmb_cache));
  }
  else
  {
    while (currentC->next)
      currentC = currentC->next;
    currentC->next = (nmb_cache *) malloc (sizeof (nmb_cache));
    currentC = currentC->next;
  }
  {
    int i;
    for (i = 0; i < 3; i++)
    {
      if (textos[i])
      {
	/*
	   int j;       j=strlen(textos[i]-1);
	 */
	/*printf("dbg:textos[%d]=%s\n",i,textos[i]);*/
	while (textos[i][strlen (textos[i]) - 1] == ' ')
	  textos[i][strlen (textos[i]) - 1] = 0;
	currentC->textos[i] = (char *) malloc (strlen (textos[i]) + 1);
	strcpy (currentC->textos[i], textos[i]);
      }
      else
	currentC->textos[i] = NULL;
    }
  }
  currentC->visited = 0;
  currentC->next = NULL;

  if (headC)
    return headC;
  else
    return currentC;
}


void
smoke_history (nmb_history * fromH)
{
  nmb_history *currentH;
  if (!fromH)
    return;
  currentH = fromH->next;
  while (currentH)
  {
    nmb_history *nextH;
    nextH = currentH->next;
    free (currentH);
    currentH = nextH;
  }
  fromH->next = NULL;
}

nmb_history *
push_nmb_history (nmb_list * record)
{
  nmb_history *currentH;
  currentH = headHS;
  if (!currentH)
  {
    currentH = headHS = (nmb_history *) malloc (sizeof (nmb_history));
    currentH->previous = NULL;
  }
  else
  {
    while (currentH->next)
      currentH = currentH->next;
    currentH->next = (nmb_history *) malloc (sizeof (nmb_history));
    currentH->next->previous = currentH;
    currentH = currentH->next;
  }
  currentH->record = record;
  currentH->next = NULL;
  return currentH;
}

nmb_list *
push_nmb (char *serverIP)
{
  nmb_list *currentN;
  currentN = headN;
  if (!currentN)
  {
    currentN = headN = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->previous = NULL;
  }
  else
  {
    while (currentN->next)
      currentN = currentN->next;
    currentN->next = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->next->previous = currentN;
    currentN = currentN->next;
  }
  currentN->next = NULL;
  currentN->server = NULL;
  currentN->netbios = NULL;
  currentN->password = (unsigned char *) malloc (strlen (default_user) + 1);
  strcpy (currentN->password, default_user);

  currentN->shares = NULL;
  currentN->servers = NULL;
  currentN->workgroups = NULL;
  currentN->loaded = 0;
  currentN->serverIP = (char *) malloc (strlen (serverIP) + 1);
  strcpy (currentN->serverIP, serverIP);
  return currentN;
}

nmb_list *
push_nmbName (unsigned char *servidor)
{
  nmb_list *currentN;
  currentN = headN;
  if (!currentN)
  {
    currentN = headN = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->previous = NULL;
  }
  else
  {
    while (currentN->next)
      currentN = currentN->next;
    currentN->next = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->next->previous = currentN;
    currentN = currentN->next;
  }
  currentN->next = NULL;
  currentN->password = (unsigned char *) malloc (strlen (default_user) + 1);
  strcpy (currentN->password, default_user);

  currentN->shares = NULL;
  currentN->servers = NULL;
  currentN->workgroups = NULL;
  currentN->loaded = 0;

  currentN->server = (unsigned char *) malloc (strlen (servidor) + 1);
  currentN->netbios = (unsigned char *) malloc (strlen (servidor) + 1);
  currentN->serverIP = NULL;

  strcpy (currentN->netbios, servidor);
  strcpy (currentN->server, servidor);

  latin_1_unreadable (currentN->netbios);
  latin_1_readable (currentN->server);
  /*debuggit("DBG:pushing server ");debuggit(currentN->server);
     debuggit(" pushing netbios ");debuggit(currentN->netbios);
     debuggit("\n"); */

  return currentN;
}


void
zap_nmb (nmb_list * currentN)
{
  if (currentN->serverIP)
    free (currentN->serverIP);
  if (currentN->server)
    free (currentN->server);
  if (currentN->netbios)
    free (currentN->netbios);
  free (currentN);
}

void
reverse_smoke_nmb (nmb_list * fromN)
{
  nmb_list *currentN;
  if (!fromN)
    return;
  currentN = fromN->previous;
  while (currentN)
  {
    nmb_list *nextN;
    nextN = currentN->previous;
    zap_nmb (currentN);
    currentN = nextN;
  }
  fromN->previous = NULL;
}

void
smoke_nmb (nmb_list * fromN)
{
  nmb_list *currentN;
  if (!fromN)
    return;
  currentN = fromN->next;
  while (currentN)
  {
    nmb_list *nextN;
    nextN = currentN->next;
    zap_nmb (currentN);
    currentN = nextN;
  }
  fromN->next = NULL;
}

void
clean_nmb (void)
{
  if (headN)
  {
    smoke_nmb (headN);
    zap_nmb (headN);
    headN = NULL;
  }
}
typedef struct dostext_t {
	unsigned char readable;
	unsigned char unreadable;
} dostext_t;

/* starts at 0xc0 */
static dostext_t dostext[]={
 {0xc0, 0xb7}, /* À */
 {0xc1, 0xb5}, /* Á */
 {0xc2, 0xb6}, /* Â */
 {0xc3, 0xc7}, /* Ã */
 {0xc4, 0x8e}, /* Ä */
 {0xc5, 0x8f}, /* Å */
 {0xc6, 0x92}, /* Æ */
 {0xc7, 0x80}, /* Ç */
 {0xc8, 0xd4}, /* È */
 {0xc9, 0x90}, /* É */
 {0xca, 0xd2}, /* Ê */
 {0xcb, 0xd3}, /* Ë */
 {0xcc, 0xde}, /* Ì */
 {0xcd, 0xd6}, /* Í */
 {0xce, 0xd7}, /* Î */
 {0xcf, 0xd8}, /* Ï */
 {0xd0, 0xd1}, /* Ð */
 {0xd1, 0xa5}, /* Ñ */
 {0xd2, 0xe3}, /* Ò */
 {0xd3, 0xe0}, /* Ó */
 {0xd4, 0xe2}, /* Ô */
 {0xd5, 0xe5}, /* Õ */
 {0xd6, 0x99}, /* Ö */
 {0xd7, 0x9e}, /* × */
 {0xd8, 0x9d}, /* Ø */
 {0xd9, 0xeb}, /* Ù */
 {0xda, 0xe9}, /* Ú */
 {0xdb, 0xea}, /* Û */
 {0xdc, 0x9a}, /* Ü */
 {0xdd, 0xed}, /* Ý */
 {0xde, 0xe8}, /* Þ */
 {0xdf, 0xe1}, /* ß */
 {0xe0, 0x85}, /* à */
 {0xe1, 0xa0}, /* á */
 {0xe2, 0x83}, /* â */
 {0xe3, 0xc6}, /* ã */
 {0xe4, 0x84}, /* ä */
 {0xe5, 0x86}, /* å */
 {0xe6, 0x91}, /* æ */
 {0xe7, 0x87}, /* ç */
 {0xe8, 0x8a}, /* è */
 {0xe9, 0x82}, /* é */
 {0xea, 0x88}, /* ê */
 {0xeb, 0x89}, /* ë */
 {0xec, 0x8d}, /* ì */
 {0xed, 0xa1}, /* í */
 {0xee, 0x8c}, /* î */
 {0xef, 0x8b}, /* ï */
 {0xf0, 0xd0}, /* ð */
 {0xf1, 0xa4}, /* ñ */
 {0xf2, 0x95}, /* ò */
 {0xf3, 0xa2}, /* ó */
 {0xf4, 0x93}, /* ô */
 {0xf5, 0xe4}, /* õ */
 {0xf6, 0x94}, /* ö */
 {0xf7, 0xf6}, /* ÷ */
 {0xf8, 0x9b}, /* ø */
 {0xf9, 0x97}, /* ù */
 {0xfa, 0xa3}, /* ú */
 {0xfb, 0x96}, /* û */
 {0xfc, 0x81}, /* ü */
 {0xfd, 0xec}, /* ý */
 {0xfe, 0xe7}, /* þ */
 {0,0}
};

void dos_txt (char *the_char,gboolean readable){
  unsigned char *c;
  dostext_t *t;
  for (c=(unsigned char *) the_char; c[0]!=0; c++)  {
	  t=dostext;
	  while (t->readable){
		  if (readable){
			if (c[0] == t->unreadable) {c[0] = t->readable; break;}
		  } else {
			if (c[0] == t->readable) {c[0] = t->unreadable; break;}
		  }
		  t++;	  
	  }
  }
}

void latin_1_readable (char *the_char) {
        /*print_diagnostics("DBG: ascii_readable=");print_diagnostics(the_char);*/
	dos_txt(the_char,TRUE);
        /*print_diagnostics("-->");print_diagnostics(the_char);print_diagnostics("\n");*/
}
void latin_1_unreadable (char *the_char) {
        /*print_diagnostics("DBG: ascii_unreadable=");print_diagnostics(the_char);*/
	dos_txt(the_char,FALSE);
        /*print_diagnostics("-->");print_diagnostics(the_char);print_diagnostics("\n");*/
}



/****************************************/
