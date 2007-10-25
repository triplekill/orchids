Summary: Home directory files for user orchids.
Name: orchids-home-demo
Group: Orchids Demo
Version: 1.0
Release: 1
License: GPL
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: gdm

%description
Files of the home directory of the demo user account orchids.

%prep

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
tar xjvf %{SOURCE0} -C $RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%pre
/usr/sbin/groupadd -g 1000 orchids 2> /dev/null
/usr/sbin/useradd -u 1000 -g 1000 -c "Orchids Live Demo" orchids 2> /dev/null
/usr/bin/passwd -d orchids
exit 0

%files
%defattr(-,orchids,orchids)
/home/orchids

%changelog
