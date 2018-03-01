## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS) $(man_dist)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = -I../ -I../libs/ @X_CFLAGS@ -DXFCE_DIR=\"@XFCE_DIR@\" \
 -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\"

bin_PROGRAMS = xfpager

xfpager_SOURCES = \
	x_pager.c \
	XfwmPager.c \
	XfwmPager.h

xfpager_LDADD= ../libs/libxfwmcore.la @DMALLOC_LIB@ ${X_LIBS} ${X_PRE_LIBS} \
 -lXext -lX11 -lXpm @XFT_LIBS@

xfpager_DEPENDENCIES = ../libs/libxfwmcore.la

man_dist=xfpager.man   # _obsolete_?
man_MANS=xfpager.1
