SUBDIRS = minizip

AM_CPPFLAGS = 		       			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/libdocument		\
	-I$(srcdir)/minizip			\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"	\
	-DLECTOR_COMPILATION			\
	$(BACKEND_CFLAGS)			\
	$(WARN_CXXFLAGS)			\
	$(EPUB_CFLAGS)			\
	$(DISABLE_DEPRECATED)

backend_LTLIBRARIES = libepubdocument.la

libepubdocument_la_SOURCES =	\
	epub-document.c		\
	epub-document.h		

libepubdocument_la_LDFLAGS = $(BACKEND_LIBTOOL_FLAGS)
libepubdocument_la_LIBADD =				\
	$(top_builddir)/libdocument/liblectordocument.la	\
	minizip/libminizip.la				\
	$(EPUB_LIBS)					\
	$(BACKEND_LIBS)


backend_in_files = epubdocument.lector-backend.desktop.in
backend_DATA = $(backend_in_files:.lector-backend.desktop.in=.lector-backend)

$(backend_DATA): $(backend_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=TypeDescription --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(backend_in_files)

CLEANFILES = $(backend_DATA)

-include $(top_srcdir)/git.mk
