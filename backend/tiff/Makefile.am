AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/libdocument		\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"	\
	-DLECTOR_COMPILATION			\
	$(BACKEND_CFLAGS)			\
	$(WARN_CFLAGS)				\
	$(DISABLE_DEPRECATED)

backend_LTLIBRARIES = libtiffdocument.la

libtiffdocument_la_SOURCES =	\
	tiff-document.c		\
	tiff-document.h		\
	tiff2ps.c		\
	tiff2ps.h

libtiffdocument_la_LDFLAGS = $(BACKEND_LIBTOOL_FLAGS)
libtiffdocument_la_LIBADD = 				\
	$(top_builddir)/libdocument/liblectordocument.la	\
	$(BACKEND_LIBS)			\
	-ltiff

backend_in_files = tiffdocument.lector-backend.desktop.in
backend_DATA = $(backend_in_files:.lector-backend.desktop.in=.lector-backend)
$(backend_DATA): $(backend_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=TypeDescription --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(backend_in_files)

CLEANFILES = $(backend_DATA)

-include $(top_srcdir)/git.mk
