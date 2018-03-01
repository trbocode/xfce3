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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfcolor.h"
#include "my_string.h"
#include "fileutil.h"
#include "xfce-common.h"
#include "lightdark.h"
#include "xpmext.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define GTK_WIDGET_USER_STYLE(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_USER_STYLE) != 0)
#define atom_read_colors_name "_XFCE_READ_COLORS"

#define LIGHT 1.15
#define DARK  0.90

typedef struct
{
  GtkStyle *old;
  GtkStyle *new;
}
gtkstyle_pair;

static GdkAtom atom_read_colors = GDK_NONE;
static GdkAtom atom_rcfiles = GDK_NONE;
static gboolean REPAINT_IN_PROGRESS = FALSE;

char *rccolor = "xfcolors";
char *gtkrcfile = "/.gtkrc";
char *xfcegtkrcfile = "/.xfce/xfcegtkrc";
char *gtktemprcfile = "/.xfce/temp.xfcegtkrc";

char *resmap[] = { "",          /* Color 0 */
  "",                           /* Color 1 */
  "",                           /* Color 2 */
  "",                           /* Color 3 */
  "*XmList.background",
  "*XmLGrid.background",
  "Netscape*XmList.background",
  "Netscape*XmLGrid.background",
  "",                           /* Color 4 */
  "*text*background",
  "*list*background",
  "*Text*background",
  "*List*background",
  "*textBackground",
  "XTerm*background",
  "*XmTextField.background",
  "*XmText.background",
  "Netscape*XmTextField.background",
  "Netscape*XmText.background",
  "",                           /* Color 5 */
  "",                           /* Color 6 */
  "*background",
  "*Background",
  "nscal*Background",
  "*Menu*background",
  "OpenWindows*WindowColor",
  "Window.Color.Background",
  "netscape*background",
  "Netscape*background",
  ".netscape*background",
  "Ddd*background",
  "Emacs*Background",
  "Emacs*backgroundToolBarColor",
  "",                           /* Color 7 */
  "",                           /* Text Color */
  NULL
};

char *resmapSel[] = { "",       /* Color 0 */
  "",                           /* Color 1 */
  "",                           /* Color 2 */
  "",                           /* Color 3 */
  "*XmList.selectBackground",
  "*XmLGrid.selectBackground",
  "Netscape*XmList.selectBackground",
  "Netscape*XmLGrid.selectBackground",
  "",                           /* Color 4 */
  "*XmTextField.selectBackground",
  "*XmText.selectBackground",
  "XTerm*selectBackground",
  "Netscape*XmTextField.selectBackground",
  "Netscape*XmText.selectBackground",
  "*selectBackground",
  "",                           /* Color 5 */
  "",                           /* Color 6 */
  "",                           /* Color 7 */
  NULL
};

char *resmapTxt[] = { "",       /* Color 0 */
  "",                           /* Color 1 */
  "",                           /* Color 2 */
  "",                           /* Color 3 */
  "*XmList.foreground",
  "*XmLGrid.foreground",
  "Netscape*XmList.foreground",
  "Netscape*XmLGrid.foreground",
  "",                           /* Color 4 */
  "*text*foreground",
  "*list*foreground",
  "*Text*foreground",
  "*List*foreground",
  "*textForeground",
  "XTerm*foreground",
  "*XmTextField*foreground",
  "*XmText*foreground",
  "Netscape*XmTextField.foreground",
  "Netscape*XmText.foreground",
  "",                           /* Color 5 */
  "",                           /* Color 6 */
  "*foreground",
  "*Foreground",
  "nscal*Foreground",
  "OpenWindows.WindowForeground",
  "Window.Color.Foreground",
  "netscape*foreground",
  "Netscape*foreground",
  ".netscape*foreground",
  "Ddd*foreground",
  "Emacs*Foreground",
  "Emacs*foregroundToolBarColor",
  /* Color 7 */
  NULL
};

char *resmapSelTxt[] = { "",    /* Color 0 */
  "",                           /* Color 1 */
  "",                           /* Color 2 */
  "",                           /* Color 3 */
  "*XmList.selectForeground",
  "*XmLGrid.selectForeground",
  "Netscape*XmList.selectForeground",
  "Netscape*XmLGrid.selectForeground",
  "",                           /* Color 4 */
  "*XmTextField.selectForeground",
  "*XmText.selectForeground",
  "XTerm*selectForeground",
  "Netscape*XmTextField.selectForeground",
  "Netscape*XmText.selectForeground",
  "*selectForeground",
  "",                           /* Color 5 */
  "",                           /* Color 6 */
  "",                           /* Color 7 */
  NULL
};

char *fntmap[] = { "*fontList",
  "Netscape*fontList",
  ".netscape*fontList",
  NULL
};

