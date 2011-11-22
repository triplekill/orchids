orchidsiodefreportsdir = $(localstatedir)/orchids/iodef
orchidsconfdir = $(sysconfdir)/orchids
orchidsiodefconfdir = $(orchidsconfdir)/iodef
orchidsiodefconf_DATA =		\
  lin24_ptrace_audit.iodef	\
  pid_tracker.iodef		\
  portscan.iodef		\
  anti_ssh_scan.iodef		\
  test_idmef.iodef

EXTRA_DIST = $(orchidsiodefconf_DATA)

install-data-hook:
	mkdir -p $(orchidsiodefreportsdir)
