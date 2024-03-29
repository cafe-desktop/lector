# We require automake 1.10 for $(abs_builddir)
AUTOMAKE_OPTIONS = 1.10

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the CTK+ Makefiles in ctk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE = liblectorview

# Uncomment for versioned docs and specify the version of the module, e.g. '2'.
DOC_MODULE_VERSION = $(EV_API_VERSION)


# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# viewing the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../ctk
DOC_SOURCE_DIR = ../../../libview

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS =

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="CTK_DISABLE_DEPRECATED"
SCAN_OPTIONS =

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS = --sgml-mode --output-format=xml --name-space=ev

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS =

# Extra options to supply to gtkdoc-mkhtml
# Add the builddir to the HTML path so that version.xml is found
MKHTML_OPTIONS = --path="$(abs_builddir)"

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../cdk/html
FIXXREF_OPTIONS = \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gio \
	--extra-dir=$(CTK_PREFIX)/share/gtk-doc/html/cdk \
	--extra-dir=$(CTK_PREFIX)/share/gtk-doc/html/gdk-pixbuf \
	--extra-dir=$(CTK_PREFIX)/share/gtk-doc/html/ctk

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/ctk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/ctk/*.c
HFILE_GLOB = \
	$(top_srcdir)/libview/*.h \
	$(top_builddir)/libview/*.h

CFILE_GLOB = \
	$(top_srcdir)/libview/*.c

# Extra header to include when scanning, which are not under DOC_SOURCE_DIR
# e.g. EXTRA_HFILES=$(top_srcdir}/contrib/extra.h
# EXTRA_HFILES = $(top_builddir)/evview/ev-version.h
EXTRA_HFILES =

# Header files to ignore when scanning. Use base file name, no paths
# e.g. IGNORE_HFILES=ctkdebug.h ctkintl.h
IGNORE_HFILES = \
	config.h \
	ev-pixbuf-cache.h \
	ev-timeline.h \
	ev-transition-animation.h \
	ev-view-accessible.h \
	ev-view-marshal.h \
	ev-view-private.h

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/ctk/stock-icons/stock_about_24.png
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files = \
	version.xml

# SGML files where gtk-doc abbrevations (#CtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files =

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. GTKDOC_CFLAGS=-I$(top_srcdir) -I$(top_builddir) $(CTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/ctk/$(ctktargetlib)
GTKDOC_CFLAGS = \
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/libview 			\
	-I$(top_srcdir)/libdocument                     \
	-I$(top_builddir)/libdocument                   \
	-I$(top_builddir)/libview			\
	-DLECTORDATADIR=\"$(pkgdatadir)\"		\
	-DCAFEDATADIR=\"$(datadir)\"			\
	-DLECTOR_UIDIR=\"$(pkgdatadir)\" 		\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"		\
	-DEV_BACKENDSDIR=\"$(backenddir)\"		\
	-DLECTOR_COMPILATION				\
	$(AM_CFLAGS)					\
	$(LIBVIEW_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(WEBKIT_CFLAGS)				\
	$(DISABLE_DEPRECATED)

GTKDOC_LIBS = \
	$(top_builddir)/libdocument/liblectordocument.la \
	$(top_builddir)/libview/liblectorview.la \
	$(LIBDOCUMENT_LIBS)						\
	$(WEBKIT_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += \
	version.xml.in

# Files not to distribute
# for --rebuild-types in $(SCAN_OPTIONS), e.g. $(DOC_MODULE).types
# for --rebuild-sections in $(SCAN_OPTIONS) e.g. $(DOC_MODULE)-sections.txt
#DISTCLEANFILES +=

# Comment this out if you want your docs-status tested during 'make check'
if ENABLE_GTK_DOC
#TESTS_ENVIRONMENT = cd $(srcsrc) &&
#TESTS = $(GTKDOC_CHECK)
endif

-include $(top_srcdir)/git.mk