char *additionnal[] = { "*DtTerm*shadowThickness: 1",
  "*enableButtonTab: True",
  "*enableEtchedInMenu: True",
  "*enableMenuInCascade: True",
  "*enableMultiKeyBindings: True",
  "*enableThinThickness: True",
  "Netscape*banner*folderDropdown*fontList: -adobe-courier-medium-r-*-*-*-100-*-*-*-*-iso8859-*,-adobe-courier-bold-r-*-*-*-100-*-*-*-*-iso8859-*=BOLD",
  "*prefs*pageTitle.fontList: -*-helvetica-medium-r-normal-*-*-120-*-*-*-*-iso8859-*=NORMAL,-*-helvetica-bold-r-*-*-*-140-*-*-*-*-iso8859-*=BOLD,-*-helvetica-medium-o-normal-*-*-120-*-*-*-*-iso8859-*=ITALIC",
  "Netscape*AddressOutlinerPopup*fontList:-*-helvetica-medium-r-*-*-*-100-*-*-*-*-iso8859-*,-*-helvetica-bold-r-*-*-*-100-*-*-*-*-iso8859-*=BOLD,-*-helvetica-medium-o-*-*-*-100-*-*-*-*-iso8859-*=ITALIC",
  "Netscape*XmLGrid*fontList: -*-helvetica-medium-r-*-*-*-100-*-*-*-*-iso8859-*,-*-helvetica-bold-r-*-*-*-100-*-*-*-*-iso8859-*=BOLD,-*-helvetica-medium-o-*-*-*-100-*-*-*-*-iso8859-*=ITALIC",
  NULL
};

char *xres[] = { "/.Xdefaults",
  "/.Xresources",
  "/.xrdb",
  NULL
};

/* 
 * The following functions are taken directly from GTK+ source code
 */

static void
rgb_to_hls (double *r,
            double *g,
            double *b)
{
  double min;
  double max;
  double red;
  double green;
  double blue;
  double h, l, s;
  double delta;
  
  red = *r;
  green = *g;
  blue = *b;
  
  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;
      
      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;
      
      if (red < blue)
        min = red;
      else
        min = blue;
    }
  
  l = (max + min) / 2;
  s = 0;
  h = 0;
  
  if (max != min)
    {
      if (l <= 0.5)
        s = (max - min) / (max + min);
      else
        s = (max - min) / (2 - max - min);
      
      delta = max -min;
      if (red == max)
        h = (green - blue) / delta;
      else if (green == max)
        h = 2 + (blue - red) / delta;
      else if (blue == max)
        h = 4 + (red - green) / delta;
      
      h *= 60;
      if (h < 0.0)
        h += 360;
    }
  
  *r = h;
  *g = l;
  *b = s;
}

static void
hls_to_rgb (double *h,
            double *l,
            double *s)
{
  double hue;
  double lightness;
  double saturation;
  double m1, m2;
  double r, g, b;
  
  lightness = *l;
  saturation = *s;
  
  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;
  
  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        r = m2;
      else if (hue < 240)
        r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        r = m1;
      
      hue = *h;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        g = m2;
      else if (hue < 240)
        g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        g = m1;
      
      hue = *h - 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        b = m2;
      else if (hue < 240)
        b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        b = m1;
      
      *h = r;
      *l = g;
      *s = b;
    }
}

void color_shade (unsigned short *r, unsigned short *g, unsigned short *b, double   k)
{
  double red;
  double green;
  double blue;
  
  red   = (double) *r / 65535.0;
  green = (double) *g / 65535.0;
  blue  = (double) *b / 65535.0;
  
  rgb_to_hls (&red, &green, &blue);
  
  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;
  
  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;
  
  hls_to_rgb (&red, &green, &blue);
  
  *r = red   * 65535.0;
  *g = green * 65535.0;
  *b = blue  * 65535.0;
}

int
brightness (int r, int g, int b)
{
  return ((100 * (r + g + b) / 765));
}

int
brightness_pal (const XFCE_palette * p, int index)
{
  if ((p) && (index >= 0) && (index < NB_XFCE_COLORS))
  {
    return (brightness ((unsigned short) p->r[index], (unsigned short) p->g[index], (unsigned short) p->b[index]));
  }
  else
  {
    return (0);
  }
}

