

/*
 * ** Strings.c: various routines for dealing with strings
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *
mymemset (char *dst, int c, int size)
{
#ifndef HAVE_MEMSET
  int i;
  char *ptr = dst;

  for (i = 0; i < size; i++)
  {
    *ptr = (char) c;
    ptr++;
  }
  return (dst);
#else
  return ((char *) memset(dst, c, (size_t) size));
#endif
}

char *
mymemcpy (char *dst, char *src, int size)
{
#ifndef HAVE_MEMCPY
  char *psrc = NULL;
  char *pdst = NULL;
  int ptr;
#endif
  if (!src)
    return NULL;
  if (!dst)
    return NULL;
  if (!size)
    return dst;
#ifdef HAVE_MEMCPY
  return ((char *) memcpy (dst, src, (size_t) size));
#else
  psrc = src;
  pdst = dst;
  ptr = size;
  while (ptr)
  {
    *pdst = *psrc;
    psrc++;
    pdst++;
    ptr--;
  }
  return dst;
#endif
}

char *
mymemmove (char *dst, char *src, int size)
{
#ifndef HAVE_MEMMOVE
  char *psrc = NULL;
  char *pdst = NULL;
  short int forward = 1;
  int ptr;
#endif
  if (!src)
    return NULL;
  if (!dst)
    return NULL;
  if (!size)
    return dst;
#ifdef HAVE_MEMMOVE
  return ((char *) memmove (dst, src, (size_t) size));
#else
  if (src > dst)
  {
    psrc = src;
    pdst = dst;
    forward = 1;
  }
  else
  {
    psrc = src + size;
    pdst = dst + size;
    forward = 0; 
  }
  ptr = size;
  while (ptr)
  {
    *pdst = *psrc;
    if (forward)
    {
      psrc++;
      pdst++;
    }
    else
    {
      psrc--;
      pdst--;
    }
    ptr--;
  }
  return dst;
#endif
}

/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/
char CatS[256];

char *
CatString3 (char *a, char *b, char *c)
{
  int len = 0;

  if (a != NULL)
    len += strlen (a);
  if (b != NULL)
    len += strlen (b);
  if (c != NULL)
    len += strlen (c);

  if (len > 255)
    return NULL;

  if (a == NULL)
    CatS[0] = 0;
  else
    strcpy (CatS, a);
  if (b != NULL)
    strcat (CatS, b);
  if (c != NULL)
    strcat (CatS, c);
  return CatS;
}

/***************************************************************************
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string 
 ***************************************************************************/
void
CopyString (char **dest, char *source)
{
  int len;
  char *start;

  while (((isspace (*source)) && (*source != '\n')) && (*source != 0))
  {
    source++;
  }
  len = 0;
  start = source;
  while ((*source != '\n') && (*source != 0))
  {
    len++;
    source++;
  }

  source--;
  while ((isspace (*source)) && (*source != 0) && (len > 0))
  {
    len--;
    source--;
  }
  *dest = safemalloc (len + 1);
  strncpy (*dest, start, len);
  (*dest)[len] = 0;
}

int
mystrcasecmp (char *s1, char *s2)
{
  int c1, c2;

  for (;;)
  {
    c1 = *s1;
    c2 = *s2;
    if (!c1 || !c2)
      return (c1 - c2);
    if (isupper (c1))
      c1 = 'a' - 1 + (c1 & 31);
    if (isupper (c2))
      c2 = 'a' - 1 + (c2 & 31);
    if (c1 != c2)
      return (c1 - c2);
    s1++, s2++;
  }
}

int
mystrncasecmp (char *s1, char *s2, int n)
{
  int c1, c2;

  for (;;)
  {
    if (!n)
      return (0);
    c1 = *s1, c2 = *s2;
    if (!c1 || !c2)
      return (c1 - c2);
    if (isupper (c1))
      c1 = 'a' - 1 + (c1 & 31);
    if (isupper (c2))
      c2 = 'a' - 1 + (c2 & 31);
    if (c1 != c2)
      return (c1 - c2);
    n--, s1++, s2++;
  }
}

/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/
char *
stripcpy (char *source)
{
  char *tmp, *ptr;
  int len;

  if (source == NULL)
    return NULL;

  while (isspace (*source))
    source++;
  len = strlen (source);
  tmp = source + len - 1;
  while (((isspace (*tmp)) || (*tmp == '\n')) && (tmp >= source))
  {
    tmp--;
    len--;
  }
  ptr = safemalloc (len + 1);
  strncpy (ptr, source, len);
  ptr[len] = 0;
  return ptr;
}

int
StrEquals (char *s1, char *s2)
{
  if (!s1 || !s2)
    return 0;
  return (mystrcasecmp (s1, s2) == 0);
}
