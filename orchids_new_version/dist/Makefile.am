orchidsconfdir = $(sysconfdir)/orchids

EXTRA_DIST = orchids.conf.dist orchids-rules.conf.dist orchids-modules.conf.dist \
		orchids-inputs.conf.dist

SUBDIRS = conf.d htmlstate iodef rules

orchidsconf_DATA = orchids.conf orchids-rules.conf orchids-modules.conf \
			orchids-inputs.conf

%.conf: $(srcdir)/%.conf.dist $(srcdir)/../config.log
	source distpatch.sh ${prefix} $(localstatedir) $(sysconfdir) $(libdir) $(ORCHIDS_RUNTIME_USER) \
	$<  >  $@
