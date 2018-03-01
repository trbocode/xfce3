## Process this file with automake to produce Makefile.in

EXTRA_DIST = $(man_MANS) $(noinst_DATA)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 @AUDIOFILE_CFLAGS@ @ARTS_CFLAGS@ -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfsound
bin_SCRIPTS  = xfplay

xfsound_SOURCES = \
	xfsound_cb.c xfsound_cb.h \
	xfsound.c xfsound.h \
	xfdsp.c xfdsp.h

xfsound_LDADD = ../libs/libxfcecore.la ../libs/libxfwmcore.la \
 @DMALLOC_LIB@ @GTK_LIBS@ @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ \
 @AUDIOFILE_LIBS@ @ARTS_LIBS@ @INTLLIBS@ @LIBS@ ../libs/libxfwmcore.la \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm @XFT_LIBS@

xfsound_DEPENDENCIES = ../libs/libxfcecore.la ../libs/libxfwmcore.la

man_MANS=xfsound.1
noinst_DATA = xfsound_icon.xpm  xfplay # really _noinst_?
