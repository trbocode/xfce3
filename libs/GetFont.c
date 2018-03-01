#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "utils.h"

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static char *
XftToXcore (char *fontname)
{
  char *t;

  t = fontname;
  while (*t != '\0')
  {
    if (*t == ' ')
    {
      *t = '_';
    }
    t++;
  }
  return (fontname);
}

static char *
XcoreToXft (char *fontname)
{
  char *t;

  t = fontname;
  while (*t != '\0')
  {
    if (*t == '_')
    {
      *t = ' ';
    }
    t++;
  }
  return (fontname);
}

/*
 * ** loads font or "fixed" on failure
 */
void
GetFontOrFixed (Display * disp, char *fontname, XfwmFont * xfwmfont)
{
  if ((!xfwmfont) || (!fontname))
  {
    return;
  }
  xfwmfont->fontset = NULL;
  xfwmfont->font = NULL;
  if (strchr (fontname, ','))
  {				/* FontSet specified */
    char **missing_fontlist = NULL;
    int missing_fontnum = 0;
    char *default_str = NULL;

    xfwmfont->fontset = XCreateFontSet (disp, fontname, &missing_fontlist, &missing_fontnum, &default_str);
    if (!(xfwmfont->fontset) || missing_fontnum > 0)
    {
      fprintf (stderr, "[GetFontOrFixed]: WARNING -- can't get fontset \"%s\", trying \"fixed\"\n", fontname);
      /* fixed should always be avail, so try that */
      if ((xfwmfont->font = XLoadQueryFont (disp, "fixed")) == NULL)
      {
	fprintf (stderr, "[GetFontOrFixed]: ERROR -- can't get font \"fixed\"\n");
      }
      if (missing_fontlist)
	XFreeStringList (missing_fontlist);
      xfwmfont->fontset = NULL;
    }
  }
  else
  {
    fontname = XftToXcore (fontname);
    if ((xfwmfont->font = XLoadQueryFont (disp, fontname)) == NULL)
    {
      fontname = XcoreToXft (fontname);
      if ((xfwmfont->font = XLoadQueryFont (disp, fontname)) == NULL)
      {
	fprintf (stderr, "[GetFontOrFixed]: WARNING -- can't get font \"%s\", trying \"fixed\"\n", fontname);
	/* fixed should always be avail, so try that */
	if ((xfwmfont->font = XLoadQueryFont (disp, "fixed")) == NULL)
	{
	  fprintf (stderr, "[GetFontOrFixed]: ERROR -- can't get font \"fixed\"\n");
	  exit (1);
	}
      }
    }
  }
#ifdef HAVE_X11_XFT_XFT_H
  /* If the fontset is in use, or if the font requested is 
   * "fixed" or at last if Xft is disabled
   * avoid loading XftFont
   */
  if ((!mystrncasecmp (fontname, "fixed", strlen ("fixed"))) || (xfwmfont->fontset) || !(xfwmfont->use_xft))
  {
    xfwmfont->xftfont = NULL;
  }
  else
  {
    fontname = XcoreToXft (fontname);
    if ((xfwmfont->xftfont = XftFontOpenXlfd (disp, DefaultScreen (disp), fontname)) == NULL)
    {
      fontname = XftToXcore (fontname);
      if ((xfwmfont->xftfont = XftFontOpenXlfd (disp, DefaultScreen (disp), fontname)) == NULL)
      {
	fprintf (stderr, "[GetFontOrFixed]: WARNING -- can't get Xft font \"%s\"\n", fontname);
	xfwmfont->xftfont = NULL;
      }
    }
  }
#endif
}