void
apply_xpalette (XFCE_palette * p, gboolean add_or_remove)
{
  char buffer[256];
  char mode[8];
  int i = 0;
  int color = 0;
  int tube[2];
  int sonpid;
  char c[20];
  char *args[4] = { "xrdb", NULL, "-all", NULL };
  char *tempstr;

  /* Add or remove the properties to the display */
  if (add_or_remove)
    strcpy (mode, "-load");
  else
    strcpy (mode, "-remove");
  args[1] = mode;

  /* Initiate communication pipe... */
  if (pipe (tube) == -1)
  {
    fprintf (stderr, "XFce : cannot open pipe\n");
    return;
  }

  /* ...And fork */
  switch (sonpid = fork ())
  {
  case 0:
    close (0);
    dup (tube[0]);
    close (tube[0]);
    close (tube[1]);
    execvp (args[0], args);
    perror ("exec failed");
    _exit (0);
    break;
  case -1:
    fprintf (stderr, "XFce : cannot execute fork()\n");
    close (tube[0]);
    close (tube[1]);
    return;
    break;
  default:
    break;
  }
  i = 0;
  color = 0;
  while (resmap[i])
  {
    if (!strlen (resmap[i]))
      color++;
    else
    {
      color_to_hex (c, p, color);
      snprintf (buffer, 255, "%s: %s\n", resmap[i], c);
      write (tube[1], buffer, strlen (buffer));
    }
    i++;
  }
  i = 0;
  color = 0;
  while (resmapSel[i])
  {
    if (!strlen (resmapSel[i]))
      color++;
    else
    {
      snprintf (buffer, 255, "%s: #%02X%02X%02X\n", resmapSel[i], ((unsigned short) p->r[color]) ^ 255, ((unsigned short) p->g[color]) ^ 128, ((unsigned short) p->b[color]) ^ 64);
      write (tube[1], buffer, strlen (buffer));
    }
    i++;
  }
  i = 0;
  color = 0;
  while (resmapTxt[i])
  {
    if (!strlen (resmapTxt[i]))
      color++;
    else
    {
      int howbright;

      howbright = brightness (p->r[color], p->g[color], p->b[color]);
      if (howbright < fadeblack)
        snprintf (buffer, 255, "%s: #FFFFFF\n", resmapTxt[i]);
      else
        snprintf (buffer, 255, "%s: #000000\n", resmapTxt[i]);
      write (tube[1], buffer, strlen (buffer));
    }
    i++;
  }
  i = 0;
  color = 0;
  while (resmapSelTxt[i])
  {
    if (!strlen (resmapSelTxt[i]))
      color++;
    else
    {
      int howbright;

      howbright = brightness (((unsigned short) p->r[color]) ^ 255, ((unsigned short) p->g[color]) ^ 128, ((unsigned short) p->b[color]) ^ 64);
      if (howbright < 50)
        snprintf (buffer, 255, "%s: #FFFFFF\n", resmapSelTxt[i]);
      else
        snprintf (buffer, 255, "%s: #000000\n", resmapSelTxt[i]);
      write (tube[1], buffer, strlen (buffer));
    }
    i++;
  }
  i = 0;
  while (fntmap[i])
  {
    snprintf (buffer, 255, "%s: %s\n", fntmap[i], p->fnt);
    write (tube[1], buffer, strlen (buffer));
    i++;
  }
  i = 0;
  while (additionnal[i])
  {
    snprintf (buffer, 255, "%s\n", additionnal[i]);
    write (tube[1], buffer, strlen (buffer));
    i++;
  }
  close (tube[0]);
  close (tube[1]);
  /* Make sure the child process has completed before continuing */
#if HAVE_WAITPID
  waitpid (sonpid, NULL, WUNTRACED);
#elif HAVE_WAIT4
  wait4 (sonpid, NULL, WUNTRACED, NULL);
#else
# error One of waitpid or wait4 is needed.
#endif

  /* Once we've applied xfce colors, merge with user's defined settings */
  i = 0;
  tempstr = (char *) g_malloc (sizeof (char) * MAXSTRLEN);
  while (xres[i])
  {
    snprintf (tempstr, MAXSTRLEN, "%s%s", (char *) getenv ("HOME"), xres[i]);
    if (existfile (tempstr))
    {
      snprintf (tempstr, MAXSTRLEN, "xrdb -merge -all %s%s", (char *) getenv ("HOME"), xres[i]);
      system (tempstr);
    }
    i++;
  }
  g_free (tempstr);
}

void
get_rgb_from_palette (XFCE_palette * p, short int col_index, int *r, int *g, int *b)
{
  *r = p->r[col_index];
  *g = p->g[col_index];
  *b = p->b[col_index];
}

char *
color_to_hex (char *s, const XFCE_palette * p, int index)
{
  if (!s)
    s = (char *) g_malloc (8 * sizeof (char));

  if ((p) && (s) && (index >= 0) && (index < NB_XFCE_COLORS))
  {
    sprintf (s, "#%02X%02X%02X", (unsigned short) p->r[index], (unsigned short) p->g[index], (unsigned short) p->b[index]);
    return (s);
  }
  return (NULL);
}

void
set_selcolor (XFCE_palette * p, int color_index, gdouble color_table[])
{
  color_table[0] = ((double) p->r[color_index]) / 255;
  color_table[1] = ((double) p->g[color_index]) / 255;
  color_table[2] = ((double) p->b[color_index]) / 255;
}

void
set_palcolor (XFCE_palette * p, int color_index, gdouble color_table[])
{
  p->r[color_index] = (int) (color_table[0] * 255);
  p->g[color_index] = (int) (color_table[1] * 255);
  p->b[color_index] = (int) (color_table[2] * 255);
}

