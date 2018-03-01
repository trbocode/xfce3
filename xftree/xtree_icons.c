/*
 * xtree_icons.c
 *
 * Copyright (C)2002 GNU-GPL
 * 
 * Edscott Wilson Garcia  for xfce project
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
#include <stdlib.h>
#include <string.h>

#include "../xftree/icons.h"
#include "../xftree/xtree_icons.h"
#include "../xftree/ft_types.h"
#include "xpmext.h"

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#ifdef HAVE_GDK_IMLIB
#include <gdk_imlib.h>
#endif

int pixmap_level=4;

/* pixmap list */
typedef struct pixmap_list {
GdkPixmap  **pixmap;
GdkBitmap  **pixmask;
char **xpm;
} pixmap_list;

typedef struct gen_pixmap_list {
GdkPixmap  **pixmap;
GdkPixmap  **pixmask;
char **xpm;
char *c;
int kolor;
} gen_pixmap_list;

enum
{
  PIX_DIR_OPEN=0, 
  PIX_DIR_CLOSE,PIX_DIR_UP,
  PIX_DIR_PD,
  PIX_DIR_RO,
  PIX_DIR_RO_OPEN,PIX_DIR_RO_OPEN_DOT,PIX_DIR_OPEN_DOT,
  PIX_DIR_OPEN_HIDDEN,PIX_DIR_RO_OPEN_HIDDEN,
  PIX_PD,
  PIX_PAGE,PIX_PAGE_C,PIX_PAGE_F,PIX_PAGE_O,
  	PIX_PAGE_H,PIX_PAGE_LNK,PIX_CORE,PIX_TAR,
	PIX_COMPRESSED,PIX_IMAGE,PIX_TEXT,PIX_MAIL,
	PIX_BAK,PIX_DUP,PIX_TAR_TABLE,PIX_TAR_EXP,
	PIX_TAR_TABLE_R,PIX_TAR_EXP_R,PIX_PS,
	PIX_ADOBE,PIX_PO,PIX_WORD,
  PIX_PAGE_AUDIO,
  PIX_PACKAGE,
  PIX_LINKFLAG, 
  PIX_EXEFLAG, 
  PIX_PAGE_HTML, 
  PIX_PAGE_MOVIE, 
  PIX_CHAR_DEV,
  PIX_FIFO,
  PIX_SOCKET,
  PIX_BLOCK_DEV,
  PIX_STALE_LNK,
  PIX_EXE,PIX_EXE_SCRIPT,PIX_EXE_LINK,
  PIX_EXE_FILE,
  PIX_COMP1,PIX_COMP2,
  PIX_WG1,PIX_WG2,
  PIX_PRINT,
  LAST_PIX
};
/* don't repeat masks that already exist */
enum
{
  PIM_DIR_OPEN=0,
  PIM_DIR_CLOSE,
  PIM_PACKAGE,
  PIM_LINKFLAG, 
  PIM_EXEFLAG, 
  PIM_DIR_PD,
  PIM_PD,
  PIM_PAGE,
  PIM_PAGE_HTML, 
  PIM_CHAR_DEV,
  PIM_FIFO,
  PIM_SOCKET,
  PIM_BLOCK_DEV,
  PIM_EXE,
  PIM_CORE,
  PIM_EXE_FILE,
  PIM_STALE_LNK,
  PIM_COMP,
  PIM_WG,
  PIM_PRINT,
  LAST_PIM
};

static GdkPixmap *gPIX[LAST_PIX*4];/*1=normal,2=exe,3=lnk,4=lnk+exe*/
static GdkPixmap *gPIM[LAST_PIM*4];
static int pix_w=16,pix_h=16;
static GtkWidget *hack=NULL; 

