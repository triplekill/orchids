SUBDIRS = modules
bin_PROGRAMS = orchids
orchids_SOURCES = \
        orchids.h orchids_defaults.h orchids_types.h \
        main.c main_priv.h \
        oevt_mgr.c evt_mgr.h evt_mgr_priv.h \
        mod_mgr.c orchids_api.c \
	engine.c engine.h engine_priv.h \
        rule_compiler.c rule_compiler.h \
	orchids_cfg.c \
        lang.c lang.h lang_priv.h \
        ovm.c ovm.h ovm_private.h \
        graph_output.c graph_output.h graph_output_priv.h \
	issdl.yy.c issdl.tab.c issdl.tab.h \
 \
	util/array.c               util/array.h           \
	util/bsdqueue.h            util/queue.c           \
	util/crc32.c               util/queue.h           \
	util/debuglog.c            util/debuglog.h        \
	                           util/safelib.c         \
	util/dlist.h               util/safelib.h         \
	util/hash.c                util/stack.c           \
	util/hash.h                util/stack.h           \
	util/linux_process_info.c  util/strhash.c         \
	util/linux_process_info.h  util/strhash.h         \
	util/list.c                util/string_util.c     \
	util/list.h                util/tree.c            \
	util/misc.c                util/tree.h            \
        util/objhash.c             util/objhash.h         \
        util/timer.h

#  \
# 	modules/mod_autohtml.c   modules/mod_cisco.c      \
# 	modules/mod_consoles.c   modules/mod_textfile.c   \
# 	modules/mod_generic.c    modules/mod_udp.c        \
# 	modules/mod_netfilter.c  modules/mod_win32evt.c   \
# 	modules/mod_rawsnare.c   modules/mod_remoteadm.c  \
# 	modules/mod_snare.c      modules/mod_snmp.c       \
# 	modules/mod_snmptrap.c   modules/rawsnare.h       \
# 	modules/mod_sockunix.c   modules/mod_sunbsm.c     \
# 	modules/mod_syslog.c \
# 	modules/snare.yy.c modules/snare.tab.c modules/snare.tab.h \
# 	modules/netfilter.yy.c modules/netfilter.tab.c modules/netfilter.tab.h

orchids_LDADD = -ldl
orchids_LDFLAGS = -export-dynamic
AM_CFLAGS= -I$(srcdir)/util

EXTRA_DIST = issdl.l issdl.y

BUILT_SOURCES = \
	issdl.yy.c issdl.tab.c issdl.tab.h
# 	modules/snare.yy.c modules/snare.tab.c modules/snare.tab.h \
# 	modules/netfilter.yy.c modules/netfilter.tab.c modules/netfilter.tab.h

$(srcdir)/issdl.tab.c $(srcdir)/issdl.tab.h: $(srcdir)/issdl.y
	$(YACC) -p issdl -d -b issdl -o $(srcdir)/issdl.tab.c $(srcdir)/issdl.y

$(srcdir)/issdl.yy.c: $(srcdir)/issdl.l issdl.tab.h
	$(LEX) -f -Pissdl -o$(srcdir)/issdl.yy.c $(srcdir)/issdl.l

# $(srcdir)/modules/netfilter.tab.c $(srcdir)/modules/netfilter.tab.h: $(srcdir)/modules/netfilter.y
# 	if test ! -d "modules" ; then mkdir modules ; fi
# 	$(YACC) -p netfilter -d -b netfilter -o $(srcdir)/modules/netfilter.tab.c $(srcdir)/modules/netfilter.y

# $(srcdir)/modules/netfilter.yy.c: $(srcdir)/modules/netfilter.l
# 	if test ! -d "modules" ; then mkdir modules ; fi
# 	$(LEX) -f -Pnetfilter -o$(srcdir)/modules/netfilter.yy.c $(srcdir)/modules/netfilter.l

# $(srcdir)/modules/snare.tab.c $(srcdir)/modules/snare.tab.h: $(srcdir)/modules/snare.y
# 	if test ! -d "modules" ; then mkdir modules ; fi
# 	$(YACC) -p snare -d -b snare -o $(srcdir)/modules/snare.tab.c $(srcdir)/modules/snare.y

# $(srcdir)/modules/snare.yy.c: $(srcdir)/modules/snare.l
# 	if test ! -d "modules" ; then mkdir modules ; fi
# 	$(LEX) -f -Psnare -o$(srcdir)/modules/snare.yy.c $(srcdir)/modules/snare.l
