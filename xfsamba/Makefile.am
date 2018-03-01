## Process this file with automake to produce Makefile.in
SUBDIRS = icons

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xfsamba 

xfsamba_SOURCES = tubo.h tubo.c \
	xfsamba.h xfsamba.c \
	xfsamba_gui.c xfsamba_mem.c \
	xfsamba_nmblookup.c xfsamba_masterlookup.c \
	xfsamba_masterresolve.c \
	xfsamba_list.c xfsamba_download.c \
	xfsamba_upload.c xfsamba_mkdir.c \
	xfsamba_rm.c xfsamba_tar.c \
	xfsamba_smblookup.c \
	xfsamba_dnd.c xfsamba_dnd.h \
	xfsamba_mnt.c xfsamba_colorsel.c \
	xfsamba_icons.c \
	uri.c uri.h \
	gtk_dlg.c gtk_dlg.h \
	io.c io.h
xfsamba_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
 @GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
 ${X_LIBS} ${X_PRE_LIBS} -lXpm

xfsamba_DEPENDENCIES = ../libs/libxfcecore.la
