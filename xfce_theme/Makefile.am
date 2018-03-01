INCLUDES = $(GTK_CFLAGS)
LIBS = $(GTK_LIBS)
enginedir = @GTK_ENGINE_DIR@

engine_LTLIBRARIES = libxfce.la
libxfce_la_SOURCES = \
	xfce_theme_draw.c	\
	xfce_theme_main.c

libxfce_la_LDFLAGS = -module -avoid-version
libxfce_la_LIBADD = $(GTK_LIBS)