unsigned long
get_pixel_from_palette (XFCE_palette * p, int color_index)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ()), &attributes);
  color.red = p->r[color_index] * 255;
  color.green = p->g[color_index] * 255;
  color.blue = p->b[color_index] * 255;

  if (!XAllocColor (GDK_DISPLAY (), attributes.colormap, &color))
  {
    fprintf (stderr, "*** Warning *** Cannot allocate color\n");
    return (WhitePixel (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (GDK_ROOT_PARENT ())));
  }
  return color.pixel;
}

XFCE_palette *
newpal (void)
{
  XFCE_palette *p;

  p = (XFCE_palette *) g_malloc (sizeof (XFCE_palette));
  p->fnt = NULL;
  p->texture = NULL;
  p->engine = NULL;

  initpal (p);
  return (p);
}

void
set_font (XFCE_palette * p, char *fnt)
{
  if (fnt && (strlen (fnt)))
  {
    p->fnt = g_realloc (p->fnt, (strlen (fnt) + 1) * sizeof (char));
    strcpy (p->fnt, fnt);
  }
  else
  {
    p->fnt = g_realloc (p->fnt, sizeof (char));
    strcpy (p->fnt, "");
  }
}

void
set_texture (XFCE_palette * p, char *texture)
{
  if (texture && (strlen (texture)))
  {
    p->texture = g_realloc (p->texture, (strlen (texture) + 1) * sizeof (char));
    strcpy (p->texture, texture);
  }
  else
  {
    p->texture = g_realloc (p->texture, sizeof (char));
    strcpy (p->texture, "");
  }
}

void
set_engine (XFCE_palette * p, char *engine)
{
  if (engine && (strlen (engine)))
  {
    p->engine = g_realloc (p->engine, (strlen (engine) + 1) * sizeof (char));
    strcpy (p->engine, engine);
  }
  else
  {
    p->engine = g_realloc (p->engine, sizeof (char));
    strcpy (p->engine, "");
  }
}

void
freepal (XFCE_palette * p)
{
  if (p->fnt)
  {
    g_free (p->fnt);
    p->fnt = NULL;
  }
  if (p->texture)
  {
    g_free (p->texture);
    p->texture = NULL;
  }
  if (p->engine)
  {
    g_free (p->engine);
    p->engine = NULL;
  }
  g_free (p);
  p = NULL;
}

XFCE_palette *
copypal (XFCE_palette * d, const XFCE_palette * s)
{
  int i;

  if (d && s)
  {
    for (i = 0; i < NB_XFCE_COLORS; i++)
    {
      d->r[i] = s->r[i];
      d->g[i] = s->g[i];
      d->b[i] = s->b[i];
    }
    d->fnt = g_realloc (d->fnt, (strlen (s->fnt) + 1) * sizeof (char));
    d->texture = g_realloc (d->texture, (strlen (s->texture) + 1) * sizeof (char));
    d->engine = g_realloc (d->engine, (strlen (s->engine) + 1) * sizeof (char));
    strcpy (d->fnt, s->fnt);
    strcpy (d->texture, s->texture);
    strcpy (d->engine, s->engine);
    return (d);
  }
  return (NULL);
}

XFCE_palette *
copyvaluepal (XFCE_palette * d, const XFCE_palette * s)
{
  int i;

  if (d && s)
  {
    for (i = 0; i < NB_XFCE_COLORS; i++)
    {
      d->r[i] = s->r[i];
      d->g[i] = s->g[i];
      d->b[i] = s->b[i];
    }
    d->fnt = g_realloc (d->fnt, (strlen (s->fnt) + 1) * sizeof (char));
    d->texture = g_realloc (d->texture, (strlen (s->texture) + 1) * sizeof (char));
    d->engine = g_realloc (d->engine, (strlen (s->engine) + 1) * sizeof (char));
    strcpy (d->fnt, s->fnt);
    strcpy (d->texture, s->texture);
    strcpy (d->engine, s->engine);
    return (d);
  }
  return (NULL);
}

void
initpal (XFCE_palette * p)
{
  if (p)
  {
    p->fnt = g_realloc (p->fnt, (strlen (XFCEDEFAULTFONT) + 1) * sizeof (char));
    p->texture = g_realloc (p->texture, sizeof (char));
    p->engine = g_realloc (p->engine, (strlen (XFCE_THEME_ENGINE) + 1) * sizeof (char));
    strcpy (p->texture, "");
    strcpy (p->engine, XFCE_THEME_ENGINE);
    strcpy (p->fnt, XFCEDEFAULTFONT);
  }
}

