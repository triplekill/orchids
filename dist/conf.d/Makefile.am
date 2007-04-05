orchidsconfddir = $(sysconfdir)/orchids/conf.d

EXTRA_DIST =                  \
  01_mod_textfile.conf.dist   \
  02_mod_syslog.conf.dist     \
  03_mod_udp.conf.dist        \
  04_mod_sockunix.conf.dist   \
  05_mod_consoles.conf.dist   \
  06_mod_remoteadm.conf.dist  \
  07_mod_snare.conf.dist      \
  08_mod_rawsnare.conf.dist   \
  09_mod_netfilter.conf.dist  \
  10_mod_prolog.conf.dist     \
  11_mod_clocks.conf.dist     \
  12_mod_generic.conf.dist

orchidsconfd_DATA =         \
  01_mod_textfile.conf      \
  02_mod_syslog.conf        \
  03_mod_udp.conf           \
  04_mod_sockunix.conf      \
  05_mod_consoles.conf      \
  06_mod_remoteadm.conf     \
  07_mod_snare.conf         \
  08_mod_rawsnare.conf      \
  09_mod_netfilter.conf     \
  10_mod_prolog.conf        \
  11_mod_clocks.conf        \
  12_mod_generic.conf

%.conf: $(srcdir)/%.conf.dist
	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
            -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
            -e 's#@@LIBDIR@@#$(libdir)#g' \
            -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
           $< \
           > $@
