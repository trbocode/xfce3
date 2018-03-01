## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.4

@SET_MAKE@

CLEANFILES=*~ *.bak core *.core *.o .depend .deps/* *.cache
USE_INCLUDED_LIBINTL = @USE_INCLUDED_LIBINTL@
USE_NLS = @USE_NLS@
USE_DT = @USE_DT@
IMLIB_LIBS = @IMLIB_LIBS@
IMLIB_CFLAGS = @IMLIB_CFLAGS@
GDK_IMLIB_LIBS = @GDK_IMLIB_LIBS@
GDK_IMLIB_CFLAGS = @GDK_IMLIB_CFLAGS@
GDK_PIXBUF_LIBS = @GDK_PIXBUF_LIBS@
GDK_PIXBUF_CFLAGS = @GDK_PIXBUF_CFLAGS@
DMALLOC_LIB = @DMALLOC_LIB@
GTK_ENGINE_DIR = @GTK_ENGINE_DIR@
XINERAMA_LIBS = @XINERAMA_LIBS@
XFT_LIBS = @XFT_LIBS@
AUDIOFILE_LIBS = @AUDIOFILE_LIBS@
AUDIOFILE_CFLAGS = @AUDIOFILE_CFLAGS@
ARTS_LIBS = @ARTS_LIBS@
ARTS_CFLAGS = @ARTS_CFLAGS@

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

SUBDIRS = intl libs xfce_theme xfce xfwm xftree xfpager xfmouse xfbd xfclock \
	xfsound xfgnome xfrun xfmenu xfdiff xfglob xfsamba xfumed xfskin scripts \
	other backdrops icons palettes textures sounds docs po dt gdm \
	fonts cursors

EXTRA_DIST = \
 README.UPGRADE-3.2.0 \
 README.UPGRADE-3.5.0 \
 autogen.sh \
 xfce.spec \
 Makefile.init


# developers can create snapshot RPMs - add a patchlevel to the version-tag (or
# increase the release-tag) in the xfce.spec (e.g. 3.8.14.20011214) and run
# "./autogen.sh && make rpm"  (it also builds xfce-3.8.14.20011214.tar.gz)
rpm : dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
