Summary: LiveCD Customization
Name: livecd-custom
Version: 1.0
Release: 1
License: GPL
Group: Orchids Demo
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: GConf2

%description

%prep

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d/

cat > $RPM_BUILD_ROOT/etc/rc.d/init.d/fedora-live << END_OF_SCRIPT
#!/bin/bash
#
# live: Init script for live image
#
# chkconfig: 345 00 99
# description: Init script for live image.

. /etc/init.d/functions

if ! strstr "\`cat /proc/cmdline\`" liveimg || [ "\$1" != "start" ] || [ -e /.liveimg-configured ] ; then
    exit 0
fi

exists() {
    which \$1 >/dev/null 2>&1 || return
    \$*
}

touch /.liveimg-configured

# mount live image
if [ -b /dev/live ]; then
   mkdir -p /mnt/live
   mount -o ro /dev/live /mnt/live
fi

# configure X, allowing user to override xdriver
for o in \`cat /proc/cmdline\` ; do
    case \$o in
    xdriver=*)
        xdriver="--set-driver=\${o#xdriver=}"
        ;;
    esac
done

exists system-config-display --noui --reconfig --set-depth=24 \$xdriver

# unmute sound card
exists alsaunmute 0 2> /dev/null

# Stopgap fix for RH #217966; should be fixed in HAL instead
touch /media/.hal-mtab

# Add virtual machine in the /etc/hosts file
if ! grep -zF "# For QEmu Virtual Machine" /etc/hosts ; then
cat << EOF >> /etc/hosts

# For QEmu Virtual Machine
10.0.0.1        host
10.0.0.100      orchidsvm
EOF
fi

END_OF_SCRIPT

chmod 755 $RPM_BUILD_ROOT/etc/rc.d/init.d/fedora-live


cat > $RPM_BUILD_ROOT/etc/rc.d/init.d/kudzu-live << END_OF_SCRIPT
#!/bin/bash
#
# live: Init script for live image
#
# chkconfig: 345 06 99
# description: Init script for live image.

. /etc/init.d/functions

if ! strstr "\`cat /proc/cmdline\`" liveimg || [ "\$1" != "start" ] ; then
    exit 0
fi

# disable all configured interface on boot.
find /etc/sysconfig/network-scripts/ \\
  -name "ifcfg-*" -not -name "ifcfg-lo" | while read f ; do
  sed -i 's/ONBOOT=yes/ONBOOT=no/' \$f
done
END_OF_SCRIPT

chmod 755 $RPM_BUILD_ROOT/etc/rc.d/init.d/kudzu-live

mkdir -p $RPM_BUILD_ROOT/etc/httpd/conf.d/

cat > $RPM_BUILD_ROOT/etc/httpd/conf.d/orchids.conf << END_OF_CONFIG
Alias /orchids/ "/home/orchids/orchids/var/orchids/htmlstate/"

<Directory "/home/orchids/orchids/var/orchids/htmlstate">
    AllowOverride None
    Order allow,deny
    Allow from all
</Directory>
END_OF_CONFIG


%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add fedora-live
/sbin/chkconfig --add kudzu-live

# disable screensaver locking
gconftool-2 --direct --config-source=xml:readwrite:/etc/gconf/gconf.xml.defaults -s -t bool /apps/gnome-screensaver/lock_enabled false >/dev/null


%files
%defattr(-,root,root,-)
%doc
/etc/rc.d/init.d/fedora-live
/etc/rc.d/init.d/kudzu-live
/etc/httpd/conf.d/orchids.conf

%changelog
