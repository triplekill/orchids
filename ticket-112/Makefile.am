SUBDIRS    = src dist

EXTRA_DIST = misc/orchids.spec misc/orchids-sysv

# orchidsstatedir must be owned by orchids
install-data-hook:
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/reports
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/log
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/ruletrace
	mkdir -p $(DESTDIR)$(localstatedir)/run/orchids
	-chown -R $(ORCHIDS_RUNTIME_USER) $(DESTDIR)$(localstatedir)/orchids $(DESTDIR)$(localstatedir)/run/orchids
