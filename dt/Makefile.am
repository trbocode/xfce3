dtconfdir = /etc/dt/config

xfce_SOURC = Xinitrc.xfce.in Xsession.xfce.in Xresources.xfce.in
xfce_FILES = Xinitrc.xfce    Xsession.xfce    Xresources.xfce
EXTRA_DIST = $(xfce_SOURC)
DISTCLEANFILES = $(xfce_FILES)

Xinitrc.xfce:     $(srcdir)/Xinitrc.xfce.in
	@sed -e "s%XFCE_CONFDIR%$(XFCE_CONFDIR)%g" \
	     -e "s%BIN_DIR%$(bindir)%g" \
					$(srcdir)/Xinitrc.xfce.in > $@

Xresources.xfce:  $(srcdir)/Xresources.xfce.in
	@sed -e "s%BIN_DIR%$(bindir)%g" \
	     -e "s%DT_CONFDIR%$(dtconfdir)%g" \
	     -e "s%DT_SUFFIX%$(DT_SUFFIX)%g" \
					$(srcdir)/Xresources.xfce.in > $@

Xsession.xfce:     $(srcdir)/Xsession.xfce.in
	@sed -e "s%DT_CONFDIR%$(dtconfdir)%g" \
	     -e "s%DT_SUFFIX%$(DT_SUFFIX)%g" \
					$(srcdir)/Xsession.xfce.in > $@

install-data-local:
	test @USE_DT@ = "no" || $(MAKE) install-dt

install-dt: $(xfce_FILES)
	$(mkinstalldirs) $(DESTDIR)$(dtconfdir)/C/Xresources.d/
	$(INSTALL_SCRIPT) Xinitrc.xfce \
	 $(DESTDIR)$(dtconfdir)/Xinitrc.xfce$(DT_SUFFIX)
	$(INSTALL_SCRIPT) Xsession.xfce \
	 $(DESTDIR)$(dtconfdir)/Xsession.xfce$(DT_SUFFIX)
	$(INSTALL_DATA) Xresources.xfce \
	 $(DESTDIR)$(dtconfdir)/C/Xresources.d/Xresources.xfce$(DT_SUFFIX)


uninstall-local: uninstall-dt

uninstall-dt:
	rm -f $(DESTDIR)$(dtconfdir)/Xinitrc.xfce$(DT_SUFFIX)
	rm -f $(DESTDIR)$(dtconfdir)/Xsession.xfce$(DT_SUFFIX)
	rm -f $(DESTDIR)$(dtconfdir)/C/Xresources.d/Xresources.xfce$(DT_SUFFIX)
