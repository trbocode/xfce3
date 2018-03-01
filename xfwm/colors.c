/*  gxfce
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

/****************************************************************************
 * This module is from original code
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/


#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>


#include "xfwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "lightdark.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *white = "white";
char *black = "black";

extern char *Hiback;
extern char *Hifore;

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

/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow (Pixel background)
{
  XColor bg_color;
  XWindowAttributes attributes;

  XGetWindowAttributes (dpy, Scr.Root, &attributes);

  bg_color.pixel = background;
  XQueryColor (dpy, attributes.colormap, &bg_color);

  color_shade (&(bg_color.red), &(bg_color.green), &(bg_color.blue), LO_MULT );
  
  if (!XAllocColor (dpy, attributes.colormap, &bg_color))
  {
    nocolor ("alloc shadow", "");
    bg_color.pixel = background;
  }

  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite (Pixel background)
{
  XColor bg_color;
  XWindowAttributes attributes;

  XGetWindowAttributes (dpy, Scr.Root, &attributes);

  bg_color.pixel = background;
  XQueryColor (dpy, attributes.colormap, &bg_color);

  color_shade (&(bg_color.red), &(bg_color.green), &(bg_color.blue), HI_MULT );

  if (!XAllocColor (dpy, attributes.colormap, &bg_color))
  {
    nocolor ("alloc hilight", "");
    bg_color.pixel = background;
  }
  return bg_color.pixel;
}

int
brightness (Pixel p)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes (dpy, Scr.Root, &attributes);
  color.pixel = p;
  XQueryColor (dpy, attributes.colormap, &color);
  return ((100 * ((color.red >> 8) + (color.green >> 8) + (color.blue >> 8)) / 765));
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void
CreateGCs (void)
{
  XGCValues gcv;
  unsigned long gcm;

  /* create scratch GC's */
  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCCapStyle;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;

  Scr.ScratchGC1 = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  Scr.ScratchGC3 = XCreateGC (dpy, Scr.Root, gcm, &gcv);

  Scr.TransMaskGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
  gcm = GCFunction | GCPlaneMask | GCGraphicsExposures | GCLineWidth | GCForeground | GCBackground | GCCapStyle;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 1;
  gcv.cap_style = CapProjecting;
  gcv.foreground = BlackPixel (dpy, Scr.screen);
  gcv.background = BlackPixel (dpy, Scr.screen);
  Scr.BlackGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);

  gcm = GCFunction | GCLineWidth | GCForeground | GCSubwindowMode;
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.foreground = BlackPixel (dpy, Scr.screen) ^ WhitePixel (dpy, Scr.screen);
  gcv.subwindow_mode = IncludeInferiors;
  Scr.HintsGC = XCreateGC (dpy, Scr.Root, gcm, &gcv);
}

XColor GetXColor (char *name)
{
  XColor color;

  color.pixel = 0;
  if (!XParseColor (dpy, Scr.XfwmRoot.attr.colormap, name, &color))
  {
    nocolor ("parse", name);
  }
  else if (!XAllocColor (dpy, Scr.XfwmRoot.attr.colormap, &color))
  {
    nocolor ("alloc", name);
  }
  return color;
}

/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/
Pixel GetColor (char *name)
{
  return GetXColor (name).pixel;
}

/****************************************************************************
 * 
 * Allocates a nonlinear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/
Pixel *
AllocNonlinearGradient (char *s_colors[], int clen[], int nsegs, int npixels)
{
  Pixel *pixels = (Pixel *) safemalloc (sizeof (Pixel) * npixels);
  int i = 0, curpixel = 0, perc = 0;
  if (nsegs < 1)
  {
    xfwm_msg (ERR, "AllocNonlinearGradient", "must specify at least one segment");
    free (pixels);
    return NULL;
  }
  for (; i < npixels; i++)
    pixels[i] = 0;

  for (i = 0; (i < nsegs) && (curpixel < npixels) && (perc <= 100); ++i)
  {
    Pixel *p;
    int j = 0, n = clen[i] * npixels / 100;
    p = AllocLinearGradient (s_colors[i], s_colors[i + 1], n);
    if (!p)
    {
      xfwm_msg (ERR, "AllocNonlinearGradient", "couldn't allocate gradient");
      free (pixels);
      return NULL;
    }
    for (; j < n; ++j)
      pixels[curpixel + j] = p[j];
    perc += clen[i];
    curpixel += n;
    free (p);
  }
  for (i = curpixel; i < npixels; ++i)
    pixels[i] = pixels[i - 1];
  return pixels;
}

/****************************************************************************
 * 
 * Allocates a linear color gradient (veliaa@rpi.edu)
 *
 ****************************************************************************/
Pixel *
AllocLinearGradient (char *s_from, char *s_to, int npixels)
{
  Pixel *pixels;
  XColor from, to, c;
  int r, dr, g, dg, b, db;
  int i = 0, got_all = 1;

  if (npixels < 1)
    return NULL;
  if (!s_from || !XParseColor (dpy, Scr.XfwmRoot.attr.colormap, s_from, &from))
  {
    nocolor ("parse", s_from);
    return NULL;
  }
  if (!s_to || !XParseColor (dpy, Scr.XfwmRoot.attr.colormap, s_to, &to))
  {
    nocolor ("parse", s_to);
    return NULL;
  }
  c = from;
  r = from.red;
  dr = (to.red - from.red) / npixels;
  g = from.green;
  dg = (to.green - from.green) / npixels;
  b = from.blue;
  db = (to.blue - from.blue) / npixels;
  pixels = (Pixel *) safemalloc (sizeof (Pixel) * npixels);
  c.flags = DoRed | DoGreen | DoBlue;
  for (; i < npixels; ++i)
  {
    if (!XAllocColor (dpy, Scr.XfwmRoot.attr.colormap, &c))
      got_all = 0;
    pixels[i] = c.pixel;
    c.red = (unsigned short) (r += dr);
    c.green = (unsigned short) (g += dg);
    c.blue = (unsigned short) (b += db);
  }
  if (!got_all)
  {
    char s[256];
    sprintf (s, "color gradient %s to %s", s_from, s_to);
    nocolor ("alloc", s);
  }
  return pixels;
}

void
nocolor (char *note, char *name)
{
  xfwm_msg (ERR, "nocolor", "can't %s color %s", note, name);
}
