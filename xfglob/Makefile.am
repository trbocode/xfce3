## Process this file with automake to produce Makefile.in
SUBDIRS = icons

EXTRA_DIST = $(man_MANS)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfglob glob 

xfglob_SOURCES = xfglob.c 

xfglob_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfglob_DEPENDENCIES = ../libs/libxfcecore.la

glob_SOURCES = glob.c globber.c globber.h

glob_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

man_MANS=xfglob.1 glob.1
