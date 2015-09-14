SUBDIRS    = src dist

EXTRA_DIST = misc/orchids.spec misc/orchids-sysv

AUTOMAKE_OPTIONS = subdir-objects

# orchidsstatedir must be owned by orchids
install-data-hook:
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/reports
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/log
	mkdir -p $(DESTDIR)$(localstatedir)/run/orchids
#	-rm $(DESTDIR)$(localstatedir)/run/orchids/void_source
#	-mknod $(DESTDIR)$(localstatedir)/run/orchids/void_source p
	mkdir -p $(DESTDIR)$(localstatedir)/orchids/save
	-chown -R $(ORCHIDS_RUNTIME_USER) $(DESTDIR)$(localstatedir)/orchids $(DESTDIR)$(localstatedir)/run/orchids
