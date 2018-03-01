/*
 * entry.c
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
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <glib.h>
#include "entry.h"
#include "io.h"
#include "uri.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

int dup_stat(struct stat *tgt, struct stat *src){
  if ((!tgt) || (!src)) return -1;
  /* struct stat info */
  tgt->st_dev = src->st_dev;
  tgt->st_ino = src->st_ino;
  tgt->st_mode = src->st_mode;
  tgt->st_uid = src->st_uid;
  tgt->st_gid = src->st_gid;
  tgt->st_size = src->st_size;
  tgt->st_atime = src->st_atime;
  tgt->st_mtime = src->st_mtime;
  tgt->st_ctime = src->st_ctime;
#if 0
  /* not used, yet */
  tgt->st_nlink = src->st_nlink;
  tgt->st_blksize = src->st_blksize;
  tgt->st_blocks = src->st_blocks;
#endif
  return 0;	

}

void set_time(mdate *tgt,struct stat *src){
  struct tm *t;
   t = localtime (&(src->st_mtime));
  
   tgt->year = 1900 + t->tm_year;
   tgt->month = t->tm_mon + 1;
   tgt->day = t->tm_mday;
   tgt->hour = t->tm_hour;
   tgt->min = t->tm_min;
}

entry *entry_dupe (entry * en)
{
  entry *new_en = NULL;

  if (!en) return (NULL);
  new_en = g_malloc (sizeof (entry));
  new_en->path = g_strdup (en->path);
  new_en->label = g_strdup (en->label);
  new_en->type = en->type;
  new_en->flags = en->flags;
  new_en->org_mem = en->org_mem;
  /* struct stat info */
  if (dup_stat(&(new_en->st),&(en->st))<0){
    entry_free (new_en);
    return NULL;    
  }
  set_time(&(new_en->date),&(en->st));
  return (new_en);
}


/*
 */
void
entry_free (entry * en)
{
  if (en) {
    if (en->path) g_free (en->path);
    if (en->label) g_free (en->label);
    g_free (en);
  }
}

/*
 */
entry *entry_new (void)
{
  entry *en = g_malloc (sizeof (entry));
  if (!en) return (NULL);
  en->path = en->label = NULL;
  en->type = en->flags = 0;
  en->org_mem = NULL;
  return (en);
}

entry *entry_new_by_path_and_label (char *path, char *label)
{
  entry *en;

  en = entry_new ();
  if (!en) return (NULL);
  en->st.st_size = -13; /* force an update */
  en->type = 0; /* avoid non initialized variable potencial trouble */
  
  en->path = g_strdup (path);
  en->label = g_strdup (label);
  /* check for valid uri types: */
  switch (uri_type (path)){
    case URI_LOCAL:
    case URI_FILE:
	    if (entry_update (en) < 0) {
	      entry_free ( en);
	      return (NULL);
	    }
	    break;
    case URI_HTTP:
    case URI_FTP:
    case URI_SMB:
    case URI_TAR:
	    break;
    default:
	    entry_free (en);
	    return (NULL);	    
  }
  return (en);
}

static char *extract_label(char *path){
  static char *label=NULL,*p;
  
  if (label) g_free(label);
  label = g_strdup (path);
  p = strrchr (label, '/');
  if (p) {
    if (p != label)
    {
      if (*(p + 1) == '\0') { /* remove slash at the end */
	*p = '\0';
	/* search again */
	p = strrchr (label, '/');
	if (!p)	{ /* give up */
	  p = label;
	}
      } 
      else {p++;}
    }
  }
  else p = label;
  return p;
}

/*
 * extract label from path and call the function above
 */
entry *entry_new_by_path (char *path)
{
  char *label;
  entry *en; 
  label=extract_label(path);
  en = entry_new_by_path_and_label (path, label);
  return en;
}