/* masks that are duplicated elsewhere are initialized to NULL */
static pixmap_list pixmaps[]={
	{gPIX+PIX_PAGE,		gPIM+PIM_PAGE,		page_xpm},
	{gPIX+PIX_PS,		NULL,			page_ps_xpm},
	{gPIX+PIX_ADOBE,	NULL,			page_adobe_xpm},
	{gPIX+PIX_PACKAGE,	gPIM+PIM_PACKAGE,	package_green_xpm},
	{gPIX+PIX_LINKFLAG,	gPIM+PIM_LINKFLAG,	link_flag_xpm},
	{gPIX+PIX_EXEFLAG,	gPIM+PIM_EXEFLAG,	exe_xpm},
	{gPIX+PIX_PAGE_MOVIE,	NULL,			page_movie_xpm},
	{gPIX+PIX_PAGE_AUDIO,	NULL,			page_audio_xpm},
	{gPIX+PIX_TEXT,		NULL,			page_text_xpm},
	{gPIX+PIX_COMPRESSED,	NULL,			page_compressed_xpm},
	{gPIX+PIX_IMAGE,	NULL,			page_image_xpm},
	{gPIX+PIX_TAR,		NULL,			page_tar_xpm},
	{gPIX+PIX_PO,		NULL,			page_po_xpm},
	{gPIX+PIX_BAK,		NULL,			page_backup_xpm},
	{gPIX+PIX_DIR_PD,	NULL,			dir_pd_xpm},
	{gPIX+PIX_PD,		gPIM+PIM_PD,		pd_xpm},
	{gPIX+PIX_DIR_RO,	NULL,			dir_ro_xpm},
	{gPIX+PIX_DIR_RO_OPEN,	NULL,			dir_ro_open_xpm},
  {gPIX+PIX_DIR_RO_OPEN_DOT,	NULL,			dir_ro_open_dot_xpm},
	{gPIX+PIX_DIR_OPEN_DOT,	NULL,			dir_open_dot_xpm},
  {gPIX+PIX_DIR_RO_OPEN_HIDDEN,	NULL,			dir_ro_open_hidden_xpm},
  {gPIX+PIX_DIR_OPEN_HIDDEN,	NULL,			dir_open_hidden_xpm},
	{gPIX+PIX_DIR_OPEN,	gPIM+PIM_DIR_OPEN,	dir_open_xpm},
	{gPIX+PIX_DIR_CLOSE,	gPIM+PIM_DIR_CLOSE,	dir_close_xpm},
	{gPIX+PIX_DIR_UP,	NULL,			dir_up_xpm},
	{gPIX+PIX_CORE,		gPIM+PIM_CORE,		page_core_xpm},
	{gPIX+PIX_CHAR_DEV,	gPIM+PIM_CHAR_DEV,	char_dev_xpm},
	{gPIX+PIX_BLOCK_DEV,	gPIM+PIM_BLOCK_DEV,	block_dev_xpm},
	{gPIX+PIX_FIFO,		gPIM+PIM_FIFO,		fifo_xpm},
	{gPIX+PIX_SOCKET,	gPIM+PIM_SOCKET,	socket_xpm},
	{gPIX+PIX_PAGE_HTML,	gPIM+PIM_PAGE_HTML,	page_html_xpm},
	{gPIX+PIX_STALE_LNK,	gPIM+PIM_STALE_LNK,	stale_lnk_xpm},
	{gPIX+PIX_EXE_FILE,	gPIM+PIM_EXE_FILE,	exe_file_xpm},
	{gPIX+PIX_COMP1,	gPIM+PIM_COMP,		comp1_xpm},
	{gPIX+PIX_COMP2,	NULL,			comp2_xpm},
	{gPIX+PIX_WG1,		gPIM+PIM_WG,		wg1_xpm},
	{gPIX+PIX_WG2,		NULL,			wg2_xpm},
	{gPIX+PIX_PRINT,	gPIM+PIM_PRINT,		print_xpm},
	{NULL,NULL,NULL}
};

