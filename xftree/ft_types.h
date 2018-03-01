/*
 * ft_types.h
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

#ifndef __FT_TYPES_H__
#define __FT_TYPES_H__


#define FT_DIR			(1<<0)
#define FT_DIR_UP		(1<<1)
#define FT_FILE			(1<<2)
#define FT_CHAR_DEV		(1<<3)
#define FT_BLOCK_DEV		(1<<4)
#define FT_FIFO			(1<<5)
#define FT_SOCKET		(1<<6)
#define FT_EXE			(1<<7)
#define FT_HIDDEN		(1<<8)
#define FT_LINK			(1<<9)
#define FT_DIR_PD		(1<<10)
#define FT_STALE_LINK		(1<<11)
#define FT_UNKNOWN		(1<<12)
#define FT_DUMMY		(1<<13)

#define FT_TAR			(1<<14)
#define FT_TARCHILD		(1<<15)
#define FT_HAS_DUMMY		(1<<16)
#define FT_GZ			(1<<17)
#define FT_COMPRESS		(1<<18)
#define FT_BZ2			(1<<19)

#define FT_RPM			(1<<20)
#define FT_RPMCHILD		(1<<21)
#define FT_DIR_RO		(1<<22)
#define FT_PD			(1<<23)
#define FT_ISROOT		(1<<24)

#define FT_READONLY		(0) /* FIXME */
#define FT_SMB			(1<<26)
#define FT_WG1			(1<<3) /* over chardev */
#define FT_WG2			(1<<4) /* over blockdev */
#define FT_COMP1		(1<<5) /* over fifo */
#define FT_COMP2		(1<<6) /* over socket */
#define FT_PRINT		(1<<12) /* over unknown */


/* bitwise or for flags that should be inherited */
#define INHERIT_FLAG_MASK	(IGNORE_HIDDEN)
#define IGNORE_HIDDEN 		0x01
#define HIDDEN_PRESENT 		0x02
#define HAS_DUMMY 		0x04



#endif
