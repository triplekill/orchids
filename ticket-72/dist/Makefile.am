orchidsconfdir = $(sysconfdir)/orchids

EXTRA_DIST = orchids.conf.dist orchids-rules.conf.dist

SUBDIRS = conf.d htmlstate rules mods-enabled rules-enabled

orchidsconf_DATA = orchids.conf orchids-rules.conf

%.conf: $(srcdir)/%.conf.dist
	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
            -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
            -e 's#@@LIBDIR@@#$(libdir)#g' \
            -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
           $<  >  $@
