## Process this file with automake to produce Makefile.in

CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

# lib_LTLIBRARIES = libxfcecore.la libxfwmcore.la
noinst_LTLIBRARIES = libxfcecore.la libxfwmcore.la

libxfcecore_la_SOURCES = \
	wild.c Strings.c safemalloc.c Parse.c \
	colorselect.c colorselect.h \
	constant.h \
	empty.h \
	my_intl.h \
	fileselect.c fileselect.h \
	fileutil.c fileutil.h \
	fontselection.c fontselection.h \
	gnome_protocol.c gnome_protocol.h \
	lightdark.h \
	my_string.c my_string.h \
	my_tooltips.c my_tooltips.h \
	mygtkclock.c mygtkclock.h \
	sendinfo.c sendinfo.h \
	snprintf.c snprintf.h \
	xfce-common.c xfce-common.h \
	xfcolor.c xfcolor.h \
	xpmext.c xpmext.h \
	defaulticon.h my_scandir.h
libxfcecore_la_LIBADD = $(GTK_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)
# libxfcecore_la_LDFLAGS = -version-info $(LIBVERSION) \
# 	-export-dynamic -rpath "@prefix@/lib"

libxfwmcore_la_SOURCES = \
	ClientMsg.c \
	GetFont.c \
	Grab.c \
	Module.c \
	Parse.c \
	Picture.c \
	Strings.c \
	System.c \
	alpha_header.h \
	configure.h \
	envvar.c  \
	module.h \
        my_scandir.c \
	safemalloc.c \
	sleep.c \
	snprintf.c snprintf.h \
	sun_headers.h \
	utils.h \
	wild.c

libxfwmcore_la_LIBADD = $(X_LIBS) $(X_EXTRA_LIBS) @XFT_LIBS@
# libxfwmcore_la_LDFLAGS = -version-info $(LIBVERSION) \
# 	-export-dynamic -rpath "@prefix@/lib"
