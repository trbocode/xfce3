## Process this file with automake to produce Makefile.in

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = -I../ -I../libs/ @GLIB_CFLAGS@ @X_CFLAGS@

bin_PROGRAMS = xfmenu

xfmenu_SOURCES = xfmenu.c

xfmenu_LDADD= ../libs/libxfwmcore.la @DMALLOC_LIB@  @GLIB_LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} ${LIBICONV} -lXext -lX11 -lXpm @XFT_LIBS@

xfmenu_DEPENDENCIES = ../libs/libxfwmcore.la
