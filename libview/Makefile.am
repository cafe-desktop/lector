lib_LTLIBRARIES = liblectorview.la

NOINST_H_FILES =			\
	ev-annotation-window.h	\
	ev-form-field-accessible.h	\
	ev-image-accessible.h	\
	ev-link-accessible.h	\
	ev-page-accessible.h	\
	ev-page-cache.h			\
	ev-pixbuf-cache.h		\
	ev-timeline.h			\
	ev-transition-animation.h	\
	ev-view-accessible.h		\
	ev-view-cursor.h		\
	ev-view-marshal.h		\
	ev-view-private.h

INST_H_SRC_FILES = 			\
	ev-document-model.h		\
	ev-jobs.h			\
	ev-job-scheduler.h		\
	ev-print-operation.h	        \
	ev-stock-icons.h		\
	ev-view.h			\
	ev-web-view.h		\
	ev-view-presentation.h

INST_H_FILES = 				\
	$(INST_H_SRC_FILES)		\
	ev-view-type-builtins.h

headerdir = $(includedir)/lector/$(EV_API_VERSION)/libview
header_DATA = $(INST_H_FILES)

liblectorview_la_SOURCES = 			\
	ev-annotation-window.c		\
	ev-document-model.c		\
	ev-form-field-accessible.c	\
	ev-image-accessible.c	\
	ev-jobs.c			\
	ev-job-scheduler.c		\
	ev-link-accessible.c	\
	ev-page-accessible.c	\
	ev-page-cache.c			\
	ev-pixbuf-cache.c		\
	ev-print-operation.c	        \
	ev-stock-icons.c		\
	ev-timeline.c			\
	ev-transition-animation.c	\
	ev-view.c			\
	ev-web-view.c	    \
	ev-view-accessible.c		\
	ev-view-marshal.c		\
	ev-view-cursor.c		\
	ev-view-presentation.c		\
	ev-view-type-builtins.c		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

liblectorview_la_CPPFLAGS = \
	-DLECTORDATADIR=\"$(pkgdatadir)\"	\
	-DG_LOG_DOMAIN=\"LectorView\"		\
	-DCAFELOCALEDIR=\"$(datadir)/locale\"	\
	-DLECTOR_COMPILATION			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/libdocument		\
	-I$(top_builddir)			\
	$(AM_CPPFLAGS)

liblectorview_la_CFLAGS = \
	$(LIBVIEW_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(DISABLE_DEPRECATED)	\
	$(EPUB_CFLAGS)		\
	$(AM_CFLAGS)

liblectorview_la_LDFLAGS = \
	-version-info $(EV_VIEW_LT_VERSION_INFO)	\
	-no-undefined	\
	-export-symbols-regex "^ev_*"	\
	$(AM_LDFLAGS)

liblectorview_la_LIBADD = \
	$(top_builddir)/libdocument/liblectordocument.la \
	$(LIBVIEW_LIBS)								 \
	$(EPUB_LIBS)

BUILT_SOURCES = 			\
	ev-view-marshal.h		\
	ev-view-marshal.c		\
	ev-view-type-builtins.c		\
	ev-view-type-builtins.h

CLEANFILES = 				\
	$(BUILT_SOURCES)		\
	stamp-ev-view-type-builtins.h	\
	stamp-ev-view-type-builtins.c

ev-view-marshal.h: $(srcdir)/ev-view-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=ev_view_marshal $(srcdir)/ev-view-marshal.list --header --internal > $@

ev-view-marshal.c: $(srcdir)/ev-view-marshal.list
	echo '#include <config.h>' > ev-view-marshal.c
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=ev_view_marshal $(srcdir)/ev-view-marshal.list --internal --body --prototypes >> $@

ev-view-type-builtins.h: stamp-ev-view-type-builtins.h
	@true
stamp-ev-view-type-builtins.h: ev-view-type-builtins.h.template $(INST_H_SRC_FILES)
	$(AM_V_GEN)$(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-etbh \
	&& (cmp -s xgen-etbh ev-view-type-builtins.h || cp xgen-etbh ev-view-type-builtins.h ) \
	&& rm -f xgen-etbh \
	&& echo timestamp > $(@F)

ev-view-type-builtins.c: stamp-ev-view-type-builtins.c
	@true
stamp-ev-view-type-builtins.c: ev-view-type-builtins.c.template $(INST_H_SRC_FILES)
	$(AM_V_GEN)$(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-etbc \
	&& (cmp -s xgen-etbc ev-view-type-builtins.c || cp xgen-etbc ev-view-type-builtins.c ) \
	&& rm -f xgen-etbc \
	&& echo timestamp > $(@F)

EXTRA_DIST = \
	ev-view-type-builtins.c.template  \
	ev-view-type-builtins.h.template  \
	ev-view-marshal.list

# GObject Introspection

if ENABLE_INTROSPECTION

LectorView-$(EV_API_VERSION).gir: liblectorview.la Makefile $(INST_H_FILES) $(filter %.c,$(liblectorview_la_SOURCES))
	$(AM_V_GEN) PKG_CONFIG_PATH=$(top_builddir):$$PKG_CONFIG_PATH \
	$(G_IR_SCANNER) -v --namespace LectorView \
	--add-include-path=$(top_builddir)/libdocument \
	--strip-prefix=Ev \
	--nsversion=$(EV_API_VERSION) \
	--include=GLib-2.0 \
	--include=GObject-2.0 \
	--include=Gio-2.0 \
	--include=Cdk-3.0 \
	--include=GdkPixbuf-2.0 \
	--include=Ctk-3.0 \
	--include=LectorDocument-$(EV_API_VERSION) \
	--library=lectorview \
	--libtool="$(LIBTOOL)" \
	--output $@ \
	--pkg lector-document-$(EV_API_VERSION) \
	--pkg lector-view-$(EV_API_VERSION) \
	-L$(top_builddir)/libdocument/ \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DLECTOR_COMPILATION \
	$(filter %.h,$^) \
	$(filter %.c,$^)

girdir = $(GIRDIR)
gir_DATA = LectorView-$(EV_API_VERSION).gir

typelibsdir = $(GIRTYPELIBDIR)
typelibs_DATA = LectorView-$(EV_API_VERSION).typelib

EXTRA_DIST += $(gir_DATA)
CLEANFILES += $(gir_DATA) $(typelibs_DATA)

%.typelib: %.gir
	$(AM_V_GEN) LD_LIBRARY_PATH=$${LD_LIBRARY_PATH:+$$LD_LIBRARY_PATH:}. $(G_IR_COMPILER) \
	--includedir=$(srcdir) \
	--includedir=$(top_builddir)/libdocument \
	$(G_IR_COMPILER_OPTS) \
	$< -o $@

endif # ENABLE_INTROSPECTION

-include $(top_srcdir)/git.mk