entry *entry_new_by_type (char *path, int type)
{
  entry *en;

  en = entry_new ();
  if (!en) return (NULL);
  if (!en->path) return (NULL);

  en->path = g_strdup (path);
  en->label = g_strdup (extract_label(path));
  en->type = type;
  return (en);
}
#if 0
static int entry_type_update (entry * en){
  /* check for stale links */
  en->type = 0;
  if (S_ISLNK (en->st.st_mode)) { 
    struct stat s;
    en->type |= FT_LINK; 
    if (stat (en->path, &s) == -1) {
      en->type |= FT_STALE_LINK;
      return 1;
    }
    if (S_ISDIR (s.st_mode)) en->type |= FT_DIR;
    else if ((s.st_mode & S_IXUSR) || (s.st_mode & S_IXGRP) || (s.st_mode & S_IXOTH))
	en->type |= FT_EXE;  
    if (S_ISREG (s.st_mode)) en->type |= FT_FILE; 
    else if (S_ISCHR (s.st_mode)) en->type |= FT_CHAR_DEV;
    else if (S_ISBLK (s.st_mode)) en->type |= FT_BLOCK_DEV;
    else if (S_ISFIFO (s.st_mode)) en->type |= FT_FIFO;
    else if (S_ISSOCK (s.st_mode)) en->type |= FT_SOCKET;
  }  
  if (S_ISDIR (en->st.st_mode)){
      en->type |= FT_DIR;
      if (access (en->path, R_OK | X_OK) != 0)	en->type |= FT_DIR_PD;
  }
  if (S_ISREG (en->st.st_mode)){
      en->type |= FT_FILE;
      if ((en->st.st_mode & S_IXUSR) || (en->st.st_mode & S_IXGRP) || (en->st.st_mode & S_IXOTH))
	en->type |= FT_EXE;
  }
  else if (S_ISCHR (en->st.st_mode)) en->type |= FT_CHAR_DEV;
  else if (S_ISBLK (en->st.st_mode)) en->type |= FT_BLOCK_DEV;
  else if (S_ISFIFO (en->st.st_mode)) en->type |= FT_FIFO;
  else if (S_ISSOCK (en->st.st_mode)) en->type |= FT_SOCKET;
  else en->type |= FT_UNKNOWN;
  if (io_is_dirup (en->label)) en->type = FT_DIR_UP | FT_DIR;
  return (0);
}
#endif

/* update struct stat..
 * return -1 on failure (path has dissappeared)
 * return 0 on nothing done
 * return 1 on something has changed
 */
