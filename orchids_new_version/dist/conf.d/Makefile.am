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
  12_mod_generic.conf.dist    \
  13_mod_pcap.conf.dist       \
  14_mod_wifi.conf.dist       \
  15_mod_sharedvars.conf.dist \
  16_mod_ruletrace.conf.dist  \
  17_mod_prelude.conf.dist    \
  18_mod_htmlstate.conf.dist  \
  19_mod_prolog_history.conf.dist \
  20_mod_idmef.conf.dist \
  21_mod_iodef.conf.dist

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
  12_mod_generic.conf       \
  13_mod_pcap.conf          \
  14_mod_wifi.conf          \
  15_mod_sharedvars.conf    \
  16_mod_ruletrace.conf     \
  17_mod_prelude.conf	    \
  18_mod_htmlstate.conf     \
  19_mod_prolog_history.conf \
  20_mod_idmef.conf \
  21_mod_iodef.conf


%.conf: $(srcdir)/%.conf.dist
	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
            -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
            -e 's#@@LIBDIR@@#$(libdir)#g' \
            -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
           $< \
           > $@
