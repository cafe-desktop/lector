AM_CPPFLAGS = \
	-I$(top_srcdir)						\
	-I$(top_srcdir)/libdocument				\
	-DCAFEICONDIR=\""${prefix}/${DATADIRNAME}/pixmaps"\" 	\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"			\
	-DLECTOR_COMPILATION			\
	$(BACKEND_CFLAGS)					\
	$(DJVU_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED)

backend_LTLIBRARIES = libdjvudocument.la

libdjvudocument_la_SOURCES = 	\
	djvu-document.c		\
	djvu-document.h 	\
	djvu-document-private.h \
	djvu-links.c		\
	djvu-links.h		\
	djvu-text-page.c 	\
	djvu-text-page.h

libdjvudocument_la_LDFLAGS = $(BACKEND_LIBTOOL_FLAGS)
libdjvudocument_la_LIBADD =				\
	$(top_builddir)/libdocument/liblectordocument.la	\
	$(BACKEND_LIBS)					\
	$(DJVU_LIBS)

backend_in_files = djvudocument.lector-backend.desktop.in
backend_DATA = $(backend_in_files:.lector-backend.desktop.in=.lector-backend)
$(backend_DATA): $(backend_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=TypeDescription --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(backend_in_files)

CLEANFILES = $(backend_DATA)

-include $(top_srcdir)/git.mk
