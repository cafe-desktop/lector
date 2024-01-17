bin_PROGRAMS = lector-thumbnailer

man_MANS = lector-thumbnailer.1

lector_thumbnailer_SOURCES = \
	lector-thumbnailer.c

lector_thumbnailer_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(AM_CPPFLAGS)

lector_thumbnailer_CFLAGS = \
	$(FRONTEND_CFLAGS)	\
	$(DISABLE_DEPRECATED)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

lector_thumbnailer_LDFLAGS = $(AM_LDFLAGS)

lector_thumbnailer_LDADD = \
	$(top_builddir)/libdocument/liblectordocument.la	\
	$(FRONTEND_LIBS)

thumbnailerdir = $(datadir)/thumbnailers
thumbnailer_in_files = lector.thumbnailer.in
thumbnailer_DATA = $(thumbnailer_in_files:.thumbnailer.in=.thumbnailer)


lector.thumbnailer: $(thumbnailer_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LECTOR_MIME_TYPES\@|$(LECTOR_MIME_TYPES)|" \
		-e "s|application/epub+zip;||" \
		$< > $@

EXTRA_DIST =         	\
	$(thumbnailer_in_files) \
	$(man_MANS)

DISTCLEANFILES = \
	$(thumbnailer_DATA)

-include $(top_srcdir)/git.mk
