Summary: Home directory files for user orchids.
Name: orchids-home-demo
Group: Orchids Demo
Version: 1.0
Release: 1
License: GPL
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: sudo gdm

# on FC7, default session is set in .dmrc file.
# Be sure to have the following file in the tarball.
# $ cat > ~orchids/.dmrc << END_OF_DMRC
# [Desktop]
# Session=WindowMaker
# END_OF_DMRC
# chown orchids:orchids ~orchids/.dmrc
# chmod 644 ~orchids/.dmrc

# Don't forget to include in the tarball
# the ~orchids/.face png image for gdm

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

%post
sed -i -e 's/\[daemon\]/[daemon]\nTimedLoginEnable=true\nTimedLogin=orchids\nTimedLoginDelay=60/' /etc/gdm/custom.conf

# Add qemu and orchids for user orchids in sudo
if ! grep -zF "# Sudo Config for Orchids Demo" /etc/sudoers ; then
cat << EOF >> /etc/sudoers

# Sudo Config for Orchids Demo
orchids ALL = NOPASSWD: /home/orchids/orchids/bin/orchids
orchids ALL = NOPASSWD: /usr/bin/qemu
orchids ALL = NOPASSWD: /usr/bin/cpufreq-set
orchids ALL = NOPASSWD: /sbin/service
EOF
fi


%files
%defattr(-,orchids,orchids)
/home/orchids

%changelog
