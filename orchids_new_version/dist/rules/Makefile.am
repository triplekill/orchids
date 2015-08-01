orchidsconfdir = $(sysconfdir)/orchids
orchidsruledir = $(orchidsconfdir)/rules

orchidsrule_DATA = \
  linux32syscall.h \
  linux64syscall.h \
  linuxsignal.h \
  linuxptrace.h \
  pid_tracker_auditd.pprule \
  apachessl.pprule \
  portscan.pprule \
  dhcp_lease_check.pprule \
  password_sniff.pprule \
  anti_ssh_scan.pprule \
  lin24_ptrace.pprule\
  listen_atm_vcc.pprule\
  lock_lease_dos.pprule\
  leak_memory.pprule\
  ssh_failed_burst.pprule\
  ssh_failed_long_window.pprule \
  taint_auditd.pprule

EXTRA_DIST = $(orchidsrule_DATA) test/*.rule

%.pprule: $(srcdir)/%.rule
	$(CPP) $< > $@