static gen_pixmap_list gen_pixmaps[]={
	{gPIX+PIX_PAGE_C,	NULL,	page_xpm,	"c",	0},
	{gPIX+PIX_PAGE_H,	NULL,	page_xpm,	"h",	1},
	{gPIX+PIX_PAGE_F,	NULL,	page_xpm,	"f",	0},
	{gPIX+PIX_PAGE_O,	NULL,	page_xpm,	"o",	3},
	{gPIX+PIX_MAIL,		NULL,	page_xpm,	"@",	4},
	{gPIX+PIX_WORD,		NULL,	page_xpm,	"W",	1},
	{gPIX+PIX_DUP,		NULL,	page_xpm,	"*",	3},
	{gPIX+PIX_TAR_TABLE,	NULL,	dir_close_xpm,	".",	0},
	{gPIX+PIX_TAR_EXP,	NULL,	dir_open_xpm,	".",	0},
	{gPIX+PIX_TAR_TABLE_R,	NULL,	dir_close_xpm,	".",	2},
	{gPIX+PIX_TAR_EXP_R,	NULL,	dir_open_xpm,	".",	2},
	{NULL,NULL,NULL,0}
};



static gboolean checkif_type(char **Type,char *loc){
  int i;
  for (i=0;Type[i]!=NULL;i++) 
	  if (strcmp(loc,Type[i])==0) return TRUE;
  return FALSE;
}

