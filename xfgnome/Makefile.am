## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = -I../ -I../libs/ @X_CFLAGS@ -DXFCE_DIR=\"@XFCE_DIR@\" \
-DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\"

bin_PROGRAMS = xfgnome

xfgnome_SOURCES = xfgnome.c xfgnome.h

xfgnome_LDADD= ../libs/libxfwmcore.la @DMALLOC_LIB@ ${X_LIBS} ${X_PRE_LIBS} \
-lXext -lX11 -lXpm @XFT_LIBS@

xfgnome_DEPENDENCIES = ../libs/libxfwmcore.la

man_MANS=xfgnome.1

