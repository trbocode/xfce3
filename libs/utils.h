#ifndef XFWMLIB_H
#define XFWMLIB_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ctype.h>

#ifdef HAVE_X11_XFT_XFT_H
#  include <X11/Xft/Xft.h>
#endif

typedef struct XfwmFont
{
  XFontStruct *font;		/* font structure */
  XFontSet fontset;		/* fontset for multi-language */
#ifdef HAVE_X11_XFT_XFT_H
  XftFont *xftfont;		/* AA font */
  Bool use_xft;
#endif
  int height;			/* height of the font */
  int y;			/* Y coordinate to draw characters */
}
XfwmFont;

/***********************************************************************
 * Routines for dealing with strings
 ***********************************************************************/
char *mymemset (char *dst, int c, int size);
char *mymemcpy (char *dst, char *src, int size);
char *mymemcpy (char *dst, char *src, int size);
int mystrcasecmp (char *a, char *b);
int mystrncasecmp (char *a, char *b, int n);
char *CatString3 (char *a, char *b, char *c);
void CopyString (char **dest, char *source);
char *stripcpy (char *source);
int StrEquals (char *s1, char *s2);

int envExpand (char *s, int maxstrlen);
char *envDupExpand (const char *s, int extra);

int matchWildcards (char *pattern, char *string);

/***********************************************************************
 * Stuff for consistent parsing
 ***********************************************************************/
#define EatWS(s) do { while ((s) && (isspace(*(s)) || *(s) == ',')) (s)++; } while (0)
#define IsQuote(c) (c == '"' || c == '\'' || c =='`')
#define IsBlockStart(c) (c == '[' || c == '{' || c == '(')
#define IsBlockEnd(c,cs) ((c == ']' && cs == '[') || (c == '}' && cs == '{') || (c == ')' && cs == '('))
#define MAX_TOKEN_LENGTH 255

char *PeekToken (const char *pstr);
char *GetToken (char **pstr);
int CmpToken (const char *pstr, char *tok);
int MatchToken (const char *pstr, char *tok);
void NukeToken (char **pstr);

/* old style parse routine: */
char *GetNextToken (char *indata, char **token);

/***********************************************************************
 * Various system related utils
 ***********************************************************************/
int mygethostname (char *client, int namelen);
int GetFdWidth (void);

char *safemalloc (int);

void sleep_a_little (int n);

/***********************************************************************
 * Stuff for modules to communicate with xfwm
 ***********************************************************************/
int ReadXfwmPacket (int fd, unsigned long *header, unsigned long **body);
void SendText (int *fd, char *message, unsigned long window);
#define SendInfo SendText
void *GetConfigLine (int *fd, char **tline);
void SetMessageMask (int *fd, unsigned long mask);

void InitPictureCMap (Display *, Window);

char *findIconFile (char *icon, char *pathlist, int type);

/***********************************************************************
 * Wrappers around various X11 routines
 ***********************************************************************/

void GetFontOrFixed (Display * disp, char *fontname, XfwmFont * xfwmfont);

void MyXGrabServer (Display * disp);
void MyXUngrabServer (Display * disp);

void send_clientmessage (Display * disp, Window w, Atom a, Time timestamp);

#endif
