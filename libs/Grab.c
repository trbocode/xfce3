
/*
 * ** MyXGrabServer & MyXUngrabServer - to handle nested grab server calls
 */

#include <X11/Xlib.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static int xgrabcount = 0;

void
MyXGrabServer (Display * disp)
{
  if (xgrabcount == 0)
  {
    XSync (disp, 0);
    XGrabServer (disp);
  }
  xgrabcount++;
}

void
MyXUngrabServer (Display * disp)
{
  if (--xgrabcount < 0)		/* should never happen */
  {
    xgrabcount = 0;
  }
  if (xgrabcount == 0)
  {
    XUngrabServer (disp);
    XFlush (disp);
  }
}