void
write_style_to_gtkrc_file (FILE * f, XFCE_palette * p, int normal, int selected, int base, char *template, gboolean textured)
{
  int c1_howbright, c1_howbrightSel;
  int c2_howbright, c2_howbrightSel;
  char *texture;
  char nobgpixmap[] = "<none>";
  /* Temporary RGB values */
  unsigned short red;
  unsigned short green;
  unsigned short blue;

  /* When running on a display with 256 colors or less, avoid using textures */
  if (DEFAULT_DEPTH > 8)
    texture = p->texture;
  else
    texture = nobgpixmap;


  c1_howbright = brightness (p->r[normal], p->g[normal], p->b[normal]);
  c1_howbrightSel = brightness (((unsigned short) p->r[normal]) ^ 255, ((unsigned short) p->g[normal]) ^ 128, ((unsigned short) p->b[normal]) ^ 64);
  c2_howbright = brightness (p->r[selected], p->g[selected], p->b[selected]);
  c2_howbrightSel = brightness (((unsigned short) p->r[selected]) ^ 255, ((unsigned short) p->g[selected]) ^ 128, ((unsigned short) p->b[selected]) ^ 64);

  fprintf (f, "style \"%s%u\"\n", template, normal);
  fprintf (f, "{\n");
  if ((p->fnt) && strlen (p->fnt))
    fprintf (f, "  font              = \"%s\"\n", p->fnt);

  if ((textured) && (p->texture) && (strlen (p->texture)))
  {
    fprintf (f, "  bg_pixmap[NORMAL] = \"%s\"\n", texture);
    fprintf (f, "  bg_pixmap[ACTIVE] = \"%s\"\n", nobgpixmap);
    fprintf (f, "  bg_pixmap[INSENSITIVE] = \"%s\"\n", (normal == selected) ? texture : nobgpixmap);
    fprintf (f, "  bg_pixmap[PRELIGHT] = \"%s\"\n", (normal == selected) ? texture : nobgpixmap);
  }
  else
  {
    fprintf (f, "  bg_pixmap[NORMAL] = \"<none>\"\n");
    fprintf (f, "  bg_pixmap[ACTIVE] = \"<none>\"\n");
    fprintf (f, "  bg_pixmap[INSENSITIVE] = \"<none>\"\n");
    fprintf (f, "  bg_pixmap[PRELIGHT] = \"<none>\"\n");
  }

  if (c1_howbright < fadeblack)
  {
    fprintf (f, "  fg[NORMAL]        = \"#ffffff\"\n");
    fprintf (f, "  text[NORMAL]      = \"#ffffff\"\n");
    fprintf (f, "  text[INSENSITIVE] = \"#ffffff\"\n");
  }
  else
  {
    fprintf (f, "  fg[NORMAL]        = \"#000000\"\n");
    fprintf (f, "  text[NORMAL]      = \"#000000\"\n");
    fprintf (f, "  text[INSENSITIVE] = \"#000000\"\n");
  }
  /* Init temp colors */
  red   = (unsigned short) p->r[normal];
  green = (unsigned short) p->g[normal];
  blue  = (unsigned short) p->b[normal];

  /* Compute shadow color */
  color_shade (&red, &green, &blue, .5);

  fprintf (f, "  fg[INSENSITIVE]   = \"#%02X%02X%02X\"\n", (unsigned short) red, (unsigned short) green, (unsigned short) blue);

  if (c2_howbright < fadeblack)
  {
    fprintf (f, "  fg[ACTIVE]        = \"#ffffff\"\n");
    fprintf (f, "  text[ACTIVE]      = \"#ffffff\"\n");
    fprintf (f, "  fg[PRELIGHT]      = \"#ffffff\"\n");
    fprintf (f, "  text[PRELIGHT]    = \"#ffffff\"\n");
  }
  else
  {
    fprintf (f, "  fg[ACTIVE]        = \"#000000\"\n");
    fprintf (f, "  text[ACTIVE]      = \"#000000\"\n");
    fprintf (f, "  fg[PRELIGHT]      = \"#000000\"\n");
    fprintf (f, "  text[PRELIGHT]    = \"#000000\"\n");
  }

  if (c2_howbrightSel < 50)
  {
    fprintf (f, "  fg[SELECTED]      = \"#ffffff\"\n");
    fprintf (f, "  text[SELECTED]    = \"#ffffff\"\n");
  }
  else
  {
    fprintf (f, "  fg[SELECTED]      = \"#000000\"\n");
    fprintf (f, "  text[SELECTED]    = \"#000000\"\n");
  }

  fprintf (f, "  bg[NORMAL]        = \"#%02X%02X%02X\"\n", (unsigned short) p->r[normal], (unsigned short) p->g[normal], (unsigned short) p->b[normal]);

  fprintf (f, "  base[NORMAL]      = \"#%02X%02X%02X\"\n", (unsigned short) p->r[base], (unsigned short) p->g[base], (unsigned short) p->b[base]);
  
  /* Init temp colors */
  red   = (unsigned short) p->r[selected];
  green = (unsigned short) p->g[selected];
  blue  = (unsigned short) p->b[selected];

  /* Compute shadow color */
  color_shade (&red, &green, &blue, DARK);

  /* Then set color */
  fprintf (f, "  bg[ACTIVE]        = \"#%02X%02X%02X\"\n", (unsigned short) red, (unsigned short) green, (unsigned short) blue);

  /* Do the same for normal GC, init temp colors */
  red   = (unsigned short) p->r[base];
  green = (unsigned short) p->g[base];
  blue  = (unsigned short) p->b[base];

  /* Compute shadow color */
  color_shade (&red, &green, &blue, DARK);

  /* Then set color */
  fprintf (f, "  base[ACTIVE]      = \"#%02X%02X%02X\"\n", (unsigned short) red, (unsigned short) green, (unsigned short) blue);

#if 0
  if (normal == selected)
  {
    fprintf (f, "  bg[PRELIGHT]      = \"#%02X%02X%02X\"\n", (unsigned short) shift (p->r[normal], LIGHT), (unsigned short) shift (p->g[normal], LIGHT), (unsigned short) shift (p->b[normal], LIGHT));
  }
  else
  {
    fprintf (f, "  bg[PRELIGHT]    = \"#%02X%02X%02X\"\n", (unsigned short) p->r[selected], (unsigned short) p->g[selected], (unsigned short) p->b[selected]);
  }
#else
  fprintf (f, "  bg[PRELIGHT]    = \"#%02X%02X%02X\"\n", (unsigned short) p->r[selected], (unsigned short) p->g[selected], (unsigned short) p->b[selected]);
#endif
  fprintf (f, "  base[PRELIGHT]    = \"#%02X%02X%02X\"\n", (unsigned short) p->r[selected], (unsigned short) p->g[selected], (unsigned short) p->b[selected]);

  fprintf (f, "  bg[SELECTED]      = \"#%02X%02X%02X\"\n", ((unsigned short) p->r[selected]) ^ 255, ((unsigned short) p->g[selected]) ^ 128, ((unsigned short) p->b[selected]) ^ 64);

  fprintf (f, "  base[SELECTED]      = \"#%02X%02X%02X\"\n", (unsigned short) p->r[base], (unsigned short) p->g[base], (unsigned short) p->b[base]);

  fprintf (f, "  bg[INSENSITIVE]   = \"#%02X%02X%02X\"\n", (unsigned short) p->r[normal], (unsigned short) p->g[normal], (unsigned short) p->b[normal]);

  fprintf (f, "  base[INSENSITIVE] = \"#%02X%02X%02X\"\n", (unsigned short) p->r[normal], (unsigned short) p->g[normal], (unsigned short) p->b[normal]);
#ifndef WIN32
  if ((p->engine) && (strlen (p->engine)) && my_strncasecmp (p->engine, "gtk", 3))
    fprintf (f, "  engine \"%s\" {}\n", p->engine);
#endif
  fprintf (f, "}\n");
}

