## Process this file with automake to produce Makefile.in

bin_PROGRAMS = glade-gtk2

glade_gtk2_CPPFLAGS = \
	-I$(top_srcdir)    \
	-I$(top_builddir)  \
	-DGLADE_GNOMEHELPDIR="\"$(HELP_DIR)\"" \
	$(GTK_CFLAGS)      \
	$(GTK_MAC_CFLAGS)  \
	$(WARN_CFLAGS)     \
	$(AM_CPPFLAGS)

glade_gtk2_CFLAGS =           \
	$(AM_CFLAGS)

glade_gtk2_LDFLAGS = $(AM_LDFLAGS)

glade_gtk2_LDADD = $(top_builddir)/gladeui/libgladeui-1.la $(GTK_MAC_LIBS)

glade_gtk2_SOURCES = \
	glade-window.c \
	glade-window.h \
	glade-close-button.c \
	glade-close-button.h \
	main.c

if NATIVE_WIN32
glade_gtk2_LDADD += glade-win32-res.o
if !GLADE_UNSTABLE
glade_gtk2_LDFLAGS += -mwindows
endif
endif

glade-win32-res.o: glade-3.rc
	$(WINDRES) $< $@

EXTRA_DIST = glade-3.rc.in
