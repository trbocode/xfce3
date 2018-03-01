## Process this file with automake to produce Makefile.in
SUBDIRS = icons

EXTRA_DIST = Changelog $(xfdiff_distcvs)
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS =  xfdiff

# _distcvs - files being in CVS but not listed anywhere else. _obsolete_?
xfdiff_distcvs = xfdiff_colorsel.h xfdiff_diag.c xfdiff_diag.h xfdiff_misc.h
xfdiff_SOURCES = xfdiff.c xfdiff_gui.c \
 xfdiff_colorsel.c xfdiff_misc.c \
 globber.c globber.h xfdiff.h  

xfdiff_LDADD =  ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfdiff_DEPENDENCIES = ../libs/libxfcecore.la

