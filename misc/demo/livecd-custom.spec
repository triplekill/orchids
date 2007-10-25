Summary: LiveCD Customization
Name: livecd-custom
Version: 1.0
Release: 1
License: GPL
Group: Orchids Demo
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

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
END_OF_SCRIPT

chmod 755 $RPM_BUILD_ROOT/etc/rc.d/init.d/fedora-live


%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add fedora-live

%files
%defattr(-,root,root,-)
%doc
/etc/rc.d/init.d/fedora-live

%changelog
