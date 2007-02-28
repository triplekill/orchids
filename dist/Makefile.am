orchidsconfdir = $(sysconfdir)/orchids

EXTRA_DIST = orchids.conf.dist

SUBDIRS = htmlstate rules

orchidsconf_DATA = orchids.conf

orchids.conf: $(srcdir)/orchids.conf.dist
	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
            -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
            -e 's#@@LIBDIR@@#$(libdir)#g' \
            -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
           $(srcdir)/orchids.conf.dist \
           > orchids.conf
