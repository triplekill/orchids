# Based on livecd-fedora-desktop.ks of livecd-tools-009
# Build iso image, as root, with the command:
# livecd-creator --config=orchids-live-demo-cd.ks

lang en_US.UTF-8
keyboard us
timezone US/Eastern
auth --useshadow --enablemd5
selinux --disabled
firewall --disabled
# repo --name=everything-base --baseurl=http://download.fedora.redhat.com/pub/fedora/linux/releases/7/Everything/i386/os/
# repo --name=everything-update --baseurl=http://download.fedora.redhat.com/pub/fedora/linux/updates/7/i386/
repo --name=everything-base --baseurl=file:///var/local/fedora-mirror/releases/7/Everything/i386/os/
repo --name=everything-update --baseurl=file:///var/local/fedora-mirror/updates/7/i386/
xconfig --startxonboot
services --enabled=httpd --disabled=network,sshd,ConsoleKit,avahi-daemon,mcstrans
firstboot --disable

%packages

# minimal base
bash
kernel
syslinux
passwd
policycoreutils
chkconfig
authconfig
rootfiles

# less minimal base
perl
openssh
openssh-clients
openssh-server
#emacs-nox
vim-minimal
sudo
rsync
wget
#man
tcpdump
cpufreq-utils
memtest86+

# dev tools
#gcc
#binutils
#make
#lsof
#lslk
#ltrace
#strace
#gdb

# Xorg
xorg-x11-xinit
xorg-x11-server-Xorg
xorg-x11-server-utils
xorg-x11-drv-*
-xorg-x11-drv-*-devel
system-config-display
gdm
#xorg-x11-xdm
WindowMaker
#fluxbox
xterm

# tools for demo
qemu
httpd
firefox
gkrellm
gkrellm-freq
gkrellm-wifi
rsh
xpdf

# Orchids dependencies
pl
net-snmp
net-snmp-libs
net-snmp-utils
graphviz
ghostscript
ImageMagick
libpcap
#gnuplot

# installer
#anaconda
#anaconda-runtime

# make sure debuginfo doesn't end up on the live image
-*debuginfo


%post
# FIXME: it'd be better to get this installed from a package
cat > /etc/rc.d/init.d/fedora-live << END_OF_SCRIPT
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

# turn off firstboot for livecd boots
#echo "RUN_FIRSTBOOT=NO" > /etc/sysconfig/firstboot

# don't start yum-updatesd for livecd boots
#chkconfig --level 345 yum-updatesd off

# don't start cron/at as they tend to spawn things which are
# disk intensive that are painful on a live image
#chkconfig --level 345 crond off
#chkconfig --level 345 atd off
#chkconfig --level 345 anacron off
#chkconfig --level 345 readahead_early off
#chkconfig --level 345 readahead_later off

# Stopgap fix for RH #217966; should be fixed in HAL instead
touch /media/.hal-mtab
END_OF_SCRIPT

chmod 755 /etc/rc.d/init.d/fedora-live
/sbin/restorecon /etc/rc.d/init.d/fedora-live
/sbin/chkconfig --add fedora-live

# add fedora user with no passwd
useradd -c "Fedora Live" fedora
passwd -d fedora > /dev/null
# disable screensaver locking
gconftool-2 --direct --config-source=xml:readwrite:/etc/gconf/gconf.xml.defaults -s -t bool /apps/gnome-screensaver/lock_enabled false >/dev/null
# set up timed auto-login for after 60 seconds
sed -i -e 's/\[daemon\]/[daemon]\nTimedLoginEnable=true\nTimedLogin=fedora\nTimedLoginDelay=60/' /etc/gdm/custom.conf
if [ -e /usr/share/icons/hicolor/96x96/apps/fedora-logo-icon.png ] ; then
    cp /usr/share/icons/hicolor/96x96/apps/fedora-logo-icon.png /home/fedora/.face
    chown fedora:fedora /home/fedora/.face
    # TODO: would be nice to get e-d-s to pick this one up too... but how?
fi

# Set default session
cat > ~fedora/.dmrc << END_OF_DMRC
[Desktop]
Session=WindowMaker
END_OF_DMRC
chown fedora:fedora ~fedora/.dmrc
chmod 644 ~fedora/.dmrc


# save a little bit of space at least...
rm -f /boot/initrd*
#find /usr/share/man/ -mindepth 1 -maxdepth 1 -not -name "man*" | xargs rm -rf
rm -rf /usr/share/man/*
rm -rf /usr/share/doc/*
rm -rf /usr/share/locale/*
rm -rf /usr/share/info/*
rm -rf /usr/share/icons/*

%end
