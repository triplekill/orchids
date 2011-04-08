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

Requires: cpp /usr/sbin/sendmail

%description
A real-time intrusion detection system.

%package mod_cisco
Summary: Module mod_cisco
Group: System Environment/Daemons
Requires: orchids
BuildRequires: flex bison

%description mod_cisco
Module mod_cisco.


%package mod_clocks
Summary: Module mod_clocks
Group: System Environment/Daemons
Requires: orchids

%description mod_clocks
Module mod_clocks.


%package mod_consoles
Summary: Module mod_consoles
Group: System Environment/Daemons
Requires: orchids

%description mod_consoles
Module mod_consoles.


%package mod_generic
Summary: Module mod_generic
Group: System Environment/Daemons
Requires: orchids

%description mod_generic
Module mod_generic.


%package mod_htmlstate
Summary: Module mod_htmlstate
Group: System Environment/Daemons
Requires: orchids graphviz ghostscript ImageMagick gnuplot

%description mod_htmlstate
Module mod_htmlstate.


%package mod_netfilter
Summary: Module mod_netfilter
Group: System Environment/Daemons
BuildRequires: flex bison
Requires: orchids

%description mod_netfilter
Module mod_netfilter.


%package mod_pcap
Summary: Module mod_pcap
Group: System Environment/Daemons
BuildRequires: libpcap
Requires: orchids libpcap

%description mod_pcap
Module mod_pcap.


%package mod_period
Summary: Module mod_period
Group: System Environment/Daemons
Requires: orchids

%description mod_period
Module mod_period.


%package mod_prism2
Summary: Module mod_prism2
Group: System Environment/Daemons
Requires: orchids orchids-mod_pcap

%description mod_prism2
Module mod_prism2.


%package mod_prolog
Summary: Module mod_prolog
Group: System Environment/Daemons
BuildRequires: pl
Requires: orchids pl

%description mod_prolog
Module mod_prolog.


%package mod_rawsnare
Summary: Module mod_rawsnare
Group: System Environment/Daemons
Requires: orchids orchids-mod_udp

%description mod_rawsnare
Module mod_rawsnare.


%package mod_remoteadm
Summary: Module mod_remoteadm
Group: System Environment/Daemons
Requires: orchids

%description mod_remoteadm
Module mod_remoteadm.


%package mod_ruletrace
Summary: Module mod_ruletrace
Group: System Environment/Daemons
Requires: orchids

%description mod_ruletrace
Module mod_ruletrace.


%package mod_sharedvars
Summary: Module mod_sharedvars
Group: System Environment/Daemons
Requires: orchids

%description mod_sharedvars
Module mod_sharedvars.


%package mod_snare
Summary: Module mod_snare
Group: System Environment/Daemons
BuildRequires: flex bison
Requires: orchids

%description mod_snare
Module mod_snare.

%package mod_snmptrap
Summary: Module mod_snmptrap
Group: System Environment/Daemons
BuildRequires: net-snmp-devel
Requires: orchids net-snmp orchids-mod_udp

%description mod_snmptrap
Module mod_snmptrap.


%package mod_sockunix
Summary: Module mod_sockunix
Group: System Environment/Daemons
Requires: orchids

%description mod_sockunix
Module mod_sockunix.


#%package mod_stats
#Summary: Module mod_stats
#Group: System Environment/Daemons
#
#%description mod_stats
#Module mod_stats.


%package mod_sunbsm
Summary: Module mod_sunbsm
Group: System Environment/Daemons
Requires: orchids

%description mod_sunbsm
Module mod_sunbsm.


%package mod_syslog
Summary: Module mod_syslog
Group: System Environment/Daemons
Requires: orchids

%description mod_syslog
Module mod_syslog.

%package mod_textfile
Summary: Module mod_textfile
Group: System Environment/Daemons
Requires: orchids

%description mod_textfile
Module mod_textfile.


%package mod_timeout
Summary: Module mod_timeout
Group: System Environment/Daemons
Requires: orchids

%description mod_timeout
Module mod_timeout.


%package mod_udp
Summary: Module mod_udp
Group: System Environment/Daemons
Requires: orchids

%description mod_udp
Module mod_udp.


%package mod_wifi
Summary: Module mod_wifi
Group: System Environment/Daemons
Requires: orchids

%description mod_wifi
Module mod_wifi.


%package mod_win32evt
Summary: Module mod_win32evt
Group: System Environment/Daemons
Requires: orchids

%description mod_win32evt
Module mod_win32evt.

%package mod_auditd
Summary: Module mod_auditd
Group: System Environment/Daemons
Requires: orchids

%description mod_auditd
Module mod_auditd.

%package mod_prelude
Summary: Module mod_prelude
Group: System Environment/Daemons
Requires: orchids

