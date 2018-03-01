/*  gxfce
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



#ifndef __MY_STRING_H__
#define __MY_STRING_H__

char *my_memmove (char *, char *, int);
char *skiphead (char *);
char *skiptail (char *);
char *nextl (char *s);
char *cleanup (char *);
char my_casecmp (char, char);
char my_strncmp (char *, char *, int);
char my_strcmp (char *, char *);
char my_strncasecmp (char *, char *, int);
char my_strcasecmp (char *, char *);
char *tohex (char *, short int);
char *my_strrchr (char *s, char c);

#endif
