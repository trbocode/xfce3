## Process this file with automake to produce Makefile.in
SUBDIRS = icons

EXTRA_DIST = $(man_MANS) $(xftree_distcvs) 
CLEANFILES = *~ *.bak core *.core *.o .depend .deps/* 

INCLUDES = @GTK_CFLAGS@ @GDK_IMLIB_CFLAGS@ @GDK_PIXBUF_CFLAGS@ \
 -I../ -I../libs/ \
 -DXFCE_DIR=\"@XFCE_DIR@\" -DXFCE_LOCALE_DIR=\"@XFCE_LOCALE_DIR@\" \
 -DXFCE_CONFDIR=\"@XFCE_CONFDIR@\"

bin_PROGRAMS = xftree 

xftree_distcvs = dlg_icons.h toolbar_icons.h # part of xftree? or _obsolete_?
xftree_SOURCES = \
	entry.c entry.h \
	gtk_dlg.c gtk_dlg.h \
	gtk_dnd.h \
	gtk_exec.c gtk_exec.h \
	gtk_get.c gtk_get.h \
	gtk_prop.c gtk_prop.h \
	i18n.h \
	io.c io.h \
	reg.c reg.h \
	top.c top.h \
	uri.c uri.h \
	xtree.c \
	xtree_cfg.h \
	xtree_dnd.c xtree_dnd.h \
	xtree_functions.c xtree_functions.h \
	xtree_gui.c xtree_gui.h icons.h \
	xtree_cb.c xtree_cb.h \
	xtree_go.c xtree_go.h \
	xtree_misc.c xtree_misc.h \
	xtree_du.c tubo.c tubo.h \
	xtree_mess.c xtree_mess.h \
	xtree_cpy.c xtree_cpy.h \
	xtree_pasteboard.c xtree_pasteboard.h \
	xtree_toolbar.c xtree_toolbar.h \
	xtree_tar.c xtree_tar.h \
	xtree_rpm.c xtree_rpm.h \
	xtree_colorsel.c  ft_types.h \
	xtree_icons.c xtree_icons.h \
	xtree_smb.c

xftree_LDADD = ../libs/libxfcecore.la @DMALLOC_LIB@ @GTK_LIBS@ \
@GDK_IMLIB_LIBS@ @GDK_PIXBUF_LIBS@ @INTLLIBS@ @LIBS@ \
${X_LIBS} ${X_PRE_LIBS} -lXpm

xftree_DEPENDENCIES = ../libs/libxfcecore.la

man_MANS=xftree.1
