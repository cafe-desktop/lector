SUBDIRS = icons
NULL=

install-data-local: update-icon-cache

#
# man file
#

man_MANS=lector.1

#
# UI descriptions
#

uidir = $(pkgdatadir)
ui_DATA =				\
	hand-open.png

#
# Desktop file
#

DESKTOP_IN_FILES = lector.desktop.in
DESKTOP_FILES = $(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)
$(desktop_DATA): $(DESKTOP_IN_FILES)
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

#
# DBus servide file
#
if ENABLE_DBUS
servicedir = $(datadir)/dbus-1/services
service_in_files = org.cafe.lector.Daemon.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
endif

#
# App data file
#
appdatadir = $(datadir)/metainfo
appdata_in_files = lector.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

#
# GSettings schema
#
gsettings_SCHEMAS = org.cafe.Lector.gschema.xml

.PRECIOUS: $(gsettings_SCHEMAS)

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@


#
# CTK icon cache
#

ctk_update_icon_cache = ctk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

update-icon-cache:
	if test -z "$(DESTDIR)"; then \
	        echo "Updating Ctk icon cache."; \
	        $(ctk_update_icon_cache); \
	else \
	        echo "*** Icon cache not updated.  After (un)install, run this:"; \
	        echo "***   $(ctk_update_icon_cache)"; \
	        fi


#
# Extra files to be included in the tarball
#

EXTRA_DIST =					\
	$(ui_DATA)				\
	$(DESKTOP_IN_FILES)			\
	$(appdata_in_files)			\
	$(gsettings_SCHEMAS)			\
	org.cafe.lector.Daemon.service.in	\
	$(man_MANS)				\
	$(NULL)

#
# Clean up properly
#

CLEANFILES = \
	lector.appdata.xml

DISTCLEANFILES = \
	$(DESKTOP_FILES)	\
	$(service_DATA)

-include $(top_srcdir)/git.mk
