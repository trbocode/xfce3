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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include "constant.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GList *tooltips = NULL;

GtkTooltips *
my_tooltips_new (guint delay)
{
  GtkTooltips *res = gtk_tooltips_new ();

  gtk_tooltips_set_delay (res, delay);

  tooltips = g_list_append (tooltips, (gpointer) res);

  return res;
}

void
update_delay_tooltips (guint delay)
{
  GList *tmp;
  tmp = tooltips;
  while (tmp)
  {
    gtk_tooltips_set_delay (GTK_TOOLTIPS (tmp->data), delay);
    tmp = tmp->next;
  }
}

void
free_tooltips_list (void)
{
  g_list_free (tooltips);
}
