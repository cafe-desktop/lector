AM_CPPFLAGS=					\
	-DLECTORDATADIR=\"$(pkgdatadir)\"	\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"	\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(FRONTEND_CFLAGS)			\
	$(BAUL_CFLAGS)			\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

noinst_LTLIBRARIES = libevproperties.la

libevproperties_la_SOURCES=			\
	ev-properties-view.c			\
	ev-properties-view.h

if ENABLE_BAUL

baulextension_LTLIBRARIES = liblector-properties-page.la

liblector_properties_page_la_CFLAGS = -I$(top_srcdir)
liblector_properties_page_la_SOURCES = 	\
	ev-properties-main.c

liblector_properties_page_la_LIBADD = 		\
	libevproperties.la			\
	$(top_builddir)/libdocument/liblectordocument.la	\
	$(FRONTEND_LIBS)			\
	$(BAUL_LIBS)

liblector_properties_page_la_LDFLAGS = -module -avoid-version -no-undefined

extensiondir = $(datadir)/baul/extensions
extension_in_files = liblector-properties-page.baul-extension.desktop.in
extension_DATA = $(extension_in_files:.baul-extension.desktop.in=.baul-extension)
$(extension_DATA): $(extension_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Description --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(extension_in_files)

CLEANFILES = $(extension_DATA)

endif # ENABLE_BAUL

-include $(top_srcdir)/git.mk
