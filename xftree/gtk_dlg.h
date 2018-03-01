/*
 * gtk_dlg.h
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

#ifndef __GTK_DLG_H__
#define __GTK_DLG_H__

/* question dialogs: */
#define DLG_OK			0x01
#define DLG_CANCEL		0x02
#define DLG_YES			0x04
#define DLG_NO			0x08
#define DLG_CONTINUE		0x10 
#define DLG_CLOSE		0x20
#define DLG_ALL			0x40
#define DLG_SKIP		0x80

/* */
#define DLG_OK_CANCEL	(DLG_OK|DLG_CANCEL)
#define DLG_YES_NO	(DLG_YES|DLG_NO)
/* */
#define DLG_ENTRY_VIEW		0x100 /* question */
#define DLG_ENTRY_EDIT		0x200 /* question */
#define DLG_COMBO		0x400 /* question */
/* */
#define DLG_INFO		0x800
#define DLG_WARN		0x1000
#define DLG_ERROR		0x2000
#define DLG_QUESTION		0x4000
#define DLG_TEXT		0x8000

#define  DLG_MASK_TYPE		0x7fff
#define  DLG_MASK_QUESTION	0x47fc


#define DLG_RC_CANCEL		0
#define DLG_RC_OK		1
#define DLG_RC_ALL		2
#define DLG_RC_CONTINUE		3
#define DLG_RC_SKIP		4
#define DLG_RC_DESTROY		5

/* FIXME: need for DLG_MAX should be removed */
#define DLG_MAX		1024

/* true modal macros: (requiere toplevel widget address, p) */
#define xf_dlg_string(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_OK_CANCEL|DLG_ENTRY_EDIT|DLG_QUESTION,1)
#define xf_dlg_continue(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_CONTINUE|DLG_CANCEL,1)
#define xf_dlg_error(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_ERROR|DLG_CLOSE,1)
#define xf_dlg_error_continue(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_ERROR|DLG_CONTINUE|DLG_CANCEL,1)
#define xf_dlg_warning(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_WARN|DLG_CLOSE,1)
#define xf_dlg_info(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_INFO|DLG_CLOSE,1)
#define xf_dlg_ask(p,a) \
			xf_dlg_new(p,a,NULL,NULL,DLG_QUESTION|DLG_YES_NO,1)
#define xf_dlg_skip(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_QUESTION|DLG_SKIP|DLG_CANCEL,1)
#define xf_dlg_ok_skip(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_QUESTION|DLG_OK|DLG_SKIP|DLG_CANCEL,1)
#define xf_dlg_question(p,a,b) \
			xf_dlg_new(p,a,b,NULL,DLG_YES_NO|DLG_ENTRY_VIEW|DLG_QUESTION,1)
#define xf_dlg_question_l(p,a,b,c) \
			xf_dlg_new(p,a,b,NULL,c|DLG_OK_CANCEL|DLG_ENTRY_VIEW|DLG_QUESTION,1)
#define xf_dlg_combo(p,a,b,c) \
			xf_dlg_new(p,a,b,c,DLG_OK_CANCEL|DLG_COMBO,1)
#define xf_dlg_comboOK(p,a,b,c) \
			xf_dlg_new(p,a,b,c,DLG_OK_CANCEL|DLG_COMBO,0)


long xf_dlg_new (GtkWidget *parent,const char *label, char *defval, void *data, int flags, gboolean def_cancel);


#endif
