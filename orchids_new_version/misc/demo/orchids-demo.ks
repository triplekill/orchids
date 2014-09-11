# Based on livecd-fedora-desktop.ks of livecd-tools-009
# Build iso image, as root, with the command:
# livecd-creator --config=orchids-demo-cd.ks

lang en_US.UTF-8
keyboard us
timezone US/Eastern
auth --useshadow --enablemd5
selinux --disabled
firewall --disabled

# Uncomment if we want to use online repositories
# repo --name=everything-base --baseurl=http://download.fedora.redhat.com/pub/fedora/linux/releases/8/Everything/i386/os/
# repo --name=everything-update --baseurl=http://download.fedora.redhat.com/pub/fedora/linux/updates/8/i386/

# local version made by rsync'ing
# rsync://mirrors.kernel.org/fedora/releases/8/Everything/i386/os/
# rsync://mirrors.kernel.org/fedora/updates/8/i386/
repo --name=everything-base --baseurl=file:///var/local/repos/releases/8/Everything/i386/os/
repo --name=everything-update --baseurl=file:///var/local/repos/updates/8/i386/

# Local repository for custom packages
# made by dropping RPMs in the directory /var/local/repos/local/
# then by running "createrepo -dv /var/local/repos/local/"
repo --name=local --baseurl=file:///var/local/repos/local/

xconfig --startxonboot
services --enabled=network,httpd --disabled=sshd,ConsoleKit,avahi-daemon,mcstrans,netfs,restorecond
firstboot --disable
network --hostname=orchids.localdomain --device=eth0 --onboot=no --bootproto=dhcp --noipv6


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
cpuspeed
cpufreq-utils
dhclient
acpid
usbutils

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
evtgen
livecd-custom
orchids-home-demo
qemu
# kqemu is not part of Fedora but can be found at ATRpms
# download from http://atrpms.net/dist/f8/kqemu/ and put right files
# into the local RPM repository for custom packages.
kqemu
kqemu-kmdl-*
httpd
firefox
gkrellm
gkrellm-freq
gkrellm-wifi
rsh
xpdf

# Orchids and its modules
orchids
orchids-mod_*

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

# installer (+14 MB on final iso if added)
# Don't forget to disable firstboot service if anaconda is installed.
#anaconda
#anaconda-runtime

# make sure debuginfo doesn't end up on the live image
-*debuginfo


%post

# create an empty /etc/resolv.conf to avoid warning messages.
touch /etc/resolv.conf

# save a little bit of space at least...
rm -f /boot/initrd*
#find /usr/share/man/ -mindepth 1 -maxdepth 1 -not -name "man*" | xargs rm -rf
rpm -ql cracklib-dicts | xargs rm -rf
rm -rf /usr/share/man/*
rm -rf /usr/share/doc/*
rm -rf /usr/share/locale/*
rm -rf /usr/share/info/*
rm -rf /usr/share/icons/*
rm -rf /usr/share/gnome/help/*

%end