%description mod_prelude
Module mod_prelude.

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
%config %{prefix}/etc/orchids/orchids-rules.conf
%config %{prefix}/etc/orchids/rules/*
%dir %{prefix}/etc/orchids/conf.d
%attr(-,nobody,nobody) %{prefix}/var/run/orchids
%attr(-,nobody,nobody) %{prefix}/var/orchids/log
%attr(-,nobody,nobody)%{prefix}/var/orchids/reports
%doc AUTHORS ChangeLog COPYING* NEWS README

%files mod_cisco
%{prefix}/usr/lib/orchids/mod_cisco.*

%files mod_clocks
%{prefix}/usr/lib/orchids/mod_clocks.*
%config %{prefix}/etc/orchids/conf.d/11_mod_clocks.conf

%files mod_consoles
%{prefix}/usr/lib/orchids/mod_consoles.*
%config %{prefix}/etc/orchids/conf.d/05_mod_consoles.conf

%files mod_generic
%{prefix}/usr/lib/orchids/mod_generic.*
%config %{prefix}/etc/orchids/conf.d/12_mod_generic.conf

%files mod_htmlstate
%{prefix}/usr/lib/orchids/mod_htmlstate.*
%attr(-,nobody,nobody)%{prefix}/var/orchids/htmlstate/*

%files mod_netfilter
%{prefix}/usr/lib/orchids/mod_netfilter.*
%config %{prefix}/etc/orchids/conf.d/09_mod_netfilter.conf

%files mod_pcap
%{prefix}/usr/lib/orchids/mod_pcap.*
%config %{prefix}/etc/orchids/conf.d/13_mod_pcap.conf

%files mod_period
%{prefix}/usr/lib/orchids/mod_period.*

%files mod_prism2
%{prefix}/usr/lib/orchids/mod_prism2.*

%files mod_prolog
%{prefix}/usr/lib/orchids/mod_prolog.*
%config %{prefix}/etc/orchids/conf.d/10_mod_prolog.conf

%files mod_rawsnare
%{prefix}/usr/lib/orchids/mod_rawsnare.*
%config %{prefix}/etc/orchids/conf.d/08_mod_rawsnare.conf

%files mod_remoteadm
%{prefix}/usr/lib/orchids/mod_remoteadm.*
%config %{prefix}/etc/orchids/conf.d/06_mod_remoteadm.conf

%files mod_ruletrace
%{prefix}/usr/lib/orchids/mod_ruletrace.*
%attr(-,nobody,nobody)%{prefix}/var/orchids/ruletrace
%config %{prefix}/etc/orchids/conf.d/16_mod_ruletrace.conf

%files mod_sharedvars
%{prefix}/usr/lib/orchids/mod_sharedvars.*
%config %{prefix}/etc/orchids/conf.d/15_mod_sharedvars.conf

%files mod_snare
%{prefix}/usr/lib/orchids/mod_snare.*
%config %{prefix}/etc/orchids/conf.d/07_mod_snare.conf

%files mod_snmptrap
%{prefix}/usr/lib/orchids/mod_snmptrap.*

%files mod_sockunix
%{prefix}/usr/lib/orchids/mod_sockunix.*
%config %{prefix}/etc/orchids/conf.d/04_mod_sockunix.conf

#%files mod_stats
#%{prefix}/usr/lib/orchids/mod_stats.*

%files mod_sunbsm
%{prefix}/usr/lib/orchids/mod_sunbsm.*

%files mod_syslog
%{prefix}/usr/lib/orchids/mod_syslog.*
%config %{prefix}/etc/orchids/conf.d/02_mod_syslog.conf

%files mod_textfile
%{prefix}/usr/lib/orchids/mod_textfile.*
%config %{prefix}/etc/orchids/conf.d/01_mod_textfile.conf

%files mod_timeout
%{prefix}/usr/lib/orchids/mod_timeout.*

%files mod_udp
%{prefix}/usr/lib/orchids/mod_udp.*
%config %{prefix}/etc/orchids/conf.d/03_mod_udp.conf

%files mod_wifi
%{prefix}/usr/lib/orchids/mod_wifi.*
%config %{prefix}/etc/orchids/conf.d/14_mod_wifi.conf

%files mod_win32evt
%{prefix}/usr/lib/orchids/mod_win32evt.*

%files mod_auditd
%{prefix}/usr/lib/orchids/mod_auditd.*

%files mod_prelude
%{prefix}/usr/lib/orchids/mod_prelude.*
%config %{prefix}/etc/orchids/conf.d/17_mod_prelude.conf

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

* Fri Feb 22 2008 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Added orchids dependecy for all module mod_* packages.

* Sun Dec 16 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Added cpp dependency for the main package.

* Sun Dec  9 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Imported change log into this spec file.

* Fri Dec  7 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Added package dependencies and build dependencies.

* Wed Dec  5 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Split modules into sub-packages.

* Fri Jun  1 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Added split config files.

* Wed Feb 28 2007 Julien Olivain <julien.olivain@lsv.ens-cachan.fr>
- Initial spec file.
