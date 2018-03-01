## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS) $(noinst_DATA)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfclock

xfclock_SOURCES = xfclock_cb.c \
		  xfclock_cb.h \
                  xfclock_colorsel.c \
                  xfclock_colorsel.h \
		  xfclock_config.c \
		  xfclock_config.h \
		  xfclock_menus.c \
		  xfclock_menus.h \
		  xfclock_style.c \
		  xfclock_style.h \
		  xfclock_opt.c \
		  xfclock_opt.h \
		  xfclock.c \
		  xfclock.h \
		  appointments.c \
		  appointments.h

xfclock_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfclock_DEPENDENCIES = ../libs/libxfcecore.la

man_MANS = xfclock.1
noinst_DATA = xfclock_icon.xpm         # really _noinst_?