void
create_gtkrc_file (XFCE_palette * p, char *name)
{
  FILE *f;
  char *home;
  char tempstr[MAXSTRLEN];
  char lineread[80];
  gboolean gtkrc_by_xfce = TRUE;
  int i;

  home =  (char *) getenv ("HOME");
  snprintf (tempstr, MAXSTRLEN, "%s%s", home, (name ? name : gtkrcfile));
  if (existfile (tempstr) && ((f = fopen (tempstr, "r"))))
  {
    fgets (lineread, 79, f);
    if (strlen (lineread))
    {
      lineread[strlen (lineread) - 1] = 0;
      gtkrc_by_xfce = !strncmp (XFCE3GTKRC, lineread, strlen (XFCE3GTKRC));
    }
    fclose (f);
  }

  if (gtkrc_by_xfce)
  {
    if ((f = fopen (tempstr, "w")))
    {
      fprintf (f, "%s\n\n", XFCE3GTKRC);
      fprintf (f, "pixmap_path \"%s:%s/.xfce/:%s/\"\n\n", build_path ("/"), home, home);
      for (i = 1; i < 8; i++)   /* Color 0 is used for the mouse pointer only */
        write_style_to_gtkrc_file (f, p, i, i, 4, "xfce_", (i == 7));
#ifndef OLD_STYLE
      write_style_to_gtkrc_file (f, p, 7, 2, 4, "xfcebar_", TRUE);
      write_style_to_gtkrc_file (f, p, 4, 2, 7, "xfcebar_taskbar", TRUE);
#endif
      fprintf (f, "style \"tooltips-style\" {\n");
      fprintf (f, "  bg[NORMAL] = \"#ffffc0\"\n");
      fprintf (f, "  fg[NORMAL] = \"#000000\"\n");
      fprintf (f, "  bg_pixmap[NORMAL] = \"<none>\"\n");
      fprintf (f, "}\n\n");
      fprintf (f, "class        \"*\"                  style \"xfce_7\"\n");
      fprintf (f, "widget_class \"*Progress*\"      style \"xfce_1\"\n");
      fprintf (f, "widget_class \"*List*\"	    style \"xfce_4\"\n");
      fprintf (f, "widget_class \"*Tree*\"	    style \"xfce_4\"\n");
      fprintf (f, "widget_class \"*Text*\"	    style \"xfce_5\"\n");
      fprintf (f, "widget_class \"*Entry*\"	    style \"xfce_5\"\n");
      fprintf (f, "widget_class \"*Spin*\"	    style \"xfce_5\"\n");
#ifndef OLD_STYLE
      fprintf (f, "widget_class \"*Menu*\"	    style \"xfcebar_7\"\n");
      fprintf (f, "widget_class \"*Button*\"	    style \"xfcebar_7\"\n");
      fprintf (f, "widget_class \"*ToggleButton*\"  style \"xfcebar_7\"\n");
#endif
      fprintf (f, "widget_class \"*CheckButton*\"   style \"xfce_7\"\n");
      fprintf (f, "widget_class \"*RadioButton*\"   style \"xfce_7\"\n");
      fprintf (f, "widget       \"*gxfce_color1*\"     style \"xfce_1\"\n");        
      fprintf (f, "widget       \"*gxfce_color2*\"     style \"xfce_2\"\n");        
      fprintf (f, "widget       \"*gxfce_color3*\"     style \"xfce_3\"\n");        
      fprintf (f, "widget       \"*gxfce_color4*\"     style \"xfce_4\"\n");        
      fprintf (f, "widget       \"*gxfce_color5*\"     style \"xfce_5\"\n");        
      fprintf (f, "widget       \"*gxfce_color6*\"     style \"xfce_6\"\n");        
      fprintf (f, "widget       \"*gxfce_color7*\"     style \"xfce_7\"\n");        
      fprintf (f, "widget       \"gtk-tooltips\"       style \"tooltips-style\"\n");
#ifndef OLD_STYLE
      fprintf (f, "widget       \"*task_active*\"             style \"xfcebar_taskbar4\"\n");        
#endif      
      fclose (f);
    }
  }
}