int entry_update (entry * en)
{
  struct stat s;
  int rc = 0;
  int tipo=0;
  
  /* don't do updates on internal tar-rpm entries */
  if (strncmp(en->path,"tar:",strlen("tar:")) == 0) return (0);
  if (strncmp(en->path,"rpm:",strlen("rpm:")) == 0) return (0);
  if (en->type & FT_ISROOT) tipo |= FT_ISROOT;
  if (en->type & FT_HAS_DUMMY) tipo |= FT_HAS_DUMMY;

  if (lstat (en->path, &s) == -1) {
	  return (-1); /* its gone */
  }
  /* don't do updates on noticed devices */
  /*if (en->type & (FT_CHAR_DEV|FT_BLOCK_DEV)) return (0);*/
  /*printf("dbg:%s 0x%x ? 0x%x\n",en->label,en->type,FT_CHAR_DEV|FT_BLOCK_DEV);*/

  
  if (S_ISLNK (s.st_mode)){
     struct stat ss;
     tipo |= FT_LINK;
     if (stat (en->path, &ss) == -1) {
        /*en->type = (FT_STALE_LINK|FT_LINK);*/
        en->type = FT_STALE_LINK;
	dup_stat(&(en->st),&s);
        set_time(&(en->date),&s);
	return (1); 
     }
     if (S_ISDIR (ss.st_mode)) {
	     tipo |= FT_DIR;
             if (access (en->path, R_OK | X_OK) != 0) tipo |= FT_DIR_PD;
	     else if (access (en->path, W_OK) != 0) tipo |= FT_DIR_RO;
             dup_stat(&s,&ss);
     } 
     if ((ss.st_mode & S_IXUSR) || (ss.st_mode & S_IXGRP) || (ss.st_mode & S_IXOTH))
             tipo |= FT_EXE;
     
     if (S_ISREG (ss.st_mode)) tipo |= FT_FILE;
     else if (S_ISCHR (ss.st_mode)) tipo |= FT_CHAR_DEV;
     else if (S_ISBLK (ss.st_mode)) tipo |= FT_BLOCK_DEV;
     else if (S_ISFIFO (ss.st_mode)) tipo |= FT_FIFO;
     else if (S_ISSOCK (ss.st_mode)) tipo |= FT_SOCKET;
     
  } else {
     if (S_ISDIR (s.st_mode)) {
	     tipo |= FT_DIR;
             if (access (en->path, R_OK | X_OK) != 0) tipo |= FT_DIR_PD;
	     else if (access (en->path, W_OK) != 0) tipo |= FT_DIR_RO;
     }
     if (S_ISREG (s.st_mode)){
        tipo |= FT_FILE;
        if ((s.st_mode & S_IXUSR) || (s.st_mode & S_IXGRP) || (s.st_mode & S_IXOTH))
             tipo |= FT_EXE;
        if (strstr(en->label,".")){
	  char *w;
	  w=strrchr(en->label,'.');
	  if (strcmp(w,".rpm")==0) tipo |= (FT_RPM|FT_HAS_DUMMY);
	  if (strcmp(w,".tar")==0) tipo |= (FT_TAR|FT_HAS_DUMMY);
	  if (strcmp(w,".tgz")==0) tipo |= (FT_TAR|FT_HAS_DUMMY|FT_GZ);
	  if (strcmp(w,".gz")==0) {
		  tipo |= (FT_GZ);
		  if (strstr(en->label,".tar.gz")) tipo |= (FT_TAR|FT_HAS_DUMMY);
	  } 
	  if (strcmp(w,".Z")==0) {
		  tipo |= (FT_COMPRESS);
		  if (strstr(en->label,".tar.Z")) tipo |= (FT_TAR|FT_HAS_DUMMY);
	  }
	  if (strcmp(w,".bz2")==0) {
		  tipo |= (FT_BZ2);
		  if (strstr(en->label,".tar.bz2")) tipo |= (FT_TAR|FT_HAS_DUMMY);
	  }
	}
     }
      else if (S_ISCHR (s.st_mode)) tipo |= FT_CHAR_DEV;
     else if (S_ISBLK (s.st_mode)) tipo |= FT_BLOCK_DEV;
     else if (S_ISFIFO (s.st_mode)) tipo |= FT_FIFO;
     else if (S_ISSOCK (s.st_mode)) tipo |= FT_SOCKET;
     else tipo |= FT_UNKNOWN;
  }
  if (io_is_dirup (en->label))tipo = FT_DIR_UP | FT_DIR;
  
  
  /*if (EN_IS_DIR (en) && (!S_ISDIR (ss.st_mode))) return (0);*/
  
  if (en->st.st_size<0) rc = 1;
  if (en->st.st_size != s.st_size) rc = 1; 
  else if (en->st.st_ino != s.st_ino) rc = 1;
  else if (en->st.st_mtime != s.st_mtime) rc = 1;
  else if (en->st.st_mode != s.st_mode) rc = 1;
  else if (en->st.st_uid != s.st_uid) rc = 1;
  else if (en->st.st_gid != s.st_gid) rc = 1;
#if 0 /* not these */
  else if (en->st.st_dev != s.st_dev) rc = 1;
  else if (en->st.st_atime != s.st_atime) rc = 1;
  else if (en->st.st_ctime != s.st_ctime) rc = 1;
#endif
  
  if (rc) {
    dup_stat(&(en->st),&s);
    set_time(&(en->date),&s);
    en->type = tipo;
  } else {  /* just check for stale links */
    if (S_ISLNK (s.st_mode)) { 
      if (stat (en->path, &s) == -1) {
        /*en->type = (FT_STALE_LINK|FT_LINK);*/
        en->type = FT_STALE_LINK;
	/*printf("dbg:stalelink2 %s\n",en->path);*/
        return 1;
      }
    }
  }  
  return (rc);
}


