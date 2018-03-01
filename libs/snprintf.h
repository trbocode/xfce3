#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef __SNPRINTF_H__
#define __SNPRINTF_H__

#ifndef HAVE_SNPRINTF

#include <stdarg.h>		/* for va_list */
int snprintf (char *str, size_t count, const char *fmt, ...);
#endif

#ifndef HAVE_VSNPRINTF
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#endif

#endif /* __SNPRINTF_H__ */
