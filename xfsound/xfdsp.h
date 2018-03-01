/*  xfsound
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
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


#ifndef __XFDSP_H__
#define __XFDSP_H__

#define XF_IND		1
#define ST_GET		0
#define DSP_NAME	"/dev/dsp"
#define ARTSD_CMD       "exec artswrapper"

typedef int ST_CONFIG[3];

void sound_init (void);
int  i_play (char *);
int  setcard (void);
void cardctl (int, ST_CONFIG *);
int  cardbusy (void);

#endif