void
create_temp_gtkrc_file (XFCE_palette * p)
{
  FILE *f;
  char *home;
  char tempstr[MAXSTRLEN];
  int i;

  home =  (char *) getenv ("HOME");
  snprintf (tempstr, MAXSTRLEN, "%s%s", home, gtktemprcfile);
  if ((f = fopen (tempstr, "w")))
  {
    fprintf (f, "%s\n\n", XFCE3GTKRC);
    fprintf (f, "pixmap_path \"%s:%s/.xfce/:%s/\"\n\n", build_path ("/"), home, home);
    for (i = 0; i < 8; i++)
      write_style_to_gtkrc_file (f, p, i, i, 4, "temp_xfce_", FALSE);
    fprintf (f, "widget       \"*temp_color0*\"  style \"temp_xfce_0\"\n");
    fprintf (f, "widget       \"*temp_color1*\"  style \"temp_xfce_1\"\n");
    fprintf (f, "widget       \"*temp_color2*\"  style \"temp_xfce_2\"\n");
    fprintf (f, "widget       \"*temp_color3*\"  style \"temp_xfce_3\"\n");
    fprintf (f, "widget       \"*temp_color4*\"  style \"temp_xfce_4\"\n");
    fprintf (f, "widget       \"*temp_color5*\"  style \"temp_xfce_5\"\n");
    fprintf (f, "widget       \"*temp_color6*\"  style \"temp_xfce_6\"\n");
    fprintf (f, "widget       \"*temp_color7*\"  style \"temp_xfce_7\"\n");
    fclose (f);
  }
}

void
apply_temp_gtkrc_file (XFCE_palette * p, GtkWidget * w)
{
  create_temp_gtkrc_file (p);
  gtk_rc_reparse_all ();
  gtk_widget_reset_rc_styles (w);
}

void
defpal (XFCE_palette * p)
{
  initpal (p);
  p->r[0] = 255;
  p->g[0] = 255;
  p->b[0] = 255;
  p->r[1] = 204;
  p->g[1] = 201;
  p->b[1] = 197;
  p->r[2] = 204;
  p->g[2] = 201;
  p->b[2] = 197;
  p->r[3] = 204;
  p->g[3] = 201;
  p->b[3] = 197;
  p->r[4] = 255;
  p->g[4] = 255;
  p->b[4] = 255;
  p->r[5] = 255;
  p->g[5] = 255;
  p->b[5] = 255;
  p->r[6] =  63;
  p->g[6] = 153;
  p->b[6] =  63;
  p->r[7] = 228;
  p->g[7] = 228;
  p->b[7] = 224;
  p->texture = g_realloc (p->texture, sizeof (char) * (strlen (DEFAULTTEXTURE) + 1));
  strcpy (p->texture, DEFAULTTEXTURE);
}

int
savenamepal (XFCE_palette * p, const char *name)
{
  FILE *f;
  int i;

  if ((f = fopen (name, "w")))
  {
    for (i = 0; i < NB_XFCE_COLORS; i++)
      fprintf (f, "%i %i %i\n", p->r[i], p->g[i], p->b[i]);
    if ((p->engine) && strlen (p->engine))
      fprintf (f, "engine=%s\n", p->engine);
    if ((p->fnt) && strlen (p->fnt))
      fprintf (f, "%s\n", p->fnt);
    if ((p->texture) && strlen (p->texture))
      fprintf (f, "%s\n", p->texture);
    fclose (f);
  }
  return ((f != NULL));
}

