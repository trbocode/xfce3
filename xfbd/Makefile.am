## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS) $(noinst_DATA) MyChangesToxfbd
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ -I../ -I../libs/ \
@X_CFLAGS@ -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
-DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfbd xfbdmgr

xfbd_SOURCES = \
	xfbd.c xfbd.h \
	xfbd_cb.c xfbd_cb.h

xfbdmgr_SOURCES = \
	xfbdmgr.c xfbdmgr.h \
	xfbdmgr_cb.c xfbdmgr_cb.h

xfbd_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ @GDK_IMLIB_LIBS@ \
@GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ ${X_LIBS} ${X_PRE_LIBS} -lXpm  @XINERAMA_LIBS@

xfbd_DEPENDENCIES = ../libs/libxfcecore.la

xfbdmgr_LDADD = ../libs/libxfwmcore.la ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ @GDK_IMLIB_LIBS@ \
@GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfbdmgr_DEPENDENCIES = ../libs/libxfwmcore.la ../libs/libxfcecore.la

man_MANS = xfbd.1
noinst_DATA = xfbd_icon.xpm        # really _noinst_?
