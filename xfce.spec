%define 	name	xfce
%define 	version	3.8.18
%define		release	1
%define		prefix 	/usr
%define		datadir /usr/share
%define		confdir /etc/X11
%define		gtkengines /usr/lib/gtk/themes/engines
%define		gdmdata /etc/X11/gdm/Sessions
%define		wmsession /etc/X11/wmsession.d
%define		switchdesk /usr/share/apps/switchdesk

Summary:	A Powerful X Environment, with Toolbar and Window Manager
Name:		%{name}
Version: 	%{version}
Release: 	%{release}
URL: 		http://www.xfce.org
Copyright:	GPL
Group: 		User Interface/Desktops
Source:		http://www.xfce.org/archive/%{name}-%{version}.tar.gz
Buildroot: 	/var/tmp/%{name}-root
# Requires: 	xscreensaver, xfce-libs, gtk+ >= 1.2.8
Requires: 	xscreensaver, gtk+ >= 1.2.8
Packager:	Olivier Fourdan <fourdan@xfce.org>

%description
XFce is a fast, lightweight desktop
environment for Linux and various Unices...

# %package libs
# Summary:	Required internal libraries for Xfce
# Group: 		User Interface/Desktops
# Packager:	Olivier Fourdan <fourdan@xfce.org>
# 
# %description libs
# A couple libraries for Xfce components.

%prep
%setup -q -n %{name}-%{version}

%build
if [ ! -f configure ]; then
  ./autogen.sh --prefix=%{prefix} --datadir=%{datadir} --sysconfdir=%{confdir} \
  --disable-dt --enable-gtk-engine=%{gtkengines} --disable-xft \
  --enable-gdm --enable-taskbar --with-gdm-prefix=%{gdmdata}
else
  ./configure --prefix=%{prefix} --datadir=%{datadir} --sysconfdir=%{confdir} \
  --disable-dt --enable-gtk-engine=%{gtkengines} --disable-xft \
  --enable-gdm --enable-taskbar --with-gdm-prefix=%{gdmdata}
fi
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT mandir=%{_mandir}
mkdir -p $RPM_BUILD_ROOT/usr/share/icons
mkdir -p $RPM_BUILD_ROOT%{gdmdata}
mkdir -p $RPM_BUILD_ROOT%{gtkengines}

(
cd $RPM_BUILD_ROOT%{datadir}/xfce
ln -sf ../icons more-icons
)

chmod 0755 $RPM_BUILD_ROOT%{confdir}/xfce/{xsession,xinitrc,xinitrc.mwm,Xclients}

# strip -s $RPM_BUILD_ROOT%{prefix}/bin/* || :
strip -s $RPM_BUILD_ROOT%{gtkengines}/* || :
gzip $RPM_BUILD_ROOT%{_mandir}/man1/* || :

%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root)
%doc INSTALL ChangeLog AUTHORS COPYING README.UPGRADE-3.*
%{_bindir}/*
%{_datadir}/xfce/*
%{_mandir}/man1/*
%config(noreplace) %{confdir}/xfce/*
%{gtkengines}/libxfce.*
%config(noreplace) %{gdmdata}/XFce
%config(noreplace) %{wmsession}/10XFce
%config(noreplace) %{switchdesk}/Xclients.xfce

# %files libs
# %defattr(-, root, root)
# %{_libdir}/lib*.so*
# %{_libdir}/lib*.a

%post
# /sbin/ldconfig
if [ "$LC_ALL" = "pt_BR" ]; then
   echo
   echo Nota :
   echo Você agora pode correr o script xfce_setup instalar o xfce como o
   echo ambiente de trabalho padrão ou iniciar o xfce usando o script startxfce
   echo
   echo Importante :
   echo Se você está atualizando uma versão anterior do xfce, por favor
   echo corra o script xfce_upgrade para cada usuário usando o xfce como
   echo ambiente de trabalho, para grantir a compatibilidade com versões
   echo anteriores.
elif [ "$LC_ALL" = "es" ]; then
   echo
   echo Nota :
   echo Ahora usted puede correr el script xfce_setup, instalar xfce como el
   echo ambiente de trabajo estandar o iniciar  xfce usando el script startxfce
   echo
   echo Importante :
   echo Si usted está actualizando una versión anterior de xfce, por favor
   echo corra el script xfce_upgrade para cada usuario usando xfce como
   echo ambiente de trabajo, esto tiene como objetivo para grantizar la
   echo compatibilidad con versiones anteriores.
else
   echo
   echo Note :
   echo You can now run the script xfce_setup to install xfce as the default
   echo desktop environment or start xfce using the script startxfce
   echo
   echo Important :
   echo If you are upgrading from a previous version of xfce, please run
   echo the script xfce_upgrade for each user running xfce as their desktop
   echo environment, to ensure backward compatibility.
   echo
fi

# %postun
# /sbin/ldconfig

%changelog
* Mon Oct 30 2000 Charles Stevenson <csteven@yellowdoglinux.com>
- A few spec file fixes that work with the new man location

* Sun Oct  1 2000 Fernando M. Roxo da Motta <roxo@conectiva.com>
- Updated to 3.5.2
- Updated brazilian portuguese potfile ( pt_BR.po )
- Updated spanish potfile ( es.po )

* Tue Sep 19 2000 Rodrigo Barbosa <rodrigob@conectiva.com>
- Misc. fixes

* Mon Sep 18 2000 Fernando M. Roxo da Motta <roxo@conectiva.com>
- Added updated brazilian portuguese potfile
- Changed specfile to be according to general RPM macro standards.
* Thu Sep 14 2000 Fernando M. Roxo da Motta <roxo@conectiva.com>
- Added updated brazilian portuguese potfile ( pt_BR.po )
- Using Conectiva macros

* Mon Apr 17 2000 Olivier Fourdan <fourdan@xfce.org>
- Update with FHS compliancy
* Thu Dec 16 1999 Tim Powers <timp@redhat.com>
- updated to 3.2.2
- general cleanups to make it a bit easier to maintain
- configure to build section
- quiet setup
- no more %pre/preun or %post/postun sections, not needed (done in %install instead)
- no more posinstall messages being displayed
- cleaned up %files section
- gzip man pages

* Sat Oct 23 1999 P. Reich -- fixed spec file to build and install 
- correctly in BUILD_ROOT.
- Added preun to remove links in xfce-datadir.
- Moved DATADIR to /usr/share
- Added link to "more-icons" in /usr/share/icons. 