int
loadnamepal (XFCE_palette * p, const char *name)
{
  char *lineread, *a;
  FILE *f;
  int i, err = 0;
  gboolean font = FALSE;

  lineread = (char *) g_malloc ((80) * sizeof (char));

  if ((f = fopen (name, "r")))
  {
    for (i = 0; i < NB_XFCE_COLORS; i++)
    {
      if (!fgets (lineread, 79, f))
        break;
      if (strlen (lineread))
      {
        lineread[strlen (lineread) - 1] = '\0';
        if ((a = strtok (lineread, " ")))
          p->r[i] = atoi (a);
        else
          err = 1;
        if ((a = strtok (NULL, " ")))
          p->g[i] = atoi (a);
        else
          err = 1;
        if ((a = strtok (NULL, " ")))
          p->b[i] = atoi (a);
        else
          err = 1;
      }
    }
    initpal (p);
    while (fgets (lineread, MAXSTRLEN - 1, f))
    {
      if (strlen (lineread))
      {
        lineread[strlen (lineread) - 1] = '\0';
        if (!my_strncasecmp (lineread, "engine=", strlen ("engine=")))
        {
          char *s = &lineread[strlen ("engine=")];
          p->engine = g_realloc (p->engine, (strlen (s) + 1) * sizeof (char));
          strcpy (p->engine, s);
        }
        else if (!font)
        {
          p->fnt = g_realloc (p->fnt, (strlen (lineread) + 1) * sizeof (char));
          strcpy (p->fnt, lineread);
          font = TRUE;
        }
        else
        {
          p->texture = g_realloc (p->texture, (strlen (lineread) + 1) * sizeof (char));
          strcpy (p->texture, lineread);
        }
      }
    }
    fclose (f);
  }
  g_free (lineread);
  return ((!err && (f != NULL)));
}

int
savepal (XFCE_palette * p)
{
  char *tempstr;
  int x;

  tempstr = (char *) g_malloc ((MAXSTRLEN + 1) * sizeof (char));
  snprintf (tempstr, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rccolor);
  x = savenamepal (p, tempstr);
  g_free (tempstr);
  return (x);
}

int
loadpal (XFCE_palette * p)
{
  char *tempstr;
  int x;

  tempstr = (char *) g_malloc ((MAXSTRLEN + 1) * sizeof (char));
  snprintf (tempstr, MAXSTRLEN, "%s/.xfce/%s", (char *) getenv ("HOME"), rccolor);
  x = loadnamepal (p, tempstr);
  if (!x)
  {
    snprintf (tempstr, MAXSTRLEN, "%s/%s", XFCE_CONFDIR, rccolor);
    x = loadnamepal (p, tempstr);
  }
  g_free (tempstr);
  return (x);
}

gint xfce_window_client_event (GtkWidget * widget, GdkEventClient * event, gpointer p)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (!atom_read_colors)
    atom_read_colors = gdk_atom_intern (atom_read_colors_name, FALSE);

  if ((event->message_type == atom_read_colors) && (p))
  {
    loadpal ((XFCE_palette *) p);
  }
  return FALSE;
}

void
reg_xfce_app (GtkWidget * widget, XFCE_palette * p)
{
  /*
     g_return_if_fail (widget != NULL);
     g_return_if_fail (GTK_IS_WINDOW (widget));

     gtk_signal_connect (GTK_OBJECT (widget), "client_event", 
     GTK_SIGNAL_FUNC (xfce_window_client_event), 
     (gpointer) p);
   */
}

void
applypal_to_all (void)
{
  GdkEventClient sev;
  int i;

  /*
     if (!atom_read_colors)
     atom_read_colors = gdk_atom_intern(atom_read_colors_name, FALSE);
   */
  if (!atom_rcfiles)
    atom_rcfiles = gdk_atom_intern ("_GTK_READ_RCFILES", FALSE);

  for (i = 0; i < 5; i++)
    sev.data.l[i] = 0;
  sev.data_format = 32;
  sev.message_type = atom_rcfiles;
  gdk_event_send_clientmessage_toall ((GdkEvent *) & sev);
}

gboolean
repaint_in_progress (void)
{
  return REPAINT_IN_PROGRESS;
}

void
init_xfce_rcfile (void)
{
  char rcfile[MAXSTRLEN];

  snprintf (rcfile, MAXSTRLEN, "%s%s", (char *) getenv ("HOME"), xfcegtkrcfile);
  gtk_rc_add_default_file (rcfile);
  snprintf (rcfile, MAXSTRLEN, "%s%s", (char *) getenv ("HOME"), gtktemprcfile);
  gtk_rc_add_default_file (rcfile);
}
