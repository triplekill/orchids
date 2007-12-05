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


%package mod_autohtml
Summary: Module mod_autohtml
Group: System Environment/Daemons

%description mod_autohtml
Module mod_autohtml.


%package mod_cisco
Summary: Module mod_cisco
Group: System Environment/Daemons

%description mod_cisco
Module mod_cisco.


%package mod_clocks
Summary: Module mod_clocks
Group: System Environment/Daemons

%description mod_clocks
Module mod_clocks.


%package mod_consoles
Summary: Module mod_consoles
Group: System Environment/Daemons

%description mod_consoles
Module mod_consoles.


%package mod_generic
Summary: Module mod_generic
Group: System Environment/Daemons

%description mod_generic
Module mod_generic.


%package mod_htmlstate
Summary: Module mod_htmlstate
Group: System Environment/Daemons

%description mod_htmlstate
Module mod_htmlstate.


%package mod_netfilter
Summary: Module mod_netfilter
Group: System Environment/Daemons

%description mod_netfilter
Module mod_netfilter.


%package mod_pcap
Summary: Module mod_pcap
Group: System Environment/Daemons

%description mod_pcap
Module mod_pcap.


%package mod_period
Summary: Module mod_period
Group: System Environment/Daemons

%description mod_period
Module mod_period.


%package mod_prism2
Summary: Module mod_prism2
Group: System Environment/Daemons

%description mod_prism2
Module mod_prism2.


%package mod_prolog
Summary: Module mod_prolog
Group: System Environment/Daemons

%description mod_prolog
Module mod_prolog.


%package mod_rawsnare
Summary: Module mod_rawsnare
Group: System Environment/Daemons

%description mod_rawsnare
Module mod_rawsnare.


%package mod_remoteadm
Summary: Module mod_remoteadm
Group: System Environment/Daemons

%description mod_remoteadm
Module mod_remoteadm.


%package mod_ruletrace
Summary: Module mod_ruletrace
Group: System Environment/Daemons

%description mod_ruletrace
Module mod_ruletrace.


%package mod_sharedvars
Summary: Module mod_sharedvars
Group: System Environment/Daemons

%description mod_sharedvars
Module mod_sharedvars.


%package mod_snare
Summary: Module mod_snare
Group: System Environment/Daemons

%description mod_snare
Module mod_snare.


#%package mod_snmp
#Summary: Module mod_snmp
#Group: System Environment/Daemons
#
#%description mod_snmp
#Module mod_snmp.


%package mod_snmptrap
Summary: Module mod_snmptrap
Group: System Environment/Daemons

%description mod_snmptrap
Module mod_snmptrap.


%package mod_sockunix
Summary: Module mod_sockunix
Group: System Environment/Daemons

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

%description mod_sunbsm
Module mod_sunbsm.


%package mod_syslog
Summary: Module mod_syslog
Group: System Environment/Daemons

%description mod_syslog
Module mod_syslog.


#%package mod_test2
#Summary: Module mod_test2
#Group: System Environment/Daemons
#
#%description mod_test2
#Module mod_test2.


#%package mod_test
#Summary: Module mod_test
#Group: System Environment/Daemons
#
#%description mod_test
#Module mod_test.


%package mod_textfile
Summary: Module mod_textfile
Group: System Environment/Daemons

%description mod_textfile
Module mod_textfile.


%package mod_timeout
Summary: Module mod_timeout
Group: System Environment/Daemons

%description mod_timeout
Module mod_timeout.


%package mod_udp
Summary: Module mod_udp
Group: System Environment/Daemons

%description mod_udp
Module mod_udp.


%package mod_wifi
Summary: Module mod_wifi
Group: System Environment/Daemons

%description mod_wifi
Module mod_wifi.


%package mod_win32evt
Summary: Module mod_win32evt
Group: System Environment/Daemons

%description mod_win32evt
Module mod_win32evt.



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
%attr(-,nobody,nobody) %{prefix}/var/orchids/log
%attr(-,nobody,nobody)%{prefix}/var/orchids/reports
%doc AUTHORS ChangeLog COPYING* NEWS README

%files mod_autohtml
%{prefix}/usr/lib/orchids/mod_autohtml.*

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

#%files mod_snmp
#%{prefix}/usr/lib/orchids/mod_snmp.*

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

#%files mod_test
#%{prefix}/usr/lib/orchids/mod_test.*

#%files mod_test2
#%{prefix}/usr/lib/orchids/mod_test2.*

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

