
gdm_prefix = @GDM_PREFIX@
switchdesk_prefix = "/usr/share/apps/switchdesk"
wmsession_prefix = "/etc/X11/wmsession.d"

noinst_DATA = $(xfce_FILES)
xfce_SOURC = XFce.in Xclients.xfce.in 10XFce.in
xfce_FILES = XFce Xclients.xfce 10XFce
EXTRA_DIST = $(xfce_SOURC)
DISTCLEANFILES = $(xfce_FILES)

XFce : $(srcdir)/XFce.in
	@sed -e "s%BIN_DIR%$(bindir)%g"     $(srcdir)/XFce.in > $@

10XFce : $(srcdir)/10XFce.in
	@sed -e "s%BIN_DIR%$(bindir)%g"     $(srcdir)/10XFce.in > $@

Xclients.xfce : $(srcdir)/Xclients.xfce.in
	@sed -e "s%BIN_DIR%$(bindir)%g"     $(srcdir)/Xclients.xfce.in > $@

install-data-local :
	test no = @USE_GDM@ || $(MAKE) install-gdm

install-gdm: $(xfce_FILES)
	$(mkinstalldirs) $(DESTDIR)$(gdm_prefix)
	$(mkinstalldirs) $(DESTDIR)$(switchdesk_prefix)
	$(mkinstalldirs) $(DESTDIR)$(wmsession_prefix)
	$(INSTALL_SCRIPT) XFce \
	 $(DESTDIR)$(gdm_prefix)/XFce
	$(INSTALL_SCRIPT) Xclients.xfce \
	 $(DESTDIR)$(switchdesk_prefix)/Xclients.xfce
	$(INSTALL_SCRIPT) 10XFce \
	 $(DESTDIR)$(wmsession_prefix)/10XFce

uninstall-local: uninstall-gdm

uninstall-gdm:
	rm -f $(DESTDIR)$(gdm_prefix)/XFce
	rm -f $(DESTDIR)$(switchdesk_prefix)/Xclients.xfce
	rm -f $(DESTDIR)$(wmsession_prefix)/10XFce

