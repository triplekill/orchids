orchidsconfdir = $(sysconfdir)/orchids
orchidsruledir = $(orchidsconfdir)/rules

orchidsrule_DATA = \
  linux32syscall.h \
  pid_tracker.rule \
  apachessl.rule \
  portscan.rule \
  dhcp_lease_check.rule \
  password_sniff.rule \
  anti_ssh_scan.rule \
  lin24_ptrace.rule\
  listen_atm_vcc.rule\
  lock_lease_dos.rule\
  leak_memory.rule\
  ssh_failed_burst.rule\
  ssh_failed_long_window.rule

EXTRA_DIST = $(orchidsrule_DATA) test/*.rule