static gboolean image_type(char *loc){
  char *Type[]={
	  ".jpg",".JPG",".gif",".GIF",".png",".PNG",
	  ".JPEG",".jpeg",".TIFF",".tiff",".xbm","XBM",
	  ".XPM",".xpm",".XCF",".xcf",".PCX",".pcx",
	  ".BMP",".bmp",".tif",".TIF",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean text_type(char *loc){
  char *Type[]={
	  ".txt",".TXT",".tex",".TEX",
	  ".doc",".DOC",
	  ".readme",".README",
	  ".pl",".sh",".csh",".py",".tsh",
	  ".sgml",".wml",".wmls",".etx",
	  ".xml",".asc",".rtf",".rtx",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean movie_type(char *loc){
  char *Type[]={
	  ".avi",".AVI",".movie",
	  ".qt",".mov",".mpeg",".mpg",".mpe",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean compressed_type(char *loc){
  char *Type[]={
	  ".gz",".tgz",".bz2",".Z",
	  ".zip",".taz",".lzh",".z",
	  ".ZIP",".bz",".tz",
	  ".arj",
	  ".lha",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean www_type(char *loc){
  char *Type[]={
	  ".html",".htm",".HTM",".HTML",
	  NULL
  };
  return checkif_type(Type,loc);			    
}
static gboolean audio_type(char *loc){
  char *Type[]={
	  ".wav",".mp3",".mid",".midi",
	  ".kar",".mpga",".mp2",
	  ".ra",".aif",".aiff",".ram",
	  ".rm",".au",".snd",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean mail_type(char *loc){
  char *Type[]={
	  "inbox","outbox","mbox","dead.letter",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean bak_type(char *loc){
  char *Type[]={
	  ".bak",".BAK",".old",".rpmsave",".rpmnew",
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static gboolean dup_type(char *loc){
  char *l;
  for (l=loc+1;*l!=0;l++){
	  if ((*l > '9')||(*l < '0')) return FALSE;
  }
  return TRUE;			    
}
static gboolean adobe_type(char *loc){
  char *Type[]={
	  ".pdf",".PDF",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean ps_type(char *loc){
  char *Type[]={
	  ".ps",".PS",
	  ".dvi",
	  NULL
  };
  return checkif_type(Type,loc);			  
}
static gboolean packed_type(char *loc){
  char *Type[]={
	  ".rpm",
	  ".deb",
	  /* ".cpio",*/
	  NULL
  };
  return checkif_type(Type,loc);			    
}

static void create_higher_pixmap(int PIXid){
  GdkGC *gc; 
  int PIXbase;

  if (pixmap_level<2) return; /* no higher icons at all */
  
  if (PIXid >= LAST_PIX*3) {
	  PIXbase = PIXid - (LAST_PIX*3);
  } else if (PIXid >= LAST_PIX*2) {
	  PIXbase = PIXid - (LAST_PIX*2);
  } else if (PIXid >= LAST_PIX) {
	  PIXbase = PIXid - LAST_PIX;
  }
  else {fprintf(stderr,"xftree: error PIXid=%d (max=%d)\n",PIXid,LAST_PIX*4);return;}
  if ((!gPIX[PIXbase])||(!hack)) {fprintf(stderr,"xftree: error 3344,pixbase=%d\n",PIXbase);return;}
  
  /*fprintf(stderr,"dbg: PIXid=%d \n",PIXid);*/
  gPIX[PIXid]=gdk_pixmap_new (hack->window,pix_w,pix_h,-1);
  gc = gdk_gc_new (hack->window);
  gdk_draw_pixmap(gPIX[PIXid],gc,gPIX[PIXbase], 
		0,0,0,0,
		pix_w,pix_h);
  
  if (PIXid >= LAST_PIX*2) {
	PIXbase = PIXid - (LAST_PIX*2);
	gdk_gc_set_clip_mask(gc,gPIM[PIM_EXEFLAG]);
	gdk_gc_set_clip_origin(gc,0,0);
	gdk_draw_pixmap(gPIX[PIXid],gc, gPIX[PIX_EXEFLAG],
        	0,0,0,0,
		pix_w,pix_h);
        gdk_gc_set_clip_mask(gc,NULL);
  } 
  if ((PIXid < LAST_PIX*2)||(PIXid >= LAST_PIX*3)) {
	PIXbase = PIXid - LAST_PIX;  
 	gdk_gc_set_clip_mask(gc,gPIM[PIM_LINKFLAG]);
	gdk_gc_set_clip_origin(gc,0,0);
	gdk_draw_pixmap(gPIX[PIXid],gc, gPIX[PIX_LINKFLAG],
        	0,0,0,0,
		pix_w,pix_h);
        gdk_gc_set_clip_mask(gc,NULL);
  }
  gdk_gc_destroy (gc);
  return; 
}

static void create_higher_bitmap(int PIMid){
  GdkGC *gc; 
  int PIMbase;


  if (pixmap_level<2) return; /* no higher icons at all */
  if (PIMid >= LAST_PIM*3) {
	  PIMbase = PIMid - (LAST_PIM*3);
  } else if (PIMid >= LAST_PIM*2) {
	  PIMbase = PIMid - (LAST_PIM*2);
  } else if (PIMid >= LAST_PIM) {
	  PIMbase = PIMid - LAST_PIM;
  }
  else {fprintf(stderr,"xftree: error PIXid=%d (max=%d)\n",PIMid,LAST_PIM*4);return;}
  if ((!gPIM[PIMbase])||(!hack)) {fprintf(stderr,"xftree: error 3345\n");return;}
  
  gPIM[PIMid]=gdk_pixmap_new (hack->window,pix_w,pix_h,1);
   if (!gPIM[PIMbase]){fprintf(stderr,"xftree: error 3348\n");return;}
  gc = gdk_gc_new (gPIM[PIMbase]);
  gdk_draw_pixmap(gPIM[PIMid],gc,gPIM[PIMbase],0,0,0,0,pix_w,pix_h);
  if (PIMid >= LAST_PIM*2) {
	PIMbase = PIMid - (LAST_PIM*2);
	gdk_gc_set_clip_mask(gc,gPIM[PIM_EXEFLAG]);
	gdk_gc_set_clip_origin(gc,0,0);
	gdk_draw_pixmap(gPIM[PIMid],gc, gPIM[PIM_EXEFLAG],
        	0,0,0,0,
		pix_w,pix_h);
        gdk_gc_set_clip_mask(gc,NULL);	
  } 
  if ((PIMid < LAST_PIM*2)||(PIMid >= LAST_PIM*3)) {
	PIMbase = PIMid - LAST_PIM;  
 	gdk_gc_set_clip_mask(gc,gPIM[PIM_LINKFLAG]);
	gdk_gc_set_clip_origin(gc,0,0);
	gdk_draw_pixmap(gPIM[PIMid],gc, gPIM[PIM_LINKFLAG],
        	0,0,0,0,
		pix_w,pix_h);
        gdk_gc_set_clip_mask(gc,NULL);
 
  }
  gdk_gc_destroy (gc);
  return; 
}



gboolean set_icon_pix(icon_pix *pix,int type,char *label,int flags) {
  char *loc,*loc2;
  gboolean isleaf=TRUE;
  int PIXid[4];
  
  /*defaults :*/
  PIXid[0]=PIXid[2]=PIX_PAGE;
  PIXid[1]=PIXid[3]=PIM_PAGE;
  /* error defaults */
  if (type&FT_PD){
   PIXid[0]=PIXid[2]=PIX_PD;
   PIXid[1]=PIXid[3]=PIM_PD;
  }

 /* smb type icons */
  if (type&FT_SMB){
      if (type&FT_PRINT){PIXid[0]=PIX_PRINT,PIXid[1]=PIM_PRINT;}
      if (type&FT_COMP1){PIXid[0]=PIX_COMP1,PIXid[1]=PIM_COMP;}
      if (type&FT_COMP2){PIXid[0]=PIX_COMP2,PIXid[1]=PIM_COMP;}
      if (type&FT_WG1){PIXid[0]=PIX_WG1,PIXid[1]=PIM_WG;}
      if (type&FT_WG2){PIXid[0]=PIX_WG2,PIXid[1]=PIM_WG;}
      /*if (type&FT_HIDDEN){PIXid[0]=PIX_HIDDEN,PIXid[1]=PIM_HIDDEN;}
      if (type&FT_READONLY){PIXid[0]=PIX_READONLY,PIXid[1]=PIM_READONLY;}*/
      goto icon_identified;	  
  }
  
  /* directories: no icon flag applied */
 if (type & FT_DIR_UP){
      PIXid[0]=PIX_DIR_UP;
      PIXid[1]=PIM_DIR_CLOSE;
      goto icon_identified;
 } 
 if (type & FT_DIR){
     PIXid[1]=PIM_DIR_CLOSE;
     PIXid[3]=PIM_DIR_OPEN;
     isleaf=FALSE;   
     if (type & FT_TARCHILD) {
	if ((type & FT_GZ)||(type & FT_COMPRESS )||(type & FT_BZ2)){
	        PIXid[0]=PIX_TAR_TABLE_R;
	        PIXid[2]=PIX_TAR_EXP_R;
	} else { /* noncompressed tar subdirectories */
	        PIXid[0]=PIX_TAR_TABLE;
	        PIXid[2]=PIX_TAR_EXP;
	}	
      } else {
 	PIXid[0]=PIX_DIR_CLOSE;
        if (flags & IGNORE_HIDDEN) {
		if (flags & HIDDEN_PRESENT) PIXid[2]=PIX_DIR_OPEN_HIDDEN;
		else PIXid[2]=PIX_DIR_OPEN;
	} else PIXid[2]=PIX_DIR_OPEN_DOT; 
        PIXid[1]=PIM_DIR_CLOSE;
        PIXid[3]=PIM_DIR_OPEN;
      }
      if (type & FT_DIR_PD) {
       PIXid[0]=PIX_DIR_PD;
       PIXid[2]=PIX_DIR_OPEN;
      }
      if (type & FT_DIR_RO) {
       PIXid[0]=PIX_DIR_RO;
       if (flags & IGNORE_HIDDEN) {
	       if (flags & HIDDEN_PRESENT) PIXid[2]=PIX_DIR_RO_OPEN_HIDDEN;
	       else PIXid[2]=PIX_DIR_RO_OPEN;
       }
       else PIXid[2]=PIX_DIR_RO_OPEN_DOT;
      }
      goto icon_identified;
  }   /* whatever it is, it's not a directory */

  /* special files : */
  if (type & FT_CHAR_DEV){ PIXid[0]=PIX_CHAR_DEV,PIXid[1]=PIM_CHAR_DEV; }
  else if (type & FT_BLOCK_DEV){PIXid[0]=PIX_BLOCK_DEV,PIXid[1]=PIM_BLOCK_DEV;}
  else if (type & FT_FIFO){PIXid[0]=PIX_FIFO,PIXid[1]=PIM_FIFO;}
  else if (type & FT_SOCKET){PIXid[0]=PIX_SOCKET,PIXid[1]=PIM_SOCKET;}
  else if (type & FT_STALE_LINK){PIXid[0]=PIX_STALE_LNK,PIXid[1]=PIM_STALE_LNK;}
  if (type & (FT_BLOCK_DEV|FT_CHAR_DEV|FT_FIFO|FT_SOCKET|FT_STALE_LINK))
	  goto icon_identified;

  /* assignment by filetype */
  /* case one: whole file label */
  if (!label) goto icon_identified;

   if (strcmp(label,"core")==0) {
       PIXid[0]=PIX_CORE;
       PIXid[1]=PIM_CORE;
       goto icon_identified;       
   }
   
  /* case two: act on extension */
   if ( (loc=strrchr(label,'.')) == NULL ) goto icon_identified;
   if (mail_type(label)) PIXid[0]=PIX_MAIL;	      
   else if (((loc2=strrchr(label,'-')) != NULL )&& dup_type(loc2)) PIXid[0]=PIX_DUP;
   else if (bak_type(loc)) PIXid[0]=PIX_BAK;
   else if (image_type(loc)) PIXid[0]=PIX_IMAGE;
   else if (text_type(loc))PIXid[0]= PIX_TEXT;
   else if (ps_type(loc))PIXid[0]=PIX_PS;
   else if (adobe_type(loc))PIXid[0]=PIX_ADOBE;
   else if (packed_type(loc)){
	   PIXid[0]=PIX_PACKAGE,PIXid[1]=PIM_PACKAGE;
	   PIXid[2]=PIX_PACKAGE,PIXid[3]=PIM_PACKAGE;
   }
   else if (compressed_type(loc)) {PIXid[0]=PIX_COMPRESSED,PIXid[2]=PIX_COMPRESSED,PIXid[3]=PIM_PAGE;}
   else if (www_type(loc)) {PIXid[0]=PIX_PAGE_HTML,PIXid[1]=PIM_PAGE_HTML;}
   else if (audio_type(loc))PIXid[0]= PIX_PAGE_AUDIO;
   else if (movie_type(loc))PIXid[0]= PIX_PAGE_AUDIO;
   else if (strcmp(loc,".obj")==0) PIXid[0]=PIX_PAGE_O;
   else if (strcmp(loc,".po")==0) PIXid[0]=PIX_PO;
   else if (strcmp(loc,".tar")==0){PIXid[0]=PIX_TAR;PIXid[2]=PIX_TAR;PIXid[3]=PIM_PAGE;}
   else if ((strcmp(loc,".exe")==0)||(strcmp(loc,".EXE")==0)){PIXid[0]=PIX_EXE_FILE;PIXid[1]=PIM_EXE_FILE;}
   else if (strlen(loc)==2) switch (loc[1]){
      case 'c': PIXid[0]=PIX_PAGE_C; break;
      case 'h': PIXid[0]=PIX_PAGE_H; break;
      case 'f': PIXid[0]=PIX_PAGE_F; break;
      case 'o': PIXid[0]=PIX_PAGE_O; break;
      default: break;				      
   }

icon_identified:
   if (type & (FT_TAR|FT_RPM)) isleaf=FALSE;   
   {
    int offsetB=0,offsetP=0; 
    if ((type & FT_EXE) && (PIXid[0]==PIX_PAGE)) {
	   PIXid[0]=PIX_EXE_FILE;PIXid[1]=PIM_EXE_FILE;
	   if (type & FT_LINK){offsetP=LAST_PIX,offsetB=LAST_PIM;}
    } else {
     if ((type & FT_LINK)&&(type & FT_DIR)){
	   offsetP=LAST_PIX,offsetB=LAST_PIM;  
     } else if ((type & FT_LINK)&&(type & FT_EXE)) {offsetP=3*LAST_PIX,offsetB=3*LAST_PIM;}
     else if (type & FT_LINK){
	     offsetP=LAST_PIX,offsetB=LAST_PIM;
     }
     else if (type & FT_EXE) {offsetP=2*LAST_PIX,offsetB=2*LAST_PIM;}
    }
    if (pixmap_level>1) {
     PIXid[0] += offsetP;
     PIXid[2] += offsetP;
     PIXid[1] += offsetB;
     PIXid[3] += offsetB;
    }
   }
  
  /* if the requested icon pixmap does not exist, 
   * create pixmap on X server */
  
  if (!gPIX[PIXid[0]]) {
          /*fprintf(stderr,"dbg: icondone PIXid=%d\n",PIXid[0]);*/
	  create_higher_pixmap(PIXid[0]);
	  create_higher_bitmap(PIXid[1]);
  }
  if (!gPIX[PIXid[2]]) {
          /*fprintf(stderr,"dbg: icondone PIXid=%d\n",PIXid[2]);*/
	  create_higher_pixmap(PIXid[2]);
	  create_higher_bitmap(PIXid[3]);
  }
  
  /* if icon pixmap creation was disabled, fallback to default */
  
  if (!gPIX[PIXid[0]]) {
	    PIXid[1]=PIXid[3]=PIM_PAGE;
	    PIXid[0]=PIXid[2]=PIX_PAGE;
  }
  
  pix->pixmap=gPIX[PIXid[0]];
  pix->pixmask=gPIM[PIXid[1]];
  pix->open=gPIX[PIXid[2]]; 
  pix->openmask=gPIM[PIXid[3]];
  
  return isleaf;
}

#
static void scale_pixmap(GtkWidget *hack,int h,GtkWidget *ctree,char **xpm,
		GdkPixmap **pixmap, GdkBitmap **pixmask){
	
#ifdef HAVE_GDK_PIXBUF
	if (pixmap_level>2 && h>0){
		GdkPixbuf *orig_pixbuf,*new_pixbuf;
		float r=0;
		int w,x,y;
	  	orig_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)(xpm));
		w=gdk_pixbuf_get_width (orig_pixbuf);
		if (w) r=(float)h/w; if (r<1.0) r=1.0;
		pix_w=x=r*w;
		pix_h=y=r*gdk_pixbuf_get_height (orig_pixbuf);

  		new_pixbuf  = gdk_pixbuf_scale_simple (orig_pixbuf,x,y,GDK_INTERP_BILINEAR);
  		gdk_pixbuf_render_pixmap_and_mask (new_pixbuf,pixmap,pixmask,
				gdk_pixbuf_get_has_alpha (new_pixbuf));
  		gdk_pixbuf_unref (orig_pixbuf);
  		gdk_pixbuf_unref (new_pixbuf);
 	        gtk_clist_set_row_height ((GtkCList *)ctree,h);
	} else
#else
#warning compilation without GDK_PIXBUF
#endif	
	{
	      	*pixmap = MyCreateGdkPixmapFromData(xpm,hack,pixmask,FALSE);
                gtk_clist_set_row_height ((GtkCList *)ctree,16);
	}


	return ;

}


static void recreate_higher_pixmaps(void){
  int i;
  for (i=LAST_PIX;i<LAST_PIX*3;i++){ 
#ifdef HAVE_GDK_PIXBUF
	  if (gPIX[i]){
		gdk_pixmap_unref(gPIX[i]);
	  	create_higher_pixmap(i);
	  }
#endif
  }
  for (i=LAST_PIM;i<LAST_PIM*3;i++){ 
#ifdef HAVE_GDK_PIXBUF
	  if (gPIM[i]) {
		gdk_pixmap_unref(gPIM[i]);
		create_higher_bitmap(i);
	  }
#endif
  }
  return; 
}

void create_pixmaps(int h,GtkWidget *ctree){
  GtkStyle  *style;
  GdkColormap *colormap;
  GdkGC *gc; 

  int i;
  if (!pixmap_level) return; /* no icons at all */
  
  /* hack: to be able to use icons globally, independent of xftree window.*/
  if (!hack) {hack = gtk_window_new (GTK_WINDOW_POPUP); gtk_widget_realize (hack);} 
#ifndef HAVE_GDK_PIXBUF
  else return; /* dont recreate without gdkpixbuf */
#endif
  
  	
  for (i=0;pixmaps[i].pixmap != NULL; i++){ 
	  if (*(pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(pixmaps[i].pixmap));
	  if ((pixmaps[i].pixmask)&&(*(pixmaps[i].pixmask) != NULL)) gdk_bitmap_unref(*(pixmaps[i].pixmask));
	  scale_pixmap(hack,h,ctree,pixmaps[i].xpm,pixmaps[i].pixmap,pixmaps[i].pixmask);
  }
  style=gtk_widget_get_style (ctree);
  if (pixmap_level>1) for (i=0;gen_pixmaps[i].pixmap != NULL; i++){
	int x,y;
	GdkColor back;
	GdkColor kolor[5];
	gint lbearing, rbearing, width, ascent, descent;

	kolor[0].pixel=0, kolor[0].red= kolor[0].green=0;kolor[0].blue =65535;
	kolor[1].pixel=1, kolor[1].red= kolor[1].blue=0; kolor[1].green=40000;
	kolor[2].pixel=2, kolor[2].blue=kolor[2].green=0;kolor[2].red  =65535;
	kolor[3].pixel=3, kolor[3].red= kolor[3].green=  kolor[3].blue =42000;
	kolor[4].pixel=4, kolor[4].red= kolor[4].green=  kolor[4].blue =0;
	
	colormap = gdk_colormap_get_system();
	gdk_colormap_alloc_color (colormap,kolor+gen_pixmaps[i].kolor,FALSE,TRUE);  
	  	  
        if (!gdk_color_white (colormap,&back)) fprintf(stderr,"DBG: no white\n");
	gc = gdk_gc_new (hack->window);
	gdk_gc_set_foreground (gc,kolor+gen_pixmaps[i].kolor);
	gdk_gc_set_background (gc,&back); 	

	if (*(gen_pixmaps[i].pixmap) != NULL) gdk_pixmap_unref(*(gen_pixmaps[i].pixmap));
	scale_pixmap(hack,h,ctree,gen_pixmaps[i].xpm,gen_pixmaps[i].pixmap,gen_pixmaps[i].pixmask);
        gdk_string_extents (style->font,gen_pixmaps[i].c,
			&lbearing,&rbearing,&width,&ascent,&descent);
#if 0	
  	fprintf(stderr,"dbg: drawing %s...lbearing=%d,rbearing=%d,width=%d,ascent=%d,descent=%d,measure=%d,width=%d,height=%d\n",
			gen_pixmaps[i].c,lbearing,rbearing,width,ascent,descent,
			gdk_char_measure(style->font,gen_pixmaps[i].c[0]),
			gdk_char_width(style->font,gen_pixmaps[i].c[0]),
			gdk_char_height (style->font,gen_pixmaps[i].c[0]));
#endif	

	/* numbers for page_xpm */
        x=h/4-lbearing+1;
        y=13*h/16-descent-1;
	gdk_draw_text ((GdkDrawable *)(*(gen_pixmaps[i].pixmap)),style->font,gc,
				x,y,gen_pixmaps[i].c,strlen(gen_pixmaps[i].c));
	gdk_gc_destroy (gc);
	  
  }  
  recreate_higher_pixmaps();
  return;
}

void init_pixmaps(void){
	int i;
  for (i=0;i<LAST_PIX*4;i++) gPIX[i]=NULL;
  for (i=0;i<LAST_PIM*4;i++) gPIM[i]=NULL;
}


