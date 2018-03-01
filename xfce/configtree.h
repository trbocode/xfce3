/*
 * Copyright (C) 2002 Guido Draheim (guidod@gmx.de)
 *
 * This program part is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * You shall treat rule 3 of the license as a dual license option herein.
 *
 * This program part is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XML_CONFIGTREE_H__
#define __XML_CONFIGTREE_H__ 1

/* returns xmlDocPtr, if reset=1 then it is filled with the default values */
extern void* gxfce_set_configs (config * newconf, int reset); 

/* both _write_ and _reset_ call _set_ and then simply do xmlSaveFile */
extern void gxfce_write_configs (void);
extern void gxfce_reset_configs (void);

extern void gxfce_read_configs (void);
extern void gxfce_backup_configs (char *extension);

#endif
