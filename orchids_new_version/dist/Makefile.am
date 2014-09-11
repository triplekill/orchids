orchidsconfdir = $(sysconfdir)/orchids

EXTRA_DIST = orchids.conf.dist orchids-rules.conf.dist orchids-modules.conf.dist \
		orchids-inputs.conf.dist

SUBDIRS = conf.d htmlstate rules

orchidsconf_DATA = orchids.conf orchids-rules.conf orchids-modules.conf \
			orchids-inputs.conf

%.conf: $(srcdir)/%.conf.dist
	sed -e 's#@@VARDIR@@#$(localstatedir)#g' \
            -e 's#@@ETCDIR@@#$(sysconfdir)#g' \
            -e 's#@@LIBDIR@@#$(libdir)#g' \
            -e 's#@@RUNUSER@@#$(ORCHIDS_RUNTIME_USER)#g' \
           $<  >  $@
