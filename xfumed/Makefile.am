## Process this file with automake to produce Makefile.in
SUBDIRS = icons

EXTRA_DIST = $(man_MANS)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfumed 

xfumed_SOURCES = xfumed.c xfumed.h \
				xfumed_gui.c xfumed_gui.h \
				xfumed_gui_cb.c xfumed_gui_cb.h 

xfumed_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfumed_DEPENDENCIES = ../libs/libxfcecore.la

man_MANS=xfumed.1
