SUBDIRS = mdvi-lib

AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/libdocument		\
	-I$(srcdir)/mdvi-lib			\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"	\
	-DLECTOR_COMPILATION			\
	$(WARN_CFLAGS)				\
	$(BACKEND_CFLAGS)			\
	$(SPECTRE_CFLAGS)			\
	$(DISABLE_DEPRECATED)

backend_LTLIBRARIES = libdvidocument.la

libdvidocument_la_SOURCES = 	\
	dvi-document.c		\
	dvi-document.h  	\
	cairo-device.c		\
	cairo-device.h		\
	fonts.c			\
	fonts.h

libdvidocument_la_LDFLAGS = $(BACKEND_LIBTOOL_FLAGS)
libdvidocument_la_LIBADD = 				\
	mdvi-lib/libmdvi.la				\
	-lkpathsea					\
	$(top_builddir)/libdocument/liblectordocument.la	\
	$(BACKEND_LIBS)					\
	$(SPECTRE_LIBS)

if WITH_TYPE1_FONTS
libdvidocument_la_LIBADD += -lt1
endif

backend_in_files = dvidocument.lector-backend.desktop.in
backend_DATA = $(backend_in_files:.lector-backend.desktop.in=.lector-backend)
$(backend_DATA): $(backend_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=TypeDescription --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(backend_in_files)

CLEANFILES = $(backend_DATA)

-include $(top_srcdir)/git.mk
