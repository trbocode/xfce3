## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS) $(noinst_DATA)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfmouse

xfmouse_SOURCES = \
	xfmouse_cb.c xfmouse_cb.h \
	xfmouse.c xfmouse.h \
	xfmouse.xpm

xfmouse_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfmouse_DEPENDENCIES = ../libs/libxfcecore.la

man_MANS = xfmouse.1
noinst_DATA = xfmouse.xpm xfmouse_icon.xpm     # really _noinst_?

