%define name	orchids
%define version 1.0
%define release 1
%define prefix  /

Summary: A real-time intrusion detection system.
Name: %{name}
Version: %{version}
Release: %{release}
License: CeCILL2
Group: System Environment/Daemons

Source: orchids-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-root

%description
A real-time intrusion detection system.

%prep

%setup

%build
%configure
make

%install
mkdir -p $RPM_BUILD_ROOT/etc/init.d
cp misc/orchids-sysv $RPM_BUILD_ROOT/etc/init.d/orchids
%makeinstall

%files
%defattr(-,root,root)
%attr(755,root,root) %{prefix}/etc/init.d/orchids
%attr(755,root,root) %{prefix}/usr/bin/orchids
%config %{prefix}/etc/orchids/orchids.conf
%config %{prefix}/etc/orchids/rules/*
%{prefix}/usr/lib/orchids/*
%attr(-,nobody,nobody)%{prefix}/var/orchids/*
%doc AUTHORS ChangeLog COPYING* NEWS README

%clean
rm -r $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add orchids

%preun
if [ $1 -eq 0 ]; then
        /sbin/service orchids stop &>/dev/null || :
        /sbin/chkconfig --del orchids
fi

%postun
/sbin/service %{name} condrestart &>/dev/null || :

%changelog

