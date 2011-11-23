#!/bin/sed -rf
# skip messages
s/^.{15} [^ ]+ last message repeated [0-9]+ times/---- syslog repeat ----/g;
s/^.{15} [^ ]+ [^[]+\[[0-9]*\]: pam_succeed_if: .*/---- pam_succeed_if ----/g;
s/^.{15} [^ ]+ [^[]+\[[0-9]*\]: input_userauth_request: illegal user (.*)/---- input_userauth_request ----/g;
# ssh http://www.openssh.com/
s/^.{15} [^ ]+ sshd\[[0-9]*\]: (Accepted|Failed|Postponed) (password|publickey|hostbased) for ([^ ]+) from ([0-9.]+) port ([0-9]+) (.*)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: (Accepted|Failed|Postponed) (password|publickey|hostbased) for (invalid user|illegal user) ([^ ]+) from ([0-9.]+) port ([0-9]+) (.*)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: (Accepted|Failed) (rhosts-rsa) for ([^ ]+) from ([0-9.]+) port ([0-9]+) ruser (.*)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: userauth_hostbased mismatch: client sends ([^,]+), but we resolve ([0-9.]+) to (.*)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: Bad protocol version identification '([^']+)' from ([0-9.]+)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: Connection closed by ([0-9.]+)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: Received disconnect from ([0-9.]+): 13: The user canceled authentication\./---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: Did not receive identification string from ([0-9.]+)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: (Illegal user|Invalid user) ([^ ]+) from ([0-9.]+)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: Received disconnect from ([0-9.]+): 11: Bye Bye/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: error: Could not get shadow information for ([^ ]+)/---- sshd ----/g;
s/^.{15} [^ ]+ sshd\[[0-9]*\]: subsystem request for ([^ ]+)/---- sshd ----/g;
#
# sudo http://www.courtesan.com/sudo/
s/^.{15} [^ ]+ sudo:  *([^ ]+) : TTY=([^ ]+) ; PWD=([^ ]+) ; USER=([^ ]+) ; COMMAND=(.+)/---- sudo ----/g;
#
# tftpd
s/^.{15} [^ ]+ tftpd\[[0-9]*\]: Serving ([^ ]+) to ([0-9.]+):([0-9]+)/---- tftpd ----/g;
s/^.{15} [^ ]+ tftpd\[[0-9]*\]: Trivial FTP server (start)ed \(([0-9.]+)\)/---- tftpd ----/g;
s/^.{15} [^ ]+ tftpd\[[0-9]*\]: Main thread (exit)ing/---- tftpd ----/g;
#
# yppasswdd
s/^.{15} [^ ]+ rpc\.yppasswdd\[[0-9]*\]: update ([^ ]+) \(uid=([0-9]+)\) from host ([0-9.]+) successful\./---- yppasswdd ----/g;
#
# pam_rhosts_auth
s/^.{15} [^ ]+ pam_rhosts_auth\[[0-9]*\]: (allow)ed to ([^@]+)@([^ ]+) as ([^ ]+)/---- pam_rhosts_auth ----/g;
#
# in.rshd
s/^.{15} [^ ]+ in\.rshd\[[0-9]*\]: ([^@]+)@([^ ]+) as ([^:]+): cmd='(.*)'/---- in.rshd ----/g;
#
# afpd http://netatalk.sourceforge.net/
s/^.{15} [^ ]+ afpd\[[0-9]*\]: login ([^ ]+) \(uid ([0-9]+), gid ([0-9]+)\) (AFP[0-9.]+)/---- afpd ----/g;
s/^.{15} [^ ]+ afpd\[[0-9]*\]: logout ([^ ]+)/---- afpd ----/g;
s/^.{15} [^ ]+ afpd\[[0-9]*\]: ([0-9]+)\.[0-9]+KB read, ([0-9]+)\.[0-9]+KB written/---- afpd ----/g;
#
# named http://www.isc.org/sw/bind/
s/^.{15} [^ ]+ named\[[0-9]*\]: lame-servers: info: lame server resolving '([^']+)' \(in '([^']+)'\?\): ([0-9.]+)#([0-9]+)/---- named ----/g;
s/^.{15} [^ ]+ named\[[0-9]*\]: xfer-out: info: client ([0-9.]+)#([0-9]+): transfer of '([^']+)': AXFR started/---- named ----/g;
s/^.{15} [^ ]+ named\[[0-9]*\]: notify: info: zone ([^:]+): sending notifies \(serial ([0-9]+)\)/---- named ----/g;
#
# arpwatch http://www-nrg.ee.lbl.gov/
s/^.{15} [^ ]+ arpwatch\[[0-9]*\]: (bogon) ([0-9.]+) ([0-9a-fA-F]+)/---- arpwatch ----/g;
s/^.{15} [^ ]+ arpwatch\[[0-9]*\]: (flip flop) ([0-9.]+) ([0-9a-fA-F]+) \(([0-9a-fA-F]+)\)/---- arpwatch ----/g;
s/^.{15} [^ ]+ arpwatch\[[0-9]*\]: (new activity) ([0-9.]+) ([0-9a-fA-F]+)/---- arpwatch ----/g;
s/^.{15} [^ ]+ arpwatch\[[0-9]*\]: (new station) ([0-9.]+) ([0-9a-fA-F]+)/---- arpwatch ----/g;
s/^.{15} [^ ]+ arpwatch\[[0-9]*\]: (changed ethernet address) ([0-9.]+) ([0-9a-fA-F]+) \(([0-9a-fA-F]+)\)/---- arpwatch ----/g;
#
# dhcpd http://www.isc.org/sw/dhcp/
s/^.{15} [^ ]+ dhcpd: (DHCPREQUEST) for ([0-9.]+) from ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPACK) on ([0-9.]+) to ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPDISCOVER) from ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPOFFER) on ([0-9.]+) to ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPNAK) on ([0-9.]+) to ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPRELEASE) of ([0-9.]+) from ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPINFORM) from ([0-9.]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: no free leases on subnet ([0-9.]+)/---- dhcpd ----/g;
s/^.{15} [^ ]+ dhcpd: (DHCPREQUEST) for ([0-9.]+) \(([0-9.]+)\) from ([0-9a-fA-F:]+) via ([^ ]+)/---- dhcpd ----/g;
#
# rsyncd http://samba.anu.edu.au/rsync/
s/^.{15} [^ ]+ rsyncd\[[0-9]*\]: rsync on ([^ ]+) from ([^ ]+) \(([0-9.]+)\)/---- rsyncd ----/g;
s/^.{15} [^ ]+ rsyncd\[[0-9]*\]: wrote ([0-9]+) bytes  read ([0-9]+) bytes  total size ([0-9]+)/---- rsyncd ----/g;
s/^.{15} [^ ]+ rsyncd\[[0-9]*\]: rsync error: (.*)/---- rsyncd ----/g;
#
# automount autofs http://www.kernel.org/pub/linux/daemons/autofs/v4/
s/^.{15} [^ ]+ automount\[[0-9]*\]: attempting to mount entry ([^ ]*)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: expired ([^ ]*)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: starting automounter version ([0-9.]+), path = ([^,]*), maptype = ([^,]*), mapname = ([^, ]*)/---- automount ----/g;
#
# xinetd http://www.xinetd.org/
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: (START): ([^ ]+) pid=([0-9]+) from=([0-9.]+|<no address>)/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: (EXIT): ([^ ]+) pid=([0-9]+) duration=([0-9]+)\(sec\)/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: Reading included configuration file: ([^ ]+) \[file=([^]]+)\] \[line=([0-9]+)]/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: removing (.*)/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: xinetd Version ([0-9.]+) (started) with ([^ ]+ )+options compiled in\./---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: Started working: ([0-9]+) available services/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: Server ([^ ]+) is not executable \[file=([^]]+)\] \[line=([0-9])+\]/---- xinetd ----/g;
s/^.{15} [^ ]+ xinetd\[[0-9]*\]: Error parsing attribute server - (DISABLING SERVICE) \[file=([^]]+)\] \[line=([0-9])+\]/---- xinetd ----/g;
#
# crond ???
s/^.{15} [^ ]+ CROND\[[0-9]*\]: \(([^)]*)\) CMD \(([^)]*)\)/---- crond ----/g;
s/^.{15} [^ ]+ crond\[[0-9]*\]: \(([^)]*)\) CMD \(([^)]*)\)/---- crond ----/g;
#
# postfix/cleanup http://www.postfix.org/
s/^.{15} [^ ]+ postfix\/cleanup\[[0-9]*\]: ([^:]+): message-id=(<[^>]*>)/---- postfix\/cleanup ----/g;
s/^.{15} [^ ]+ postfix\/cleanup\[[0-9]*\]: ([^:]+): resent-message-id=(<[^>]*>)/---- postfix\/cleanup ----/g;
#
# postfix/local
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), orig_to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(forwarded as ([^)]+)\)/---- postfix\/local ----/g;
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), orig_to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(\"\|([^\"]+)\"\)/---- postfix\/local ----/g;
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), orig_to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/local ----/g;
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(forwarded as ([^)]+)\)/---- postfix\/local ----/g;
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(\"\|([^\"]+)\"\)/---- postfix\/local ----/g;
s/^.{15} [^ ]+ postfix\/local\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/local ----/g;
#
# postfix/pickup
s/^.{15} [^ ]+ postfix\/pickup\[[0-9]*\]: ([^:]+): uid=([0-9]+) from=(<[^>]*>)/---- postfix\/pickup ----/g;
#
# postfix/nqmgr
s/^.{15} [^ ]+ postfix\/qmgr\[[0-9]*\]: ([^:]+): from=(<[^>]*>), size=([0-9]*), nrcpt=([0-9]*) \(([^)]*)\)/---- postfix\/qmgr ----/g;
s/^.{15} [^ ]+ postfix\/qmgr\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/qmgr ----/g;
s/^.{15} [^ ]+ postfix\/qmgr\[[0-9]*\]: ([^:]+): removed/---- postfix\/qmgr ----/g;
#
# postfix/smtp
s/^.{15} [^ ]+ postfix\/smtp\[[0-9]*\]: ([^:]+): to=(<[^>]*>), orig_to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/smtp ----/g;
s/^.{15} [^ ]+ postfix\/smtp\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/smtp ----/g;
#
# postfix/smtpd
s/^.{15} [^ ]+ postfix\/smtpd\[[0-9]*\]: connect from (.*)/---- postfix\/smtpd ----/g;
s/^.{15} [^ ]+ postfix\/smtpd\[[0-9]*\]: ([^:]+): client=(.*)/---- postfix\/smtpd ----/g;
s/^.{15} [^ ]+ postfix\/smtpd\[[0-9]*\]: disconnect from (.*)/---- postfix\/smtpd ----/g;
s/^.{15} [^ ]+ postfix\/smtpd\[[0-9]*\]: ([^:]+): reject: RCPT from ([^[]*)\[([0-9.]+)\]: (.*)/---- postfix\/smtpd ----/g;
s/^.{15} [^ ]+ postfix\/smtpd\[[0-9]*\]: reject: RCPT from ([^:]*)/---- postfix\/smtpd ----/g;
#
#postfix/error
s/^.{15} [^ ]+ postfix\/error\[[0-9]*\]: ([^:]+): to=(<[^>]*>), orig_to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/error ----/g;
s/^.{15} [^ ]+ postfix\/error\[[0-9]*\]: ([^:]+): to=(<[^>]*>), relay=([^,]*), delay=([0-9]*), status=([^ ]*) \(([^)]*)\)/---- postfix\/error ----/g;
#
# kernel  http://www.kernel.org/
s/^.{15} [^ ]+ kernel: fw:([^:]+):([^:]+):([^:]+):(.+)/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: device ([^ ]+) (enter)ed promiscuous mode/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: device ([^ ]+) (left) promiscuous mode/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: spurious 8259A interrupt: IRQ([0-9]+)\./---- kernel ----/g;
s/^.{15} [^ ]+ kernel: modprobe: Can't locate module ([^ ]+)/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: FATAL: Error running install command for ([^ ]+)/---- kernel ----/g;
#
# httpd http://httpd.apache.org/
s/^.{15} [^ ]+ httpd\[[0-9]*\]: ([^ ]+) +([^ ]+) +([^ ]+) +\[([^]]+)\] +\"([^ ]+) +([^ "]+) +([^"]*)\" +([0-9]+) +([0-9]+) +\"(http:\/\/www\.google\.[a-zA-Z]+\/search\?([^"]+))\" +\"([^"]+)\"/---- httpd google ----/g;
s/^.{15} [^ ]+ httpd\[[0-9]*\]: ([^ ]+) +([^ ]+) +([^ ]+) +\[([^]]+)\] +\"([^ ]+) +([^ "]+) +([^"]*)\" +([0-9]+) +([0-9-]+) +\"([^"]+)\" +\"([^"]+)\"/---- httpd ----/g;
#
# httpsd http://www.modssl.org/
s/^.{15} [^ ]+ httpsd\[[0-9]*\]: \[([^]]+)\] +([^ ]+) +([^ ]+) +([^ ]+) +\"([^ ]+) +([^ "]+) +([^"]*)\" +([0-9]+|-)/---- httpsd ----/g;
#
# rpc.mountd
s/^.{15} [^ ]+ rpc.mountd: authenticated (mount|unmount) request from ([^:]+):([0-9]+) for ([^ ]+) \(([^)]+)\)/---- rpc\.mountd ----/g;
s/^.{15} [^ ]+ rpc.mountd: export request from ([0-9.]+)/---- rpc\.mountd ----/g;
#
# ntpd http://www.ntp.org/
s/^.{15} [^ ]+ ntpd\[[0-9]*\]: (synchronized) to ([0-9.]+), stratum=([0-9]+)/---- ntpd ----/g;
s/^.{15} [^ ]+ ntpd\[[0-9]*\]: (synchronisation lost)/---- ntpd ----/g;
s/^.{15} [^ ]+ ntpd\[[0-9]*\]: (no servers reachable)/---- ntpd ----/g;
s/^.{15} [^ ]+ ntpd\[[0-9]*\]: (kernel time sync status) ([0-9]+)/---- ntpd ----/g;
#
# snort http://www.snort.org/
s/^.{15} [^ ]+ snort: \[([0-9]+):([0-9]+):([0-9]+)\] ([^ ]+) ([^[]+)\[Classification: ([^]]+)\] \[Priority: ([0-9]+)\]: \{(TCP|UDP)\} ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+)/---- snort ----/g;
s/^.{15} [^ ]+ snort: \[([0-9]+):([0-9]+):([0-9]+)\] ([^ ]+) ([^[]+)\[Classification: ([^]]+)\] \[Priority: ([0-9]+)\]: \{(ICMP)\} ([0-9.]+) -> ([0-9.]+)/---- snort ----/g;
s/^.{15} [^ ]+ snort: spp_portscan: (PORTSCAN DETECTED) from ([0-9.]+) \(THRESHOLD ([0-9]+) connections exceeded in ([0-9]+) seconds\)/---- snort ----/g;
s/^.{15} [^ ]+ snort: spp_portscan: (portscan status) from ([0-9.]+): ([0-9]+) connections across ([0-9]+) hosts: TCP\(([0-9]+)\), UDP\(([0-9]+)\)/---- snort ----/g;
s/^.{15} [^ ]+ snort: spp_portscan: (End of portscan) from ([0-9.]+): TOTAL time\(([0-9]+)s\) hosts\(([0-9]+)\) TCP\(([0-9]+)\) UDP\(([0-9]+)\)/---- snort ----/g;
s/^.{15} [^ ]+ snort: \[([0-9]+):([0-9]+):([0-9]+)\] \(snort_decoder\): Short UDP packet, length field > payload length \{(UDP)\} ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+)/---- snort ----/g;
#
# httpsd_error
s/^.{15} [^ ]+ httpsd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] (mod_ssl): (SSL handshake failed) \(server ([^:]+):([0-9]+), client ([0-9.]+)\) \(OpenSSL library error follows\)/---- httpsd_error ----/g;
s/^.{15} [^ ]+ httpsd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] (OpenSSL): ([^:]+):([0-9A-F]+):(.+)/---- httpsd_error ----/g;
s/^.{15} [^ ]+ httpsd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] ([^:]+): (.*)/---- httpsd_error ----/g;
#
# net-entropy http://www.lsv.ens-cachan.fr/~olivain/net-entropy/
s/^.{15} [^ ]+ net-entropy\[[0-9]*\]: (RISING ALARM) on ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+) offset=([0-9]+) packets=([0-9]+) entropy=([0-9.]+)/---- net-entropy ----/g;
s/^.{15} [^ ]+ net-entropy\[[0-9]*\]: (Falling alarm) on ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+) offset=([0-9]+) packets=([0-9]+) entropy=([0-9.]+)/---- net-entropy ----/g;
s/^.{15} [^ ]+ net-entropy\[[0-9]*\]: (Stop tracking) ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+) offset=([0-9]+) packets=([0-9]+)/---- net-entropy ----/g;
s/^.{15} [^ ]+ net-entropy\[[0-9]*\]: (End of connection) ([0-9.]+):([0-9]+) -> ([0-9.]+):([0-9]+) offset=([0-9]+) packets=([0-9]+) \(([^)]+)\)/---- net-entropy ----/g;
#
# cupsd http://www.cups.org/
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: (REQUEST) ([^ ]+) ([^ ]+) ([^ ]+) \"([^ ]+) ([^ ]+) ([^ ]+)\" ([0-9]+) ([0-9]+)/---- cupsd ----/g;
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: (PAGE) ([^ ]+) ([^ ]+) ([0-9]+) ([0-9]+) ([0-9]+) ([^ ]+) ([^ ]+)/---- cupsd ----/g;
#
# pam_unix
s/^.{15} [^ ]+ [^(]+\(pam_unix\)\[[0-9]*\]: session closed for user (.+)/---- pam_unix ----/g;
#
# NEW
#
# pam_unix
s/^.{15} [^ ]+ [^(]+\(pam_unix\)\[[0-9]*\]: session (open)ed for user ([^)]+) by \(uid=([0-9]+)\)/---- pam_unix ----/g;
s/^.{15} [^ ]+ [^(]+\(pam_unix\)\[[0-9]*\]: session (open)ed for user ([^)]+) by ([^)]+)\(uid=([0-9]+)\)/---- pam_unix ----/g;
s/^.{15} [^ ]+ [^(]+\(pam_unix\)\[[0-9]*\]: check pass; user unknown/---- pam_unix ----/g;
s/^.{15} [^ ]+ [^(]+\(pam_unix\)\[[0-9]*\]: authentication failure; logname= uid=([0-9]+) euid=([0-9]+) tty=([^ ]+) ruser= rhost=(.*)/---- pam_unix ----/g;
#
# dhclient
s/^.{15} [^ ]+ dhclient: bound to ([0-9.]+) -- renewal in ([0-9]+) seconds\./---- dhclient ----/g;
s/^.{15} [^ ]+ dhclient: DHCPACK from ([0-9.]+)/---- dhclient ----/g;
s/^.{15} [^ ]+ dhclient: DHCPDISCOVER on ([^ ]+) to ([0-9.]+) port ([0-9]+) interval ([0-9]+)/---- dhclient ----/g;
s/^.{15} [^ ]+ dhclient: DHCPOFFER from ([0-9.]+)/---- dhclient ----/g;
s/^.{15} [^ ]+ dhclient: DHCPREQUEST on ([^ ]+) to ([0-9.]+) port ([0-9]+)/---- dhclient ----/g;
s/^.{15} [^ ]+ dhclient: receive_packet failed on ([\^:]): Network is down/---- dhclient ----/g;
#
# automount
s/^.{15} [^ ]+ automount\[[0-9]*\]: lookup\(([^)]+)\): looking up (.*)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: lookup\(([^)]+)\): (.*) -> (.*)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: expanded entry: (.*)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: running expiration on path ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: parse\(([^)]+)\): core of entry: ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): ([^ ]+) is local, doing bind/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): calling mkdir ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): calling mount --bind ([^ ]+) ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): mounted ([^ ]+) type ([^ ]+) on ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): calling mount -t ([^ ]+) ([^ ]+) ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: mount\(([^)]+)\): mounted ([^ ]+) on ([^ ]+)/---- automount ----/g;
s/^.{15} [^ ]+ automount\[[0-9]*\]: failed to mount ([^ ]+)/---- automount ----/g;
#
# dovecot  http://www.dovecot.org/
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: (Login|Aborted login): user=<([^>]+)>, method=([^,]+), rip=([^,]+), lip=([^,]+), (TLS)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: (Login|Aborted login): user=<([^>]+)>, method=([^,]+), rip=([^,]+), lip=([^,]+)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: (Aborted login): method=([^,]+), rip=([^,]+), lip=([^,]+), (TLS)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: (Aborted login): method=([^,]+), rip=([^,]+), lip=([^,]+)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: Disconnected: Inactivity: rip=([^,]+), lip=([^,]+), (TLS)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: Disconnected: Inactivity: rip=([^,]+), lip=([^,]+)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: Disconnected: rip=([^,]+), lip=([^,]+), (TLS)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: (pop3|imap)-login: Disconnected: rip=([^,]+), lip=([^,]+)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: POP3\(([^)]+)\): (Logout\.|Disconnected|Disconnected for inactivity.) top=([0-9]+)\/([0-9]+), retr=([0-9]+)\/([0-9]+), del=([0-9]+)\/([0-9]+), size=([0-9]+)/---- dovecot ----/g;
s/^.{15} [^ ]+ dovecot: Dovecot ([^ ]+) (starting up)/---- dovecot ----/g;
#
# sendmail http://www.sendmail.org/
s/^.{15} [^ ]+ sendmail\[[0-9]*\]: ([^:]+): from=([^,]+), size=([0-9]+), class=([0-9]+), nrcpts=([0-9]+), msgid=([^,]+), relay=([^,]+)/---- sendmail ----/g;
s/^.{15} [^ ]+ sendmail\[[0-9]*\]: ([^:]+): to=([^,]+), ctladdr=([^ ]+) \(([0-9]+)\/([0-9]+)\), delay=([0-9]+):([0-9]+):([0-9]+), xdelay=([0-9]+):([0-9]+):([0-9]+), mailer=([^,]+), pri=([0-9]+), relay=([^,]+), dsn=([^,]+), stat=([^ ]+) \(Ok: queued as ([0-9A-F]+)\)/---- sendmail ----/g;
s/^.{15} [^ ]+ sendmail\[[0-9]*\]: ([^:]+): to=([^,]+), ctladdr=([^ ]+) \(([0-9]+)\/([0-9]+)\), delay=([0-9]+):([0-9]+):([0-9]+), xdelay=([0-9]+):([0-9]+):([0-9]+), mailer=([^,]+), pri=([0-9]+), relay=([^,]+), dsn=([^,]+), stat=([^ ]+) (.*)/---- sendmail ----/g;
#
# anacron http://freshmeat.net/projects/anacron/
s/^.{15} [^ ]+ anacron\[[0-9]*\]: Updated timestamp for job `([^'])+' to (.+)/---- anacron ----/g;
#
# httpd_error
s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] \[client ([^]]+)\] File does not exist: (.*)/---- httpd_error ----/g;
s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] \[client ([^]]+)\] Symbolic link not allowed: (.*)/---- httpd_error ----/g;
s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] \[client ([^]]+)\] Directory index forbidden by rule: (.*)/---- httpd_error ----/g;
#s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] \[client ([^]]+)\] no acceptable variant: (.*)/---- httpd_error ----/g;
s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[client ([^]]+)\] script '([^']+)' not found or unable to stat/---- httpd_error ----/g;
s/^.{15} [^ ]+ httpd_error\[[0-9]*\]: \[([^]]+)\] \[([^]]+)\] \[client ([^]]+)\] ([^:]+): (.*)/---- httpd_error ----/g;
#
# postgres http://www.postgresql.org/
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  connection received: host=([0-9.]+) port=([0-9]+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  connection authorized: user=([^ ]+) database=([^ ]+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  received (fast shutdown) request/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  (shutting down)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  database system was shut down at (.+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  checkpoint record is at (.+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  redo record is at ([0-9A-F]+)\/([0-9A-F]+); undo record is at ([0-9A-F]+)\/([0-9A-F]+); shutdown (TRUE|FALSE)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  next transaction ID: ([0-9]+); next OID: ([0-9]+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  database system is (ready)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  syntax error at or near \"([^"]+)\" at character ([0-9]+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  syntax error at end of input at character ([0-9]+)/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  connection received: host=\[local\] port=/---- postgres ----/g;
s/^.{15} [^ ]+ postgres\[[0-9]*\]: \[([0-9]+)-([0-9]+)\] ([^:]+):  Password authentication failed for user \"([^"]+)\"/---- postgres ----/g;
#
# cupsd
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: (Adding ([^ ]+) banner page) \"([^"]+)\" to job ([0-9]+)\./---- cupsd ----/g;
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: Job ([0-9]+) (queued) on '([^']+)' by '([^']+)'\./---- cupsd ----/g;
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: (Started) (backend|filter) ([^ ]+) \(PID ([0-9]+)\) for job ([0-9]+)\./---- cupsd ----/g;
s/^.{15} [^ ]+ cupsd\[[0-9]*\]: \[Job ([0-9]+)\] Network host '([^']+)' is busy, down, or unreachable; will retry in ([0-9]+) seconds\.\.\./---- cupsd ----/g;
#
# kernel
s/^.{15} [^ ]+ kernel: klogd ([0-9.]+), log source = ([^ ]+) started\./---- kernel ----/g;
s/^.{15} [^ ]+ kernel: Kernel log daemon terminating\./---- kernel ----/g;
# str_field uts_release 1
# str_field compile_by 2
# str_field compile_host 3
# str_field compiler 4
# str_field uts_version
s/^.{15} [^ ]+ kernel: Linux version ([^ ]+) \(([^@]+)@([^)]+)\) \((.*)\) (.*)/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: Kernel command line: (.*)/---- kernel ----/g;
s/^.{15} [^ ]+ kernel: nfs: server ([^ ]+) (OK)/---- kernel ----/g;
# ypserv
s/^.{15} [^ ]+ ypserv\[[0-9]*\]: refused connect from ([0-9.]+):([0-9]+) to procedure ([^ ]+) \(([^,]+),([^;]+);(-?[0-9]+)\)/---- ypserv ----/g;
#
# ypbind
s/^.{15} [^ ]+ ypserv: (bound to NIS server) (.*)/---- ypbind ----/g;
s/^.{15} [^ ]+ ypserv: ypbind (shutdown|startup) succeeded/---- ypbind ----/g;
s/^.{15} [^ ]+ ypserv: Setting NIS domain name ([^:]+):  succeeded/---- ypbind ----/g;
s/^.{15} [^ ]+ ypserv\[[0-9]*\]: broadcast: RPC: Timed out\./---- ypbind ----/g;
#
# modprobe
s/^.{15} [^ ]+ modprobe: FATAL: Error running install command for ([^ ]+)/---- kernel ----/g;
#
# udev http://www.kernel.org/pub/linux/utils/kernel/hotplug/udev.html
s/^.{15} [^ ]+ udev\[[0-9]*\]: (creating|removing) device node '([^']+)'/---- udev ----/g;
s/^.{15} [^ ]+ udev\[[0-9]*\]: configured rule in '([^']+)' at line ([0-9]+) applied, '([^']+)' becomes '([^']+)'/---- udev ----/g;
